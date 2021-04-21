#include <ctime>
#include <iomanip>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#include <gflags/gflags.h>
#include <grpcpp/grpcpp.h>

#include "caw.pb.h"
#include "caw/caw_client.h"

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
DEFINE_string(stream, "", "Stream all caws containing the hashtag.");
DEFINE_bool(profile, false, "Gets the user’s profile of following and followers");
DEFINE_bool(hook_all, false, "Hooks all Caw functions to the Faz layer.");
DEFINE_bool(unhook_all, false, "Unhooks all Caw functions from the Faz layer.");
DEFINE_validator(port, &ValidatePort);

// Outputs a ProfileReply message to an output stream.
ostream& operator<<(ostream& os, const caw::ProfileReply& profile) {
  os << "{" << endl;
  // Output the following list.
  os << "  following (size=" << profile.following_size() << "): [ ";
  for (std::string other : profile.following()) {
    os << other << ", ";
  }
  os << "]" << endl;
  // Output the follower list.
  os << "  followers (size=" << profile.followers_size() << "): [ ";
  for (std::string other : profile.followers()) {
    os << other << ", ";
  }
  os << "]" << endl;
  os << "}";
  return os;
}

// Outputs a Caw message to an output stream.
ostream& operator<<(ostream& os, const caw::Caw& caw) {
  std::string parent_id = caw.parent_id();
  if (parent_id.empty()) { parent_id = "null"; }
  std::time_t t = caw.timestamp().seconds();
  os << "{" << endl
     << "  username: " << caw.username() << "," << endl
     << "  text: " << caw.text() << ","  << endl
     << "  id: " << caw.id() << ","  << endl
     << "  parent_id: " << parent_id << ","  << endl
     << "  time: " << std::asctime(std::localtime(&t)) << endl
     << "}";
  return os;
}

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
  // Handle flag --profile.
  if (FLAGS_profile) {
    if (FLAGS_user.empty()) {
      cout << "You need to login to get the user's profile." << endl;
    } else {
      auto profile = client.Profile(FLAGS_user);
      if (profile.has_value()) {
        cout << profile.value() << endl;
      }
    }
  }
  // Handle flag --caw.
  if (!FLAGS_caw.empty()) {
    if (FLAGS_user.empty()) {
      cout << "You need to login to post a caw." << endl;
    } else {
      auto caw = client.Caw(FLAGS_user, FLAGS_caw, FLAGS_reply);
      if (caw.has_value()) {
        cout << "Successfully posted the caw." << endl
             << caw.value() << endl;
      }
    }
  }
  // Handle flag --reply.
  if (!FLAGS_reply.empty() && FLAGS_caw.empty()) {
    cout << "You need to give the content with --caw to post a reply." << endl;
  }
  // Handle flag --read
  if (!FLAGS_read.empty()) {
    auto caws = client.Read(FLAGS_read);
    for (auto& caw : caws) {
      cout << caw << endl;
    }
  }
  // Handle flag --stream
  if (!FLAGS_stream.empty()) {
    if (FLAGS_stream[0] != '#')
      cout << "The format of the hashtag should be started with #" << endl;
    else {
      cout << "Stream all new caws containing the " + FLAGS_stream << endl;
      // Use the current time as the timestamp for the StreamRequest for the first time
      // Update the timestamp to the most recent timestamp of the caw to avoid duplicates
      auto now = std::chrono::system_clock::now().time_since_epoch();
      google::protobuf::Arena arena;
      caw::Timestamp* timestamp = google::protobuf::Arena::CreateMessage<caw::Timestamp>(&arena);
      timestamp->set_seconds(std::chrono::duration_cast<std::chrono::seconds>(now).count());
      timestamp->set_useconds(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
      while (true) {
        auto caws = client.Stream(FLAGS_stream.substr(1), timestamp);
        if (caws.has_value()) {
          auto caws_vec = caws.value();
          for (const auto &caw : caws_vec) {
            cout << caw << endl;
          }
          if (caws_vec.size()) {
            *timestamp = caws_vec.at(caws_vec.size() - 1).timestamp();
          }
        }
        std::chrono::seconds duration(5);
        std::this_thread::sleep_for(duration);
      }
    }
  }
  // Handle flag --unhook_all.
  if (FLAGS_unhook_all) {
    cout << "Unhooking all Caw functions from the Faz layer..." << endl;
    client.UnhookAll();
  }

  return 0;
}
