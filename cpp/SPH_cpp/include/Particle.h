#pragma once

#include <iostream>

#include "glm/glm.hpp"

/// @brief Simple struct for a particle
struct Particle {
    Particle(glm::vec3 pos, glm::vec3 vel);

    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 force;
    float density;
    float pressure;
    float color_field;
};

std::ostream &operator<<(std::ostream &os, const Particle &P);