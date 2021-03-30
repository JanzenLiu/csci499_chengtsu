#include "faz/faz_service.h"

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
    return Status(StatusCode::NOT_FOUND,
                  "Function not found in predefined functions.");
  }
  registered_funcs_[event_type] = iter->second;
  return Status::OK;
}

Status FazServiceImpl::unhook(
    ServerContext* context,
    const UnhookRequest* request, UnhookReply* response) {
  int event_type = request->event_type();
  auto iter = registered_funcs_.find(event_type);
  if (iter == registered_funcs_.end()) {
    return Status(StatusCode::NOT_FOUND,
                  "Function not found in registered functions.");
  }
  registered_funcs_.erase(iter);
  return Status::OK;
}

Status FazServiceImpl::event(
    ServerContext* context,
    const EventRequest* request, EventReply* response) {
  int event_type = request->event_type();
  Any payload = request->payload();
  auto iter = registered_funcs_.find(event_type);
  if (iter == registered_funcs_.end()) {
    return Status(StatusCode::NOT_FOUND,
                  "Function not found in registered functions.");
  }
  FazFunc func = iter->second;
  return func(&payload, response->mutable_payload(), kvstore_.get());
}
