#include <storage_engine.h>
#include <gtest/gtest.h>
#include <random>

std::string generateRandomString(size_t length, int ascii_start = 0, int ascii_end = 255) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(ascii_start, ascii_end);

    std::string randomString;
    for (size_t i = 0; i < length; ++i) {
        randomString += static_cast<char>(dist(gen));
    }
    return randomString;
}

class StorageEngineTest : public ::testing::Test {
protected:
    StorageEngineTest() : storageEngine("test_data", 512, StorageEngine::INITIALIZATION_MODE::CREATE) {}

    void SetUp() override {
    }

    void TearDown() override {
    }

    StorageEngine storageEngine;
};

TEST_F(StorageEngineTest, SetAndGetLargeSequential) {
    int const numEntries = 250000;
    int const keyLength = 10;
    int const valueLength = 20;
    int const progressInterval = numEntries / 100;

    std::unordered_map<std::string, std::string> data;

    for (int i = 0; i < numEntries; ++i) {
        std::string key = generateRandomString(keyLength);
        std::string value = generateRandomString(valueLength);
        data[key] = value;
        storageEngine.insert(key, value);

        if ((i + 1) % progressInterval == 0) {
            std::cout << "Insertion Progress: " << ((i + 1) * 100 / numEntries) << "%\r" << std::flush;
        }
    }
    std::cout << "Insertion Completed!" << std::endl;

    int verifiedCount = 0;
    for (const auto& [key, value] : data) {
        EXPECT_EQ(storageEngine.get(key), value);

        if (++verifiedCount % progressInterval == 0) {
            std::cout << "Verification Progress: " << (verifiedCount * 100 / numEntries) << "%\r" << std::flush;
        }
    }
    std::cout << "Verification Completed!                          " << std::endl;
}

TEST_F(StorageEngineTest, SetAndGetSmallSequential) {
    const int numEntries = 1000;         // Number of predefined keys
    const int numRandomOperations = 500000; // Number of random operations
    const int progressInterval = numRandomOperations / 100; // Progress interval for updates

    std::vector<std::pair<std::string, std::string>> data;

    // Predefine keys and values
    for (int i = 0; i < numEntries; ++i) {
        std::string key = "key" + std::to_string(i);
        std::string value = "value" + std::to_string(i);
        data.emplace_back(key, value);
    }

    // Insert predefined keys and values
    for (const auto& [key, value] : data) {
        storageEngine.insert(key, value);
    }
    std::cout << "Insertion Completed!" << std::endl;

    // Randomized read/write operations
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, numEntries - 1);
    std::uniform_int_distribution<> operationDist(0, 1); // 0 for get, 1 for set

    for (int i = 0; i < numRandomOperations; ++i) {
        int randomIndex = dist(gen);
        const auto& [key, oldValue] = data[randomIndex];

        if (operationDist(gen) == 0) {
            // Perform a get operation
            EXPECT_EQ(storageEngine.get(key), oldValue);
        } else {
            // Perform a set operation with a new value
            std::string newValue = "new_value" + std::to_string(i);
            storageEngine.insert(key, newValue);
            data[randomIndex].second = newValue; // Update the local data for verification
        }

        // Display progress
        if ((i + 1) % progressInterval == 0) {
            std::cout << "Random Operations Progress: " << ((i + 1) * 100 / numRandomOperations) << "%\r" << std::flush;
        }
    }
    std::cout << "Random Operations Completed!" << std::endl;

    // Final verification
    for (const auto& [key, value] : data) {
        EXPECT_EQ(storageEngine.get(key), value);
    }
    std::cout << "Final Verification Completed!                           " << std::endl;
}

