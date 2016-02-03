#include "Particle.h"

Particle::Particle(glm::vec3 pos, glm::vec3 vel)
        : position(pos), velocity(vel),
          force(0), density(0), pressure(0), color_field(0) {
}

std::ostream &operator<<(std::ostream &out, const Particle &p) {
    out << "x: " << p.position[0] << " y: " << p.position[1] << " z: " << p.position[2] << std::endl;

    return out;
}
