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

void CppParticleSimulator::updateSimulation(const Parameters &params, float dt_seconds) {

    // Set forces to 0 and calculate densities
    for (int i = 0; i < positions.size(); ++i) {
        forces[i] = {0, 0, 0};
        float density = 0;

        for (int j = 0; j < positions.size(); ++j) {
            glm::vec3 relativePos = positions[i] - positions[j];
            density += params.get_particle_mass() * Wpoly6(relativePos, params.kernel_size);
        }

        densities[i] = density;
    }

    // Calculate forces
    for (int i = 0; i < positions.size(); ++i) {
        float iPressure = (densities[i] - params.rest_density) * params.k_gas;
        float cs = 0;
        glm::vec3 n = {0, 0, 0};
        float laplacianCs = 0;

        glm::vec3 pressureForce = {0, 0, 0};
        glm::vec3 viscosityForce = {0, 0, 0};

        for (int j = 0; j < positions.size(); ++j) {
            glm::vec3 relativePos = positions[i] - positions[j];

            // Particle j's pressure force on i
            float jPressure = (densities[j] - params.rest_density) * params.k_gas;
            pressureForce = pressureForce - params.get_particle_mass() *
                ((iPressure + jPressure) / (2 * densities[j])) *
                gradWspiky(relativePos, params.kernel_size);

            // Particle j's viscosity force in i
            viscosityForce += params.k_viscosity *
                params.get_particle_mass() * ((velocities[j] - velocities[i]) / densities[j]) *
                laplacianWviscosity(relativePos, params.kernel_size);

            // cs for particle j
            cs += params.get_particle_mass() * (1 / densities[j]) * Wpoly6(relativePos, params.kernel_size);

            // Gradient of cs for particle j
            n += params.get_particle_mass() * (1 / densities[j]) * gradWpoly6(relativePos, params.kernel_size);

            // Laplacian of cs for particle j
            laplacianCs += params.get_particle_mass() * (1 /densities[j]) * laplacianWpoly6(relativePos, params.kernel_size);
        }

        glm::vec3 tensionForce;

        if (glm::length(n) < params.k_threshold) {
            tensionForce = {0, 0, 0};
        } else {
            tensionForce = params.sigma * (- laplacianCs / glm::length(n)) * n;
        }

        //glm::vec3 n = {0, 0, 0};
        glm::vec3 boundaryForce = {0, 0, 0};
        boundaryForce = calculateBoundaryForceGlass(params, i);

        // Add external forces on i
        forces[i] = pressureForce + viscosityForce + tensionForce + params.gravity + boundaryForce;

        // Euler time step
        velocities[i] += (forces[i] / densities[i]) * dt_seconds;
        positions[i] += velocities[i] * dt_seconds;

    }

    checkBoundariesGlass(params);

    glBindBuffer (GL_ARRAY_BUFFER, vbo_pos);
    glBufferData (GL_ARRAY_BUFFER, positions.size() * 3 * sizeof (float), positions.data(), GL_STATIC_DRAW);
}


glm::vec3 CppParticleSimulator::calculateBoundaryForce(const Parameters &params, int i){

    glm::vec3 boundaryForce = {0, 0, 0};
    glm::vec3 r = {0, 0, 0};
    float radius = 0;
    float hardness = 10.0f;


    // Future solution
    //glm::vec3 n = {0, 1, 0};
    //float d = positions[i].y - Params::bottomBound;
    //glm::vec3 r = n*d;

    // BOTTOM BOUND
    r = {0, positions[i].y - params.bottom_bound, 0};
    radius = sqrt(pow(r.x,2.0f) + pow(r.y,2.0f) + pow(r.z,2.0f));
    if(radius < params.kernel_size){
        boundaryForce = -params.get_particle_mass() * hardness * gradWspiky(r, params.kernel_size);
    }

    // RIGHT BOUND
    r = {positions[i].x - params.right_bound, 0, 0};
    radius = sqrt(pow(r.x,2.0f) + pow(r.y,2.0f) + pow(r.z,2.0f));
    if(radius < params.kernel_size){
        boundaryForce = boundaryForce -params.get_particle_mass() * hardness * gradWspiky(r, params.kernel_size);
    }

    // LEFT BOUND
    r = {positions[i].x - params.left_bound, 0, 0};
    radius = sqrt(pow(r.x,2.0f) + pow(r.y,2.0f) + pow(r.z,2.0f));
    if(radius < params.kernel_size){
        boundaryForce = boundaryForce -params.get_particle_mass() * hardness * gradWspiky(r, params.kernel_size);
    }

    // FAR BOUND
    r = {0, 0, positions[i].z - params.far_bound};
    radius = sqrt(pow(r.x,2.0f) + pow(r.y,2.0f) + pow(r.z,2.0f));
    if(radius < params.kernel_size){
        boundaryForce = boundaryForce -params.get_particle_mass() * hardness * gradWspiky(r, params.kernel_size);
    }

    // NEAR BOUND
    r = {0, 0, positions[i].z - params.near_bound};
    radius = sqrt(pow(r.x,2.0f) + pow(r.y,2.0f) + pow(r.z,2.0f));
    if(radius < params.kernel_size){
        boundaryForce = boundaryForce -params.get_particle_mass() * hardness * gradWspiky(r, params.kernel_size);
    }

    return boundaryForce;
}


