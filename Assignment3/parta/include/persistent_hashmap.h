#pragma once
#include <lru_cache.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <shared_mutex>
#include <thread>

/**
 * @brief Defines the capacity of a LRU cache segment.
 * 
 * If LRU_CACHE_SIZE is defined, the capacity of a LRU cache segment is set to the value of LRU_CACHE_SIZE.
 * Otherwise, the capacity of a LRU cache segment is set to 64.
 */
#ifdef LRU_CACHE_SIZE
int const LRU_CACHE_CAPACITY = LRU_CACHE_SIZE;
#else
int const LRU_CACHE_CAPACITY = 64;
#endif

/**
 * @brief Defines the interval for the garbage collector (in seconds).
 * 
 * If GC_INTERVAL is defined, the interval for the garbage collector is set to the value of GC_INTERVAL (in seconds).
 */
#ifdef GC_INTERVAL
int const GARBAGE_COLLECTOR_INTERVAL = GC_INTERVAL;
#else
int const GARBAGE_COLLECTOR_INTERVAL = 30;
#endif

/**
 * @class BinControl
 * @brief A class to represent a bin in the persistent hash map.
 * 
 * This class provides a class to represent a bin in the persistent hash map. A bin is a file that stores key-value pairs.
 * The class provides methods to insert, erase, and get key-value pairs from the bin. It also provides methods to compress the file and check/fix the file.
 */
class BinControl {
    /**
     * @brief The ID of the bin.
     * 
     * This is a unique identifier for the bin.
     */
    int bin_id {};

    /**
     * @brief The path of the bin.
     * 
     * This is the path of the file that stores the key-value pairs of the bin.
     */
    std::string bin_path {};

    /**
     * @brief A mutex to protect the bin during read and write operations.
     * 
     * This mutex is used to protect the bin during read and write operations. It is a shared mutex, which allows multiple readers or a single writer at a time.
     */
    std::shared_mutex mutex {};

    /**
     * @brief The LRU cache segment used to cache key-value pairs.
     * 
     * This LRU cache segment is used to cache key-value pairs read from the bin. This helps reduce the number of disk reads and improve performance.
     */
    LRUCacheSegment cache {LRU_CACHE_CAPACITY};

public:
    /**
     * @brief Constructs a new BinControl object.
     * 
     * @param bin_id The ID of the bin.
     * @param bin_path The path of the bin.
     * @param cache_capacity The capacity of the LRU cache segment.
     */
    BinControl(int bin_id, std::string bin_path, int cache_capacity);

    /**
     * @brief Inserts a key-value pair into the bin.
     * 
     * @param key The key of the key-value pair.
     * @param value The value of the key-value pair.
     */
    void insert(std::string key, std::string value);

    /**
     * @brief Erases a key-value pair from the bin.
     * 
     * @param key The key of the key-value pair to erase.
     * @return True if the key-value pair was erased, false otherwise.
     */
    bool erase(std::string key);

    /**
     * @brief Retrieves the value of a key from the bin.
     * 
     * @param key The key to retrieve the value for.
     * @return The value of the key, if it exists.
     */
    std::optional<std::string> get(std::string key);

    /**
     * @brief Compresses the bin file.
     * 
     * This method compresses the bin file by removing any deleted key-value pairs and reorganizing the file accordingly.
     */
    void compressFile();

    /**
     * @brief Checks and fixes the bin file.
     * 
     * This method checks the bin file for any inconsistencies and fixes them if found.
     */
    void binCheck();
};

/**
 * @class PersistentHashMap
 * @brief A class to represent a disk-persistent hash map.
 * 
 * This class provides a class to represent a persistent hash map. The persistent hash map stores key-value pairs in bins, which are files that store the key-value pairs.
 * The class provides methods to insert, erase, and get key-value pairs from the hash map. It also provides a garbage collector to compress the bins.
 */
class PersistentHashMap {
    /**
     * @brief The directory where the bins are stored.
     * 
     * This is the directory where the bins are stored. Each bin is a file that stores key-value pairs.
     */
    std::string directory {};

    /**
     * @brief The number of bins in the hash map.
     */
    int num_bins {};

    /**
     * @brief The list of bin controls for each bin.
     * 
     * This list contains the bin controls for each bin in the hash map. A bin control manages the operations on a bin, such as insert, erase, and get.
     */
    std::vector<std::unique_ptr<BinControl>> bin_controls {};

    /**
     * @brief The garbage collector thread.
     * 
     * A reference to the garbage collector thread that runs in the background to compress the bins.
     */
    std::thread gc_thread {};

    /**
     * @brief A flag to stop the garbage collector.
     * 
     * This flag is used to stop the garbage collector thread when the hash map is destroyed.
     */
    std::atomic<bool> stop_gc {false};

    /**
     * @brief Runs the garbage collector.
     * 
     * This method runs the garbage collector in the background. The garbage collector compresses the bins by removing any deleted key-value pairs and reorganizing the files.
     */
    void runGarbageCollector();

    /**
     * @brief Computes the hash of a key.
     * 
     * @param key The key to compute the hash for.
     * @return The hash value of the key.
     */
    int hash(std::string key) const;

public:
    /**
     * @enum INITIALIZATION_MODE
     * @brief An enumeration to specify the initialization mode of the persistent hash map.
     * 
     * This enumeration specifies the initialization mode of the persistent hash map. The persistent hash map can be created or opened.
     */
    enum class INITIALIZATION_MODE {
        /**
         * @brief Create a new persistent hash map.
         */
        CREATE = 0,

        /**
         * @brief Opens an existing persistent hash map.
         */
        OPEN = 1
    };

    /**
     * @brief Constructs a new PersistentHashMap object.
     * 
     * @param directory The directory where the bins are stored.
     * @param num_bins The number of bins in the hash map.
     * @param mode The initialization mode of the hash map.
     */
    PersistentHashMap(std::string directory, int num_bins, INITIALIZATION_MODE mode = INITIALIZATION_MODE::CREATE);

    /**
     * @brief Destroys the PersistentHashMap object.
     */
    ~PersistentHashMap();

    /**
     * @brief Inserts a key-value pair into the hash map.
     * 
     * @param key The key of the key-value pair.
     * @param value The value of the key-value pair.
     */
    void insert(std::string key, std::string value);

    /**
     * @brief Erases a key-value pair from the hash map.
     * 
     * @param key The key of the key-value pair to erase.
     * @return True if the key-value pair was erased, false otherwise.
     */
    bool erase(std::string key);

    /**
     * @brief Retrieves the value of a key from the hash map.
     * 
     * @param key The key to retrieve the value for.
     * @return The value of the key, if it exists.
     */
    std::optional<std::string> get(std::string key);
};
