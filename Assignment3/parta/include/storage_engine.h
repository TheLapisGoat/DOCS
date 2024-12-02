#pragma once
#include <string>
#include <memory>
#include <optional>

class PersistentHashMap;

/**
 * @class StorageEngine
 * @brief A class to represent a disk-persistent key-value store.
 * 
 * This class provides a class to represent a disk-persistent key-value store. The key-value store stores key-value pairs in a persistent hash map.
 * The class provides an interface to insert, erase, and get key-value pairs from the key-value store.
 */
class StorageEngine {
public:
    /**
     * @enum INITIALIZATION_MODE
     * @brief An enumeration to specify the initialization mode of the database.
     * 
     * This enumeration specifies the initialization mode of the database. The database can be created or opened.
     */
    enum class INITIALIZATION_MODE {
        /**
         * @brief Create a new database.
         */
        CREATE = 0,

        /**
         * @brief Opens an existing database.
         */
        OPEN = 1
    };

    /**
     * @brief Constructs a new StorageEngine object.
     * 
     * @param directory The directory where the key-value pairs are stored.
     * @param num_bins The number of bins in the hash map.
     * @param mode The initialization mode of the database.
     */
    StorageEngine(std::string directory, int num_bins, INITIALIZATION_MODE mode = INITIALIZATION_MODE::CREATE);

    /**
     * @brief Destroys the StorageEngine object.
     */
    ~StorageEngine();

    /**
     * @brief Inserts a key-value pair into the key-value store.
     * 
     * @param key The key of the key-value pair.
     * @param value The value of the key-value pair.
     */
    void insert(std::string key, std::string value);

    /**
     * @brief Erases a key-value pair from the key-value store.
     * 
     * @param key The key of the key-value pair to erase.
     * @return True if the key-value pair was erased, false otherwise.
     */
    bool erase(std::string key);

    /**
     * @brief Retrieves the value of a key from the key-value store.
     * 
     * @param key The key to retrieve the value for.
     * @return The value of the key, if it exists.
     */
    std::optional<std::string> get(std::string key);

private:
    /**
     * @brief The implementation of the StorageEngine class.
     * 
     * This is a pointer to the implementation of the StorageEngine class. This is used to hide the implementation details of the StorageEngine class from the user.
     */
    std::unique_ptr<PersistentHashMap> pImpl;
};