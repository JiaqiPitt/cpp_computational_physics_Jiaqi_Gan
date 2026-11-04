cmake_minimum_required(VERSION 3.28)
set (target seir)
project(${target} LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qat REQUIRED)

set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_BUILD_RPATH "${Qat_DIR}/../..")

add_executable(${target} main.cpp)
target_link_libraries(${target} PRIVATE Qat::QatPlotWidgets Qat::QatPlotting
  Qat::QatDataModeling Qat::QatDataAnalysis Qat::QatGenericFunctions
  Qt6::Widgets Qt6::Core)
