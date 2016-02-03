cmake_minimum_required(VERSION 3.4)
project(OpenCLtest)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmakemodules)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package(OpenCL REQUIRED)
include_directories(${OPENCL_INCLUDE_DIR})

set(SOURCE_FILES main.cpp)
add_executable(OpenCLtest ${SOURCE_FILES})
target_link_libraries(OpenCLtest ${OPENCL_LIBRARY})