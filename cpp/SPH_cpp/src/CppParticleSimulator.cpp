#include "CppParticleSimulator.hpp"

#include <iostream>
#include <vector>
#include "Parameters.h"
#include "sph_kernels.h"

void CppParticleSimulator::setupSimulation(const std::vector <glm::vec3> &particle_positions,
                                           const std::vector <glm::vec3> &particle_velocities,
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

void CppParticleSimulator::updateSimulation(float dt_seconds) {

    // Set forces to 0 and calculate densities
    for (int i = 0; i < positions.size(); ++i) {
        forces[i] = {0, 0, 0};
        float density = 0;

        for (int j = 0; j < positions.size(); ++j) {
            glm::vec3 relativePos = positions[i] - positions[j];
            density += Parameters::mass * Wpoly6(relativePos, Parameters::kernelSize);
        }

        densities[i] = density;
    }

    // Calculate forces
    for (int i = 0; i < positions.size(); ++i) {
        float iPressure = (densities[i] - Parameters::restDensity) * Parameters::gasConstantK;
        float cs = 0;
        glm::vec3 n = {0, 0, 0};
        float laplacianCs = 0;

        glm::vec3 pressureForce = {0, 0, 0};
        glm::vec3 viscosityForce = {0, 0, 0};

        for (int j = 0; j < positions.size(); ++j) {
            glm::vec3 relativePos = positions[i] - positions[j];

            // Particle j's pressure force on i
            float jPressure = (densities[j] - Parameters::restDensity) * Parameters::gasConstantK;
            pressureForce = pressureForce - Parameters::mass *
                ((iPressure + jPressure) / (2 * densities[j])) *
                gradWspiky(relativePos, Parameters::kernelSize);

            // Particle j's viscosity force in i
            viscosityForce += Parameters::viscosityConstant *
                Parameters::mass * ((velocities[j] - velocities[i]) / densities[j]) *
                laplacianWviscosity(relativePos, Parameters::kernelSize);

            // cs for particle j
            cs += Parameters::mass * (1 / densities[j]) * Wpoly6(relativePos, Parameters::kernelSize);

            // Gradient of cs for particle j
            n += Parameters::mass * (1 / densities[j]) * gradWpoly6(relativePos, Parameters::kernelSize);

            // Laplacian of cs for particle j
            laplacianCs += Parameters::mass * (1 /densities[j]) * laplacianWpoly6(relativePos, Parameters::kernelSize);
        }

        glm::vec3 tensionForce;

        if (glm::length(n) < Parameters::nThreshold) {
            tensionForce = {0, 0, 0};
        } else {
            tensionForce = Parameters::sigma * (- laplacianCs / glm::length(n)) * n;
        }

        // Add external forces on i
        forces[i] = pressureForce + viscosityForce + tensionForce + Parameters::gravity;

        // Euler time step
        velocities[i] += (forces[i] / densities[i]) * dt_seconds;
        positions[i] += velocities[i] * dt_seconds;

    }

    checkBoundaries();

    glBindBuffer (GL_ARRAY_BUFFER, vbo_pos);
    glBufferData (GL_ARRAY_BUFFER, positions.size() * 3 * sizeof (float), positions.data(), GL_STATIC_DRAW);
}

void CppParticleSimulator::checkBoundaries() {
    for (int i = 0; i < positions.size(); ++i) {
        if (positions[i].x < Parameters::leftBound || positions[i].x > Parameters::rightBound) {
            positions[i].x = fmax(fmin(positions[i].x, Parameters::rightBound), Parameters::leftBound);
            velocities[i].x = Parameters::wallDamper * (- velocities[i].x);
        }

        if (positions[i].y < Parameters::bottomBound || positions[i].y > Parameters::topBound) {
            positions[i].y = fmax(fmin(positions[i].y, Parameters::topBound), Parameters::bottomBound);
            velocities[i].y = Parameters::wallDamper * (- velocities[i].y);
        }

        if (positions[i].z < Parameters::nearBound || positions[i].z > Parameters::farBound) {
            positions[i].z = fmax(fmin(positions[i].z, Parameters::farBound), Parameters::nearBound);
            velocities[i].z = Parameters::wallDamper * (- velocities[i].z);
        }
    }
}