#pragma once

#include "ParticleSimulator.hpp"

#ifdef __APPLE__

#include <OpenCL/opencl.h>
#include <OpenGL/OpenGL.h>

#else
#include <CL/cl.hpp>
#endif

#ifdef _WIN32
#include <windows.h>
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
    int n_particles;

    void *cl_positions_buffer, *cl_velocities_buffer;

    cl_mem cl_positions, cl_velocities;

    std::vector<cl_platform_id> platformIds;

    std::vector<cl_device_id> deviceIds;

    int chosen_device_id;

    cl_context context;

    cl_command_queue command_queue;

    cl_kernel kernel = NULL;

    cl_mem cl_dt_obj;

    void initOpenCL();
};