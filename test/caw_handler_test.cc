#include "caw/caw_handler.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>

#include "caw.pb.h"
#include "kvstore/kvstore.h"
#include "kvstore/kvstore_interface.h"

using google::protobuf::Any;
using grpc::Status;
using grpc::StatusCode::ALREADY_EXISTS;
using grpc::StatusCode::NOT_FOUND;
using std::move;
using std::set;
using std::string;
using std::vector;

// Tests whether a repeated protobuf field contains and only
// contains the expected values, regardless of the order.
template<typename T>
::testing::AssertionResult
ElemsEq(vector<T>&& expected,
        const ::google::protobuf::RepeatedPtrField<T>& actual){
  if (expected.size() != actual.size()) {
    return ::testing::AssertionFailure()
        << "actual.size() (" << actual.size() << ") != "
        << "expected.size() (" << expected.size() << ")";
  }
  set<T> actual_set(actual.begin(), actual.end());
  for (auto& val : expected){
    if (!actual_set.count(val)){
      return ::testing::AssertionFailure()
          << "expected value " << val << " is not in actual.";
    }
  }
  return ::testing::AssertionSuccess();
}

// Returns number of microseconds passed since beginning of UNIX epoch.
int64_t GetCurrentTimestamp() {
  return std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
}

// A test fixture for testing of Caw handler functions.
// It encapsulates the packing and unpacking of the generic gRPC
// request and response messages, and the calling of the
// corresponding Caw handler functions.
class CawHandlerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    kvstore_.reset(new KVStore);
  }

  // Calls `caw::handler::RegisterUser()` and returns the status.
  Status RegisterUser(const string& username) {
    caw::RegisteruserRequest request;
    request.set_username(username);
    Any in;
    in.PackFrom(request);
    Any out;
    return caw::handler::RegisterUser(&in, &out, kvstore_.get());
  }

  // Calls `caw::handler::Follow()` and returns the status.
  Status Follow(const string& username, const string& to_follow) {
    caw::FollowRequest request;
    request.set_username(username);
    request.set_to_follow(to_follow);
    Any in;
    in.PackFrom(request);
    Any out;
    return caw::handler::Follow(&in, &out, kvstore_.get());
  }

  // Calls `caw::handler::Profile()` and returns the status.
  Status Profile(const string& username, caw::ProfileReply* response) {
    caw::ProfileRequest request;
    request.set_username(username);
    Any in;
    in.PackFrom(request);
    Any out;
    Status status = caw::handler::Profile(&in, &out, kvstore_.get());
    out.UnpackTo(response);
    return status;
  }

  // Calls `caw::handler::Caw()` and returns the status.
  Status Caw(const string& username, const string& text,
             const string& parent_id, caw::Caw* caw) {
    caw::CawRequest request;
    request.set_username(username);
    request.set_text(text);
    request.set_parent_id(parent_id);
    Any in;
    in.PackFrom(request);
    Any out;
    Status status = caw::handler::Caw(&in, &out, kvstore_.get());
    caw::CawReply response;
    out.UnpackTo(&response);
    if (response.has_caw()) {
      caw->CopyFrom(response.caw());
    }
    return status;
  }

  // Tests whether a caw::Caw message is as expected.
  ::testing::AssertionResult
  CawCorrect(const string& expected_username, const string& expected_text,
             const string& expected_parent_id,
             const int64_t expected_time_lowerbound,
             const int64_t expected_time_upperbound,
             const caw::Caw& actual){
    if (expected_username != actual.username()) {
      return ::testing::AssertionFailure()
          << "actual.username() (" << actual.username() << ") != "
          << expected_username;
    }
    if (expected_text != actual.text()) {
      return ::testing::AssertionFailure()
          << "actual.text() (" << actual.text() << ") != " << expected_text;
    }
    if (expected_parent_id != actual.parent_id()) {
      return ::testing::AssertionFailure()
          << "actual.parent_id() (" << actual.parent_id() << ") != "
          << (expected_parent_id.empty()? "(empty)":expected_parent_id);
    }
    int64_t us = actual.timestamp().useconds();
    int64_t s = actual.timestamp().seconds();
    if (us < expected_time_lowerbound || us > expected_time_upperbound) {
      return ::testing::AssertionFailure()
          << "actual.timestamp().useconds() (" << us << ") is not between "
          << "request send time and response receive time";
    }
    if (s != us / 1000000) {
      return ::testing::AssertionFailure()
          << "relationship between actual.timestamp().useconds() (" << us
          << ") and actual.timestamp().seconds() (" << s << ") does not make"
          << "sense";
    }
    return ::testing::AssertionSuccess();
  }

  // KVStoreInterface through which the Caw handler functions
  // interact with the actual KVStore.
  std::unique_ptr<KVStoreInterface> kvstore_;
};

