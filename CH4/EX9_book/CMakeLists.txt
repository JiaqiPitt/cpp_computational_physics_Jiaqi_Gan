cmake_minimum_required(VERSION 3.28)
set (target colorband)
project(${target} LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qat REQUIRED)
find_package(GSL REQUIRED)

set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_BUILD_RPATH "${Qat_DIR}/../..")

set(EX8_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../EX8_book")

add_executable(${target} main.cpp ${EX8_DIR}/Photoreceptor.cpp)
target_include_directories(${target} PRIVATE ${EX8_DIR})
target_link_libraries(${target} PRIVATE Qat::QatGenericFunctions
  Qt6::Widgets Qt6::Core GSL::gsl)