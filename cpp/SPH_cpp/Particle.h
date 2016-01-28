
#ifndef PARTICLE_H
#define PARTICLE_H

/**********************************
* Class Particle               *
***********************************/

class Particle
{
public:

    //Constructor
    Particle();
    ~Particle();

    float position [3] = {0.0f, 0.0f, 0.0f};
    float velocity [3] = {0.0f, 0.0f, 0.0f};
    float force [3] = {0.0f, 0.0f, 0.0f};
    float density = 0.0f;
    float pressure = 0.0f;
    float cs = 0.0f;

    friend ostream& operator<<(ostream& os, const Particle& P);

};

