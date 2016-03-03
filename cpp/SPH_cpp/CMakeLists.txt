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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -DGL_GLEXT_PROTOTYPES")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE} -O2")

if (APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGLFW_INCLUDE_GLCOREARB")
endif (APPLE)

## Comment out this line to use C++ simulation
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_OPENCL_SIMULATION")

## Define if in debug mode or not (comment out line below to disable logging -> faster)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMY_DEBUG")

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(PROJECT_BINARY_DIR ${PROJECT_SOURCE_DIR}/build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build)
set(CMAKE_BINARY_DIR ${PROJECT_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)

set(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(PROJECT_KERNELS_DIR ${PROJECT_SOURCE_DIR}/kernels)
set(PROJECT_CPP_DIR ${PROJECT_SOURCE_DIR}/src)
set(PROJECT_EXT_DIR ${PROJECT_SOURCE_DIR}/external)

# Configuration of OpenCL simulation parameters
set(VOXEL_CELL_PARTICLE_COUNT 64)
add_definitions(-DVOXEL_CELL_PARTICLE_COUNT=${VOXEL_CELL_PARTICLE_COUNT})
configure_file(${PROJECT_KERNELS_DIR}/configurable/simulate_fluid_particles.cl ${PROJECT_KERNELS_DIR}/simulate_fluid_particles.cl @ONLY)

if (CMAKE_BUILD_TYPE EQUAL "DEBUG")
    message(WARNING "Debug mode")
elseif (CMAKE_BUILD_TYPE EQUAL "DEBUG")
    message(WARNING "Release mode")
endif (CMAKE_BUILD_TYPE EQUAL "DEBUG")

##########################
### External libraries ###
##########################

### OpenGL ###
set(ALL_LIBRARIES ${ALL_LIBRARIES} ${OPENGL_glu_LIBRARY})

### GLM ###
set(EXTERNAL_INCLUDE_DIRS ${EXTERNAL_INCLUDE_DIRS} ${PROJECT_EXT_DIR}/glm)

### OpenCL ###
find_package(OPENCL)
set(EXTERNAL_INCLUDE_DIRS ${EXTERNAL_INCLUDE_DIRS} ${OPENCL_INCLUDE_DIRS})
if (NOT DEFINED OPENCL_LIBRARIES)
    # Hardcoded fix for correct Windows DLL linking
    message(WARNING "Whoopsie")
    set(OPENCL_LIBRARIES "C:/Windows/System32/OpenCL.dll")
endif (NOT DEFINED OPENCL_LIBRARIES)
set(ALL_LIBRARIES ${ALL_LIBRARIES} ${OPENCL_LIBRARIES})

### NanoGUI ###
set(NANOGUI_BUILD_EXAMPLE NO)
add_subdirectory(${PROJECT_EXT_DIR}/nanogui)
set(ALL_LIBRARIES ${ALL_LIBRARIES} nanogui)
set(EXTERNAL_INCLUDE_DIRS ${EXTERNAL_INCLUDE_DIRS} ${NanoGUI_SOURCE_DIR}/include ${GLFW_SOURCE_DIR}/include)

### GLEW on Windows
if (MINGW)
    set(GLEW_DIR ${PROJECT_EXT_DIR}/glew)

    set(GLEW_INCLUDE_DIR ${GLEW_DIR}/include)
    #set(GLEW_LIBRARIES ${GLEW_DIR}/lib/Release/x64/glew32s.lib)
    #set(GLEW_LIBRARIES "C:/Windows/System32/glew32.dll")
    set(GLEW_LIBRARIES ${GLEW_DIR}/bin/Release/x64/glew32.dll)

    set(EXTERNAL_INCLUDE_DIRS ${EXTERNAL_INCLUDE_DIRS} ${GLEW_INCLUDE_DIR})
    set(ALL_LIBRARIES ${ALL_LIBRARIES} ${GLEW_LIBRARIES})
endif (MINGW)

# Lump all external- and project includes into one variable
set(ALL_INCLUDES ${EXTERNAL_INCLUDE_DIRS} ${PROJECT_INCLUDE_DIR} ${PROJECT_KERNELS_DIR})

# Set the include directories so that #include statements work
include_directories(${ALL_INCLUDES})
message(WARNING "All include dirs: ${ALL_INCLUDES}")

# Get all source files by traversing the source directory recursively
file(GLOB_RECURSE PROJECT_CPP_FILES ${PROJECT_CPP_DIR}/*.cpp)

set(SOURCE_FILES main.cpp ${PROJECT_CPP_FILES})
add_executable(SPH_cpp ${SOURCE_FILES})

target_link_libraries(SPH_cpp ${ALL_LIBRARIES})
message(WARNING "All libraries: ${ALL_LIBRARIES}")