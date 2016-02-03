#pragma once

#include "ParticleSimulator.hpp"

class CppParticleSimulator : public ParticleSimulator {
public:
    void setupSimulation(const std::vector <glm::vec3> &particle_positions,
                         const std::vector <glm::vec3> &particle_velocities,
                         const GLuint &vbo_positions,
                         const GLuint &vbo_velocities);

    void updateSimulation(float dt_seconds);

private:
    std::vector <glm::vec3> positions, velocities;
    GLuint vbo_pos, vbo_vel;
};