#include "caw/caw_handler.h"

#include <chrono>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <set>

#include <glog/logging.h>
#include <google/protobuf/any.pb.h>

#include "caw.pb.h"

using google::protobuf::Any;
using grpc::Status;
using grpc::StatusCode;
using std::endl;
using std::string;
using std::to_string;
using std::vector;

const string kUserPrefix = "user.";
const string kUserFollowingsPrefix = "user_followings.";
const string kUserFollowersPrefix = "user_followers.";
const string kFollowingPairPrefix = "following_pair.";
const string kCawPrefix = "caw.";
const string kReplyPrefix = "caw_reply.";
const string kHashtagPrefix = "caw_hashtag.";

// Returns true if the user exists in the KVStore.
bool UserExists(const string& username, KVStoreInterface* kvstore){
  string key = kUserPrefix + username;
  return !kvstore->Get(key).empty();
}

// Returns true if the caw exists in the KVStore.
bool CawExists(const string& caw_id, KVStoreInterface* kvstore) {
  string key = kCawPrefix + caw_id;
  return !kvstore->Get(key).empty();
}

// Returns number of microseconds passed since beginning of UNIX epoch.
int64_t GetMicrosecondsSinceEpoch() {
  return std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
}

// Returns a randomly generated ID, which is of the given length and
// each digit is sampled from 0-9 plus a-f (inclusive).
string GenerateRandomID(int length) {
  const char *digits = "0123456789abcdef";
  string res(length, ' ');
  for (int i = 0; i < length; ++i) {
    res[i] = digits[rand() % 16];
  }
  return res;
}

