
#include "Particle.h"

Particle::Particle(){
    position[0] = rand();
    position[1] = rand();
    position[2] = rand();
}

ostream& operator<<(ostream& out, const Particle& p)
{
    out << "x: " << p.position[0] << " y: " << p.position[1] << " z: " << p.position[2] << endl;

    return out;
}
