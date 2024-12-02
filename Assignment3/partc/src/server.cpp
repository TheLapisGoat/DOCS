#include <storage_engine.h>
#include <iostream>
#include <unordered_map>
#include <asio.hpp>
#include <memory>
#include <mutex>
#include <sstream>
#include <vector>

using asio::ip::tcp;

#ifdef N_THREADS
int const num_threads = N_THREADS;
#else
int const num_threads = 4;
#endif

/**
 * @class Session
 * @brief Represents a session with a client.
 * 
 * The session class handles reading commands from the client, parsing the commands in RESP format, processing the commands,
 * and sending the response back to the client. The session class uses a reference to the storage engine to process the commands.
 */
class Session : public std::enable_shared_from_this<Session> {
private:
    /**
     * @brief The socket used to communicate with the client.
     */
    tcp::socket socket_;

    /**
     * @brief The buffer used to read data from the socket.
     */
    asio::streambuf buffer_;

    /**
     * @brief The storage engine used to process commands and execute operations.
     */
    StorageEngine& storageEngine;

    std::vector<std::string> current_command_;  ///< @brief The current command being processed.
    int expected_parts_ = -1;                   ///< @brief The number of parts expected in the current command.
    bool reading_bulk_string_ = false;          ///< @brief Flag to indicate if a bulk string is being read.
    int bulk_string_length_ = 0;                ///< @brief The length of the bulk string being read.
    std::string bulk_string_ {};                ///< @brief The bulk string buffer.

public:
    /**
     * @brief Constructs a new Session object with the specified socket and storage engine.
     * 
     * @param socket The socket used to communicate with the client
     * @param storageEngine The storage engine used to process commands
     */
    explicit Session(tcp::socket socket, StorageEngine& storageEngine)
        : socket_(std::move(socket)), storageEngine(storageEngine) {}

    /**
     * @brief Starts the session by reading commands from the client.
     */
    void start() {
        read_line();
    }

private:
    /**
     * @brief Reads a line from the client.
     * 
     * This function reads a line from the client until a newline character is encountered. It then processes the line and continues reading.
     * The reading is done asynchronously, and after every read, the function adds itself to the callback chain to read the next line.
     */
    void read_line() {
        auto self(shared_from_this());

        /// Asynchronously read until a CR-LF sequence is encountered
        asio::async_read_until(socket_, buffer_, "\r\n",
            [this, self](std::error_code ec, std::size_t /*bytes_transferred*/) {
                if (!ec) {
                    std::istream input_stream(&buffer_);
                    std::string line;
                    std::getline(input_stream, line);

                    // Remove trailing '\r' left by getline
                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }

                    process_line(line);

                    /// Add itself to the callback chain to read the next line
                    read_line();
                } else if (ec != asio::error::eof) {
                    std::cerr << "Error reading from client: " << ec.message() << "\n";
                }
            });
    }

    /**
     * @brief Processes a line read from the client.
     * 
     * This function processes a line read from the client in RESP format. It parses the line, processes the command, and sends the response back to the client.
     */
    void process_line(const std::string& line) {
        if (reading_bulk_string_) {
            /// If reading a bulk string, append the line to the bulk string buffer
            bulk_string_ += line;

            /// Check if the bulk string has been fully read
            if (bulk_string_.size() == bulk_string_length_) {
                current_command_.push_back(bulk_string_);
                reading_bulk_string_ = false;
            } else {
                bulk_string_ += "\r\n";
                return;
            }

            /// Check if the command is complete
            if (current_command_.size() == expected_parts_) {
                std::string response = process_command(current_command_);
                send_response(response);

                reset_state();
            }
            return;
        }

        if (line.empty()) return;

        if (line[0] == '*') {
            /// RESP Array
            expected_parts_ = std::stoi(line.substr(1));
            current_command_.clear();
        } else if (line[0] == '$') {
            /// RESP Bulk String
            bulk_string_length_ = std::stoi(line.substr(1));
            reading_bulk_string_ = true;
            bulk_string_.clear();
        } else {
            /// For current implementation, only RESP Arrays and Bulk Strings are required
            std::cerr << "Unexpected input: " << line << "\n";
        }
    }

    /**
     * @brief Processes a command in RESP format.
     * 
     * This function processes a command in RESP format. It executes the command using the storage engine and returns the response.
     * 
     * @param args The arguments of the command
     * @return The response to the command
     */
    std::string process_command(std::vector<std::string>& args) {
        if (args.empty()) {
            return "-ERR empty command\r\n";
        }

        std::string& command = args[0];
        if (command == "SET" && args.size() == 3) {
            storageEngine.insert(args[1], args[2]);
            return "+OK\r\n";
        } else if (command == "GET" && args.size() == 2) {
            std::optional<std::string> result = storageEngine.get(args[1]);
            if (!result) {
                return "$-1\r\n";
            }
            return "$" + std::to_string(result.value().size()) + "\r\n" + result.value() + "\r\n";
        } else if (command == "DEL" && args.size() == 2) {
            bool result = storageEngine.erase(args[1]);
            if (!result) {
                return ":0\r\n";
            }
            return ":1\r\n";
        } else {
            return "-ERR unknown command or wrong number of arguments\r\n";
        }
    }

    /**
     * @brief Sends a response to the client.
     * 
     * This function sends a response to the client using the socket.
     * 
     * @param response The response to send
     */
    void send_response(const std::string& response) {
        auto self(shared_from_this());
        asio::async_write(socket_, asio::buffer(response),
            [this, self](std::error_code ec, std::size_t /*length*/) {
                if (ec) {
                    std::cerr << "Error writing response: " << ec.message() << "\n";
                }
            });
    }

    /**
     * @brief Resets the state of the session.
     * 
     * This function resets the state of the session by clearing the current command, resetting the expected parts, and clearing the bulk string buffer.
     */
    void reset_state() {
        current_command_.clear();
        expected_parts_ = -1;
        reading_bulk_string_ = false;
        bulk_string_length_ = 0;
        bulk_string_.clear();
    }
};

