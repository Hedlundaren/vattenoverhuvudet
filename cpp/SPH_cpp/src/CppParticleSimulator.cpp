#include "CppParticleSimulator.hpp"

#include <iostream>
#include <vector>
#include "sph_kernels.h"
#include "Parameters.hpp"

void CppParticleSimulator::setupSimulation(const Parameters &parameters,
                                           const std::vector<glm::vec3> &particle_positions,
                                           const std::vector<glm::vec3> &particle_velocities,
                                           const GLuint &vbo_positions,
                                           const GLuint &vbo_velocities) {
    positions = particle_positions;
    velocities = particle_velocities;

    vbo_pos = vbo_positions;
    vbo_vel = vbo_velocities;

    forces.resize(positions.size());
    velocities.resize(positions.size());
    densities.resize(positions.size());
}

void CppParticleSimulator::updateSimulation(const Parameters &parameters, float dt_seconds) {

    // Set forces to 0 and calculate densities
    for (int i = 0; i < positions.size(); ++i) {
        forces[i] = {0, 0, 0};
        float density = 0;

        for (int j = 0; j < positions.size(); ++j) {
            glm::vec3 relativePos = positions[i] - positions[j];
            density += Params::mass * Wpoly6(relativePos, Params::kernelSize);
        }

        densities[i] = density;
    }

    // Calculate forces
    for (int i = 0; i < positions.size(); ++i) {
        float iPressure = (densities[i] - Params::restDensity) * Params::gasConstantK;
        float cs = 0;
        glm::vec3 n = {0, 0, 0};
        float laplacianCs = 0;

        glm::vec3 pressureForce = {0, 0, 0};
        glm::vec3 viscosityForce = {0, 0, 0};

        for (int j = 0; j < positions.size(); ++j) {
            glm::vec3 relativePos = positions[i] - positions[j];

            // Particle j's pressure force on i
            float jPressure = (densities[j] - Params::restDensity) * Params::gasConstantK;
            pressureForce = pressureForce - Params::mass *
                ((iPressure + jPressure) / (2 * densities[j])) *
                gradWspiky(relativePos, Params::kernelSize);

            // Particle j's viscosity force in i
            viscosityForce += Params::viscosityConstant *
                Params::mass * ((velocities[j] - velocities[i]) / densities[j]) *
                laplacianWviscosity(relativePos, Params::kernelSize);

            // cs for particle j
            cs += Params::mass * (1 / densities[j]) * Wpoly6(relativePos, Params::kernelSize);

            // Gradient of cs for particle j
            n += Params::mass * (1 / densities[j]) * gradWpoly6(relativePos, Params::kernelSize);

            // Laplacian of cs for particle j
            laplacianCs += Params::mass * (1 /densities[j]) * laplacianWpoly6(relativePos, Params::kernelSize);
        }

        glm::vec3 tensionForce;

        if (glm::length(n) < Params::nThreshold) {
            tensionForce = {0, 0, 0};
        } else {
            tensionForce = Params::sigma * (- laplacianCs / glm::length(n)) * n;
        }

        //glm::vec3 n = {0, 0, 0};
        glm::vec3 boundaryForce = {0, 0, 0};
        boundaryForce = calculateBoundaryForceGlass(i);

        // Add external forces on i
        forces[i] = pressureForce + viscosityForce + tensionForce + Params::gravity + boundaryForce;

        // Euler time step
        velocities[i] += (forces[i] / densities[i]) * dt_seconds;
        positions[i] += velocities[i] * dt_seconds;

    }

     //checkBoundariesGlass();

    glBindBuffer (GL_ARRAY_BUFFER, vbo_pos);
    glBufferData (GL_ARRAY_BUFFER, positions.size() * 3 * sizeof (float), positions.data(), GL_STATIC_DRAW);
}


glm::vec3 CppParticleSimulator::calculateBoundaryForce(int i){

    glm::vec3 boundaryForce = {0, 0, 0};
    glm::vec3 r = {0, 0, 0};
    float radius = 0;
    float hardness = 10.0f;


    // Future solution
    //glm::vec3 n = {0, 1, 0};
    //float d = positions[i].y - Params::bottomBound;
    //glm::vec3 r = n*d;

    // BOTTOM BOUND
    r = {0, positions[i].y - Params::bottomBound, 0};
    radius = sqrt(pow(r.x,2.0f) + pow(r.y,2.0f) + pow(r.z,2.0f));
    if(radius < Params::kernelSize){
        boundaryForce = -Params::mass * hardness * gradWspiky(r, Params::kernelSize);
    }

    // RIGHT BOUND
    r = {positions[i].x - Params::rightBound, 0, 0};
    radius = sqrt(pow(r.x,2.0f) + pow(r.y,2.0f) + pow(r.z,2.0f));
    if(radius < Params::kernelSize){
        boundaryForce = boundaryForce -Params::mass * hardness * gradWspiky(r, Params::kernelSize);
    }

    // LEFT BOUND
    r = {positions[i].x - Params::leftBound, 0, 0};
    radius = sqrt(pow(r.x,2.0f) + pow(r.y,2.0f) + pow(r.z,2.0f));
    if(radius < Params::kernelSize){
        boundaryForce = boundaryForce -Params::mass * hardness * gradWspiky(r, Params::kernelSize);
    }

    // FAR BOUND
    r = {0, 0, positions[i].z - Params::farBound};
    radius = sqrt(pow(r.x,2.0f) + pow(r.y,2.0f) + pow(r.z,2.0f));
    if(radius < Params::kernelSize){
        boundaryForce = boundaryForce -Params::mass * hardness * gradWspiky(r, Params::kernelSize);
    }

    // NEAR BOUND
    r = {0, 0, positions[i].z - Params::nearBound};
    radius = sqrt(pow(r.x,2.0f) + pow(r.y,2.0f) + pow(r.z,2.0f));
    if(radius < Params::kernelSize){
        boundaryForce = boundaryForce -Params::mass * hardness * gradWspiky(r, Params::kernelSize);
    }

    return boundaryForce;
}


