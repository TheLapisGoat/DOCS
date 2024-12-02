#include <iostream>
#include <asio.hpp>
#include <string>

using asio::ip::tcp;

/**
 * @class Client
 * @brief Represents a client that connects to a Redis server and sends commands in RESP format (GET, SET, DEL)
 * 
 * The client connects to the server and sends commands in RESP format. It serializes the commands into RESP format.
 * The client can send GET, SET, and DEL commands to the server. The client runs an interactive REPL loop where the user can
 * input commands to send to the server. The client reads the server's response and displays it to the user.
 */
class Client {
private:
    /**
     * @brief The asio IO context for handling asynchronous I/O operations
     */
    asio::io_context io_context_;
    
    /**
     * @brief The socket used to communicate with the server
     */
    tcp::socket socket_;

public:
    /**
     * @brief Constructs a new Client object and connects to the server at the specified host and port.
     * 
     * @param host The host address of the server
     * @param port The port number of the server
     */
    Client(const std::string& host, const std::string& port)
        : socket_(io_context_) {
        tcp::resolver resolver(io_context_);
        asio::connect(socket_, resolver.resolve(host, port));
        std::cout << "Connected to server " << host << ":" << port << "\n";
    }

    /**
     * @brief Sends a command to the server in RESP format and reads the server's response.
     * 
     * @param command The command to send to the server
     */
    void send_command(const std::string& command) {
        try {
            /// Serialize the command into RESP format
            std::string resp_command = serialize_command(command);
            if (resp_command.empty()) {
                std::cerr << "Invalid command format.\n";
                return;
            }

            /// Send the serialized command to the server
            asio::write(socket_, asio::buffer(resp_command));

            /// Read the server's response and display it to the user
            std::string response = read_response();
            std::cout << "Server Response: " << response << "\n";
        } catch (std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
    }

private:
    /**
     * @brief Serializes a command into RESP format.
     * 
     * @param command The command to serialize
     * @return The serialized command in RESP format
     */
    std::string serialize_command(const std::string& command) {
        std::istringstream command_stream(command);
        std::string cmd, key, value;
        command_stream >> cmd;

        if (cmd == "SET") {
            command_stream >> key;
            std::getline(command_stream, value);
            /// Removes quotes from the value
            value = value.substr(2, value.size() - 3);

            return format_resp({cmd, key, value});
        } else if (cmd == "GET" || cmd == "DEL") {
            command_stream >> key;
            return format_resp({cmd, key});
        }

        return "";
    }

    /**
     * @brief Formats a vector of strings into a RESP formatted string.
     * 
     * @param args The vector of strings to format
     * @return The formatted RESP string
     */
    std::string format_resp(const std::vector<std::string>& args) {
        std::ostringstream resp;
        resp << "*" << args.size() << "\r\n";   //.< Number of elements in the array

        for (const auto& arg : args) {
            resp << "$" << arg.size() << "\r\n" << arg << "\r\n"; ///< Length of the string and the string itself
        }

        return resp.str();
    }

    /**
     * @brief Reads the server's response in RESP format.
     * 
     * @return The server's response as a string
     */
    std::string read_response() {
        asio::streambuf buffer;
        asio::read_until(socket_, buffer, "\r\n");
        std::istream response_stream(&buffer);
        std::string response;
        std::getline(response_stream, response);

        /// Re-add the \n character
        response += '\n';

        if (response[0] == '+') {
            /// Simple string, return the string without the + character and \r\n
            return response.substr(1, response.size() - 3);
        } else if (response[0] == '-') {
            ///Error, return the string without the - character and \r\n
            return response.substr(1, response.size() - 3);
        } else if (response[0] == ':') {
            ///Integer, return the string without the : character and \r\n
            return response.substr(1, response.size() - 3);
        } else if (response[0] == '$') {
            ///Bulk string, first get the length of the string
            int length = std::stoi(response.substr(1, response.size() - 3));

            if (length == -1) {
                return "nil";
            }

            /// Keep doing read_until until length bytes are read
            std::string bulk_string {};
            while (bulk_string.length() != length + 2) {
                asio::read_until(socket_, buffer, "\r\n");
                std::string temp_string {};
                std::getline(response_stream, temp_string);

                temp_string += '\n';
                bulk_string += temp_string;
            }

            ///Return the bulk string without the \r\n
            return bulk_string.substr(0, bulk_string.size() - 2);
        } else {
            ///Handling arrays in not required for this client
            return "Unknown response type";
        }
    }
};

/**
 * @brief Starts the Read-Eval-Print Loop (REPL) for the client.
 * 
 * The REPL allows the user to input commands to send to the server. The user can type 'exit' to quit the REPL.
 * 
 * @param client The client object to send commands to the server
 */ 
void startREPL(Client& client) {
    std::string command {};
    std::string key {};
    std::string value {};

    std::cout << "REPL Started. Type 'exit' to quit." << std::endl;

    while (true) {
        std::cout << "> ";

        std::getline(std::cin, command);

        if (command == "exit") {
            break;
        }

        client.send_command(command);
    }
}

int main(int argc, char* argv[]) {
    try {
        /// Default host and port
        std::string host = "192.168.20.1";
        std::string port = "6379";

        /// If host and port are provided as command line arguments
        if (argc > 2) {
            host = argv[1];
            port = argv[2];
        }

        Client client(host, port);
        startREPL(client);
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }

    return 0;
}