#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <stdexcept>
#include <iomanip>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

/**
 * @brief Performance client for TCP server
 * 
 * This class is used to run performance benchmarks on a TCP server.
 */
class PerformanceClient {
public:
    /**
     * @brief Performance metrics
     */
    struct PerformanceMetrics {
        double total_time_ms;
        double avg_latency_ms;
        double total_data_sent_mb;
        double bandwidth_mbps;
    };

    /**
     * @brief Constructs a new PerformanceClient
     * @param server_ip Server IP address
     * @param port Server port
     */
    explicit PerformanceClient(const std::string& server_ip, int port)
        : server_ip_(server_ip), port_(port) {}

    /**
     * @brief Runs performance benchmark
     * @param num_packets Number of packets to send
     * @param payload_size Size of each packet
     * @return PerformanceMetrics Calculated performance metrics
     */
    PerformanceMetrics run_benchmark(
        int num_packets,
        int payload_size = 1024
    ) {
        PerformanceMetrics metrics;
        std::vector<long long> latencies;


        // Start total time measurement
        auto total_start = std::chrono::high_resolution_clock::now();

        // Create new socket for each connection to simulate real-world scenario
        int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        // Prepare server address
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);

        if (inet_pton(AF_INET, server_ip_.c_str(), &server_addr.sin_addr) <= 0) {
            close(socket_fd);
            throw std::runtime_error("Invalid address");
        }


        // Connect to server
        if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(socket_fd);
            throw std::runtime_error("Connection failed");
        }
        // Benchmark loop
        double atcual_total_payload_size = 0.0;
        for (int i = 0; i < num_packets; ++i) {

            // Prepare payload
            // taking a random size payload between payload_size/2 and payload_size
            int actual_payload_size = payload_size / 2 + rand() % (payload_size / 2 + 1);
            atcual_total_payload_size += (double)actual_payload_size;
            std::vector<uint8_t> payload(actual_payload_size);
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> byte_dist(0, 255);

            for (auto& byte : payload) {
                byte = byte_dist(gen);
            }

            // Measure individual packet latency
            auto packet_start = std::chrono::high_resolution_clock::now();

            // Send payload
            ssize_t bytes_sent = send(socket_fd, payload.data(), payload.size(), 0);
            if (bytes_sent < 0) {
                close(socket_fd);
                throw std::runtime_error("Packet sending failed");
            }

            // Receive server response
            int received_length;
            recv(socket_fd, &received_length, sizeof(received_length), 0);

            // Close connection

            // Calculate packet latency
            auto packet_end = std::chrono::high_resolution_clock::now();
            auto packet_latency = std::chrono::duration_cast<std::chrono::microseconds>(
                packet_end - packet_start
            ).count();
            latencies.push_back(packet_latency);
        }
        close(socket_fd);

        // Calculate total time
        auto total_end = std::chrono::high_resolution_clock::now();
        auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            total_end - total_start
        ).count();

        // Calculate metrics
        metrics.total_time_ms = total_duration;
        metrics.total_data_sent_mb = atcual_total_payload_size / (8 * 1024.0 * 1024.0);
        metrics.bandwidth_mbps = (metrics.total_data_sent_mb * 8000.0) / total_duration;

        // Calculate average latency
        long long total_latency = 0;
        for (auto lat : latencies) {
            total_latency += lat;
        }
        metrics.avg_latency_ms = static_cast<double>(total_latency) / (1000 * num_packets);

        return metrics;
    }

    /**
     * @brief Print performance metrics
     * @param metrics Performance metrics to print
     */
    static void print_metrics(const PerformanceMetrics& metrics) {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Performance Metrics:\n";
        std::cout << "  Total Time:       " << metrics.total_time_ms << " ms\n";
        std::cout << "  Average Latency:  " << metrics.avg_latency_ms << " ms\n";
        std::cout << "  Total Data Sent:  " << metrics.total_data_sent_mb << " MB\n";
        std::cout << "  Bandwidth:        " << metrics.bandwidth_mbps << " Mbps\n";
    }

private:
    /**
     * @brief Server IP address
     */
    std::string server_ip_;

    /**
     * @brief Server port
     */
    int port_;
};

int main() {
    try {
        std::string server_ip = "127.0.0.0";
        int port = 8080;

        PerformanceClient client(server_ip, port);

        // Benchmark configurations
        std::vector<int> packet_counts = {10000};
        for (int num_packets : packet_counts) {
                std::cout << "\nBenchmark: " << num_packets << " packets, " << "\n";

                auto metrics = client.run_benchmark(num_packets, 1024);
                PerformanceClient::print_metrics(metrics);
        }

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}