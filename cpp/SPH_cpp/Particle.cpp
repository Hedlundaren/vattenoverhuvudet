#include "Particle.h"

Particle::Particle() {
    position[0] = rand();
    position[1] = rand();
    position[2] = rand();
}


Particle::~Particle() {

}

std::ostream &operator<<(std::ostream &out, const Particle &p) {
    out << "x: " << p.position[0] << " y: " << p.position[1] << " z: " << p.position[2] << std::endl;

    return out;
}
