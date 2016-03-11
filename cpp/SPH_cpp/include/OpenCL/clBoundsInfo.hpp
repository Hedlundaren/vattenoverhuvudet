#pragma once

#ifdef __APPLE__

#include <OpenCL/opencl.h>

#else
#include <CL/cl.hpp>
#endif

struct clBoundsInfo {
    cl_float k_wall_density;
    cl_float k_wall_force;
};