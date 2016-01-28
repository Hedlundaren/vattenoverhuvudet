#include <iostream>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.hpp>
#endif

#include "Particle.h"
#include "kernels.h"

#include "glm/glm.hpp"
#include "glm/ext.hpp"

int main() {
    const glm::vec3 vec1(1.0f, 0.0f, 0.5f), vec2(0.0f, 1.0f, 0.5f);
    const glm::vec3 sum = vec1 + vec2;

    std::cout << glm::to_string(sum) << std::endl;

    const int n_Particles = 100;

    Particle particles[n_Particles];

    std::cout << "Hello, World!" << std::endl;
    return 0;
}
