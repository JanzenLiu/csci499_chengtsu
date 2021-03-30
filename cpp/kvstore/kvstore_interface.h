#ifndef CSCI499_CHENGTSU_KVSTORE_INTERFACE_H
#define CSCI499_CHENGTSU_KVSTORE_INTERFACE_H

#include <string>
#include <vector>

class KVStoreInterface {
 public:
  virtual ~KVStoreInterface() {};

  // Adds a value under the key, and returns true if the put was successful.
  virtual bool Put(const std::string& key, const std::string& value) = 0;

  // Returns all previously stored values under the key.
  virtual std::vector<std::string> Get(const std::string& key) const = 0;

  // Deletes all previously stored values under the key and
  // returns true if the key existed and the delete was successful.
  virtual bool Remove(const std::string& key) = 0;
};

#endif //CSCI499_CHENGTSU_KVSTORE_INTERFACE_H
