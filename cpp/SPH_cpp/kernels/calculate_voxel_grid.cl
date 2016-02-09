#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable

typedef struct __attribute__ ((packed)) def_ParticleData {
    float density;      // 4 bytes

    float3 force;       // 12 bytes
    float color_field;  // 4 bytes
} ParticleData;

typedef struct __attribute__ ((packed)) def_VoxelGridInfo {
	// How many grid cells there are in each dimension (i.e. [x=8 y=8 z=10])
	uint3 grid_dimensions;

	// How many grid cells there are in total
	uint total_grid_cells;

	// The size (x/y/z) of each cell
	float grid_cell_size;

	// The bottom-most corner of the grid, where the grid cell [0 0 0] starts
	float3 grid_origin;

	uint max_cell_particle_count;
} VoxelGridInfo;

uint3 calculate_voxel_cell_indices(const float3 position, const VoxelGridInfo grid_info) {
	// todo investigate if ceil, floor or round should be used
	return clamp(convert_uint3(ceil((position - grid_info.grid_origin) / grid_info.grid_cell_size)), 
		(uint3)(0, 0, 0), // Minimum indices
		(grid_info.grid_dimensions - (uint3)(1, 1, 1)) // Maximum indices
	);
}

uint calculate_voxel_cell_index(const uint3 voxel_cell_indices, const VoxelGridInfo grid_info) {
	return voxel_cell_indices.x + grid_info.grid_dimensions.x * (voxel_cell_indices.y + grid_info.grid_dimensions.y * voxel_cell_indices.z);
}

__kernel void calculate_voxel_grid(__global const float3 *positions, // The position of each particle
								   __global volatile uint *indices, // Indices from each voxel cell to each particle. Is [max_cell_particle_count * total_grid_cells] long
								   __global volatile uint *cell_particle_count, // Particle counter for each voxel cell. Is [total_grid_cells] long
								   const VoxelGridInfo grid_info) { 
	const uint particle_id = get_global_id(0);

	const uint3 voxel_cell_indices = calculate_voxel_cell_indices(position[particle_id], grid_info);
	const uint voxel_cell_index = calculate_voxel_cell_index(voxel_cell_indices, grid_info);

	// todo This should be able to be commented out
	if (voxel_cell_index != clamp(voxel_cell_index, 0, (grid_info.total_grid_cells - 1))) {
		return;
	}

	// Increment the counter for this voxel cell, returning the old counter. This is used to index the voxel cell indices array
	const uint old_count = atomic_inc(&(cell_particle_count[voxel_cell_index]));

	// todo This should be able to be commented out
	if (old_count >= grid_info.max_cell_particle_count) {
		return;
	}

	// Store this particle's buffer array index in the voxel cell
	indices[voxel_cell_index * grid_info.max_cell_particle_count + old_count] = particle_id;
}