#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <stdexcept>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

/**
 * @brief SystemMonitor class to monitor CPU and memory usage of the current process
 * 
 * This class starts a separate thread to monitor CPU and memory
 * usage of the current process. The monitoring thread reads CPU
 * and memory usage every 100ms and stores the readings in vectors.
 */
class SystemMonitor {
public:
    /**
     * @brief Start monitoring system resources
     */
    void start_monitoring() {
        is_monitoring_ = true;
        total_packets_ = 0;
        monitor_thread_ = std::thread(&SystemMonitor::monitor_loop, this);
    }

    /**
     * @brief Stop monitoring system resources
     * @param processed_packets Total packets processed
     */
    void stop_monitoring(uint64_t processed_packets) {
        is_monitoring_ = false;
        total_packets_ = processed_packets;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
        print_summary();
    }

    /**
     * @brief Destructor to stop monitoring if still running
     */
    static double get_process_cpu_usage() {
        static long long prev_total = 0;
        static long long prev_idle = 0;

        // Read system-wide CPU times
        std::ifstream cpustat("/proc/stat");
        if (!cpustat.is_open()) return 0.0;

        std::string cpu_line;
        std::getline(cpustat, cpu_line);
        cpustat.close();

        // Parse CPU times
        std::istringstream iss(cpu_line);
        std::string cpu_label;
        unsigned long long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;

        iss >> cpu_label >> user >> nice >> system >> idle >> iowait
            >> irq >> softirq >> steal >> guest >> guest_nice;

        // Adjust user and nice times by subtracting guest times
        user -= guest;
        nice -= guest_nice;

        // Calculate idle time
        unsigned long long idle_all_time = idle + iowait;
        unsigned long long system_all_time = system + irq + softirq;
        unsigned long long virt_all_time = guest + guest_nice;
        unsigned long long total_time = user + nice + system_all_time + idle_all_time + steal + virt_all_time;

        // Calculate differences
        long long total_diff = total_time - prev_total;
        long long idle_diff = idle_all_time - prev_idle;

        // Update previous values
        prev_total = total_time;
        prev_idle = idle_all_time;

        // Prevent division by zero
        if (total_diff == 0) return 0.0;

        // Calculate CPU usage percentage
        double cpu_percentage = 100.0 * (total_diff - idle_diff) / total_diff;

        // Ensure the percentage is between 0 and 100
        return std::min(std::max(cpu_percentage, 0.0), 100.0);
    }


