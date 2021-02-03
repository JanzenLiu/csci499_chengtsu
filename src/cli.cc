#include <iostream>
#include <string>

#include <gflags/gflags.h>

using std::cout;
using std::endl;

//--registeruser <username>	Registers the given username
//--user <username>		Logs in as the given username
//--caw <caw text>		Creates a new caw with the given text
//--reply <reply caw id>	Indicates that the new caw is a reply to the given id
//--follow <username>		Starts following the given username
//--read <caw id>		Reads the caw thread starting at the given id
//--profile			Gets the user’s profile of following and followers
DEFINE_string(registeruser, "", "Registers the given username");
DEFINE_string(user, "", "Logs in as the given username");
DEFINE_string(caw, "", "Creates a new caw with the given text");
DEFINE_string(reply, "", "Indicates that the new caw is a reply to the given id");
DEFINE_string(follow, "", "Starts following the given username");
DEFINE_string(read, "", "Reads the caw thread starting at the given id");
DEFINE_bool(profile, false, "Gets the user’s profile of following and followers");

int main(int argc, char *argv[]) {
  gflags::SetUsageMessage("CLI Usage");
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  cout << FLAGS_user << endl;
  cout << FLAGS_profile << endl;
  return 0;
}
