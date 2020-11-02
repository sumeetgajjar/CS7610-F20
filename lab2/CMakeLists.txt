cmake_minimum_required(VERSION 3.17)
project(lab2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_COMPILER gcc-7)
set(CMAKE_CXX_COMPILER g++-7)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -Wall)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
find_package(gflags 2.2.2 REQUIRED COMPONENTS nothreads_static)
find_package(glog 0.4.0 REQUIRED)

configure_file(hostfile hostfile COPYONLY)

add_executable(lab2
        src/main.cpp
        src/network_utils.cpp
        src/network_utils.h
        src/utils.cpp
        src/utils.h
        src/membership.cpp
        src/membership.h
        src/message.cpp
        src/message.h
        src/serde.cpp
        src/serde.h
        src/failure_detector.h
        src/failure_detector.cpp)

target_link_libraries(lab2 glog::glog gflags::gflags)