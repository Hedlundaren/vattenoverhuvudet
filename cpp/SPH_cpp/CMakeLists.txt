cmake_minimum_required(VERSION 3.4)
project(SPH_cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)
set(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(PROJECT_CPP_DIR ${PROJECT_SOURCE_DIR}/src)
set(PROJECT_EXT_DIR ${PROJECT_SOURCE_DIR}/external)

##########################
### External libraries ###
##########################

### GLM ###
set(EXTERNAL_INCLUDE_DIRS ${EXTERNAL_INCLUDE_DIRS} ${PROJECT_EXT_DIR}/glm)

# Lump all external- and project includes into one variable
set(ALL_INCLUDES ${EXTERNAL_INCLUDE_DIRS} ${PROJECT_INCLUDE_DIR})

# Set the include directories so that #include statements work
include_directories(${ALL_INCLUDES})

# Get all source files by traversing the source directory recursively
file(GLOB_RECURSE PROJECT_CPP_FILES ${PROJECT_CPP_DIR}/*.cpp)

set(SOURCE_FILES main.cpp ${PROJECT_CPP_FILES} src/calculateForces.cpp include/Parameters.h src/Parameters.cpp)
add_executable(SPH_cpp ${SOURCE_FILES})