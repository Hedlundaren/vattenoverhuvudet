#pragma once

#ifdef __APPLE__

#include <OpenCL/opencl.h>

#else
#include <CL/cl.hpp>
#endif

#include <sstream>

struct clVoxelGridInfo {
    cl_uint3 grid_dimensions;
    cl_uint total_grid_cells;
    cl_float grid_cell_size;
    cl_float3 grid_origin;
    cl_uint max_cell_particle_count;
};

inline std::string print_clVoxelGridInfo(const clVoxelGridInfo &inf) {
    std::stringstream ss;

    ss <<
    "clVoxelGridInfo: {grid_dimensions=[" << inf.grid_dimensions.s[0] << " " << inf.grid_dimensions.s[1] << " " <<
    inf.grid_dimensions.s[2] << " " <<
    "] total_grid_cells=" << inf.total_grid_cells <<
    " grid_cell_size=" << inf.grid_cell_size <<
    " grid_origin=[" << inf.grid_origin.s[0] << " " << inf.grid_origin.s[1] << " " << inf.grid_origin.s[2] <<
    "] max_cell_particle_count=" << inf.max_cell_particle_count <<
    "}";

    return ss.str();
}