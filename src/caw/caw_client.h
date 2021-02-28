#ifndef CSCI499_CHENGTSU_CAW_CLIENT_H
#define CSCI499_CHENGTSU_CAW_CLIENT_H

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include <grpcpp/grpcpp.h>

#include "caw.pb.h"
#include "faz.grpc.pb.h"

// A client to make RPC to the remote Faz gRPC service on
// behalf of the Caw platform.
class CawClient {
 public:
  // Caw event types to register with the corresponding functions.
  enum EventType { kRegisterUser, kFollow, kProfile, kCaw, kRead };

  CawClient(std::shared_ptr<grpc::Channel> channel)
      : stub_(faz::FazService::NewStub(channel)) {}

  // Hooks all Caw functions on Faz, and returns true on success.
  bool HookAll();

  // Unhooks all Caw functions from Faz, and returns true on success.
  bool UnhookAll();

  // Sends an `EventType::kRegisterUser` event to Faz,
  // and returns true on success.
  bool RegisterUser(const std::string& username);

  // Sends an `EventType::kFollow` event to Faz, and
  // returns true on success.
  bool Follow(const std::string& username, const std::string& to_follow);

  // Sends an `EventType::kProfile` event to Faz, and returns a
  // ProfileReply message containing the desired information on success.
  std::optional<caw::ProfileReply> Profile(const std::string& username);

  // Sends an `EventType::kCaw` event to Faz, and returns the Caw
  // message posted on success.
  std::optional<caw::Caw> Caw(const std::string& username,
                              const std::string& text,
                              const std::string& parent_id = "");

  // Sends an `EventType::kRead` event to Faz, and returns Caw
  // messages of all threads read.
  std::vector<caw::Caw> Read(const std::string& caw_id);

 private:
  // Table that maps a Caw event type to the predefined function
  // name known by the Faz service.
  static const std::unordered_map<EventType, std::string> kFuncs;
  // Stub to make the actual RPC.
  std::unique_ptr<faz::FazService::Stub> stub_;
};

#endif //CSCI499_CHENGTSU_CAW_CLIENT_H
