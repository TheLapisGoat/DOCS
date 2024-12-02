#include <storage_engine.h>
#include <persistent_hashmap.h>

StorageEngine::StorageEngine(std::string directory, int num_bins, INITIALIZATION_MODE mode) : pImpl{std::make_unique<PersistentHashMap>(directory, num_bins, static_cast<PersistentHashMap::INITIALIZATION_MODE>(mode))} {}

StorageEngine::~StorageEngine() = default;

void StorageEngine::insert(std::string key, std::string value) {
    pImpl->insert(key, value);
}

bool StorageEngine::erase(std::string key) {
    return pImpl->erase(key);
}

std::optional<std::string> StorageEngine::get(std::string key) {
    return pImpl->get(key);
}