/**
 * @class Server
 * @brief Represents a server that listens for incoming connections.
 * 
 * The server class listens for incoming connections from clients and creates a new session for each client.
 * The server uses an acceptor to accept connections and an io_context to handle asynchronous operations.
 * The server passes a reference to the storage engine to each session to process commands.
 */
class Server {
private:
    /**
     * @brief The asio IO context for handling asynchronous I/O operations.
     */
    asio::io_context io_context_;

    /**
     * @brief The acceptor used to accept incoming connections.
     */
    tcp::acceptor acceptor_;

    /**
     * @brief The storage engine used to process commands and execute operations.
     */
    std::unique_ptr<StorageEngine> storageEngine;

public:
    /**
     * @brief Constructs a new Server object with the specified port and storage engine.
     * 
     * @param port The port number to listen on
     * @param storageEngine The storage engine used to process commands
     */
    explicit Server(short port, std::unique_ptr<StorageEngine> storageEngine)
        : acceptor_(io_context_, tcp::endpoint(tcp::v4(), port)), storageEngine{std::move(storageEngine)} {
        accept_connections();
    }

    /**
     * @brief Runs the server.
     * 
     * This function runs the server by starting the io_context event loop.
     */
    void run() {
        io_context_.run();
    }

private:
    /**
     * @brief Accepts incoming connections.
     * 
     * This function accepts incoming connections from clients and creates a new session for each client.
     */
    void accept_connections() {
        acceptor_.async_accept(
            [this](std::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<Session>(std::move(socket), *storageEngine)->start();
                }
                accept_connections();
            });
    }
};

int main(int argc, char* argv[]) {
    std::unique_ptr<StorageEngine> storageEngine = std::make_unique<StorageEngine>("data", 512, StorageEngine::INITIALIZATION_MODE::OPEN);

    try {
        short port = 6379;
        if (argc > 1) {
            port = std::atoi(argv[1]);
        }

        Server server(port, std::move(storageEngine));
        std::cout << "Server running on port " << port << "...\n";

        std::vector<std::thread> threads;
        for (int i = 0; i < num_threads; i++) {
            threads.emplace_back([&server]() {
                server.run();
            });
        }

        for (auto& t : threads) {
            t.join();
        }
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }

    return 0;
}