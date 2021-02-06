cmake_minimum_required(VERSION 3.16)
project(cs499_chengtsu)

set(CMAKE_CXX_STANDARD 17)

find_package(gflags REQUIRED)

add_executable(cli src/cli.cc)
target_link_libraries(cli gflags)
