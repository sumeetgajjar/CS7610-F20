cmake_minimum_required(VERSION 3.17)
project(lab1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_COMPILER gcc-7)
set(CMAKE_CXX_COMPILER g++-7)

configure_file(hostfile hostfile COPYONLY)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake_modules ${CMAKE_MODULE_PATH})
find_package(gflags 2.2.2 REQUIRED COMPONENTS nothreads_static)
find_package(glog 0.4.0 REQUIRED)

add_executable(lab1
        src/main.cpp
        src/message.h
        src/message.cpp
        src/utils.h
        src/utils.cpp
        src/network_utils.h
        src/network_utils.cpp
        src/probe_utils.h
        src/probe_utils.cpp
        src/multicast.h
        src/multicast.cpp
        src/snapshot.h
        src/snapshot.cpp
        src/serde.h
        src/serde.cpp)

target_link_libraries(lab1 glog::glog gflags::gflags)