    /**
     * @brief Get current process memory usage
     * @return long Memory usage in KB
     */
    static long get_process_memory_usage() {
        std::ifstream status_file("/proc/self/status");
        std::string line;
        while (std::getline(status_file, line)) {
            if (line.substr(0, 6) == "VmRSS:") {
                std::istringstream iss(line);
                std::string tag;
                long memory;
                iss >> tag >> memory;
                return memory;
            }
        }
        return 0;
    }

private:
    /**
     * @brief Monitor loop to read CPU and memory usage every 100ms
     */
    void monitor_loop() {
        while (is_monitoring_) {
            double cpu_usage = get_process_cpu_usage();
            long memory_usage = get_process_memory_usage();

            cpu_readings_.push_back(cpu_usage);
            memory_readings_.push_back(memory_usage);

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    /**
     * @brief Print summary of CPU and memory usage
     */
    void print_summary() {
        if (cpu_readings_.empty() || memory_readings_.empty()) return;

        // Calculate CPU statistics
        double cpu_avg = calculate_average(cpu_readings_);
        double cpu_max = *std::max_element(cpu_readings_.begin(), cpu_readings_.end());

        // Calculate memory statistics
        double memory_avg = calculate_average(memory_readings_);
        long memory_max = *std::max_element(memory_readings_.begin(), memory_readings_.end());

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\nResource Utilization Summary:\n";
        std::cout << "  Total Packets Processed: " << total_packets_ << "\n";
        std::cout << "  CPU Usage:   " << cpu_avg << "%, Max " << cpu_max << "%\n";
        std::cout << "  Memory Usage: Avg " << memory_avg << " KB\n";
    }

    /**
     * @brief Calculate average of values in a vector
     * @tparam T Data type of values
     * @param values Vector of values
     * @return T Average of values
     */
    template <typename T>
    T calculate_average(const std::vector<T>& values) {
        if (values.empty()) return T();
        return std::accumulate(values.begin(), values.end(), T()) / values.size();
    }

    /**
     * @brief Atomic flag to indicate if monitoring is active
     */
    std::atomic<bool> is_monitoring_{false};

    /**
     * @brief Monitoring thread
     */
    std::thread monitor_thread_;

    /**
     * @brief Vectors to store CPU and memory readings
     */
    std::vector<double> cpu_readings_;

    /**
     * @brief Vectors to store CPU and memory readings
     */
    std::vector<long> memory_readings_;

    /**
     * @brief Total packets processed
     */
    std::atomic<uint64_t> total_packets_{0};
};

class MonitoredSingleClientServer {
public:
    /**
     * @brief Constructs a new Monitored Single Client Server
     * @param port Port number to listen on
     * @param monitor Pointer to SystemMonitor instance
     */
    MonitoredSingleClientServer(int port, SystemMonitor* monitor)
        : port_(port), server_socket_(-1), client_socket_(-1), monitor_(monitor) {
        // Create server socket
        server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket_ < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        // Allow socket reuse
        int opt = 1;
        setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    }

    /**
     * @brief Destructor to clean up server resources
     */
    ~MonitoredSingleClientServer() {
        closeConnections();
    }

    /**
     * @brief Runs the monitored single client server
     * @param num_packets_to_process Number of packets to process (-1 for unlimited)
     */
    void run(int num_packets_to_process = -1) {
        // Prepare server address
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port_);

        // Bind socket
        if (bind(server_socket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
            throw std::runtime_error("Bind failed");
        }

        // Listen for connections (only one connection in backlog)
        if (listen(server_socket_, 1) < 0) {
            throw std::runtime_error("Listen failed");
        }

        // Start system monitoring
        if (monitor_) {
            monitor_->start_monitoring();
        }

        std::cout << "Server waiting for a single client on port " << port_ << std::endl;

        // Accept a single client
        sockaddr_in client_address;
        socklen_t client_addr_len = sizeof(client_address);
        client_socket_ = accept(
            server_socket_,
            (struct sockaddr*)&client_address,
            &client_addr_len
        );

        if (client_socket_ < 0) {
            throw std::runtime_error("Accept failed");
        }

        // Print client details
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_address.sin_addr), client_ip, INET_ADDRSTRLEN);
        std::cout << "Client connected from: " << client_ip
                  << ":" << ntohs(client_address.sin_port) << std::endl;

        // Handle the single client
        uint64_t total_packets = handleClient(num_packets_to_process);

        // Stop system monitoring
        if (monitor_) {
            monitor_->stop_monitoring(total_packets);
        }
    }

private:
    /**
     * @brief Handles communication with the single connected client
     * @param max_packets Maximum number of packets to process (-1 for unlimited)
     * @return uint64_t Total packets processed
     */
    uint64_t handleClient(int max_packets = -1) {
        std::vector<char> buffer(1024);
        uint64_t total_packets = 0;

        while (max_packets == -1 || total_packets < static_cast<uint64_t>(max_packets)) {
            // Receive data
            ssize_t bytes_received = recv(client_socket_, buffer.data(), buffer.size(), 0);

            if (bytes_received <= 0) {
                // Client disconnected or error occurred
                std::cout << "Client disconnected. Total packets processed: " << total_packets << std::endl;
                break;
            }

            // Increment packets processed
            total_packets++;

            // Send back the length of received data
            int length = static_cast<int>(bytes_received);
            send(client_socket_, &length, sizeof(length), 0);
        }

        return total_packets;
    }

    /**
     * @brief Closes all open socket connections
     */
    void closeConnections() {
        if (client_socket_ >= 0) {
            close(client_socket_);
        }
        if (server_socket_ >= 0) {
            close(server_socket_);
        }
    }

    int server_socket_;  ///< Server socket file descriptor
    int client_socket_;  ///< Client socket file descriptor
    int port_;           ///< Port to listen on
    SystemMonitor* monitor_;  ///< Pointer to system monitor
};

int main(int argc, char* argv[]) {
    try {
        // Default port and number of packets
        int port = 8080;
        int num_packets = -1;  // Unlimited by default

        // Parse command line arguments
        if (argc > 1) {
            port = std::stoi(argv[1]);
        }
        if (argc > 2) {
            num_packets = std::stoi(argv[2]);
        }

        // Create system monitor
        SystemMonitor monitor;

        // Create and run server
        MonitoredSingleClientServer server(port, &monitor);
        server.run(num_packets);

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
}