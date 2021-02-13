#include "kvstore.h"

#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

using std::initializer_list;
using std::pair;
using std::string;
using std::vector;

KVStore::KVStore() : map_(), mutex_() {}

KVStore::KVStore(initializer_list<pair<string, vector<string>>> args)
    : map_(), mutex_() {
  for (auto& p : args) {
    map_[p.first] = p.second;
  }
}

vector<string> KVStore::Get(const string& key) const {
  // A read-write lock is needed here to avoid deleted
  // or changed iterator.
  std::shared_lock<std::shared_mutex> lock(mutex_);
  auto iter = map_.find(key);
  if (iter != map_.end()) {
    return iter->second;
  }
  return {};
}

bool KVStore::Put(const string& key, const string& value) {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  auto iter = map_.find(key);
  if (iter != map_.end()) {  // The key already exists.
    iter->second.push_back(value);
    return false;
  }
  map_[key] = {value};
  return true;
}

bool KVStore::Remove(const string& key) {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  return map_.erase(key);
}

void KVStore::Clear() {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  map_.clear();
}

size_t KVStore::Size() const noexcept {
  return map_.size();
}

bool KVStore::Empty() const noexcept {
  return map_.empty();
}

void KVStore::Print() const {
  // A read-write lock is needed here to avoid deleted
  // or changed iterator.
  std::shared_lock<std::shared_mutex> lock(mutex_);
  for (auto iter = map_.begin(); iter != map_.end(); ++iter) {
    std::cout << iter->first << ": [ ";
    for (auto value : iter->second) {
      std::cout << value << " ";
    }
    std::cout << "]" << std::endl;
  }
}