// Tests the correctness of the return status of
// `caw::handler::RegisterUser()`.
TEST_F(CawHandlerTest, RegisterUserTest) {
  // Register new users.
  EXPECT_TRUE(RegisterUser("eren").ok());
  EXPECT_TRUE(RegisterUser("mikasa").ok());
  // Register an existed user.
  EXPECT_EQ(RegisterUser("eren").error_code(), ALREADY_EXISTS);
  // Username is case-senstive.
  EXPECT_TRUE(RegisterUser("Mikasa").ok());
}

// Tests the correctness of the return status of
// `caw::handler::Follow()`.
TEST_F(CawHandlerTest, FollowTest) {
  // An non-existent user follows another non-existent user.
  EXPECT_EQ(Follow("mikasa", "eren").error_code(), NOT_FOUND);
  
  RegisterUser("eren");
  // An non-existent user follows an existed user.
  EXPECT_EQ(Follow("mikasa", "eren").error_code(), NOT_FOUND);
  // An existed user follows an non-existent  user.
  EXPECT_EQ(Follow("eren", "armin").error_code(), NOT_FOUND);

  RegisterUser("mikasa");
  RegisterUser("armin");
  // An existed user follows another existed user.
  EXPECT_TRUE(Follow("mikasa", "eren").ok());
  // Follow an already following user.
  EXPECT_EQ(Follow("mikasa", "eren").error_code(), ALREADY_EXISTS);
  // The following relationship is uni-directional.
  EXPECT_TRUE(Follow("eren", "mikasa").ok());
}

// Tests the functionality (side effect) of the user related Caw
// functions: `caw::handler::RegisterUser()`, `caw::handler::Follow()`
// and `caw::handler::Profile()`, and the return status of
// `caw::handler::Profile()`.
TEST_F(CawHandlerTest, UserFuncsTest) {
  caw::ProfileReply response;

  // Non-existent user.
  EXPECT_EQ(Profile("eren", &response).error_code(), NOT_FOUND);
  // Newly registered user.
  RegisterUser("eren");
  EXPECT_TRUE(Profile("eren", &response).ok());
  EXPECT_TRUE(response.following().empty());
  EXPECT_TRUE(response.followers().empty());

  RegisterUser("mikasa");
  RegisterUser("armin");
  RegisterUser("annie");
  RegisterUser("erwin");
  Follow("mikasa", "eren");
  Follow("eren", "armin");
  Follow("eren", "erwin");
  Follow("annie", "armin");
  // User with following only.
  EXPECT_TRUE(Profile("mikasa", &response).ok());
  EXPECT_TRUE(ElemsEq({"eren"}, response.following()));
  EXPECT_TRUE(response.followers().empty());
  // User with followers only.
  EXPECT_TRUE(Profile("armin", &response).ok());
  EXPECT_TRUE(response.following().empty());
  EXPECT_TRUE(ElemsEq({"annie", "eren"}, response.followers()));
  EXPECT_TRUE(Profile("erwin", &response).ok());
  EXPECT_TRUE(response.following().empty());
  EXPECT_TRUE(ElemsEq({"eren"}, response.followers()));
  // User with both following and followers.
  EXPECT_TRUE(Profile("eren", &response).ok());
  EXPECT_TRUE(ElemsEq({"erwin", "armin"}, response.following()));
  EXPECT_TRUE(ElemsEq({"mikasa"}, response.followers()));

  // No duplicate followers and following.
  Follow("mikasa", "eren");
  EXPECT_TRUE(Profile("mikasa", &response).ok());
  EXPECT_TRUE(ElemsEq({"eren"}, response.following()));
  EXPECT_TRUE(Profile("eren", &response).ok());
  EXPECT_TRUE(ElemsEq({"mikasa"}, response.followers()));
}

// Tests the correctness of the return status and
// Caw message of `caw::handler::Caw()`.
TEST_F(CawHandlerTest, CawTest) {
  caw::Caw caw;
  Status status;
  string parent_id;
  int64_t callTime;  // time calling the wrapper function.
  int64_t returnTime;  // time getting return from the wrapper function.

  // Non-existent user.
  status = Caw("reiner", "I am the Armored Titan", "", &caw);
  EXPECT_EQ(status.error_code(), NOT_FOUND);

  RegisterUser("reiner");
  // Non-existent caw to reply.
  status = Caw("reiner", "He is the Colossal Titan", "fake12345", &caw);
  EXPECT_EQ(status.error_code(), NOT_FOUND);
  // Caw without parent.
  callTime = GetCurrentTimestamp();
  EXPECT_TRUE(Caw("reiner", "Come with us", "", &caw).ok());
  returnTime = GetCurrentTimestamp();
  EXPECT_TRUE(CawCorrect(
      "reiner", "Come with us", "", callTime, returnTime, caw));

  RegisterUser("bertholdt");
  // Caw replying some other Caw.
  callTime = GetCurrentTimestamp();
  parent_id = caw.id();
  EXPECT_TRUE(Caw("bertholdt", "Are we doing it?", parent_id, &caw).ok());
  returnTime = GetCurrentTimestamp();
  EXPECT_TRUE(CawCorrect("bertholdt", "Are we doing it?", parent_id,
                         callTime, returnTime, caw));
}
