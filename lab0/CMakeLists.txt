cmake_minimum_required(VERSION 3.17)
project(lab0)

set(CMAKE_CXX_STANDARD 17)

if (NOT DEFINED UDP_PORT)
    set(UDP_PORT 7610)
endif()

if (NOT DEFINED HOST_FILE_PATH)
    message(WARNING "hostfile path not defined")
    message("Setting HOST_FILE_PATH to '${PROJECT_SOURCE_DIR}/hostfile'")
    set(HOST_FILE_PATH \"${PROJECT_SOURCE_DIR}/hostfile\")
endif()

configure_file(hostfile hostfile COPYONLY)
configure_file(config.h.in config.h)
include_directories("${PROJECT_BINARY_DIR}")
add_executable(lab0 main.cpp)