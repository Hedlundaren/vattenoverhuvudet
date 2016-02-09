#pragma once

#ifdef __APPLE__

#include <OpenCL/opencl.h>

#else
#include <CL/cl.hpp>
#endif

#include <sstream>

struct clParticleData {
    cl_float density;
    cl_float3 force;
    cl_float color_field;
};

std::string print_clParticleData(const clParticleData &data) {
    std::stringstream ss;

    ss <<
    "clParticleData: {density=" << data.density <<
    " force=[" << data.force.s[0] << " " << data.force.s[1] << " " << data.force.s[2] << "]" <<
    " color_field=" << data.color_field <<
    "}";

    return ss.str();
}