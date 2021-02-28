#ifndef CSCI499_CHENGTSU_CAW_HANDLER_H
#define CSCI499_CHENGTSU_CAW_HANDLER_H

#include <grpcpp/grpcpp.h>

#include "kvstore/kvstore_client.h"

namespace caw {
namespace handler {
// Stateless functions for the Caw platform.
//
// All these functions are in the same form:
// @param in: Request payload containing the information needed
//   by the specific function. The format depends on the the
//   specific function.
// @param out: Response payload containing the detailed information
//   the function would like to return to the remote caller. The
//   format depends on the specific function.
// @param kvstore: A kVStore client through which the function can
//   interact with the KVStore to retrieve or update data.
// @return: A gRPC Status indicating success or failure of the
//    function call, and in the case of failure, pointing out the
//    error type and carrying some error message.

// Registers a given user on the Caw platform.
// @param in: Carries a `RegisteruserRequest` message.
// @param out: Carries a `RegisteruserReply` message.
// See <project_root>/protos/caw.proto for more details.
grpc::Status RegisterUser(const google::protobuf::Any *in,
                          google::protobuf::Any *out,
                          KVStoreInterface *kvstore);

// Lets a given user follow another given user.
// @param in: Carries a `RegisteruserRequest` message.
// @param out: Carries a `RegisteruserReply` message.
// See <project_root>/protos/caw.proto for more details.
grpc::Status Follow(const google::protobuf::Any *in,
                    google::protobuf::Any *out,
                    KVStoreInterface *kvstore);

// Gets a given user’s profile of following and followers.
// @param in: Carries a `ProfileRequest` message.
// @param out: Carries a `ProfileReply` message.
// See <project_root>/protos/caw.proto for more details.
grpc::Status Profile(const google::protobuf::Any *in,
                     google::protobuf::Any *out,
                     KVStoreInterface *kvstore);

}  // namespace handler
}  // namespace caw

#endif //CSCI499_CHENGTSU_CAW_HANDLER_H
