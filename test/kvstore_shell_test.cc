#include "kvstore/kvstore_client.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <gflags/gflags.h>
#include <grpcpp/grpcpp.h>

DEFINE_int32(port, 50001, "Port number for the kvstore GRPC interface to use.");

using std::cout;
using std::endl;
using std::ostream;
using std::string;
using std::vector;

// Outputs a vector to an output stream.
template<typename T>
ostream& operator<<(ostream& os, const vector<T>& vec) {
  os << "[ ";
  for (auto& elem : vec) {
    os << elem << " ";
  }
  os << "]";
  return os;
}

// Parses the command input by user in the CLI and returns
// a vector of tokens constituting the command.
vector<string> ParseCommand(const string& command) {
  vector<string> tokens;
  size_t start = 0;  // Begin index of the next search.
  while (start < command.length()) {
    // Simply split tokens by the space character.
    size_t pos = command.find(" ", start);
    if (pos == string::npos) { pos = command.length(); }
    if (pos > start) {
      string token = command.substr(start, pos - start);
      tokens.push_back(token);
    }
    start = pos + 1;
  }
  return tokens;
}

void promptUsage() {
  cout << "KVStore CLI Usage:" << endl
       << "put <key> <value>  Add a value under a key." << endl
       << "get <key>          Get all values under a key." << endl
       << "delete <key>       Delete all values under a key." << endl
       << "exit               Exit the command-line tool." << endl;
}

void promptInvalid() {
  cout << "Invalid command." << endl;
  promptUsage();
};

// Runs client command line tool for the KVStore gRPC service.
int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  // Validate command line arguments.
  if (FLAGS_port < 0 or FLAGS_port > 65535) {
    cout << "Invalid port number: " << FLAGS_port << "." << endl;
  }

  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint (in this case,
  // localhost at port 50001). We indicate that the channel isn't authenticated
  // (use of InsecureChannelCredentials()).
  string target_str = "localhost:" + std::to_string(FLAGS_port);
  auto channel = grpc::CreateChannel(
      target_str, grpc::InsecureChannelCredentials());
  KVStoreClient client(channel);

  // Loop until system interrupt or command to exit.
  for (string line; std::getline(std::cin, line);) {
    auto tokens = ParseCommand(line);
    if (tokens.empty()) {
      promptInvalid();
      continue;
    }
    string op = tokens[0]; // Operation: put/get/remove/exit.
    if (op == "put" && tokens.size() >= 3) {
      client.Put(tokens[1], tokens[2]);
    } else if (op == "get" && tokens.size() >= 2) {
      cout << client.Get(tokens[1]) << endl;
    } else if (op == "delete" && tokens.size() >= 2) {
      client.Remove(tokens[1]);
    } else if (op == "help") {
      promptUsage();
    } else if (op == "exit") {
      break;
    } else {
      promptInvalid();
    }
  }

  return 0;
}
