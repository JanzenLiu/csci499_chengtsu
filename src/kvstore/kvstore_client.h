#ifndef CSCI499_CHENGTSU_KVSTORE_CLIENT_H
#define CSCI499_CHENGTSU_KVSTORE_CLIENT_H

#include <memory>
#include <string>
#include <vector>

#include <grpcpp/grpcpp.h>

#include "kvstore.grpc.pb.h"

class KVStoreClient {
 public:
  KVStoreClient(std::shared_ptr<grpc::Channel> channel)
      : stub_(kvstore::KeyValueStore::NewStub(channel)) {}

  // Adds a value under the key, and returns true/false
  // if the put was successful/failed.
  bool Put(const std::string& key, const std::string& value);

  // Returns all previously stored values under the key.
  std::vector<std::string> Get(const std::string& key);

  // Deletes all previously stored values under the key.
  void Remove(const std::string& key);

 private:
  std::unique_ptr<kvstore::KeyValueStore::Stub> stub_;
};

#endif //CSCI499_CHENGTSU_KVSTORE_CLIENT_H
