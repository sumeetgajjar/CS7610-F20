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
        src/common/message.h
        src/common/message.cpp
        src/common/utils.h
        src/common/utils.cpp
        src/common/network_utils.h
        src/common/network_utils.cpp
        src/common/serde.h
        src/common/serde.cpp
        src/part1/multicast.h
        src/part1/multicast.cpp
        src/part2/snapshot.h
        src/part2/snapshot.cpp
        )

target_link_libraries(lab1 glog::glog gflags::gflags)