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

  // Constructs a KVStore with a given file.
  // If the file already exists, the KVStore will load
  // changes from the file upon construction; if the file
  // is corrupted (for example, due to interruption from
  // last run), the KVStore will automatically detect where
  // the corrupted content starts and automatically remove
  // all content from that point from the file.
  // The KVStore will be associated with the file, so that
  // every change (Put/Remove/Clear) made to the KVStore
  // will be immediately appended to the file.
  KVStore(const std::string& filename);

  // Returns all previously stored values under the key.
  // A copy instead of a reference is returned here (unlike
  // std::unordered_map), to make sure the user can only add
  // values to a key through the `Put()` interface, which is
  // guaranteed to be thread-safe.
  std::vector<std::string> Get(const std::string& key) const;

  // Note that if an interruption occurs when writing to the file, we don't
  // handle it immediately, so will end up with a corrupted file. However,
  // in the constructor, when reloading the file, the KVStore will
  // automatically detect corrupted data and discard them. So all data will
  // stay intact except the last operation. Since the program is terminated
  // when doing put/remove/clear, it will never return acknowledgment to the
  // user, so the user shouldn't assume the last operation is done.

  // Adds a value under the key, and returns true if the put
  // was successful.
  bool Put(const std::string& key, const std::string& value);

  // Deletes all previously stored values under the key and
  // returns true if the key existed and the delete was successful.
  bool Remove(const std::string& key);

  // Deletes all keys and values, and returns true if the
  // clear was successful.
  bool Clear();

  // Returns the number of keys in the KVStore.
  size_t Size() const noexcept;

  // Returns true if the KVStore is empty;
  bool Empty() const noexcept;

  // Prints all keys and values stored the KVStore.
  void Print()  const;

 private:
  // Deletes all previously stored values under the key, sets
  // `key_existed` to true if the key existed, and returns true
  // if the key existed and the delete was successful.
  bool Remove(const std::string& key, bool& key_existed);

  // Loads the next change from the given file stream and
  // returns true on success.
  // Assuming the caller will always make sure EOF has not
  // been reached before calling this function, we consider
  // all failures are caused by corrupted data and will
  // truncate all trailing content from that point afterwards.
  bool LoadChange(std::ifstream& infile);

  // Loads a string from the given file stream into `str`,
  // and returns true on success.
  bool LoadString(std::ifstream& infile, std::string& str);

  // Dumps the given string to the associated file stream
  // `log_`, and returns true on success.
  bool DumpString(const std::string& str);

  void TruncateTrailingContent(int start_pos);

  // Hash map that stores the actual data.
  std::unordered_map<std::string, std::vector<std::string>> map_;
  // Associated file stream to dump all changes into.
  std::optional<std::ofstream> log_;
  // Read-write lock to enforce thread-safety.
  mutable std::shared_mutex mutex_;
};

#endif //CSCI499_CHENGTSU_KVSTORE_H
