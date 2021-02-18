#include <iostream>
#include <string>
#include <memory>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <grpcpp/grpcpp.h>

#include "faz_service.h"

static bool ValidatePort(const char* flagname, int32_t value) {
  if (value > 0 && value < 65536) { return true; }
  LOG(FATAL) << "Invalid value for --" << std::string(flagname)
    << ": " << value << std::endl;
  return false;
}

DEFINE_int32(faz_port, 50000, "Port number for the Faz GRPC interface to use.");
DEFINE_int32(kvstore_port, 50001, "Port number for the kvstore GRPC interface to use.");
DEFINE_validator(faz_port, &ValidatePort);
DEFINE_validator(kvstore_port, &ValidatePort);

// Runs the Faz gRPC service at a given port, with an abstraction to
// interact with a KVStore gRPC service running at another given port.
void RunServer(int faz_port, int kvstore_port) {
  // Create a FazService who uses the KVStore running at `kvstore_port`.
  std::string target_str = "localhost:" + std::to_string(kvstore_port);
  auto channel = grpc::CreateChannel(
      target_str, grpc::InsecureChannelCredentials());
  FazService service(channel);

  std::string server_address("0.0.0.0:" + std::to_string(faz_port));
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
  RunServer(FLAGS_faz_port, FLAGS_kvstore_port);
  return 0;
}

