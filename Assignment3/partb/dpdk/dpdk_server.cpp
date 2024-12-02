#include <rte_common.h>
#include <rte_log.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_eal.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_ip.h>
#include <rte_udp.h>

#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024
#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32
#define SERVER_PACKET_LENGTH 1024

class DPDKServer {
private:
    uint16_t port_id;
    struct rte_mempool* mbuf_pool;

    int configure_port() {
        struct rte_eth_conf port_conf{};
        struct rte_eth_dev_info dev_info;

        rte_eth_dev_info_get(port_id, &dev_info);

        port_conf.rxmode.max_rx_pkt_len = RTE_ETHER_MAX_LEN;
        port_conf.txmode.mq_mode = RTE_ETH_MQ_TX_NONE;

        int ret = rte_eth_dev_configure(port_id, 1, 1, &port_conf);
        if (ret != 0) {
            std::cerr << "Port " << port_id << " configuration failed" << std::endl;
            return ret;
        }

        ret = rte_eth_rx_queue_setup(port_id, 0, RX_RING_SIZE,
                                     rte_eth_dev_socket_id(port_id),
                                     NULL, mbuf_pool);
        if (ret < 0) {
            std::cerr << "RX queue setup failed for port " << port_id << std::endl;
            return ret;
        }

        ret = rte_eth_tx_queue_setup(port_id, 0, TX_RING_SIZE,
                                     rte_eth_dev_socket_id(port_id),
                                     NULL);
        if (ret < 0) {
            std::cerr << "TX queue setup failed for port " << port_id << std::endl;
            return ret;
        }

        ret = rte_eth_dev_start(port_id);
        if (ret < 0) {
            std::cerr << "Failed to start port " << port_id << std::endl;
            return ret;
        }

        rte_eth_promiscuous_enable(port_id);
        return 0;
    }

public:
    DPDKServer(uint16_t port, struct rte_mempool* pool) :
        port_id(port), mbuf_pool(pool) {}

    int initialize() {
        return configure_port();
    }

    void receive_packets(int duration_seconds = 10) {
        auto start_time = std::chrono::steady_clock::now();

        while (true) {
            struct rte_mbuf* rx_bufs[BURST_SIZE];
            uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, rx_bufs, BURST_SIZE);

            if (nb_rx > 0) {
                std::cout << "Received " << nb_rx << " packets" << std::endl;

                for (uint16_t i = 0; i < nb_rx; i++) {
                    char* payload = rte_pktmbuf_mtod(rx_bufs[i], char*);
                    std::cout << "Payload: " << payload << std::endl;

                    // Optional: Send acknowledgment
                    send_ack();

                    rte_pktmbuf_free(rx_bufs[i]);
                }
            }

            // Check elapsed time
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                current_time - start_time
            ).count();

            if (elapsed >= duration_seconds) {
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void send_ack() {
        struct rte_mbuf* tx_bufs[1];
        tx_bufs[0] = rte_pktmbuf_alloc(mbuf_pool);

        if (!tx_bufs[0]) {
            std::cerr << "Failed to allocate mbuf for ACK" << std::endl;
            return;
        }

        const char* ack_msg = "ACK";
        char* payload = rte_pktmbuf_append(tx_bufs[0], strlen(ack_msg));
        strcpy(payload, ack_msg);

        uint16_t nb_tx = rte_eth_tx_burst(port_id, 0, tx_bufs, 1);

        if (nb_tx == 0) {
            rte_pktmbuf_free(tx_bufs[0]);
        }
    }
};

int main(int argc, char** argv) {
    // Initialize DPDK EAL
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        std::cerr << "DPDK EAL initialization failed" << std::endl;
        return -1;
    }

    // Check available ports
    uint16_t nb_ports = rte_eth_dev_count_avail();
    if (nb_ports == 0) {
        std::cerr << "No Ethernet ports available" << std::endl;
        rte_eal_cleanup();
        return -1;
    }

    // Create memory pool
    struct rte_mempool* mbuf_pool = rte_pktmbuf_pool_create(
        "SERVER_MBUF_POOL", NUM_MBUFS, MBUF_CACHE_SIZE, 0,
        RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id()
    );

    if (!mbuf_pool) {
        std::cerr << "Cannot create mbuf pool" << std::endl;
        rte_eal_cleanup();
        return -1;
    }

    // Use first available port
    DPDKServer server(0, mbuf_pool);

    // Initialize server
    if (server.initialize() != 0) {
        std::cerr << "Server initialization failed" << std::endl;
        rte_eal_cleanup();
        return -1;
    }

    std::cout << "DPDK Server started. Waiting for packets..." << std::endl;

    // Receive packets for 10 seconds
    server.receive_packets(10);

    // Cleanup
    rte_eal_cleanup();
    return 0;
}