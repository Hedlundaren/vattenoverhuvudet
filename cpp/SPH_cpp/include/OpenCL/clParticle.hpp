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

/// C++ representation of the OpenCL struct Particle.
/// Meant for initializing Particles and sending to GPU.
struct clParticle {
    cl_float3 position;
    cl_float3 velocity;

    cl_float density;

    cl_float3 force;
    cl_float color_field;
};