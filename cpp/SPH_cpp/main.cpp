#include <iostream>
#include <fstream>
#include <vector>

#include "Particle.h"
#include "Parameters.h"
#include "sph_kernels.h"

#include "glm/glm.hpp"
#include "glm/ext.hpp"

#include "math/randomized.hpp"
#include "common/stream_utils.hpp"

#include "opencl_context_info.hpp"

int main() {
    PrintOpenClContextInfo();

    const glm::vec3 vec1(1.0f, 0.0f, 0.5f), vec2(0.0f, 1.0f, 0.5f);
    const glm::vec3 sum = vec1 + vec2;

    std::cout << glm::to_string(sum) << std::endl;

    const int n_Particles = 100;

    Particle particles[n_Particles];

    std::cout << "Hello, World!" << std::endl;
    return 0;
}
