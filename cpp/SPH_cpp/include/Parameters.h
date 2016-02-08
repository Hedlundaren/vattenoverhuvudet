//
// Created by Simon on 2016-01-28.
//

#pragma once

#include <iostream>
#include <algorithm>

#include "glm/glm.hpp"
#include "glm/ext.hpp"

namespace Parameters {
    constexpr float mass = 0.5f;
    constexpr float kernelSize = 0.5f;
    constexpr float gasConstantK = 1.0f;
    constexpr float viscosityConstant = 10.0f;
    constexpr float restDensity = 0.0f;
    constexpr float sigma = 0.0072f;
    constexpr float nThreshold = 0.02f;
    const glm::vec3 gravity(0.0f, -9.82f, 0.0f);
    constexpr float leftBound = -1.9f;
    constexpr float rightBound = 1.9f;
    constexpr float bottomBound = -1.5f;
    constexpr float topBound = 1.9f;
    constexpr float nearBound = -1.9f;
    constexpr float farBound = 1.9f;
    constexpr float wallDamper = 1.0f;

    inline float get_max_volume_side() {
        return std::max(std::max(rightBound - leftBound, topBound - bottomBound), farBound - nearBound);
    }
}
