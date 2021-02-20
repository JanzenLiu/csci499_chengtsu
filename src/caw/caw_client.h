#ifndef CSCI499_CHENGTSU_CAW_CLIENT_H
#define CSCI499_CHENGTSU_CAW_CLIENT_H

#include <memory>
#include <string>
#include <unordered_map>

#include <grpcpp/grpcpp.h>

#include "faz.grpc.pb.h"

// A client to make RPC to the remote Faz gRPC service on
// behalf of the Caw platform.
class CawClient {
 public:
  // Caw event types to register with the corresponding functions.
  enum EventType { kRegisterUser, kFollow };

  CawClient(std::shared_ptr<grpc::Channel> channel)
      : stub_(faz::FazService::NewStub(channel)) {}

  // Hooks all Caw functions on Faz.
  bool HookAll();

  // Unhooks all Caw functions from Faz.
  bool UnhookAll();

  // Sends an `EventType::kRegisterUser` event to Faz.
  bool RegisterUser(const std::string& username);

  // Sends an `EventType::kFollow` event to Faz.
  bool Follow(const std::string& username, const std::string& to_follow);

 private:
  // Table that maps a Caw event type to the predefined function
  // name known by the Faz service.
  static const std::unordered_map<EventType, std::string> kFuncs;
  // Stub to make the actual RPC.
  std::unique_ptr<faz::FazService::Stub> stub_;
};

#endif //CSCI499_CHENGTSU_CAW_CLIENT_H
