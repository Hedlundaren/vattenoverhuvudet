cmake_minimum_required(VERSION 3.4)
project(SPH_cpp)

# 32bit or 64bit shenanigans
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    MESSAGE(WARNING "64 bits compiler detected")
    SET(EX_PLATFORM 64)
    SET(EX_PLATFORM_NAME "x64")
else (CMAKE_SIZEOF_VOID_P EQUAL 8)
    MESSAGE(WARNING "32 bits compiler detected")
    SET(EX_PLATFORM 32)
    SET(EX_PLATFORM_NAME "x86")
endif (CMAKE_SIZEOF_VOID_P EQUAL 8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11  -DGL_GLEXT_PROTOTYPES")


set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(PROJECT_BINARY_DIR ${PROJECT_SOURCE_DIR}/build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build)
set(CMAKE_BINARY_DIR ${PROJECT_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)

set(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(PROJECT_CPP_DIR ${PROJECT_SOURCE_DIR}/src)
set(PROJECT_EXT_DIR ${PROJECT_SOURCE_DIR}/external)

##########################
### External libraries ###
##########################

### GLM ###
set(EXTERNAL_INCLUDE_DIRS ${EXTERNAL_INCLUDE_DIRS} ${PROJECT_EXT_DIR}/glm)

### OpenCL ###
find_package(OPENCL)
set(EXTERNAL_INCLUDE_DIRS ${EXTERNAL_INCLUDE_DIRS} ${OPENCL_INCLUDE_DIRS})

### GLFW ###
set(GLFW_DIR ${PROJECT_EXT_DIR}/glfw-3.1.2)
add_subdirectory(${GLFW_DIR})
set(EXTERNAL_INCLUDE_DIRS ${EXTERNAL_INCLUDE_DIRS} ${GLFW_DIR}/include)

# Lump all external- and project includes into one variable
set(ALL_INCLUDES ${EXTERNAL_INCLUDE_DIRS} ${PROJECT_INCLUDE_DIR})
message(WARNING "External include dirs: ${EXTERNAL_INCLUDE_DIRS}")

# Set the include directories so that #include statements work
include_directories(${ALL_INCLUDES})

# Get all source files by traversing the source directory recursively
file(GLOB_RECURSE PROJECT_CPP_FILES ${PROJECT_CPP_DIR}/*.cpp)

set(SOURCE_FILES main.cpp ${PROJECT_CPP_FILES})
add_executable(SPH_cpp ${SOURCE_FILES})

# Hardcoded fix for correct Windows DLL linking
if (NOT DEFINED OPENCL_LIBRARIES)
    message(WARNING "Whoopsie")
    set(OPENCL_LIBRARIES "C:/Windows/System32/OpenCL.dll")
endif (NOT DEFINED OPENCL_LIBRARIES)

target_link_libraries(SPH_cpp glfw ${OPENGL_glu_LIBRARY} ${GLFW_LIBRARIES})
target_link_libraries(SPH_cpp ${OPENCL_LIBRARIES})