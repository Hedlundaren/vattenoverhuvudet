#pragma once

#include "ParticleSimulator.hpp"
#include "Parameters.h"

class CppParticleSimulator : public ParticleSimulator {
public:
    void setupSimulation(const Parameters &params,
                         const std::vector<glm::vec3> &particle_positions,
                         const std::vector<glm::vec3> &particle_velocities,
                         const GLuint &vbo_positions,
                         const GLuint &vbo_velocities,
                         const std::vector<float> &density_voxel_sampler,
                         const std::vector<glm::vec3> &normal_voxel_sampler,
                         const glm::uvec3 voxel_sampler_size);

    void updateSimulation(const Parameters &params, float dt_seconds);

    void checkBoundaries(const Parameters &params);

    void checkBoundariesGlass(const Parameters &params);

    glm::vec3 calculateBoundaryForce(const Parameters &params, int i);

    glm::vec3 calculateBoundaryForceGlass(const Parameters &params, int i);

private:
    std::vector<glm::vec3> positions, velocities;
    GLuint vbo_pos, vbo_vel;
    std::vector<glm::vec3> forces;
    std::vector<float> densities;
};