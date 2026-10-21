cmake_minimum_required(VERSION 3.28)
set (target cone)
project(${target} LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SoQt REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(${target} main.cpp)
target_link_libraries(${target} PRIVATE SoQt::SoQt Qt6::Widgets)