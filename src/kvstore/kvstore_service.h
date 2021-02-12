#ifndef CSCI499_CHENGTSU_KVSTORE_SERVICE_H
#define CSCI499_CHENGTSU_KVSTORE_SERVICE_H

#include <grpcpp/grpcpp.h>

#include "kvstore.grpc.pb.h"
#include "kvstore.h"

class KeyValueStoreServiceImpl final : public kvstore::KeyValueStore::Service {
 public:
  grpc::Status put(grpc::ServerContext* context,
                   const kvstore::PutRequest* request,
                   kvstore::PutReply* response);
  grpc::Status get(grpc::ServerContext* context,
                   grpc::ServerReaderWriter<kvstore::GetReply,
                                            kvstore::GetRequest>* stream);
  grpc::Status remove(grpc::ServerContext* context,
                      const kvstore::RemoveRequest* request,
                      kvstore::RemoveReply* response);
 private:
  KVStore store_;
};

typedef KeyValueStoreServiceImpl KVStoreService;

#endif //CSCI499_CHENGTSU_KVSTORE_SERVICE_H
