#include "faz/faz_service.h"

#include <glog/logging.h>
#include <grpcpp/grpcpp.h>

#include "caw/caw_handler.h"

using faz::EventReply;
using faz::EventRequest;
using faz::HookReply;
using faz::HookRequest;
using faz::UnhookReply;
using faz::UnhookRequest;
using google::protobuf::Any;
using grpc::ServerContext;
using grpc::Status;
using grpc::StatusCode;
using std::string;
using std::unordered_map;

// Initialize the predefined table of known functions.
const unordered_map<string, FazFunc> FazServiceImpl::kPredefinedFuncs = {
    {"RegisterUser", caw::handler::RegisterUser},
    {"Follow", caw::handler::Follow},
    {"Profile", caw::handler::Profile},
    {"Caw", caw::handler::Caw},
    {"Read", caw::handler::Read}};

Status FazServiceImpl::hook(
    ServerContext* context,
    const HookRequest* request, HookReply* response) {
  int event_type = request->event_type();
  string function_name = request->event_function();
  auto iter = kPredefinedFuncs.find(function_name);
  if (iter == kPredefinedFuncs.end()) {
    LOG(ERROR) << "Failed to hook function " << function_name <<
               << ": not found in the predefined table.";
    return Status(StatusCode::NOT_FOUND,
                  "Function not found in predefined functions.");
  }
  registered_funcs_[event_type] = iter->second;
  LOG(INFO) << "Successfully hooked function " << function_name <<
            << " with event type " << event_type;
  return Status::OK;
}

Status FazServiceImpl::unhook(
    ServerContext* context,
    const UnhookRequest* request, UnhookReply* response) {
  int event_type = request->event_type();
  auto iter = registered_funcs_.find(event_type);
  if (iter == registered_funcs_.end()) {
    LOG(ERROR) << "Failed to unhook event type " << event_type <<
               << ": not found in the registered table.";
    return Status(StatusCode::NOT_FOUND,
                  "Function not found in registered functions.");
  }
  registered_funcs_.erase(iter);
  LOG(INFO) << "Successfully unhooked function from event type " << event_type;
  return Status::OK;
}

Status FazServiceImpl::event(
    ServerContext* context,
    const EventRequest* request, EventReply* response) {
  int event_type = request->event_type();
  Any payload = request->payload();
  auto iter = registered_funcs_.find(event_type);
  if (iter == registered_funcs_.end()) {
    LOG(ERROR) << "Failed to execute event(" << event_type <<
               << "): not found in the registered table.";
    return Status(StatusCode::NOT_FOUND,
                  "Function not found in registered functions.");
  }
  FazFunc func = iter->second;
  Status status = func(&payload, response->mutable_payload(), kvstore_.get());
  LOG(ERROR) << "Successfully executed event(" << event_type << << ")";
  return status;
}
