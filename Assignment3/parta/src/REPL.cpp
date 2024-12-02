#include <iostream>
#include <string>
#include <sstream>
#include <storage_engine.h>

void startREPL() {
    StorageEngine storageEngine("data", 512, StorageEngine::INITIALIZATION_MODE::OPEN);

    std::string command {};
    std::string key {};
    std::string value {};

    std::cout << "StorageEngine REPL. Type 'exit' to quit." << std::endl;

    while (true) {
        std::cout << "> ";

        std::getline(std::cin, command);

        if (command == "exit") {
            break;
        }

        /// Parse the command
        std::istringstream iss {command};
        std::string cmd {};

        iss >> cmd;

        if (cmd == "SET") {
            /// SET <key> "<value>"
            if (iss >> key && std::getline(iss, value)) {
                ///Removing the surrounding quotes from the value
                value = value.substr(2, value.size() - 3);
                storageEngine.insert(key, value);
            } else {
                std::cout << "Invalid SET command. Format: SET <key> \"<value>\"" << std::endl;
            }
        } else if (cmd == "GET") {
            /// GET <key>
            if (iss >> key) {
                std::optional<std::string> result = storageEngine.get(key);
                if (!result) {
                    std::cout << "Key not found." << std::endl;
                    continue;
                }
                std::cout << "Value: " << "\"" + result.value() + "\"" << std::endl;
            } else {
                std::cout << "Invalid GET command. Format: GET <key>" << std::endl;
            }
        } else if (cmd == "DEL") {
            /// DEL <key>
            if (iss >> key) {
                bool result = storageEngine.erase(key);
                if (!result) {
                    std::cout << "Key not found." << std::endl;
                } else {
                    std::cout << "Key deleted." << std::endl;
                }
            } else {
                std::cout << "Invalid DEL command. Format: DEL <key>" << std::endl;
            }
        } else {
            std::cout << "Unknown command. Supported commands: SET, GET, DEL." << std::endl;
        }
    }
}

int main() {
    startREPL();
    return 0;
}