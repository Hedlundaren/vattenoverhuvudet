#pragma once

#include <vector>

#ifdef _WIN32
#include "GL/glew.h"
#endif

#include <GLFW/glfw3.h>

#include "glm/glm.hpp"

#include "Parameters.hpp"

class ParticleSimulator {
public:
    virtual void setupSimulation(const Parameters &parameters,
                                 const std::vector<glm::vec3> &particle_positions,
                                 const std::vector<glm::vec3> &particle_velocities,
                                 const GLuint &vbo_positions,
                                 const GLuint &vbo_velocities,
                                 const std::vector<float> &density_voxel_sampler,
                                 const std::vector<glm::vec3> &normal_voxel_sampler,
                                 const glm::uvec3 voxel_sampler_size) = 0;

    virtual void updateSimulation(const Parameters &parameters, float dt_seconds) = 0;
};