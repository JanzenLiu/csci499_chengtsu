#include "caw/caw_handler.h"

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

class CawHandlerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    kvstore_.reset(new KVStore);
  }

  Status RegisterUser(const string& username) {
    caw::RegisteruserRequest request;
    request.set_username(username);
    Any in;
    in.PackFrom(request);
    Any out;
    return caw::handler::RegisterUser(&in, &out, kvstore_.get());
  }

  Status Follow(const string& username, const string& to_follow) {
    caw::FollowRequest request;
    request.set_username(username);
    request.set_to_follow(to_follow);
    Any in;
    in.PackFrom(request);
    Any out;
    return caw::handler::Follow(&in, &out, kvstore_.get());
  }

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

  std::unique_ptr<KVStoreInterface> kvstore_;
};

TEST_F(CawHandlerTest, RegisterUserTest) {
  // Register new users.
  EXPECT_TRUE(RegisterUser("eren").ok());
  EXPECT_TRUE(RegisterUser("mikasa").ok());
  // Register an existed user.
  EXPECT_EQ(RegisterUser("eren").error_code(), ALREADY_EXISTS);
  // Username is case-senstive.
  EXPECT_TRUE(RegisterUser("Mikasa").ok());
}

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

TEST_F(CawHandlerTest, ProfileTest) {
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
