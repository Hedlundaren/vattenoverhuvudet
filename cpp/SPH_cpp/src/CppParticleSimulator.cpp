#include "CppParticleSimulator.hpp"

void CppParticleSimulator::setupSimulation(const std::vector<glm::vec3> &particle_positions,
                                           const std::vector<glm::vec3> &particle_velocities,
                                           const GLuint &vbo_positions,
                                           const GLuint &vbo_velocities) {
    positions = particle_positions;
    velocities = particle_velocities;

    vbo_pos = vbo_positions;
    vbo_vel = vbo_velocities;
}

void CppParticleSimulator::updateSimulation(float dt_seconds) {
    // Update velocities with Euler integration
    for (int i = 0; i < positions.size(); ++i) {
        positions[i] += dt_seconds * velocities[i];
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo_pos);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * 3 * sizeof(float), positions.data(), GL_STATIC_DRAW);
}