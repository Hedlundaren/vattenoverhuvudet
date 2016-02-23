#pragma once

#include "ParticleSimulator.hpp"

class CppParticleSimulator : public ParticleSimulator {
public:
    void setupSimulation(const Parameters &parameters,
                         const std::vector<glm::vec3> &particle_positions,
                         const std::vector<glm::vec3> &particle_velocities,
                         const GLuint &vbo_positions,
                         const GLuint &vbo_velocities);

    void updateSimulation(const Parameters &parameters, float dt_seconds);

    void checkBoundaries(const Parameters &parameters);

private:
    std::vector<glm::vec3> positions, velocities;
    GLuint vbo_pos, vbo_vel;
    std::vector<glm::vec3> forces;
    std::vector<float> densities;
};