// Returns hashtags included in the text of the caw
// Hashtag is defined as one or more alphanumeric characters following the '#'
vector<string> GetHashtags(const string& text) {
  string raw(text);
  std::regex pattern("#([0-9a-zA-Z]+)");
  vector<string> hashtags;
  auto begin = std::sregex_iterator(raw.begin(), raw.end(), pattern);
  auto end = std::sregex_iterator();
  for (auto it = begin; it != end; ++it) {
    std::smatch match = *it;
    hashtags.push_back(match.str(1));
  }
  return hashtags;
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
               << "to_follow=" << to_follow;
    return Status(StatusCode::UNAVAILABLE,
                  "Failed to add following to the kvstore.");
  }
  key = kUserFollowersPrefix + to_follow;
  if (!kvstore->Put(key, username)) {
    LOG(ERROR) << "Added the following pair and updated the following"
               << "list, but failed to update the follower list. "
               << "username=" << username << ", "
               << "to_follow=" << to_follow;
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

Status caw::handler::Caw(const Any *in, Any *out,
                         KVStoreInterface *kvstore) {
  // Unpack the request message.
  caw::CawRequest request;
  in->UnpackTo(&request);
  string username = request.username();
  string text = request.text();
  string parent_id = request.parent_id();
  // Check the existence of the user.
  if (!UserExists(username, kvstore)) {
    return Status(StatusCode::NOT_FOUND, "User not found.");
  }
  // Check the existence of the caw to reply (if specified).
  if (!parent_id.empty() && !CawExists(parent_id, kvstore)) {
    return Status(StatusCode::NOT_FOUND, "Caw to reply not found.");
  }
  // Generate required information and make the Caw message.
  int64_t us = GetMicrosecondsSinceEpoch();
  Timestamp* timestamp = new Timestamp();
  timestamp->set_seconds(us / 1000000);
  timestamp->set_useconds(us);
  string id = to_string(us) + "-" + GenerateRandomID(4);  // caw id.
  caw::Caw* caw = new caw::Caw();
  caw->set_username(username);
  caw->set_text(text);
  caw->set_id(id);
  caw->set_parent_id(parent_id);
  caw->set_allocated_timestamp(timestamp);
  // Store the caw to the KVStore.
  string key = kCawPrefix + id;
  if (!kvstore->Put(key, caw->SerializeAsString())) {
    return Status(StatusCode::UNAVAILABLE,
                  "Failed to add caw post to the kvstore.");
  }
  if (!parent_id.empty()) {
    key = kReplyPrefix + parent_id;
    if (!kvstore->Put(key, id)) {
      return Status(StatusCode::UNAVAILABLE,
                    "Failed to add caw post to the kvstore.");
    }
  }
  // Adds <hashtag, id> pairs to the storage.
  auto hashtag_vec = GetHashtags(text);
  std::set<string> hashtags(hashtag_vec.begin(), hashtag_vec.end());
  for (const auto& hashtag : hashtags) {
    string hashtag_key = kHashtagPrefix + hashtag;
    if (!kvstore->Put(hashtag_key, id)) {
      return Status(StatusCode::UNAVAILABLE,
                    "Failed to add hashtag, id mappings to the kvstore.");
    }
  }
  // Pack the response message.
  caw::CawReply response;
  response.set_allocated_caw(caw);
  out->PackFrom(response);
  return Status::OK;
}

Status caw::handler::Read(const Any *in, Any *out,
                          KVStoreInterface *kvstore) {
  // Unpack the request message.
  caw::ReadRequest request;
  in->UnpackTo(&request);
  string caw_id = request.caw_id();
  // Check the existence of the caw.
  if (!CawExists(caw_id, kvstore)) {
    return Status(StatusCode::NOT_FOUND,
                  "Caw " + caw_id + " not found.");
  }
  // Find all threads starting at the given caw and put them
  // into the caw::ReadReply message in a BFS approach.
  caw::ReadReply response;
  std::deque<string> q;  // Queue of threads to read.
  q.push_back(caw_id);
  while (!q.empty()) {
    bool current_caw_success = false;
    string current_caw_id = q.front();
    q.pop_front();
    string key = kCawPrefix + current_caw_id;
    vector<string> values = kvstore->Get(key);
    if (values.size() == 1) {
      // Add the Caw message and populate it with the
      // information retrieved from the KVStore.
      caw::Caw* caw = response.add_caws();
      string caw_str = values[0];
      if (caw->ParseFromString(caw_str)) {
        // Get reply ids and add into the queue.
        key = kReplyPrefix + current_caw_id;
        vector<string> reply_caw_ids = kvstore->Get(key);
        q.insert(q.end(), reply_caw_ids.begin(), reply_caw_ids.end());
        current_caw_success = true;
      } else {
        LOG(ERROR) << "Error decoding caw " << current_caw_id;
      }
    } else {
      LOG(ERROR) << "Error finding caw " << current_caw_id << ": "
                 << values.size() << " records found, expected 1.";
    }
    // Notify failure.
    if (!current_caw_success) {
      return Status(StatusCode::UNAVAILABLE,
                    "Error reading caw " + current_caw_id + ".");
    }
  }
  // Pack the response message.
  out->PackFrom(response);
  return Status::OK;
}

Status caw::handler::Stream(const Any *in, Any *out,
                            KVStoreInterface *kvstore) {
  // Unpack the request message.
  caw::StreamRequest request;
  in->UnpackTo(&request);
  string hashtag_key = kHashtagPrefix + request.hashtag();
  Timestamp timestamp = request.timestamp();
  // Only retrieve caws with timestamp after start time of the stream request
  int64_t start = timestamp.useconds();
  caw::StreamReply response;
  vector<string> values = kvstore->Get(hashtag_key);
  for (const auto& id : values) {
    string key = kCawPrefix + id;
    auto value = kvstore->Get(key);
    if (value.size() == 1) {
      caw::Caw temp_caw;
      string caw_str = value[0];
      if (temp_caw.ParseFromString(caw_str)) {
        int64_t time = temp_caw.timestamp().useconds();
        if (time > start) {
          response.add_caws()->CopyFrom(temp_caw);
        }
      } else {
        LOG(ERROR) << "Error decoding caw " << id;
      }
    } else {
      LOG(ERROR) << "Error finding caw " << id << ": "
                 << values.size() << " records found, expected 1.";
    }
  }
  // Pack the response message.
  out->PackFrom(response);
  return Status::OK;
}
