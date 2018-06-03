cmake_minimum_required(VERSION 3.11)
project(big_integer)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES big_integer.cpp big_integer.h big_integer_testing.cpp gtest/gtest.h gtest/gtest-all.cc gtest/gtest_main.cc dynamic_storage/dynamic_storage.h)
add_executable(big_integer ${SOURCE_FILES})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall")