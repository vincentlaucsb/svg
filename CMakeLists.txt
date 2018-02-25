cmake_minimum_required(VERSION 3.9)
project(SVG)

set(CMAKE_CXX_FLAGS "-lpthread -static-libgcc -static-libstdc++")
set(CMAKE_CXX_STANDARD 11)

include_directories(${CMAKE_SOURCE_DIR}/src/)
include_directories(${CMAKE_SOURCE_DIR}/tests/)
add_executable(SVG_Test ${SOURCES} src/svg.hpp tests/catch.hpp tests/svg_tests.cpp)
