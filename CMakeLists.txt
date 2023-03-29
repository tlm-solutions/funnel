cmake_minimum_required(VERSION 3.6)
project(funnel LANGUAGES CXX VERSION 0.0.1)

# require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)

set(CMAKE_CXX_FLAGS "-O3 -Wall -Wextra")

include(FindPkgConfig)

find_package (Threads REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(prometheus-cpp CONFIG REQUIRED)
find_package(Protobuf REQUIRED)
pkg_check_modules(CURLPP REQUIRED curlpp)

add_executable(funnel 
    src/main.cpp
    src/broadcast_server.cpp
    src/receives_waypoints.cpp
    src/protobuf/telegram.pb.cc
    src/protobuf/telegram.grpc.pb.cc
    src/prometheus.cpp
    src/prometheus.hpp
    src/api_request.hpp
)


target_link_libraries( funnel PRIVATE 
    protobuf::libprotobuf
    gRPC::grpc++
    Threads::Threads
    prometheus-cpp::pull
    ${CURLPP_LDFLAGS}
    ${PROTOBUF_LIBRARIES}
)