glm::vec3 CppParticleSimulator::calculateBoundaryForceGlass(int i){

    glm::vec3 boundaryForce = {0, 0, 0};
    glm::vec3 r = {0, 0, 0};
    float distance = 0.0f;
    float diff = 0.0f;
    float radius = 0;
    float hardness = 10.0f;


    // BOTTOM BOUND
    r = {0, positions[i].y - Params::bottomBound, 0};
    radius = sqrt(pow(r.x,2.0f) + pow(r.y,2.0f) + pow(r.z,2.0f));
    if(radius < Params::kernelSize){
        boundaryForce = -Params::mass * hardness * gradWspiky(r, Params::kernelSize);
        velocities[i] *= Params::wallFriction;
    }

    //WALLS BOUND
    distance = sqrt(pow(positions[i].x,2.0f) + pow(positions[i].z,2.0f));
    diff = Params::rightBound - distance;
    r = -positions[i]*diff/distance;

    if(diff < Params::kernelSize){
        boundaryForce += -Params::mass * hardness * gradWspiky(r, Params::kernelSize);
        velocities[i] *= Params::wallFriction;
    }

    return boundaryForce;
}

void CppParticleSimulator::checkBoundaries() {
    for (int i = 0; i < positions.size(); ++i) {


        if (positions[i].y < Params::bottomBound || positions[i].y > Params::topBound) {
            positions[i].y = fmax(fmin(positions[i].y, Params::topBound), Params::bottomBound);
            velocities[i].y = Params::wallDamper * (- velocities[i].y);
        }

        if (positions[i].x < Params::leftBound || positions[i].x > Params::rightBound) {
            positions[i].x = fmax(fmin(positions[i].x, Params::rightBound), Params::leftBound);
            velocities[i].x = Params::wallDamper * (- velocities[i].x);
        }

        if (positions[i].z < Params::nearBound || positions[i].z > Params::farBound) {
            positions[i].z = fmax(fmin(positions[i].z, Params::farBound), Params::nearBound);
            velocities[i].z = Params::wallDamper * (- velocities[i].z);
        }
    }
}

void CppParticleSimulator::checkBoundariesGlass() {
    for (int i = 0; i < positions.size(); ++i) {

        float dp = 0.01f;
        float radius = sqrt(pow(positions[i].x,2.0f) + pow(positions[i].z,2.0f));

        if (radius > Params::rightBound && positions[i].x > 0 && positions[i].z > 0) {
            positions[i].x -= dp;
            positions[i].z -= dp;
            velocities[i].x = Params::wallDamper * (- velocities[i].x);
            velocities[i].z = Params::wallDamper * (- velocities[i].x);
        }

        if (radius > Params::rightBound && positions[i].x < 0 && positions[i].z > 0) {
            positions[i].x += dp;
            positions[i].z -= dp;
            velocities[i].x = Params::wallDamper * (- velocities[i].x);
            velocities[i].z = Params::wallDamper * (- velocities[i].x);
        }
        if (radius > Params::rightBound && positions[i].x > 0 && positions[i].z < 0) {
            positions[i].x -= dp;
            positions[i].z += dp;
            velocities[i].x = Params::wallDamper * (- velocities[i].x);
            velocities[i].z = Params::wallDamper * (- velocities[i].x);
        }

        if (radius > Params::rightBound && positions[i].x < 0 && positions[i].z < 0) {
            positions[i].x += dp;
            positions[i].z += dp;
            velocities[i].x = Params::wallDamper * (- velocities[i].x);
            velocities[i].z = Params::wallDamper * (- velocities[i].x);
        }

        if (positions[i].y < Params::bottomBound || positions[i].y > Params::topBound) {
            positions[i].y = fmax(fmin(positions[i].y, Params::topBound), Params::bottomBound);
            velocities[i].y = Params::wallDamper * (- velocities[i].y);
        }

    }
}