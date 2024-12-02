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
#include <vector>

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024
#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

// Configuration for ports and packet generation
struct PortConfig {
    uint16_t port_id;
    struct rte_mempool* mbuf_pool;
};

// Function to configure DPDK port
static int configure_dpdk_port(uint16_t port_id, struct rte_mempool* mbuf_pool) {
    struct rte_eth_conf port_conf;
    struct rte_eth_dev_info dev_info;
    int retval;

    // Clear the configuration
    memset(&port_conf, 0, sizeof(port_conf));

    // Retrieve device information
    rte_eth_dev_info_get(port_id, &dev_info);

    // Configure the Ethernet device
    port_conf.rxmode.max_rx_pkt_len = RTE_ETHER_MAX_LEN;
    port_conf.txmode.mq_mode = RTE_ETH_MQ_TX_NONE;

    // Configure the port
    retval = rte_eth_dev_configure(port_id, 1, 1, &port_conf);
    if (retval != 0) {
        std::cerr << "Failed to configure port " << port_id << std::endl;
        return retval;
    }

    // Configure RX and TX queues
    retval = rte_eth_rx_queue_setup(port_id, 0, RX_RING_SIZE,
                                    rte_eth_dev_socket_id(port_id),
                                    NULL, mbuf_pool);
    if (retval < 0) {
        std::cerr << "Failed to setup RX queue for port " << port_id << std::endl;
        return retval;
    }

    retval = rte_eth_tx_queue_setup(port_id, 0, TX_RING_SIZE,
                                    rte_eth_dev_socket_id(port_id),
                                    NULL);
    if (retval < 0) {
        std::cerr << "Failed to setup TX queue for port " << port_id << std::endl;
        return retval;
    }

    // Start the port
    retval = rte_eth_dev_start(port_id);
    if (retval < 0) {
        std::cerr << "Failed to start port " << port_id << std::endl;
        return retval;
    }

    // Enable RX in promiscuous mode
    rte_eth_promiscuous_enable(port_id);

    return 0;
}

// Packet sender function
int packet_sender(void* arg) {
    PortConfig* config = static_cast<PortConfig*>(arg);
    uint16_t port_id = config->port_id;
    struct rte_mempool* mbuf_pool = config->mbuf_pool;

    // Prepare packet data
    const char* packet_data = "DPDK Packet Transmission Test";

    // Send multiple packets in a burst
    struct rte_mbuf* tx_bufs[BURST_SIZE];

    // Allocate mbufs for packet transmission
    for (unsigned i = 0; i < BURST_SIZE; i++) {
        tx_bufs[i] = rte_pktmbuf_alloc(mbuf_pool);
        if (!tx_bufs[i]) {
            std::cerr << "Failed to allocate mbuf" << std::endl;
            return -1;
        }

        // Prepare packet payload
        char* payload = rte_pktmbuf_append(tx_bufs[i], strlen(packet_data));
        if (!payload) {
            std::cerr << "Failed to append payload" << std::endl;
            return -1;
        }
        strcpy(payload, packet_data);
    }

    // Send packet burst
    uint16_t nb_tx = rte_eth_tx_burst(port_id, 0, tx_bufs, BURST_SIZE);

    std::cout << "Sent " << nb_tx << " packets on port " << port_id << std::endl;

    // Free any unsent packets
    for (unsigned i = nb_tx; i < BURST_SIZE; i++) {
        rte_pktmbuf_free(tx_bufs[i]);
    }

    return 0;
}

// Packet receiver function
int packet_receiver(void* arg) {
    PortConfig* config = static_cast<PortConfig*>(arg);
    uint16_t port_id = config->port_id;

    // Receive packet burst
    struct rte_mbuf* rx_bufs[BURST_SIZE];
    uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, rx_bufs, BURST_SIZE);

    if (nb_rx > 0) {
        std::cout << "Received " << nb_rx << " packets on port " << port_id << std::endl;

        // Process received packets
        for (uint16_t i = 0; i < nb_rx; i++) {
            char* data = rte_pktmbuf_mtod(rx_bufs[i], char*);
            std::cout << "Packet " << i << " payload: " << data << std::endl;

            // Free the mbuf
            rte_pktmbuf_free(rx_bufs[i]);
        }
    }

    return 0;
}

int main(int argc, char** argv) {
    // Initialize DPDK Environment Abstraction Layer (EAL)
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        std::cerr << "Failed to initialize DPDK EAL" << std::endl;
        return -1;
    }

    // Check available ports
    uint16_t nb_ports = rte_eth_dev_count_avail();
    if (nb_ports < 2) {
        std::cerr << "Not enough Ethernet ports" << std::endl;
        rte_eal_cleanup();
        return -1;
    }

    // Create memory pool
    struct rte_mempool* mbuf_pool = rte_pktmbuf_pool_create(
        "MBUF_POOL", NUM_MBUFS * nb_ports, MBUF_CACHE_SIZE, 0,
        RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id()
    );
    if (!mbuf_pool) {
        std::cerr << "Cannot create mbuf pool" << std::endl;
        rte_eal_cleanup();
        return -1;
    }

    // Configure two ports for sender and receiver
    std::vector<PortConfig> ports(2);
    for (uint16_t portid = 0; portid < 2; portid++) {
        ports[portid].port_id = portid;
        ports[portid].mbuf_pool = mbuf_pool;

        if (configure_dpdk_port(portid, mbuf_pool) != 0) {
            std::cerr << "Failed to configure port " << portid << std::endl;
            rte_eal_cleanup();
            return -1;
        }
    }

    // Send packets on first port
    packet_sender(&ports[0]);

    // Receive packets on second port
    packet_receiver(&ports[1]);

    // Cleanup
    rte_eal_cleanup();
    return 0;
}