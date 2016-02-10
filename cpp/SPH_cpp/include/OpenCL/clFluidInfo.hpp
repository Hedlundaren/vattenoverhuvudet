#pragma once

#ifdef __APPLE__

#include <OpenCL/opencl.h>
#include <OpenGL/OpenGL.h>

#else
#include <CL/cl.hpp>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

struct clFluidInfo {
    // The mass of each particle
    cl_float mass;
};