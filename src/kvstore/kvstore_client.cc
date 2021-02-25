#include "kvstore/kvstore_client.h"

#include <string>
#include <vector>

#include <grpcpp/grpcpp.h>

#include "kvstore.grpc.pb.h"

using grpc::ClientContext;
using grpc::Status;
using kvstore::GetReply;
using kvstore::GetRequest;
using kvstore::PutReply;
using kvstore::PutRequest;
using kvstore::RemoveReply;
using kvstore::RemoveRequest;
using std::string;
using std::vector;

bool KVStoreClient::Put(const string& key, const string& value) {
  PutRequest request;
  request.set_key(key);
  request.set_value(value);

  ClientContext context;
  PutReply response;
  Status status = stub_->put(&context, request, &response);
  return status.ok();
}

vector<string> KVStoreClient::Get(const string& key) {
  ClientContext context;
  auto stream = stub_->get(&context);

  GetRequest request;
  request.set_key(key);
  stream->Write(request);
  stream->WritesDone();

  vector<string> values;
  GetReply response;
  while (stream->Read(&response)) {
    values.push_back(response.value());
  }
  return values;
}

void KVStoreClient::Remove(const string& key) {
  RemoveRequest request;
  request.set_key(key);

  ClientContext context;
  RemoveReply response;
  stub_->remove(&context, request, &response);
}
