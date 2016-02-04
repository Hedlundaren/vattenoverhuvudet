//
// Created by Simon on 2016-01-28.
//

#ifndef SPH_CPP_PARAMETERS_H
#define SPH_CPP_PARAMETERS_H

#endif //SPH_CPP_PARAMETERS_H


#include <iostream>
#include "glm/glm.hpp"
#include "glm/ext.hpp"

namespace Parameters {
    float dt = 1.0f / 30.0f;
    float mass = 1.0f;
    float kernelSize = 1.0f;
    float gasConstantK = 1.0f;
    float viscosityConstant = 5.0f;
    float restDensity = 0.0f;
    float sigma = 0.0072f;
    float nThreshold = 0.02f;
    const glm::vec3 gravity(0.0f, -9.82f, 0.0f);
    float leftBound = -0.9f;
    float rightBound = 0.9f;
    float bottomBound = -0.9f;
    float topBound = 0.9f;
    float nearBound = -0.9f;
    float farBound = 0.9f;
    float wallDamper = 1.0f;
}