TEST_F(StorageEngineTest, SetAndGetSingleKey) {
    const std::string key = "frequently_accessed_key";
    int const numSetOperations = 50000;
    int const setProgessInterval = numSetOperations / 100;
    int const numGetOperations = 500000;
    int const getProgressInterval = numGetOperations / 100;
    int const numGetSetOperations = 100000;
    int const getSetProgressInterval = numGetSetOperations / 100;

    // Initial value
    std::string value = "initial_value";
    storageEngine.insert(key, value);

    // Set the key repeatedly
    for (int i = 0; i < numSetOperations; ++i) {
        // Update the key with a new value
        value = "value_" + std::to_string(i);
        storageEngine.insert(key, value);

        // Display progress
        if ((i + 1) % setProgessInterval == 0) {
            std::cout << "Set Progress: " << ((i + 1) * 100 / numSetOperations) << "%\r" << std::flush;
        }
    }
    std::cout << "Set Test Completed!" << std::endl;

    // Retrieve the key repeatedly
    for (int i = 0; i < numGetOperations; ++i) {
        // Retrieve the key and verify the value
        EXPECT_EQ(storageEngine.get(key), value);

        // Display progress
        if ((i + 1) % getProgressInterval == 0) {
            std::cout << "Get Progress: " << ((i + 1) * 100 / numGetOperations) << "%\r" << std::flush;
        }
    }
    std::cout << "Get Test Completed!" << std::endl;

    // Set and retrieve the key repeatedly
    for (int i = 0; i < numGetSetOperations; ++i) {
        // Update the key with a new value
        value = "value_" + std::to_string(i);
        storageEngine.insert(key, value);

        // Retrieve the key and verify the value
        EXPECT_EQ(storageEngine.get(key), value);

        // Display progress
        if ((i + 1) % getSetProgressInterval == 0) {
            std::cout << "Set and Get Progress: " << ((i + 1) * 100 / numGetSetOperations) << "%\r" << std::flush;
        }
    }

    std::cout << "Set and Get Test Completed!                            " << std::endl;
}

TEST_F(StorageEngineTest, GetAndSetAndDelRandomSequential) {
    const int numKeys = 5000;             // Number of keys to generate
    const int numOperations = 500000;     // Total random operations
    const int progressInterval = numOperations / 100; // Progress update interval

    std::unordered_map<std::string, std::string> keyValueStore; // Tracks expected values
    std::vector<std::string> keys; // Stores the generated keys

    // Generate keys and insert initial values
    for (int i = 0; i < numKeys; ++i) {
        std::string key = "key" + std::to_string(i);
        std::string value = "value" + std::to_string(i);
        keys.push_back(key);
        keyValueStore[key] = value;
        storageEngine.insert(key, value);
    }
    std::cout << "Initial Insertions Completed!" << std::endl;

    // Randomized operations
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> keyDist(0, numKeys - 1);
    std::uniform_int_distribution<> operationDist(0, 2); // 0 for get, 1 for set, 2 for delete

    for (int i = 0; i < numOperations; ++i) {
        int randomIndex = keyDist(gen);
        std::string key = keys[randomIndex];
        int operation = operationDist(gen);

        if (operation == 0) {
            // Perform a get operation
            if (keyValueStore.count(key)) {
                EXPECT_EQ(storageEngine.get(key), keyValueStore[key]);
            } else {
                EXPECT_EQ(storageEngine.get(key), std::nullopt);
            }
        } else if (operation == 1) {
            // Perform a set operation
            std::string newValue = "new_value" + std::to_string(i);
            keyValueStore[key] = newValue;
            storageEngine.insert(key, newValue);
        } else if (operation == 2) {
            // Perform a delete operation
            if (keyValueStore.count(key)) {
                EXPECT_EQ(storageEngine.erase(key), true);
                keyValueStore.erase(key);
            } else {
                EXPECT_EQ(storageEngine.erase(key), false);
            }
        }

        // Display progress
        if ((i + 1) % progressInterval == 0) {
            std::cout << "Random Operations Progress: " << ((i + 1) * 100 / numOperations) << "%\r" << std::flush;
        }
    }
    std::cout << "Random Operations Completed!                          " << std::endl;

    // Final verification
    for (const auto& key : keys) {
        if (keyValueStore.count(key)) {
            EXPECT_EQ(storageEngine.get(key), keyValueStore[key]);
        } else {
            EXPECT_EQ(storageEngine.get(key), std::nullopt);
        }
    }
    std::cout << "Final Verification Completed!" << std::endl;
}

#include <thread>

