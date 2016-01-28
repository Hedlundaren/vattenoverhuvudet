cmake_minimum_required(VERSION 3.4)
project(SPH_cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp kernels.cpp Particle.cpp)
add_executable(SPH_cpp ${SOURCE_FILES})