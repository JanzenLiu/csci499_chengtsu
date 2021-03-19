#ifndef CSCI499_CHENGTSU_KVSTORE_PERSISTENCE_H
#define CSCI499_CHENGTSU_KVSTORE_PERSISTENCE_H

#include <iostream>
#include <memory>
#include <string>
#include <variant>

namespace kvstore {

enum ChangeType : char { kPut, kRemove, kClear };

struct PutParams {
  std::string key;
  std::string value;
};
struct RemoveParams {
  std::string key;
};
struct ClearParams {};

struct Change {
  ChangeType type;
  std::variant<PutParams, RemoveParams, ClearParams> params;
};

class PersistenceHelper {
 public:
  PersistenceHelper();

  PersistenceHelper(const std::string &filename);

  PersistenceHelper(const std::shared_ptr<std::iostream> &io);

  bool DumpPut(const std::string& key, const std::string& value);

  bool DumpRemove(const std::string& key);

  bool DumpClear();

  bool LoadChange(Change& change);

 private:
  bool DumpString(const std::string& str);

  bool LoadString(std::string& str);

  std::shared_ptr<std::iostream> log_;

  bool log_open_;
};

}  // namespace kvstore

#endif //CSCI499_CHENGTSU_KVSTORE_PERSISTENCE_H
