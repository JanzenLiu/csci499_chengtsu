#ifndef CSCI499_CHENGTSU_FAZ_SERVICE_H
#define CSCI499_CHENGTSU_FAZ_SERVICE_H

#include <memory>
#include <unordered_map>

#include <grpcpp/grpcpp.h>

#include "faz.grpc.pb.h"
#include "kvstore/kvstore.h"
#include "kvstore/kvstore_client.h"
#include "kvstore/kvstore_interface.h"

// Function format the FazService accepts.
//
// @param in: Request payload containing the information needed
//   by the specific function. The format depends on the the
//   specific function.
// @param out: Response payload containing the detailed information
//   the function would like to return to the remote caller. The
//   format depends on the specific function.
// @param kvstore: kVStore client through which the function can
//   interact with the KVStore to retrieve or update data.
// @return: gRPC Status indicating success or failure of the
//    function call, and in the case of failure, pointing out the
//    error type and carrying some error message.
using FazFunc =  std::function<grpc::Status(
    const google::protobuf::Any* in, google::protobuf::Any* out,
    KVStoreInterface* kvstore)>;

// A Function-as-a-Service (FaaS) service who executes a registered
// function f when receiving an event that matches an event type e
// hooked with f.
class FazServiceImpl final : public faz::FazService::Service {
 public:
  FazServiceImpl(std::shared_ptr<grpc::Channel> channel)
      : registered_funcs_(), kvstore_(new KVStoreClient(channel)) {}

  // gRPC interface to register a function with an associated event
  // type for future execution by Faz.
  grpc::Status hook(grpc::ServerContext* context,
                    const faz::HookRequest* request,
                    faz::HookReply* response);

  // gRPC interface to unregister a event type and the associated function.
  grpc::Status unhook(grpc::ServerContext* context,
                      const faz::UnhookRequest* request,
                      faz::UnhookReply* response);

  // gRPC interface to process an arriving event with an arbitrary
  // message payload.
  grpc::Status event(grpc::ServerContext* context,
                     const faz::EventRequest* request,
                     faz::EventReply* response);
 private:
  // Predefined table of known functions that maps a function name
  // to the actual function.
  static const std::unordered_map<std::string, FazFunc> kPredefinedFuncs;
  // Table of registered functions that maps an event type to the
  // function registered with that event type.
  std::unordered_map<int, FazFunc> registered_funcs_;
  // key-value store abstraction that enables storage and retrieval of data
  // for functions that are being executed.
  std::unique_ptr<KVStoreInterface> kvstore_;
};

typedef FazServiceImpl FazService;

#endif //CSCI499_CHENGTSU_FAZ_SERVICE_H
