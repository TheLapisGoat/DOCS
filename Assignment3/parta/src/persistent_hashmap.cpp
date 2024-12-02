#include <persistent_hashmap.h>
#include <filesystem>
#include <mutex>

BinControl::BinControl(int bin_id, std::string bin_path, int cache_capacity) : bin_id{bin_id}, bin_path{bin_path}, cache{cache_capacity} {}

void BinControl::insert(std::string key, std::string value) {
    /// Ensure that only one thread can write to the bin at a time. This includes writing to the cache and the file.
    std::unique_lock lock(mutex);
    cache.put(key, value);

    std::fstream file(bin_path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + bin_path);
    }

    ///Iterating through the file to find the key if it exists, and mark it as deleted if it does
    while (file.peek() != EOF) {
        /// Storing the starting position of the current entry, to be used for marking the entry as deleted
        std::streampos entryStart = file.tellg();

        int key_length = 0;
        int value_length = 0;
        bool deleted = false;

        file.read(reinterpret_cast<char*>(&key_length), sizeof(int));
        file.read(reinterpret_cast<char*>(&value_length), sizeof(int));
        file.read(reinterpret_cast<char*>(&deleted), sizeof(bool));

        if (file.eof()) break;

        /// Skip this entry if it is marked as deleted or if lengths mismatch, as it is not the key we are looking for
        if (deleted || key_length != key.length()) {
            file.seekg(key_length + value_length, std::ios::cur);
            continue;
        }

        std::string current_key(key_length, '\0');
        file.read(&current_key[0], key_length);

        if (current_key == key) {
            ///If the key is found, mark it as deleted
            file.seekp(static_cast<std::streamoff>(entryStart) + sizeof(int) + sizeof(int), std::ios::beg);

            deleted = true;
            file.write(reinterpret_cast<const char*>(&deleted), sizeof(bool));
            file.seekg(0, std::ios::end);
        }

        ///We don't need to read the value, so we skip it
        file.seekg(value_length, std::ios::cur);
    }

    file.clear();

    ///Write the new key-value pair to the end of the file
    file.seekp(0, std::ios::end);
    int key_length = key.length();
    int value_length = value.length();
    bool deleted = false;

    file.write(reinterpret_cast<const char*>(&key_length), sizeof(int));
    file.write(reinterpret_cast<const char*>(&value_length), sizeof(int));
    file.write(reinterpret_cast<const char*>(&deleted), sizeof(bool));
    file.write(key.c_str(), key_length);
    file.write(value.c_str(), value_length);

    file.close();
}

std::optional<std::string> BinControl::get(std::string key) {
    /// Reader lock to read from the cache and the file, allowing multiple readers at a time
    std::shared_lock lock(mutex);
    auto result = cache.get(key);

    /// If the key is found in the cache, return the value
    if (result) {
        return *result;
    }

    std::fstream file(bin_path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + bin_path);
    }

    ///Iterating through the file to find the key, and if found, updating the cache and returning the value
    while (true) {
        int key_length = 0;
        int value_length = 0;
        bool deleted = false;

        file.read(reinterpret_cast<char*>(&key_length), sizeof(int));
        file.read(reinterpret_cast<char*>(&value_length), sizeof(int));
        file.read(reinterpret_cast<char*>(&deleted), sizeof(bool));

        if (file.eof()) break;

        if (file.fail()) {
            throw std::runtime_error("Error reading from file: " + bin_path);
        }

        /// Skip this entry if it is marked as deleted or if lengths mismatch
        if (deleted || key_length != key.length()) {
            file.seekg(key_length + value_length, std::ios::cur);
            continue;
        }

        std::string current_key(key_length, '\0');
        file.read(&current_key[0], key_length);

        if (file.fail()) {
            throw std::runtime_error("Error reading key from file: " + bin_path);
        }

        if (current_key == key) {
            /// If the key is found, read the value, update the cache, and return the value
            std::string value(value_length, '\0');
            file.read(&value[0], value_length);

            if (file.fail()) {
                throw std::runtime_error("Error reading value from file: " + bin_path);
            }

            file.close();

            cache.put(key, value);

            return value;
        }

        file.seekg(value_length, std::ios::cur);
    }

    file.close();
    return std::nullopt;
}

