#include <iostream>
#include "Particle.h"
#include "Parameters.h"
#include "sph_kernels.h"
//#include <GLFW/glfw3.h>

#include "glm/glm.hpp"
#include "glm/ext.hpp"

int main() {
    const glm::vec3 vec1(1.0f, 0.0f, 0.5f), vec2(0.0f, 1.0f, 0.5f);
    const glm::vec3 sum = vec1 + vec2;

    std::cout << glm::to_string(sum) << std::endl;

    const int n_Particles = 100;

    Particle particles[n_Particles];
    Parameters p;

    std::cout << "gas constant: " << p.gasConstantK << std::endl;


    std::cout << "First particle: " << particles[0];

    std::cout << "Hello, World!" << std::endl;
    return 0;
}
