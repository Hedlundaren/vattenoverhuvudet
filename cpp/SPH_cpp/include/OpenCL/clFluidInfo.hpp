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
    // The mass of each fluid particle
    cl_float mass;

    cl_float k_gas;
    cl_float k_viscosity;
    cl_float rest_density;
    cl_float sigma;
    cl_float k_threshold;
    cl_float k_wall_damper;
    cl_float k_wall_friction;

    cl_float3 gravity;
};