bool BinControl::erase(std::string key) {
    /// Writer lock to write to the cache and the file, allowing only one writer at a time
    std::unique_lock lock(mutex);

    /// Remove the key from the cache
    cache.remove(key);

    std::fstream file(bin_path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + bin_path);
    }

    ///Iterating through the file to find the key, and if found, mark it as deleted and return true
    while (true) {
        std::streampos entryStart = file.tellg();

        int key_length = 0;
        int value_length = 0;
        bool deleted = false;

        file.read(reinterpret_cast<char*>(&key_length), sizeof(int));
        file.read(reinterpret_cast<char*>(&value_length), sizeof(int));
        file.read(reinterpret_cast<char*>(&deleted), sizeof(bool));

        if (file.eof()) break;

        if (file.fail()) {
            throw std::runtime_error("Error reading from file: " + bin_path);
        }

        /// Skip this entry if it is marked as deleted or if lengths mismatch
        if (deleted || key_length != key.length()) {
            file.seekg(key_length + value_length, std::ios::cur);
            continue;
        }

        std::string current_key(key_length, '\0');
        file.read(&current_key[0], key_length);

        if (file.fail()) {
            throw std::runtime_error("Error reading key from file: " + bin_path);
        }

        if (current_key == key) {
            /// If the key is found, mark it as deleted and return true
            file.seekp(static_cast<std::streamoff>(entryStart) + sizeof(int) + sizeof(int), std::ios::beg);

            deleted = true;
            file.write(reinterpret_cast<const char*>(&deleted), sizeof(bool));

            if (file.fail()) {
                throw std::runtime_error("Error writing to file: " + bin_path);
            }

            file.close();
            return true;
        }

        file.seekg(value_length, std::ios::cur);
    }

    file.close();
    return false;
}

void BinControl::compressFile() {
    /// Writer lock to write to the file, allowing only one writer at a time. Ensures no other thread accesses the file while it is being compressed.
    std::unique_lock lock(mutex);

    std::fstream file(bin_path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + bin_path);
    }

    std::streampos read_pos = 0; ///< The current read position in the file
    std::streampos write_pos = 0; ///< The current write position in the file

    ///Iterating through the file to read each entry, and writing it back to the file if it is not marked as deleted
    while (true) {
        file.seekg(read_pos);

        int key_length = 0, value_length = 0;
        bool deleted = false;

        file.read(reinterpret_cast<char*>(&key_length), sizeof(int));
        file.read(reinterpret_cast<char*>(&value_length), sizeof(int));
        file.read(reinterpret_cast<char*>(&deleted), sizeof(bool));

        if (file.eof()) break;

        if (file.fail()) {
            throw std::runtime_error("Error reading metadata from file: " + bin_path);
        }

        ///If the entry is marked as deleted, skip it
        if (deleted) {
            read_pos = static_cast<std::streamoff>(file.tellg()) + key_length + value_length;
            continue;
        }

        std::string key(key_length, '\0');
        std::string value(value_length, '\0');
        file.read(&key[0], key_length);
        file.read(&value[0], value_length);

        if (file.fail()) {
            throw std::runtime_error("Error reading key/value from file: " + bin_path);
        }

        read_pos = file.tellg();

        ///Writing the non-deleted entry back to the file
        file.seekp(write_pos);

        file.write(reinterpret_cast<const char*>(&key_length), sizeof(int));
        file.write(reinterpret_cast<const char*>(&value_length), sizeof(int));
        file.write(reinterpret_cast<const char*>(&deleted), sizeof(bool));
        file.write(key.data(), key_length);
        file.write(value.data(), value_length);

        if (file.fail()) {
            throw std::runtime_error("Error writing to file: " + bin_path);
        }

        ///Updating the write position to the end of the written entry
        write_pos = file.tellp();
    }
    ///Truncating the file to the final write position
    file.close();
    std::filesystem::resize_file(bin_path, write_pos);
}

