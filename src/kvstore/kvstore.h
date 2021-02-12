#ifndef CSCI499_CHENGTSU_KVSTORE_H
#define CSCI499_CHENGTSU_KVSTORE_H

#include <initializer_list>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// A Concurrent Hashmap storing multiple string values
// for each unique string key.
class KVStore {
 public:
  KVStore();

  // Constructs a KVStore with given key-value pairs.
  // If there are duplicate keys, for each unique key,
  // the last occurrence counts.
  KVStore(std::initializer_list<
      std::pair<std::string, std::vector<std::string>>
  > args);

  // Returns all previously stored values under the key.
  // A copy instead of a reference is returned here (unlike
  // std::unordered_map), to make sure the user can only add
  // values to a key through the `Put()` interface, which is
  // guaranteed to be thread-safe.
  std::vector<std::string> Get(std::string key);

  // Adds a value under the key, and returns true if it is a new key.
  bool Put(std::string key, std::string value);

  // Deletes all previously stored values under the key and
  // returns true if the key exists in the KVStore.
  bool Remove(std::string key);

  // Deletes all keys and values.
  void Clear();

  // Returns the number of keys in the KVStore.
  size_t Size() const noexcept;

  // Returns true if the KVStore is empty;
  bool Empty() const noexcept;

  // Prints all keys and values stored the KVStore.
  void Print();

 private:
  std::unordered_map<std::string, std::vector<std::string>> map_;
  std::shared_mutex mutex_;
};

#endif //CSCI499_CHENGTSU_KVSTORE_H