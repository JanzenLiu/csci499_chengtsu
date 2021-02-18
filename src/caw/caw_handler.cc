#include "caw_handler.h"

#include <string>

#include "caw.pb.h"

using caw::RegisteruserReply;
using caw::RegisteruserRequest;
using caw::FollowReply;
using caw::FollowRequest;
using google::protobuf::Any;
using grpc::Status;
using grpc::StatusCode;
using std::string;

const string kUserPrefix = "user.";
const string kFollowingPrefix = "following.";

// Returns true if the user exists in the KVStore.
bool UserExists(const string& username, KVStoreClient* kvstore){
  string key = kUserPrefix + username;
  return !kvstore->Get(key).empty();
}

Status caw::handler::RegisterUser(const Any* in, Any* out,
                                  KVStoreClient* kvstore) {
  // Unpack the request message.
  RegisteruserRequest request;
  in->UnpackTo(&request);
  string username = request.username();
  // Check the existence of the user.
  if (UserExists(username, kvstore)) {
    return Status(StatusCode::ALREADY_EXISTS, "User already exists.");
  }
  // Store the user in the KVStore.
  string key = kUserPrefix + username;
  if (!kvstore->Put(key, "")) {
    return Status(StatusCode::UNAVAILABLE,
                  "Failed to add user to the kvstore.");
  }
  // Pack the response message.
  RegisteruserReply response;
  out->PackFrom(response);
  return Status::OK;
}

Status caw::handler::Follow(const Any* in, Any* out,
                            KVStoreClient* kvstore) {
  // Unpack the request message.
  FollowRequest request;
  in->UnpackTo(&request);
  string username = request.username();
  string to_follow = request.to_follow();
  // Check the existence of both users.
  if (!UserExists(username, kvstore) || !UserExists(to_follow, kvstore)) {
    return Status(StatusCode::NOT_FOUND, "User not found.");
  }
  // Store the relationship to the KVStore.
  string key = kFollowingPrefix + username;
  if (!kvstore->Put(key, to_follow)) {
    return Status(StatusCode::UNAVAILABLE,
                  "Failed to add following to the kvstore.");
  }
  // Pack the response message.
  FollowReply response;
  out->PackFrom(response);
  return Status::OK;
}
