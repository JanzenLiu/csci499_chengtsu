#include "caw/caw_handler.h"

#include <iostream>
#include <string>

#include <glog/logging.h>

#include "caw.pb.h"

using google::protobuf::Any;
using grpc::Status;
using grpc::StatusCode;
using std::endl;
using std::string;
using std::to_string;

const string kUserPrefix = "user.";
const string kUserFollowingsPrefix = "user_followings.";
const string kUserFollowersPrefix = "user_followers.";
const string kFollowingPairPrefix = "following_pair.";

// Returns true if the user exists in the KVStore.
bool UserExists(const string& username, KVStoreInterface* kvstore){
  string key = kUserPrefix + username;
  return !kvstore->Get(key).empty();
}

Status caw::handler::RegisterUser(const Any* in, Any* out,
                                  KVStoreInterface* kvstore) {
  // Unpack the request message.
  caw::RegisteruserRequest request;
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
  caw::RegisteruserReply response;
  out->PackFrom(response);
  return Status::OK;
}

Status caw::handler::Follow(const Any* in, Any* out,
                            KVStoreInterface* kvstore) {
  // Unpack the request message.
  caw::FollowRequest request;
  in->UnpackTo(&request);
  string username = request.username();
  string to_follow = request.to_follow();
  // Check the existence of both users.
  if (!UserExists(username, kvstore) || !UserExists(to_follow, kvstore)) {
    return Status(StatusCode::NOT_FOUND, "User not found.");
  }
  // Check whether the user is already following the other.
  // Encode the `username` length into the key to avoid ambiguity.
  string key = kFollowingPairPrefix + to_string(username.length())
      + "." + username + "." + to_follow;
  if (!kvstore->Get(key).empty()) {
    return Status(StatusCode::ALREADY_EXISTS,
                  "User is already following the followee.");
  }
  // Store the relationship to the KVStore.
  if (!kvstore->Put(key, "")) {
    return Status(StatusCode::UNAVAILABLE,
                  "Failed to add the following pair to the kvstore.");
  }
  key = kUserFollowingsPrefix + username;
  if (!kvstore->Put(key, to_follow)) {
    LOG(ERROR) << "Added the following pair but failed to update the "
               << "following list. username=" << username << ", "
               << "to_follow=" << to_follow << endl;
    return Status(StatusCode::UNAVAILABLE,
                  "Failed to add following to the kvstore.");
  }
  key = kUserFollowersPrefix + to_follow;
  if (!kvstore->Put(key, username)) {
    LOG(ERROR) << "Added the following pair and updated the following"
               << "list, but failed to update the follower list. "
               << "username=" << username << ", "
               << "to_follow=" << to_follow << endl;
    return Status(StatusCode::UNAVAILABLE,
                  "Failed to add following to the kvstore.");
  }
  // Pack the response message.
  caw::FollowReply response;
  out->PackFrom(response);
  return Status::OK;
}

Status caw::handler::Profile(const Any *in, Any *out,
                             KVStoreInterface *kvstore) {
  // Unpack the request message.
  caw::ProfileRequest request;
  in->UnpackTo(&request);
  string username = request.username();
  // Check the existence of the user.
  if (!UserExists(username, kvstore)) {
    return Status(StatusCode::NOT_FOUND, "User not found.");
  }
  // Get followings and followers from the KVStore and
  // put them into the response message.
  ProfileReply response;
  string key = kUserFollowingsPrefix + username;
  for (string& other : kvstore->Get(key)) {
    response.add_following(other);
  }
  key = kUserFollowersPrefix + username;
  for (string& other : kvstore->Get(key)) {
    response.add_followers(other);
  }
  out->PackFrom(response);
  return Status::OK;
}
