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
#include <iostream>
#include <string>

#include "OpenCL/clVoxelGridInfo.hpp"

class OpenClParticleSimulator : public ParticleSimulator {
public:
    ~OpenClParticleSimulator();

    void setupSimulation(const std::vector<glm::vec3> &particle_positions,
                         const std::vector<glm::vec3> &particle_velocities,
                         const GLuint &vbo_positions,
                         const GLuint &vbo_velocities);

    void updateSimulation(float dt_seconds);

private:
    std::vector<glm::vec3> positions;

    std::vector<cl_mem> cgl_objects;

    int n_particles;

    clVoxelGridInfo grid_info;

    cl_mem cl_voxel_cell_particle_indices;

    cl_mem cl_voxel_cell_particle_count;

    void *cl_positions_buffer, *cl_velocities_buffer;

    cl_mem cl_positions, cl_velocities;

    std::vector<cl_platform_id> platformIds;

    std::vector<cl_device_id> deviceIds;

    int chosen_device_id;

    cl_context context;

    cl_command_queue command_queue;

    cl_mem cl_dt_obj;

    void initOpenCL();

    void setupSharedBuffers(const GLuint &vbo_positions, const GLuint &vbo_velocities);

    void createAndBuildKernel(cl_kernel &kernel_out, std::string kernel_name, std::string kernel_file_name);

    void allocateVoxelGridBuffer();

    /* Kernels */

    /// A simple, stand-alone kernel that integrates the positions based on the velocities
    /// Is not a part of the fluid simulation processing chain
    void runSimpleIntegratePositionsKernel(float dt_seconds);

    cl_kernel simple_integration = NULL;

    void runCalculateVoxelGridKernel(float dt_seconds);

    cl_kernel calculate_voxel_grid = NULL;

    void runResetVoxelGridKernel();

    cl_kernel reset_voxel_grid = NULL;

    void runSimpleVoxelGridMoveKernel(float dt_seconds);

    cl_kernel simple_voxel_grid_move = NULL;

    void runCalculateParticleDensitiesKernel(float dt_seconds);

    cl_kernel calculate_particle_densities = NULL;

    void runCalculateParticleForcesAndIntegrateStatesKernel(float dt_seconds);

    cl_kernel calculate_particle_forces_integrate = NULL;
};