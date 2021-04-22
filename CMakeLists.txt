cmake_minimum_required(VERSION 3.16)
project(cs499_chengtsu)

set(CMAKE_CXX_STANDARD 17)

find_package(gflags REQUIRED)
find_package(glog REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
find_package(gRPC CONFIG REQUIRED)
set(_GRPC_GRPCPP gRPC::grpc++)
set(_REFLECTION gRPC::grpc++_reflection)
find_package(Threads REQUIRED)
find_package(GTest REQUIRED)

# Group gRPC related libraries together.
set(GRPC_LIBS ${_REFLECTION} ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF})

add_subdirectory(protos)

# Include the output directory of the generated proto files
# and the project source directory.
include_directories(${CMAKE_BINARY_DIR}/protos cpp)

# Target: KVStore server
set(_kvstore_server kvstore_server)
add_executable(${_kvstore_server}
        cpp/kvstore/kvstore_server.cc
        cpp/kvstore/kvstore_service.cc
        cpp/kvstore/kvstore.cc)
target_link_libraries(${_kvstore_server} PUBLIC
        kvstore_grpc ${GRPC_LIBS} glog gflags)

# Target: KVStore client
set(_kvstore_client kvstore_client)
add_library(${_kvstore_client} STATIC
        cpp/kvstore/kvstore_client.cc)
target_link_libraries(${_kvstore_client} PUBLIC
        kvstore_grpc ${GRPC_LIBS})

# Target: Faz server
set(_faz_server faz_server)
add_executable(${_faz_server}
        cpp/faz/faz_server.cc
        cpp/faz/faz_service.cc
        cpp/caw/caw_handler.cc)
target_link_libraries(${_faz_server} PUBLIC
        caw_grpc faz_grpc ${_kvstore_client} ${GRPC_LIBS} glog gflags)

# Target: Caw client
set(_caw_client caw_client)
add_library(${_caw_client} STATIC
        cpp/caw/caw_client.cc)
target_link_libraries(${_caw_client} PUBLIC
        caw_grpc faz_grpc ${GRPC_LIBS})

# Target: Caw CLI (built from C++ sources)
set(_caw_cli caw_cli)
add_executable(${_caw_cli}
        cpp/caw/caw_cli.cc)
target_link_libraries(${_caw_cli} PUBLIC
        caw_grpc ${_caw_client} ${GRPC_LIBS} gflags)

# Target: KVStore Test
set(_kvstore_test kvstore_test)
add_executable(${_kvstore_test}
        test/kvstore_test.cc
        cpp/kvstore/kvstore.cc)
target_link_libraries(${_kvstore_test} PUBLIC
        gtest glog pthread)

# Target: KVStore Shell (for testing use)
set(_kvstore_shell_test kvstore_shell_test)
add_executable(${_kvstore_shell_test}
        test/kvstore_shell_test.cc)
target_link_libraries(${_kvstore_shell_test}
        ${_kvstore_client} gflags)

# Target: Caw Handler Test
set(_caw_handler_test caw_handler_test)
add_executable(${_caw_handler_test}
        test/caw_handler_test.cc
        cpp/caw/caw_handler.cc
        cpp/kvstore/kvstore.cc)
target_link_libraries(${_caw_handler_test}
        GTest::GTest glog caw_grpc ${GRPC_LIBS})

# Target: Caw CLI (built from Go sources)
set(_caw_cli_go caw_cli_go)
add_custom_target(${_caw_cli_go} ALL
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/go.mod
            ${CMAKE_CURRENT_SOURCE_DIR}/go.sum
            ${CMAKE_CURRENT_SOURCE_DIR}/go/caw_cli.go
            ${CMAKE_CURRENT_SOURCE_DIR}/go/caw_client.go
            ${CMAKE_BINARY_DIR}
        COMMAND GO111MODULE=on go build -o ${_caw_cli_go}
        COMMAND ${CMAKE_COMMAND} -E remove
            ${CMAKE_BINARY_DIR}/go.mod
            ${CMAKE_BINARY_DIR}/go.sum
            ${CMAKE_BINARY_DIR}/caw_cli.go
            ${CMAKE_BINARY_DIR}/caw_client.go
        )
add_dependencies(${_caw_cli_go} caw_grpc_go faz_grpc_go)