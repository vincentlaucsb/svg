cmake_minimum_required(VERSION 3.9)
project(SVG)

set(CMAKE_CXX_FLAGS "-lpthread -static-libgcc -static-libstdc++")
set(CMAKE_CXX_STANDARD 14)
set(SOURCES src/svg.hpp)

include_directories(${CMAKE_SOURCE_DIR}/src/)
include_directories(${CMAKE_SOURCE_DIR}/tests/)
add_executable(SVG_Test ${SOURCES} tests/catch.hpp tests/svg_tests.cpp)
add_executable(basic ${SOURCES} examples/basic.cpp)

enable_testing()
add_test(test SVG_TEST)