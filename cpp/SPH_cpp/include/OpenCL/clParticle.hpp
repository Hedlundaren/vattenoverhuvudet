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

#include <sstream>

/// C++ representation of the OpenCL struct Particle.
/// Meant for initializing Particles and sending to GPU.
struct clParticle {
    /*TODO investigate if constructor is needed clParticle() : density(0), color_field(0) {
        position = {0};
        velocity = {0};
        force = {0};
    }*/

    cl_float3 position;
    cl_float3 velocity;

    cl_float density;

    cl_float3 force;
    cl_float color_field;
};

std::string print_clParticle(const clParticle &p) {
    std::stringstream ss;

    ss << "{" << "color_field=" << p.color_field << ", density=" << p.density << ", " <<
    "force=[" << p.force.s[0] << ", " << p.force.s[1] << ", " << p.force.s[2] << ", " << p.force.s[3] << "], " <<
    "position=[" << p.position.s[0] << ", " << p.position.s[1] << ", " << p.position.s[2] << ", " << p.position.s[3] << "], " <<
    "velocity=[" << p.velocity.s[0] << ", " << p.velocity.s[1] << ", " << p.velocity.s[2] << ", " << p.velocity.s[3] << "]" <<
    "}" << "\n";

    return ss.str();
}