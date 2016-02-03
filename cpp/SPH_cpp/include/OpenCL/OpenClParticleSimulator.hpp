#pragma once

#include "ParticleSimulator.hpp"

#ifdef __APPLE__

#include <OpenCL/opencl.h>
#include <OpenGL/OpenGL.h>

#else
#include <CL/cl.hpp>
#endif

#include <vector>

class OpenClParticleSimulator : public ParticleSimulator {
public:
    ~OpenClParticleSimulator();

    void setupSimulation(const std::vector<glm::vec3> &particle_positions,
                         const std::vector<glm::vec3> &particle_velocities,
                         const GLuint &vbo_positions,
                         const GLuint &vbo_velocities);

    void updateSimulation(float dt_seconds);

private:
    std::vector<cl_platform_id> platformIds;

    std::vector<cl_device_id> deviceIds;

    cl_context context;

    cl_command_queue command_queue;

    void initOpenCL();
};