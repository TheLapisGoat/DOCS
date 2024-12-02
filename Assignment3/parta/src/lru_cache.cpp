#include <lru_cache.h>
#include <mutex>

void SpinLock::lock() {
    while (flag.test_and_set(std::memory_order_acquire));
}

void SpinLock::unlock() {
    flag.clear(std::memory_order_release);
}

LRUCacheElement::LRUCacheElement(std::string key, std::string value) : key(key), value(value) {}

LRUCacheSegment::LRUCacheSegment(int capacity) : capacity(capacity) {}

void LRUCacheSegment::put(std::string key, std::string value) {
    /// Ensure that only one thread can access the cache segment at a time
    std::lock_guard<SpinLock> guard(lock);

    /**
     * Check if the key already exists in the cache segment. If it does, update the value and move the element to the front of the list.
     * Otherwise, insert a new element at the front of the list and evict the least recently used element if the capacity is exceeded.
     */
    auto it = lruMap.find(key);
    if (it != lruMap.end()) {
        /// Update existing element
        auto element = it->second;
        element->value = value;
        lruList.remove(element);
        lruList.push_front(element);
    } else {
        /// Insert new element
        auto element = std::make_shared<LRUCacheElement>(key, value);
        lruList.push_front(element);
        lruMap[key] = element;

        /// Evict least recently used element if capacity is exceeded
        if (lruList.size() > capacity) {
            auto element = lruList.back();
            lruList.pop_back();
            lruMap.erase(element->key);
        }
    }
}

std::optional<std::string> LRUCacheSegment::get(std::string key) {
    /// Ensure that only one thread can access the cache segment at a time
    std::lock_guard<SpinLock> guard(lock);

    /// Check if the key exists in the cache segment. If it does, move the element to the front of the list.
    /// Otherwise, return an empty optional.
    auto it = lruMap.find(key);
    if (it != lruMap.end()) {
        auto element = it->second;
        lruList.remove(element);
        lruList.push_front(element);
        return element->value;
    } else {
        return std::nullopt;
    }
}

void LRUCacheSegment::remove(std::string key) {
    /// Ensure that only one thread can access the cache segment at a time
    std::lock_guard<SpinLock> guard(lock);

    /// Check if the key exists in the cache segment. If it does, remove the element from the list and map.
    auto it = lruMap.find(key);
    if (it != lruMap.end()) {
        auto element = it->second;
        lruList.remove(element);
        lruMap.erase(key);
    }
}