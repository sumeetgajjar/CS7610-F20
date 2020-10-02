cmake_minimum_required(VERSION 3.17)
project(lab1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_COMPILER gcc-7)
set(CMAKE_CXX_COMPILER g++-7)

configure_file(hostfile hostfile COPYONLY)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake_modules ${CMAKE_MODULE_PATH})
find_package(gflags 2.2.2 REQUIRED)
find_package(glog 0.4.0 REQUIRED)

add_executable(lab1 src/main.cpp src/message.h src/utils.h src/utils.cpp src/network_utils.h src/network_utils.cpp src/flags.h src/config.h)

target_link_libraries(lab1 glog::glog gflags::gflags)