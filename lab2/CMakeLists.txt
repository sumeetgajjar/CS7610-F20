cmake_minimum_required(VERSION 3.17)
project(lab2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_COMPILER gcc-7)
set(CMAKE_CXX_COMPILER g++-7)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
find_package(gflags 2.2.2 REQUIRED COMPONENTS nothreads_static)
find_package(glog 0.4.0 REQUIRED)

configure_file(hostfile hostfile COPYONLY)

add_executable(lab2
        src/main.cpp
        src/network_utils.h
        src/network_utils.cpp
        src/utils.h
        src/utils.cpp
        )

target_link_libraries(lab2 glog::glog gflags::gflags)