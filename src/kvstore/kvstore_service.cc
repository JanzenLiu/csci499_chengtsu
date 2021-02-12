#include "kvstore_service.h"

#include <iostream>
#include <string>

#include <grpcpp/grpcpp.h>

using grpc::ServerContext;
using grpc::ServerReaderWriter;
using grpc::Status;
using kvstore::GetReply;
using kvstore::GetRequest;
using kvstore::PutReply;
using kvstore::PutRequest;
using kvstore::RemoveReply;
using kvstore::RemoveRequest;
using std::string;

Status KeyValueStoreServiceImpl::put(
    ServerContext* context, const PutRequest* request, PutReply* response) {
  store_.Put(request->key(), request->value());
  return Status::OK;
}

Status KeyValueStoreServiceImpl::get(
    ServerContext* context, ServerReaderWriter<GetReply, GetRequest>* stream) {
  GetRequest request;
  while (stream->Read(&request)) {
    for (string& value : store_.Get(request.key())) {
      GetReply response;
      response.set_value(value);
      stream->Write(response);
    }
  }
  return Status::OK;
}

Status KeyValueStoreServiceImpl::remove(
    ServerContext* context, const RemoveRequest* request,
    RemoveReply* response) {
  store_.Remove(request->key());
  return Status::OK;
}
