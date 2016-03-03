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
#include "OpenCL/clFluidInfo.hpp"

class OpenClParticleSimulator : public ParticleSimulator {
public:
    ~OpenClParticleSimulator();

    void setupSimulation(const Parameters &parameters,
                         const std::vector<glm::vec3> &particle_positions,
                         const std::vector<glm::vec3> &particle_velocities,
                         const GLuint &vbo_positions,
                         const GLuint &vbo_velocities,
                         const std::vector<float> &density_voxel_sampler,
                         const std::vector<glm::vec3> &normal_voxel_sampler,
                         const glm::uvec3 voxel_sampler_size);

    void updateSimulation(const Parameters &parameters, float dt_seconds);

private:
    std::vector<glm::vec3> positions;

    std::vector<cl_mem> cgl_objects;

    int n_particles;

    clVoxelGridInfo grid_info;
    clFluidInfo fluid_info;

    // points to array of 3 size_t
    size_t *grid_cells_count;

    cl_mem cl_voxel_cell_particle_indices;

    cl_mem cl_voxel_cell_particle_count;

    void *cl_positions_buffer, *cl_velocities_buffer;

    cl_mem cl_positions, cl_velocities;

    cl_mem cl_densities;

    cl_mem cl_forces;

    std::vector<cl_platform_id> platformIds;

    std::vector<cl_device_id> deviceIds;

    int chosen_device_id;

    cl_context context;

    cl_command_queue command_queue;

    // Heightmap voxel sampler buffers
    cl_mem cl_hmap_density_voxel_sampler;
    cl_mem cl_hmap_normal_voxel_sampler;
    cl_uint3 cl_voxel_sampler_size;

    void initOpenCL();

    void setupSharedBuffers(const GLuint &vbo_positions, const GLuint &vbo_velocities);

    void setupHeightmapVoxelSamplerBuffers(const std::vector<float> &density_voxel_sampler,
                                           const std::vector<glm::vec3> &normal_voxel_sampler,
                                           const glm::uvec3 voxel_sampler_size);

    void createAndBuildKernel(cl_kernel &kernel_out, std::string kernel_name, std::string kernel_file_name);

    void allocateVoxelGridBuffer(const Parameters &params);

    /* Kernels */

    void runCalculateVoxelGridKernel(float dt_seconds);
    cl_kernel calculate_voxel_grid = NULL;

    void runResetVoxelGridKernel();
    cl_kernel reset_voxel_grid = NULL;

    void runCalculateParticleDensitiesKernel(float dt_seconds);
    cl_kernel calculate_particle_densities = NULL;

    void runCalculateParticleForcesKernel();
    cl_kernel calculate_particle_forces = NULL;

    void runIntegrateParticleStatesKernel(float dt_seconds);
    cl_kernel integrate_particle_states;
};