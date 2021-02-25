#ifndef CSCI499_CHENGTSU_KVSTORE_SERVICE_H
#define CSCI499_CHENGTSU_KVSTORE_SERVICE_H

#include <grpcpp/grpcpp.h>

#include "kvstore.grpc.pb.h"
#include "kvstore/kvstore.h"

// A key-value store gRPC service who accepts incoming requests, interacts
// with the backend storage system, and responds to the remote callers.
class KeyValueStoreServiceImpl final : public kvstore::KeyValueStore::Service {
 public:
  KeyValueStoreServiceImpl(): store_() {}

  // gRPC interface to add a value under a key.
  grpc::Status put(grpc::ServerContext* context,
                   const kvstore::PutRequest* request,
                   kvstore::PutReply* response);

  // gRPC interface to get all previously stored values under given keys.
  grpc::Status get(grpc::ServerContext* context,
                   grpc::ServerReaderWriter<kvstore::GetReply,
                                            kvstore::GetRequest>* stream);

  // gRPC interface to remove all previously stored values under a key.
  grpc::Status remove(grpc::ServerContext* context,
                      const kvstore::RemoveRequest* request,
                      kvstore::RemoveReply* response);
 private:
  KVStore store_;
};

typedef KeyValueStoreServiceImpl KVStoreService;

#endif //CSCI499_CHENGTSU_KVSTORE_SERVICE_H
