cmake_minimum_required(VERSION 3.28)
set (target polarization)
project(${target} LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(${target} main.cpp)
target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})