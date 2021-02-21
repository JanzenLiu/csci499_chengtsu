#include <iostream>
#include <string>

#include <gflags/gflags.h>
#include <grpcpp/grpcpp.h>

#include "caw_client.h"

using std::cout;
using std::endl;
using std::ostream;

// Returns true if the port specified by the flag is valid.
static bool ValidatePort(const char* flagname, int32_t value) {
  if (value > 0 && value < 65536) { return true; }
  cout << "Invalid value for --" << std::string(flagname)
    << ": " << value << std::endl;
  return false;
}

DEFINE_int32(port, 50000, "Port number for the Faz GRPC interface to use.");
DEFINE_string(registeruser, "", "Registers the given username");
DEFINE_string(user, "", "Logs in as the given username");
DEFINE_string(caw, "", "Creates a new caw with the given text");
DEFINE_string(reply, "", "Indicates that the new caw is a reply to the given id");
DEFINE_string(follow, "", "Starts following the given username");
DEFINE_string(read, "", "Reads the caw thread starting at the given id");
DEFINE_bool(profile, false, "Gets the user’s profile of following and followers");
DEFINE_bool(hook_all, false, "Hooks all Caw functions to the Faz layer.");
DEFINE_bool(unhook_all, false, "Unhooks all Caw functions from the Faz layer.");
DEFINE_validator(port, &ValidatePort);

int main(int argc, char *argv[]) {
  gflags::SetUsageMessage("Caw command-line tool Usage");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint (in this case,
  // localhost at port 50000). We indicate that the channel isn't authenticated
  // (use of InsecureChannelCredentials()).
  std::string target_str = "localhost:" + std::to_string(FLAGS_port);
  auto channel = grpc::CreateChannel(
      target_str, grpc::InsecureChannelCredentials());
  CawClient client(channel);

  // Handle flag --hook_all.
  if (FLAGS_hook_all) {
    cout << "Hooking all Caw functions to the Faz layer..." << endl;
    client.HookAll();
  }

  // Handle flag --registeruser.
  if (!FLAGS_registeruser.empty()) {
    client.RegisterUser(FLAGS_registeruser);
  }
  // Handle flag --follow.
  if (!FLAGS_follow.empty()) {
    if (FLAGS_user.empty()) {
      cout << "You need to login to follow a user." << endl;
    } else {
      client.Follow(FLAGS_user, FLAGS_follow);
    }
  }

  // Handle flag --unhook_all.
  if (FLAGS_unhook_all) {
    cout << "Unhooking all Caw functions from the Faz layer..." << endl;
    client.UnhookAll();
  }

  return 0;
}
