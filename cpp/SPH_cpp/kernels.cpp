#include <iostream>
#include <vector>

#define _USE_MATH_DEFINES
#include <cmath>

#include "kernels.h"

float normVector(std::vector<float> v) {
    float sum = 0.0f;

    //c++11
    for (float f : v) {
        sum += f * f;
    }

    return sqrt(sum);
}

float Wpoly6(std::vector<float> r, float h) {
    float w = 0.0f;
    float radius = normVector(r);

    if (radius < h && radius >= 0)
        w = (315 / (64 * M_PI * std::pow(h, 9))) * std::pow((std::pow(h, 2) - std::pow(radius, 2)), 3);
    else
        w = 0;

    return w;
}
