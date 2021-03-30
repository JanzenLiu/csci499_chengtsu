#ifndef CSCI499_CHENGTSU_KVSTORE_CLIENT_H
#define CSCI499_CHENGTSU_KVSTORE_CLIENT_H

#include "kvstore/kvstore_interface.h"

#include <memory>
#include <string>
#include <vector>

#include <grpcpp/grpcpp.h>

#include "kvstore.grpc.pb.h"

// A client to make RPC to the remote key-value store gRPC service.
class KVStoreClient : public KVStoreInterface {
 public:
  KVStoreClient(std::shared_ptr<grpc::Channel> channel)
      : stub_(kvstore::KeyValueStore::NewStub(channel)) {}

  // Adds a value under the key, and returns true
  // if the put was successful.
  bool Put(const std::string& key, const std::string& value);

  // Returns all previously stored values under the key.
  std::vector<std::string> Get(const std::string& key) const;

  // Deletes all previously stored values under the key and
  // returns true if the key existed and the delete was successful.
  bool Remove(const std::string& key);

 private:
  // Stub to make the actual RPC.
  mutable std::unique_ptr<kvstore::KeyValueStore::Stub> stub_;
};

#endif //CSCI499_CHENGTSU_KVSTORE_CLIENT_H
