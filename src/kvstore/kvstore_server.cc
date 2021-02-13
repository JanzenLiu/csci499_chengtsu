#include <iostream>
#include <string>
#include <memory>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <grpcpp/grpcpp.h>

#include "kvstore_service.h"

DEFINE_int32(port, 50001, "Port number for the kvstore GRPC interface to use.");

// Runs the key-value store gRPC service at a given port.
void RunServer(int port) {
  std::string server_address("0.0.0.0:" + std::to_string(port));
  KVStoreService service;

  grpc::ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register `service` as the instance through which we'll communicate with
  // clients. In this case, it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  LOG(INFO) << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  // Validate the command line arguments.
  if (FLAGS_port < 0 or FLAGS_port > 65535) {
    LOG(FATAL) << "Invalid port number: " << FLAGS_port << "." << std::endl;
  }
  RunServer(FLAGS_port);
  return 0;
}
