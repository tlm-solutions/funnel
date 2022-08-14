cmake_minimum_required(VERSION 3.6)
project(funnel LANGUAGES CXX VERSION 0.0.1)

# require C++17
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package (Threads REQUIRED)
find_package(gRPC CONFIG REQUIRED)
#pkg_search_module(GRPCPP REQUIRED grpc++>=1.22.0)
#find_library(BOOST boost)

add_executable(funnel src/main.cpp src/protobuf/telegram.pb.cc src/protobuf/telegram.grpc.pb.cc )


target_link_libraries( funnel PRIVATE 
    protobuf::libprotobuf
    gRPC::grpc++
    #GRPCPP
    Threads::Threads
)

#target_compile_definitions(asio PUBLIC ASIO_STANDALONE)

