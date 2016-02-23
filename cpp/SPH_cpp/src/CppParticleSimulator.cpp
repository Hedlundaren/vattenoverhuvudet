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
            density += parameters.get_particle_mass() * Wpoly6(relativePos, parameters.kernel_size);
        }

        densities[i] = density;
    }

    // Calculate forces
    for (int i = 0; i < positions.size(); ++i) {
        float iPressure = (densities[i] - parameters.rest_density) * parameters.k_gas;
        float cs = 0;
        glm::vec3 n = {0, 0, 0};
        float laplacianCs = 0;

        glm::vec3 pressureForce = {0, 0, 0};
        glm::vec3 viscosityForce = {0, 0, 0};

        for (int j = 0; j < positions.size(); ++j) {
            glm::vec3 relativePos = positions[i] - positions[j];

            // Particle j's pressure force on i
            float jPressure = (densities[j] - parameters.rest_density) * parameters.k_gas;
            pressureForce = pressureForce - parameters.get_particle_mass() *
                                            ((iPressure + jPressure) / (2 * densities[j])) *
                                            gradWspiky(relativePos, parameters.kernel_size);

            // Particle j's viscosity force in i
            viscosityForce += parameters.k_viscosity *
                              parameters.get_particle_mass() * ((velocities[j] - velocities[i]) / densities[j]) *
                              laplacianWviscosity(relativePos, parameters.kernel_size);

            // cs for particle j
            cs += parameters.get_particle_mass() * (1 / densities[j]) * Wpoly6(relativePos, parameters.kernel_size);

            // Gradient of cs for particle j
            n += parameters.get_particle_mass() * (1 / densities[j]) * gradWpoly6(relativePos, parameters.kernel_size);

            // Laplacian of cs for particle j
            laplacianCs += parameters.get_particle_mass() * (1 / densities[j]) *
                           laplacianWpoly6(relativePos, parameters.kernel_size);
        }

        glm::vec3 tensionForce;

        if (glm::length(n) < parameters.k_threshold) {
            tensionForce = glm::vec3(0, 0, 0);
        } else {
            tensionForce = parameters.sigma * (-laplacianCs / glm::length(n)) * n;
        }

        // Add external forces on i
        forces[i] = pressureForce + viscosityForce + tensionForce + parameters.gravity;

        // Euler time step
        velocities[i] += (forces[i] / parameters.get_particle_mass()) * dt_seconds;
        positions[i] += velocities[i] * dt_seconds;

    }

    checkBoundaries(parameters);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_pos);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * 3 * sizeof(float), positions.data(), GL_STATIC_DRAW);
}

void CppParticleSimulator::checkBoundaries(const Parameters &parameters) {
    for (int i = 0; i < positions.size(); ++i) {
        if (positions[i].x < parameters.left_bound || positions[i].x > parameters.right_bound) {
            positions[i].x = static_cast<float>(fmax(fmin(positions[i].x, parameters.right_bound), parameters.left_bound));
            velocities[i].x = parameters.k_wall_damper * (-velocities[i].x);
        }

        if (positions[i].y < parameters.bottom_bound || positions[i].y > parameters.top_bound) {
            positions[i].y = static_cast<float>(fmax(fmin(positions[i].y, parameters.top_bound), parameters.bottom_bound));
            velocities[i].y = parameters.k_wall_damper * (-velocities[i].y);
        }

        if (positions[i].z < parameters.near_bound || positions[i].z > parameters.far_bound) {
            positions[i].z = static_cast<float>(fmax(fmin(positions[i].z, parameters.far_bound), parameters.near_bound));
            velocities[i].z = parameters.k_wall_damper * (-velocities[i].z);
        }
    }
}