TEST_F(StorageEngineTest, SetAndGetLargeConcurrent) {
    const int numEntries = 250000;      // Total number of entries
    const int numThreads = 8;          // Number of threads
    const int entriesPerThread = numEntries / numThreads; // Entries each thread handles
    const int keyLength = 10;
    const int valueLength = 20;
    const int progressInterval = numEntries / 100; // Progress for all threads

    std::vector<std::thread> threads;
    std::unordered_map<std::string, std::string> data; // Shared data for validation
    std::mutex dataMutex;                              // Mutex to protect shared data

    // Generate random strings
    auto generateRandomData = [&]() {
        std::vector<std::pair<std::string, std::string>> localData;
        for (int i = 0; i < entriesPerThread; ++i) {
            std::string key = generateRandomString(keyLength);
            std::string value = generateRandomString(valueLength);
            localData.emplace_back(key, value);
        }
        return localData;
    };

    // Thread function for insertion
    auto insertTask = [&](int threadId) {
        auto localData = generateRandomData();
        for (const auto& [key, value] : localData) {
            {
                std::lock_guard<std::mutex> lock(dataMutex);
                data[key] = value;
                storageEngine.insert(key, value);
            }
        }
    };

    // Thread function for verification
    auto verifyTask = [&](int threadId) {
        int verifiedCount = 0;
        for (const auto& [key, value] : data) {
            EXPECT_EQ(storageEngine.get(key), value);
            if (++verifiedCount % progressInterval == 0) {
                std::cout << "Verification Progress: " << (verifiedCount * 100 / numEntries) << "%\r" << std::flush;
            }
        }
    };

    // Launch threads for insertion
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(insertTask, i);
    }

    // Join all threads after insertion
    for (auto& thread : threads) {
        thread.join();
    }
    std::cout << "Insertion Completed!" << std::endl;

    threads.clear(); // Clear thread vector for reuse

    // Launch threads for verification
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(verifyTask, i);
    }

    // Join all threads after verification
    for (auto& thread : threads) {
        thread.join();
    }
    std::cout << "Verification Completed!                          " << std::endl;
}

TEST_F(StorageEngineTest, SetAndGetSmallConcurrent) {
    const int numEntries = 1000;         // Number of predefined keys
    const int numRandomOperations = 500000; // Number of random operations
    const int numThreads = 8;             // Number of threads
    const int progressInterval = numRandomOperations / 100; // Progress interval for updates

    std::vector<std::pair<std::string, std::string>> data;
    std::mutex dataMutex; // Mutex to protect shared data map

    // Predefine keys and values
    for (int i = 0; i < numEntries; ++i) {
        std::string key = "key" + std::to_string(i);
        std::string value = "value" + std::to_string(i);
        data.emplace_back(key, value);
    }

    // Insert predefined keys and values
    auto insertTask = [&](int threadId) {
        for (const auto& [key, value] : data) {
            storageEngine.insert(key, value);
        }
    };

    // Randomized read/write operations
    auto randomOperationsTask = [&](int threadId) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, numEntries - 1);
        std::uniform_int_distribution<> operationDist(0, 1); // 0 for get, 1 for set

        for (int i = 0; i < numRandomOperations / numThreads; ++i) {
            int randomIndex = dist(gen);

            {
                std::lock_guard<std::mutex> lock(dataMutex);
                const auto& [key, oldValue] = data[randomIndex];

                if (operationDist(gen) == 0) {
                    // Perform a get operation
                    EXPECT_EQ(storageEngine.get(key), oldValue);
                } else {
                    // Perform a set operation with a new value
                    std::string newValue = "new_value" + std::to_string(i);
                    storageEngine.insert(key, newValue);
                    data[randomIndex].second = newValue;
                }
            }

            // Display progress
            if ((i + 1) % progressInterval == 0) {
                std::cout << "Random Operations Progress: " << ((i + 1) * 100 / numRandomOperations) << "%\r" << std::flush;
            }
        }
    };

    // Launch threads for insertion
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(insertTask, i);
    }

    // Join all threads after insertion
    for (auto& thread : threads) {
        thread.join();
    }
    std::cout << "Insertion Completed!" << std::endl;

    threads.clear(); // Clear thread vector for reuse

    // Launch threads for random operations (get/set)
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(randomOperationsTask, i);
    }

    // Join all threads after random operations
    for (auto& thread : threads) {
        thread.join();
    }
    std::cout << "Random Operations Completed!" << std::endl;

    // Final verification
    for (const auto& [key, value] : data) {
        EXPECT_EQ(storageEngine.get(key), value);
    }
    std::cout << "Final Verification Completed!" << std::endl;
}