/// Checks the bin file for consistency. Reads entries and checks if they are valid. Invalid entries, which may occur towards the end of the file, are removed.
void BinControl::binCheck() {
    /// Writer lock to write to the file, allowing only one writer at a time. Ensures no other thread accesses the file while it is being checked.
    std::unique_lock lock(mutex);

    std::fstream file(bin_path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + bin_path);
    }

    std::streampos read_pos = 0;
    uintmax_t file_size = std::filesystem::file_size(bin_path);

    ///Iterating through the file to read each entry and check if it is valid
    while (true) {
        std::streampos entryStart = file.tellg();

        int key_length = 0;
        int value_length = 0;
        bool deleted = false;

        file.read(reinterpret_cast<char*>(&key_length), sizeof(int));
        file.read(reinterpret_cast<char*>(&value_length), sizeof(int));
        file.read(reinterpret_cast<char*>(&deleted), sizeof(bool));

        if (file.eof() || file.fail() || key_length < 0 || value_length < 0) {
            read_pos = entryStart;
            break;
        }

        ///Checking if the entry fits within the file size
        if (static_cast<std::streamoff>(file.tellg()) + key_length + value_length > file_size) {
            read_pos = entryStart;
            break;
        } else {
            file.seekg(key_length + value_length, std::ios::cur);
        }
    }
    file.close();
    std::filesystem::resize_file(bin_path, read_pos);
}

PersistentHashMap::PersistentHashMap(std::string directory, int num_bins, INITIALIZATION_MODE mode) : directory(directory), num_bins(num_bins) {
    /// Creating bin controls for each bin
    for (int i = 0; i < num_bins; i++) {
        std::string bin_path = directory + "/" + std::to_string(i) + ".bkt";
        bin_controls.push_back(std::make_unique<BinControl>(i, bin_path, LRU_CACHE_CAPACITY));
    }

    std::filesystem::create_directory(directory);

    if (mode == INITIALIZATION_MODE::CREATE) {
        /// If the mode is CREATE, clear the directory and create new bucket files.
        /// Overwrite existing files if they exist.
        for (const auto &entry : std::filesystem::directory_iterator(directory)) {
            std::filesystem::remove(entry);
        }

        for (int i = 0; i < num_bins; i++) {
            std::string filename = directory + "/" + std::to_string(i) + ".bkt";
            std::fstream file {filename, std::ios::in | std::ios::out | std::ios::app};
            file.close();
        }
    } else {
        /// If the mode is OPEN, create new bucket files if they do not exist. Otherwise, check the existing files for consistency.
        for (int i = 0; i < num_bins; i++) {
            std::string filename = directory + "/" + std::to_string(i) + ".bkt";
            if (!std::filesystem::exists(filename)) {
                std::fstream file {filename, std::ios::in | std::ios::out | std::ios::app};
                file.close();
            } else {
                bin_controls[i]->binCheck();
            }
        }
    }

    /// Start the garbage collector thread
    gc_thread = std::thread([this]() {
        runGarbageCollector();
    });
}

PersistentHashMap::~PersistentHashMap() {
    /// Stop the garbage collector thread
    stop_gc = true;
    gc_thread.join();
}

int PersistentHashMap::hash(std::string key) const {
    return std::hash<std::string>{}(key) % num_bins;
}

void PersistentHashMap::insert(std::string key, std::string value) {
    int bucket = hash(key);
    bin_controls[bucket]->insert(key, value);
}

std::optional<std::string> PersistentHashMap::get(std::string key) {
    int bucket = hash(key);
    return bin_controls[bucket]->get(key);
}

bool PersistentHashMap::erase(std::string key) {
    int bucket = hash(key);
    return bin_controls[bucket]->erase(key);
}

void PersistentHashMap::runGarbageCollector() {
    while (!stop_gc) {
        /// Sleep for the garbage collector interval
        std::this_thread::sleep_for(std::chrono::seconds(GARBAGE_COLLECTOR_INTERVAL));

        /// Compress each bin file
        for (auto &bin : bin_controls) {
            try {
                bin->compressFile();
            } catch (const std::runtime_error &e) {
                std::cerr << "Garbage Collection Error: " << e.what() << std::endl;
            }
        }
    }
}