#include "caw/caw_handler.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <glog/logging.h>
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

  // Calls `caw::handler::Read()` and returns the status.
  Status Read(const string& caw_id, vector<caw::Caw>& caws) {
    caw::ReadRequest request;
    request.set_caw_id(caw_id);
    Any in;
    in.PackFrom(request);
    Any out;
    Status status = caw::handler::Read(&in, &out, kvstore_.get());
    caw::ReadReply response;
    out.UnpackTo(&response);
    caws.assign(response.caws().begin(), response.caws().end());
    return status;
  }

  // Tests whether a caw::Caw message is as expected.
  ::testing::AssertionResult
  CawIsCorrect(const string& expected_username, const string& expected_text,
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
  EXPECT_TRUE(CawIsCorrect(
      "reiner", "Come with us", "", callTime, returnTime, caw));

  RegisterUser("bertholdt");
  // Caw replying some other Caw.
  callTime = GetCurrentTimestamp();
  parent_id = caw.id();
  EXPECT_TRUE(Caw("bertholdt", "Are we doing it?", parent_id, &caw).ok());
  returnTime = GetCurrentTimestamp();
  EXPECT_TRUE(CawIsCorrect("bertholdt", "Are we doing it?", parent_id,
                           callTime, returnTime, caw));
}

// Tests the functionality (side effect) of the caw related Caw
// functions: `caw::handler::Caw()`, `caw::handler::Read()`,
// and the return status of `caw::handler::Read()`.
TEST_F(CawHandlerTest, CawFuncsTest) {
  vector<caw::Caw> caws;

  // Non-existent caw.
  EXPECT_EQ(Read("fake12345", caws).error_code(), NOT_FOUND);

  // Make caws:
  //       0
  //      / \
  //     1   2
  //    / \   \
  //   /| |\   7
  //  3 4 5 6   \
  //             8
  //              \
  //               9
  RegisterUser("reiner");
  RegisterUser("bertholdt");
  RegisterUser("eren");
  vector<string> caw_ids;

  // Calls `CawHandlerTest::Caw()` to post a caw with the given information,
  // and add the id of the created caw to the vector `caw_ids`.
  auto addCaw = [&caw_ids, this](const string& username, const string& text,
                                 const string& parent_id) {
    caw::Caw caw;
    Caw(username, text, parent_id, &caw);
    caw_ids.push_back(caw.id());
  };
  addCaw("reiner", "I am the Armored Titan", "");  // caw 0
  addCaw("reiner", "He is the Colossal Titan", caw_ids[0]);  // caw 1
  addCaw("reiner", "Come with us", caw_ids[0]);  // caw 2
  addCaw("bertholdt", "Reiner!", caw_ids[1]);  // caw 3
  addCaw("bertholdt", "Are we doing it?", caw_ids[1]);  // caw 4
  addCaw("bertholdt", "Right now?!", caw_ids[1]);  // caw 5
  addCaw("bertholdt", "Right here?!", caw_ids[1]);  // caw 6
  addCaw("reiner", " I see", caw_ids[2]);  // caw 7
  addCaw("reiner", " I've just... been here way too long",
         caw_ids[7]);  // caw 8
  addCaw("eren", "Sit down Reiner", caw_ids[8]);  // caw 9

  // Since we have examined the content of posted caws in the previous test,
  // we only examine the ids here.

  // Tests whether a vector of `caw::Caw` contains and only
  // contains the caws with expected ids, regardless of the order.
  auto cawIdsEq = [&caw_ids](const vector<int>& expected_indices,
                             const vector<caw::Caw>& actual) {
    if (expected_indices.size() != actual.size()) {
      return ::testing::AssertionFailure()
          << "actual.size() (" << actual.size() << ") != "
          << "expected_indices.size() (" << expected_indices.size() << ")";
    }
    set<string> actual_ids;
    for (auto& caw : actual) {
      actual_ids.insert(caw.id());
    }
    for (int i : expected_indices){
      string expected_id = caw_ids[i];
      if (!actual_ids.count(expected_id)){
        return ::testing::AssertionFailure()
            << "expected caw " << i << " is not in actual.";
      }
    }
    return ::testing::AssertionSuccess();
  };

  // Caw without replies.
  EXPECT_TRUE(Read(caw_ids[9], caws).ok());
  EXPECT_TRUE(cawIdsEq({9}, caws));
  // Caw with one direct reply only.
  EXPECT_TRUE(Read(caw_ids[8], caws).ok());
  EXPECT_TRUE(cawIdsEq({8, 9}, caws));
  // Caw with multiple direct replies.
  EXPECT_TRUE(Read(caw_ids[1], caws).ok());
  EXPECT_TRUE(cawIdsEq({1, 3, 4, 5, 6}, caws));
  // Caw with one direct reply and multiple indirect replies in one branch.
  EXPECT_TRUE(Read(caw_ids[2], caws).ok());
  EXPECT_TRUE(cawIdsEq({2, 7, 8, 9}, caws));
  // Caw with multiple direct and indirect replies in multiple branches.
  EXPECT_TRUE(Read(caw_ids[0], caws).ok());
  EXPECT_TRUE(cawIdsEq({0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, caws));
}

int main(int argc, char **argv) {
  // Use a self-defined main function here to call InitGoogleLogging(),
  // otherwise, all glog messages (including INFO) will be directed to
  // STDERR and hence displayed to the terminal.
  testing::InitGoogleTest(&argc, argv);
  google::InitGoogleLogging(argv[0]);
  return RUN_ALL_TESTS();
}
