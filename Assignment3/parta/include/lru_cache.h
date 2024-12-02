#pragma once
#include <list>
#include <unordered_map>
#include <string>
#include <atomic>
#include <memory>
#include <optional>

/**
 * @class SpinLock
 * @brief A simple spin lock implementation using atomic_flag.
 * 
 * This class provides a simple spin lock implementation using atomic_flag.
 */
class SpinLock {
    /**
     * @brief The atomic flag used for locking.
     */
    std::atomic_flag flag {ATOMIC_FLAG_INIT};

public:
    /**
     * @brief Locks the spin lock.
     */
    void lock();
    
    /**
     * @brief Unlocks the spin lock.
     */
    void unlock();
};

/**
 * @class LRUCacheElement
 * @brief A class to represent an element in the LRU cache.
 * 
 * This class provides a class to represent an element in the LRU cache.
 */
struct LRUCacheElement {
    /**
     * @brief The key of the element.
     */
    std::string key {};

    /**
     * @brief The value of the element.
     */
    std::string value {};

    /**
     * @brief Constructs a new LRUCacheElement object.
     * 
     * @param key The key of the element.
     * @param value The value of the element.
     */
    LRUCacheElement(std::string key, std::string value);
};

/**
 * @class LRUCacheSegment
 * @brief A class to represent a segment in the LRU cache.
 * 
 * This class provides a class to represent a segment in the LRU cache.
 */
class LRUCacheSegment {
    /**
     * @brief The list of elements in the LRU cache.
     * 
     * This list contains the elements in the LRU cache in the order of their access, with the most recently accessed element at the front.
     */
    std::list<std::shared_ptr<LRUCacheElement>> lruList {};

    /**
     * @brief The map of elements in the LRU cache.
     * 
     * This map contains the elements in the LRU cache, with the key being the key of the element and the value being a shared pointer to the element.
     */
    std::unordered_map<std::string, std::shared_ptr<LRUCacheElement>> lruMap {};

    /**
     * @brief The capacity of the LRU cache segment.
     * 
     * This is the maximum number of elements that the LRU cache segment can hold.
     */
    int capacity {};

    /**
     * @brief The spin lock used for synchronization.
     */
    SpinLock lock {};

public:
    /**
     * @brief Constructs a new LRUCacheSegment object.
     * 
     * @param capacity The capacity of the LRU cache segment.
     */
    LRUCacheSegment(int capacity);

    /**
     * @brief Inserts a new element into the LRU cache segment.
     * 
     * @param key The key of the element.
     * @param value The value of the element.
     */
    void put(std::string key, std::string value);

    /**
     * @brief Retrieves the value of an element from the LRU cache segment.
     * 
     * @param key The key of the element.
     * @return The value of the element, if it exists.
     */
    std::optional<std::string> get(std::string key);

    /**
     * @brief Removes an element from the LRU cache segment.
     * 
     * @param key The key of the element.
     */
    void remove(std::string key);
};
