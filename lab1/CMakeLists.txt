cmake_minimum_required(VERSION 3.17)
project(lab1)

set(CMAKE_CXX_STANDARD 17)

include(dependenciesCMake/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

add_executable(lab1 main.cpp)
target_link_libraries(lab1 CONAN_PKG::glog CONAN_PKG::gflags)