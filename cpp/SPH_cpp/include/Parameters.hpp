//
// Created by Simon on 2016-01-28.
//

#pragma once

#include <iostream>
#include <algorithm>

#include "glm/glm.hpp"
#include "glm/ext.hpp"

#ifdef __APPLE__

#include <OpenCL/opencl.h>

#else
#include <CL/cl.hpp>
#endif

#include "OpenCL/clFluidInfo.hpp"
#include "OpenCL/clVoxelGridInfo.hpp"
#include "OpenCL/clBoundsInfo.hpp"

struct Parameters {
    Parameters(unsigned int particle_count) : n_particles(particle_count) {};

    unsigned int n_particles;
    float total_mass;
    float kernel_size;
    float k_gas;
    float k_viscosity;
    float rest_density;
    float sigma;
    float k_threshold;
    glm::vec3 gravity;
    float left_bound;
    float right_bound;
    float bottom_bound;
    float top_bound;
    float near_bound;
    float far_bound;
    float k_wall_damper;
    float k_wall_friction;
    float k_wall_density;
    float k_wall_force;

    float fps;

    glm::vec3 bg_color;

    inline float get_particle_mass() const {
        return total_mass / n_particles;
    }

    inline float get_max_volume_side() const {
        return std::max(std::max(right_bound - left_bound, top_bound - bottom_bound), far_bound - near_bound);
    }

    inline float get_volume_size_x() const {
        return right_bound - left_bound;
    }

    inline float get_volume_size_y() const {
        return top_bound - bottom_bound;
    }

    inline float get_volume_size_z() const {
        return far_bound - near_bound;
    }

    inline void set_fluid_info(clFluidInfo &fluid_info, const float n_particles) const {
        fluid_info.gravity.s[0] = gravity.x;
        fluid_info.gravity.s[1] = gravity.y;
        fluid_info.gravity.s[2] = gravity.z;

        fluid_info.k_gas = k_gas;
        fluid_info.k_threshold = k_threshold;
        fluid_info.k_viscosity = k_viscosity;
        fluid_info.k_wall_damper = k_wall_damper;
        fluid_info.k_wall_friction = k_wall_friction;
        fluid_info.rest_density = rest_density;
        fluid_info.sigma = sigma;

        fluid_info.mass = total_mass / n_particles;
    }

    inline void set_voxel_grid_info(clVoxelGridInfo &grid_info) const {
        grid_info.grid_cell_size = kernel_size;

        grid_info.grid_cells.s[0] = static_cast<unsigned int>(ceilf(get_volume_size_x() / kernel_size));
        grid_info.grid_cells.s[1] = static_cast<unsigned int>(ceilf(get_volume_size_y() / kernel_size));;
        grid_info.grid_cells.s[2] = static_cast<unsigned int>(ceilf(get_volume_size_z() / kernel_size));;

        grid_info.grid_origin.s[0] = left_bound;
        grid_info.grid_origin.s[1] = bottom_bound;
        grid_info.grid_origin.s[2] = near_bound;

        grid_info.grid_dimensions.s[0] = right_bound - left_bound;
        grid_info.grid_dimensions.s[1] = top_bound - bottom_bound;
        grid_info.grid_dimensions.s[2] = far_bound - near_bound;

        grid_info.max_cell_particle_count = VOXEL_CELL_PARTICLE_COUNT;

        grid_info.total_grid_cells = grid_info.grid_cells.s[0] *
                                     grid_info.grid_cells.s[1] *
                                     grid_info.grid_cells.s[2];
    }

    inline void set_bounds_info(clBoundsInfo &bounds_info) const {
        bounds_info.k_wall_density = k_wall_density;
        bounds_info.k_wall_force = k_wall_force;
    }


    inline static Parameters set_default_parameters(Parameters &p) {
        p.total_mass = 1000000.0f;
        p.kernel_size = 0.2f;
        p.k_gas = 0.5f;
        p.k_viscosity = 20.0f;
        p.rest_density = 100.0f;
        p.sigma = 1.0f;
        p.k_threshold = 0.1f;
        p.gravity = glm::vec3(0.0f, -9.82f, 0.0f);

        p.left_bound = -7.5f;
        p.right_bound = 7.5f;
        p.bottom_bound = 0.0f;
        p.top_bound = 5.0f;
        p.near_bound = -7.5f;
        p.far_bound = 7.5f;

        p.k_wall_damper = 0.75f;
        p.k_wall_friction = 1.0f;
        p.k_wall_density = 1.0f;
        p.k_wall_force = 1.0f;

        p.fps = 0.0f;

        p.bg_color = glm::vec3(0.1f, 0.1f, 0.1f);
    }
};