cmake_minimum_required(VERSION 3.17)
project(lab0)

set(CMAKE_CXX_STANDARD 17)
#SET(CMAKE_CXX_FLAGS -pthread)

if (NOT DEFINED UDP_PORT)
    set(UDP_PORT 7610)
endif()

find_package(glog 0.4.0 REQUIRED)

configure_file(hostfile hostfile COPYONLY)
configure_file(config.h.in config.h)
include_directories("${PROJECT_BINARY_DIR}")

add_executable(lab0 main.cpp utils.cpp utils.h socket_utils.cpp socket_utils.h heartbeat.cpp heartbeat.h)
target_link_libraries(lab0 glog::glog)