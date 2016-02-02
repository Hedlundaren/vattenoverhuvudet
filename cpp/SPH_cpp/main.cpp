#include <iostream>
#include "Particle.h"
#include "Parameters.h"
#include "sph_kernels.h"

#include "math/randomized.hpp"
#include "common/stream_utils.hpp"

int main() {
    auto vec3s = generate_uniform_vec3s(10);

    std::cout << std::endl << to_string(vec3s, ", \n") << std::endl;

    const int n_Particles = 100;

    Particle particles[n_Particles];
    Parameters p;

    std::cout << "gas constant: " << p.gasConstantK << std::endl;


    std::cout << "First particle: " << particles[0];

    std::cout << "Hello, World!" << std::endl;
    return 0;
}
