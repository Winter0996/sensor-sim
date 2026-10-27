cmake_minimum_required(VERSION 3.14)
project(sensor_sim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Use extraction-time timestamps for downloaded archives (silences CMP0135 warning).
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

# Core library shared by the sender, receiver, and tests
add_library(sensor_parser src/parser.cpp src/sequence_tracker.cpp)
target_include_directories(sensor_parser PUBLIC src)
target_compile_options(sensor_parser PRIVATE -Wall -Wextra -Werror)

# UDP sender and receiver
add_executable(sender src/sender.cpp)
target_link_libraries(sender PRIVATE sensor_parser)
target_compile_options(sender PRIVATE -Wall -Wextra -Werror)

add_executable(receiver src/receiver.cpp)
target_link_libraries(receiver PRIVATE sensor_parser)
target_compile_options(receiver PRIVATE -Wall -Wextra -Werror)

# Google Test
include(FetchContent)
FetchContent_Declare(googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(unit_tests
  tests/test_parser.cpp
  tests/test_sequence_tracker.cpp)
target_link_libraries(unit_tests PRIVATE sensor_parser GTest::gtest_main)
target_compile_options(unit_tests PRIVATE -Wall -Wextra -Werror)

include(GoogleTest)
gtest_discover_tests(unit_tests)