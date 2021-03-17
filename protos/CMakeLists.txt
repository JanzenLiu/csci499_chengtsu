# Minimum CMake required
cmake_minimum_required(VERSION 3.16)

# Project
project(cs499_chengtsu_protos LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# Find Protobuf installation
# Looks for protobuf-config.cmake file installed by Protobuf's cmake installation.
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")
set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
find_program(_PROTOBUF_PROTOC protoc)

# Find gRPC installation
# Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")
set(_GRPC_GRPCPP gRPC::grpc++)
find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)

# Get the filenames of all the proto files.
file(GLOB PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.proto)

# Generate header and source files for all the proto files.
foreach(proto ${PROTO_FILES})
    # Get filename without extension.
    get_filename_component(proto_name_we ${proto} NAME_WE)
    get_filename_component(proto_name ${proto} NAME)
    get_filename_component(proto_dir "${proto}" PATH)
    message(STATUS "Configuring for ${proto_name}")

    # Generate header and source files.
    set(proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name_we}.pb.cc")
    set(proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name_we}.pb.h")
    set(grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name_we}.grpc.pb.cc")
    set(grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name_we}.grpc.pb.h")
    add_custom_command(
            OUTPUT "${proto_srcs}" "${proto_hdrs}" "${grpc_srcs}" "${grpc_hdrs}"
            COMMAND ${_PROTOBUF_PROTOC}
            ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
            --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
            -I "${proto_dir}"
            --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
            "${proto}"
            DEPENDS "${proto}")

    # Compile to a static library.
    set(_target ${proto_name_we}_grpc)
    add_library(${_target} STATIC ${proto_srcs} ${proto_hdrs} ${grpc_srcs} ${grpc_hdrs})
    target_include_directories(${_target} PUBLIC ${Protobuf_INCLUDE_DIRS})

    # Generate go files.
    set(go_output_dir "${CMAKE_CURRENT_BINARY_DIR}/${proto_name_we}")
    set(proto_gos "${go_output_dir}/${proto_name_we}.pb.go")
    set(grpc_gos "${go_output_dir}/${proto_name_we}_grpc.pb.go")
    add_custom_command(
            OUTPUT "${proto_gos}" "${grpc_gos}"
            COMMAND mkdir -p "${go_output_dir}"
            COMMAND ${_PROTOBUF_PROTOC}
            ARGS --go_out="${go_output_dir}"
            --go_opt=paths=source_relative
            --go-grpc_out="${go_output_dir}"
            --go-grpc_opt=paths=source_relative
            -I "${proto_dir}"
            "${proto}"
            DEPENDS "${proto}")
    add_custom_target(${proto_name_we}_grpc_go
            DEPENDS "${proto_gos}" "${grpc_gos}")
endforeach()