cmake_minimum_required(VERSION 3.14)
project(sensor_sim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Parsing library (reused later by the UDP sender and receiver)
add_library(sensor_parser src/parser.cpp)
target_include_directories(sensor_parser PUBLIC src)
target_compile_options(sensor_parser PRIVATE -Wall -Wextra -Werror)

# Google Test
include(FetchContent)
FetchContent_Declare(googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(unit_tests tests/test_parser.cpp)
target_link_libraries(unit_tests PRIVATE sensor_parser GTest::gtest_main)
target_compile_options(unit_tests PRIVATE -Wall -Wextra -Werror)

include(GoogleTest)
gtest_discover_tests(unit_tests)