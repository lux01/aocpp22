cmake_minimum_required(VERSION 3.23)
project(AdventOfCode VERSION 2022)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}")
find_package(Boost REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(day11 day11.cpp)
add_executable(day12 day12.cpp)
add_executable(day13 day13.cpp)
target_link_libraries(day13 PRIVATE Boost::headers Boost::json)
add_executable(day14 day14.cpp)