cmake_minimum_required(VERSION 3.17)
project(lab1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_COMPILER gcc-7)
set(CMAKE_CXX_COMPILER g++-7)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/dependenciesCMake ${CMAKE_MODULE_PATH})
find_package(gflags 2.2.2 REQUIRED)
find_package(glog 0.4.0 REQUIRED)

add_executable(lab1 main.cpp)

target_link_libraries(lab1 glog::glog gflags::gflags)