#include "caw/caw_client.h"

#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <grpcpp/grpcpp.h>

#include "caw.grpc.pb.h"
#include "faz.grpc.pb.h"

using faz::EventReply;
using faz::EventRequest;
using google::protobuf::Any;
using grpc::ClientContext;
using grpc::Status;
using std::cout;
using std::endl;
using std::string;
using std::unordered_map;
using std::vector;

// Initialize the table of Caw event types and function names.
const unordered_map<CawClient::EventType, string> CawClient::kFuncs = {
    {EventType::kRegisterUser, "RegisterUser"},
    {EventType::kFollow, "Follow"},
    {EventType::kProfile, "Profile"},
    {EventType::kCaw, "Caw"},
    {EventType::kRead, "Read"},
    {EventType::kStream, "Stream"}};

bool CawClient::HookAll() {
  bool success = true;
  for (auto [event_type, function_name] : kFuncs) {
    ClientContext context;
    faz::HookRequest request;
    faz::HookReply response;
    request.set_event_type(event_type);
    request.set_event_function(function_name);
    Status status = stub_->hook(&context, request, &response);
    if (!status.ok()) {
      cout << status.error_message() << endl;
      success = false;
    } else {
      cout << "Successfully hooked the function." << endl;
    }
  }
  return success;
}

bool CawClient::UnhookAll() {
  bool success = true;
  for (auto [event_type, _] : kFuncs) {
    ClientContext context;
    faz::UnhookRequest request;
    faz::UnhookReply response;
    request.set_event_type(event_type);
    Status status = stub_->unhook(&context, request, &response);
    if (!status.ok()) {
      cout << status.error_message() << endl;
      success = false;
    } else {
      cout << "Successfully unhooked the function." << endl;
    }
  }
  return success;
}

bool CawClient::RegisterUser(const string& username) {
  // Make the inner request packed in the generic request payload.
  caw::RegisteruserRequest inner_request;
  inner_request.set_username(username);
  // Make the generic request.
  ClientContext context;
  EventRequest request;
  EventReply response;
  request.set_event_type(EventType::kRegisterUser);
  request.mutable_payload()->PackFrom(inner_request);
  // Make RPC to the Faz service.
  Status status = stub_->event(&context, request, &response);
  if (!status.ok()) {
    cout << status.error_message() << endl;
    return false;
  }
  cout << "Successfully registered the user." << endl;
  return true;
}

bool CawClient::Follow(const string& username, const string& to_follow) {
  // Make the inner request packed in the generic request payload.
  caw::FollowRequest inner_request;
  inner_request.set_username(username);
  inner_request.set_to_follow(to_follow);
  // Make the generic request.
  ClientContext context;
  EventRequest request;
  EventReply response;
  request.set_event_type(EventType::kFollow);
  request.mutable_payload()->PackFrom(inner_request);
  // Make RPC to the Faz service.
  Status status = stub_->event(&context, request, &response);
  if (!status.ok()) {
    cout << status.error_message() << endl;
    return false;
  }
  cout << "Successfully followed the user." << endl;
  return true;
}

std::optional<caw::ProfileReply> CawClient::Profile(
    const string& username) {
  // Make the inner request packed in the generic request payload.
  caw::ProfileRequest inner_request;
  inner_request.set_username(username);
  // Make the generic request.
  ClientContext context;
  EventRequest request;
  EventReply response;
  request.set_event_type(EventType::kProfile);
  request.mutable_payload()->PackFrom(inner_request);
  // Make RPC to the Faz service.
  Status status = stub_->event(&context, request, &response);
  if (!status.ok()) {
    cout << status.error_message() << endl;
    return {};
  }
  // Get the ProfileReply message.
  if (!response.has_payload()) { return {}; }
  caw::ProfileReply inner_response;
  response.payload().UnpackTo(&inner_response);
  return inner_response;
}

std::optional<caw::Caw> CawClient::Caw(
    const string& username, const string& text,
    const string& parent_id) {
  // Make the inner request packed in the generic request payload.
  caw::CawRequest inner_request;
  inner_request.set_username(username);
  inner_request.set_text(text);
  inner_request.set_parent_id(parent_id);
  // Make the generic request.
  ClientContext context;
  EventRequest request;
  EventReply response;
  request.set_event_type(EventType::kCaw);
  request.mutable_payload()->PackFrom(inner_request);
  // Make RPC to the Faz service.
  Status status = stub_->event(&context, request, &response);
  if (!status.ok()) {
    cout << status.error_message() << endl;
    return {};
  }
  // Get the Caw message.
  if (!response.has_payload()) { return {}; }
  caw::CawReply inner_response;
  response.payload().UnpackTo(&inner_response);
  if (!inner_response.has_caw()) { return {}; }
  return inner_response.caw();
}

vector<caw::Caw> CawClient::Read(const string &caw_id) {
  // Make the inner request packed in the generic request payload.
  caw::ReadRequest inner_request;
  inner_request.set_caw_id(caw_id);
  // Make the generic request.
  ClientContext context;
  EventRequest request;
  EventReply response;
  request.set_event_type(EventType::kRead);
  request.mutable_payload()->PackFrom(inner_request);
  // Make RPC to the Faz service.
  Status status = stub_->event(&context, request, &response);
  if (!status.ok()) {
    cout << status.error_message() << endl;
    return {};
  }
  // Get the Caw message.
  if (!response.has_payload()) { return {}; }
  caw::ReadReply inner_response;
  response.payload().UnpackTo(&inner_response);
  auto& caws = inner_response.caws();
  return vector<caw::Caw>(caws.begin(), caws.end());
}

std::optional<std::vector<caw::Caw>> CawClient::Stream(const std::string hashtag, caw::Timestamp* timestamp) {
  // Make the inner request packed in the generic request payload.
  caw::StreamRequest inner_request;
  inner_request.set_hashtag(hashtag);
  inner_request.set_allocated_timestamp(timestamp);
  // Make the generic request
  ClientContext context;
  EventRequest request;
  EventReply response;
  request.set_event_type(EventType::kStream);
  request.mutable_payload()->PackFrom(inner_request);
  // Make RPC to the Faz service
  Status status = stub_->event(&context, request, &response);
  if (!status.ok()) {
    cout << status.error_message() << endl;
    return {};
  }
  if (!response.has_payload()) { return {}; }
  caw::StreamReply inner_response;
  response.payload().UnpackTo(&inner_response);
  auto& caws = inner_response.caws();
  return vector<caw::Caw>(caws.begin(), caws.end());
}
