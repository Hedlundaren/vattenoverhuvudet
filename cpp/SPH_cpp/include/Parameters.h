//
// Created by Simon on 2016-01-28.
//

#pragma once

#include <iostream>
#include <algorithm>

#include "glm/glm.hpp"
#include "glm/ext.hpp"

#ifdef __APPLE__

#include <OpenCL/opencl.h>

#else
#include <CL/cl.hpp>
#endif

namespace Parameters {
    constexpr float mass = 0.5f;
    constexpr float kernelSize = 0.25f;
    constexpr float gasConstantK = 1.0f;
    constexpr float viscosityConstant = 10.0f;
    constexpr float restDensity = 0.0f;
    constexpr float sigma = 0.0072f;
    constexpr float nThreshold = 0.02f;
    const glm::vec3 gravity(0.0f, -9.82f, 0.0f);
    constexpr float leftBound = -0.9f;
    constexpr float rightBound = 0.9f;
    constexpr float bottomBound = -0.9f;
    constexpr float topBound = 0.9f;
    constexpr float nearBound = -0.9f;
    constexpr float farBound = 0.9f;
    constexpr float wallDamper = 1.0f;

    inline float get_max_volume_side() {
        return std::max(std::max(rightBound - leftBound, topBound - bottomBound), farBound - nearBound);
    }

    constexpr float get_volume_size_x() {
        return rightBound - leftBound;
    }

    constexpr float get_volume_size_y() {
        return topBound - bottomBound;
    }

    constexpr float get_volume_size_z() {
        return farBound - nearBound;
    }

    inline const cl_float3 get_volume_origin_corner_cl() {
        cl_float3 origin;
        origin.s[0] = leftBound;
        origin.s[1] = bottomBound;
        origin.s[2] = nearBound;

        return origin;
    }
}
