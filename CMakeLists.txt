cmake_minimum_required(VERSION 3.29)
project(safe_ranges)

set(CMAKE_CXX_STANDARD 23)

add_executable(safe_ranges main.cpp
        safe_ranges.cpp)
