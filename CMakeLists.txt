cmake_minimum_required(VERSION 3.0)
project(big_integer)

include_directories(${big_integer_SOURCE_DIR})

add_executable(big_integer_testing
        big_integer_testing.cpp
        big_integer.h
        big_integer.cpp
        dynamic_storage.h
        dynamic_storage.tpp
        gtest/gtest-all.cc
        gtest/gtest.h
        gtest/gtest_main.cc)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++17 -pedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG")
endif()

target_link_libraries(big_integer_testing -lpthread)