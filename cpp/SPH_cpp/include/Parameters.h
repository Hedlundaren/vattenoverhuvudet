//
// Created by Simon on 2016-01-28.
//

#ifndef SPH_CPP_PARAMETERS_H
#define SPH_CPP_PARAMETERS_H

#endif //SPH_CPP_PARAMETERS_H


#include <iostream>
#include "glm/glm.hpp"
#include "glm/ext.hpp"

/**********************************
* Class Particle               *
***********************************/

class Parameters {
public:

    //Constructor
    Parameters();

    ~Parameters();


    float dt = 1.0 / 30;
    float mass = 1.0f;
    float kernelSize = 10.0f;
    float gasConstantK = 462.0f;
    float viscosityConstant = 0.001f;
    float restDensity =  1.0f;
    float sigma = 0.0072;
    float nThreshold = 0.02;
    //const glm::vec3 gravity(0.0f, -9.82f, 0.0f);
    int leftBound = 0;
    int rightBound = 100;
    int bottomBound = 0;
    int topBound = 100;
    float wallDamper = 0.6;

};