glm::vec3 CppParticleSimulator::calculateBoundaryForceGlass(const Parameters &params, int i){

    glm::vec3 boundaryForce = {0, 0, 0};
    glm::vec3 r = {0, 0, 0};
    float distance = 0.0f;
    float diff = 0.0f;
    float radius = 0;
    float hardness = 10.0f;


    // BOTTOM BOUND
    r = {0, positions[i].y - params.bottom_bound, 0};
    radius = sqrt(pow(r.x,2.0f) + pow(r.y,2.0f) + pow(r.z,2.0f));
    if(radius < params.kernel_size){
        boundaryForce = -params.get_particle_mass() * hardness * gradWspiky(r, params.kernel_size);
        velocities[i] *= params.k_wall_friction;
    }

    //WALLS BOUND
    distance = sqrt(pow(positions[i].x,2.0f) + pow(positions[i].z,2.0f));
    diff = params.top_bound - distance;
    r = -positions[i]*diff/distance;

    if(diff < params.kernel_size){
        boundaryForce += -params.get_particle_mass() * hardness * gradWspiky(r, params.kernel_size);
        velocities[i] *= params.k_wall_friction;
    }

    return boundaryForce;
}

void CppParticleSimulator::checkBoundaries(const Parameters &params) {
    for (int i = 0; i < positions.size(); ++i) {


        if (positions[i].y < params.bottom_bound || positions[i].y > params.top_bound) {
            positions[i].y = fmax(fmin(positions[i].y, params.top_bound), params.bottom_bound);
            velocities[i].y = params.k_wall_damper * (- velocities[i].y);
        }

        if (positions[i].x < params.left_bound || positions[i].x > params.right_bound) {
            positions[i].x = fmax(fmin(positions[i].x, params.right_bound), params.left_bound);
            velocities[i].x = params.k_wall_damper * (- velocities[i].x);
        }

        if (positions[i].z < params.near_bound || positions[i].z > params.far_bound) {
            positions[i].z = fmax(fmin(positions[i].z, params.far_bound), params.near_bound);
            velocities[i].z = params.k_wall_damper * (- velocities[i].z);
        }
    }
}

void CppParticleSimulator::checkBoundariesGlass(const Parameters &params) {
    for (int i = 0; i < positions.size(); ++i) {

        float dp = 0.01f;
        float radius = sqrt(pow(positions[i].x,2.0f) + pow(positions[i].z,2.0f));

        if (radius > params.right_bound && positions[i].x > 0 && positions[i].z > 0) {
            positions[i].x -= dp;
            positions[i].z -= dp;
            velocities[i].x = params.k_wall_damper * (- velocities[i].x);
            velocities[i].z = params.k_wall_damper * (- velocities[i].x);
        }

        if (radius > params.right_bound && positions[i].x < 0 && positions[i].z > 0) {
            positions[i].x += dp;
            positions[i].z -= dp;
            velocities[i].x = params.k_wall_damper * (- velocities[i].x);
            velocities[i].z = params.k_wall_damper * (- velocities[i].x);
        }
        if (radius > params.right_bound && positions[i].x > 0 && positions[i].z < 0) {
            positions[i].x -= dp;
            positions[i].z += dp;
            velocities[i].x = params.k_wall_damper * (- velocities[i].x);
            velocities[i].z = params.k_wall_damper * (- velocities[i].x);
        }

        if (radius > params.right_bound && positions[i].x < 0 && positions[i].z < 0) {
            positions[i].x += dp;
            positions[i].z += dp;
            velocities[i].x = params.k_wall_damper * (- velocities[i].x);
            velocities[i].z = params.k_wall_damper * (- velocities[i].x);
        }

        if (positions[i].y < params.bottom_bound || positions[i].y > params.top_bound) {
            positions[i].y = fmax(fmin(positions[i].y, params.top_bound), params.bottom_bound);
            velocities[i].y = params.k_wall_damper * (- velocities[i].y);
        }

    }
}