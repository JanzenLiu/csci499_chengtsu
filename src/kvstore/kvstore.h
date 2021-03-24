#ifndef CSCI499_CHENGTSU_KVSTORE_H
#define CSCI499_CHENGTSU_KVSTORE_H

#include "kvstore/kvstore_interface.h"

#include <fstream>
#include <initializer_list>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// A Concurrent Hashmap storing multiple string values
// for each unique string key.
class KVStore : public KVStoreInterface {
 public:
  KVStore();

  // Constructs a KVStore with given key-value pairs.
  // If there are duplicate keys, for each unique key,
  // the last occurrence counts.
  KVStore(std::initializer_list<
      std::pair<std::string, std::vector<std::string>>> args);

  KVStore(const std::string& filename);

  // Returns all previously stored values under the key.
  // A copy instead of a reference is returned here (unlike
  // std::unordered_map), to make sure the user can only add
  // values to a key through the `Put()` interface, which is
  // guaranteed to be thread-safe.
  std::vector<std::string> Get(const std::string& key) const;

  // Adds a value under the key, and returns true if the put
  // was successful.
  bool Put(const std::string& key, const std::string& value);

  // Deletes all previously stored values under the key and
  // returns true if the key exists in the KVStore.
  bool Remove(const std::string& key);

  // Deletes all keys and values.
  void Clear();

  // Returns the number of keys in the KVStore.
  size_t Size() const noexcept;

  // Returns true if the KVStore is empty;
  bool Empty() const noexcept;

  // Prints all keys and values stored the KVStore.
  void Print()  const;

 private:
  bool LoadChange(std::ifstream& infile);

  bool LoadString(std::ifstream& infile, std::string& str);

  bool DumpString(const std::string& str);

  // Hash map that stores the actual data.
  std::unordered_map<std::string, std::vector<std::string>> map_;

  std::optional<std::ofstream> log_;
  // Read-write lock to enforce thread-safety.
  mutable std::shared_mutex mutex_;
};

#endif //CSCI499_CHENGTSU_KVSTORE_H
