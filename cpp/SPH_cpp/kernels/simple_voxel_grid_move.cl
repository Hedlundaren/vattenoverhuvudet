#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable

//typedef struct __attribute__ ((packed)) def_ParticleData {
typedef struct def_ParticleData {
    float density;      // 4 bytes

    float3 force;       // 12 bytes
    float color_field;  // 4 bytes
} ParticleData;

//typedef struct __attribute__ ((packed)) def_VoxelGridInfo {
typedef struct def_VoxelGridInfo {
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

__kernel void simple_voxel_grid_move(__global float3 *positions, // The position of each particle
									 __global float3 *velocities, // The velocity of each particle
								   	 __global const uint *indices, // Indices from each voxel cell to each particle. Is [max_cell_particle_count * total_grid_cells] long
								   	 __global const uint *cell_particle_count, // Particle counter for each voxel cell. Is [total_grid_cells] long
								   	 const VoxelGridInfo grid_info,
								   	 const float dt) {
	const uint voxel_cell_index = get_global_id(0);
	const uint particle_count = cell_particle_count[voxel_cell_index];

	for (uint i = 0; i < particle_count; ++i) {
		const uint particle_id = indices[voxel_cell_index * grid_info.max_cell_particle_count + i];

		float3 new_position = positions[particle_id] + dt * velocities[particle_id];
		float3 new_velocity = velocities[particle_id];

		// x-boundaries
		if (new_position.x != clamp(new_position.x, -1.0f, 1.0f)) {
			new_position.x = clamp(new_position.x, -1.0f, 1.0f);
			new_velocity.x = - new_velocity.x;
		}

		// y-boundaries
		if (new_position.y != clamp(new_position.y, -1.0f, 1.0f)) {
			new_position.y = clamp(new_position.y, -1.0f, 1.0f);
			new_velocity.y = - new_velocity.y;
		}

		// z-boundaries
		if (new_position.z != clamp(new_position.z, -1.0f, 1.0f)) {
			new_position.z = clamp(new_position.z, -1.0f, 1.0f);
			new_velocity.z = - new_velocity.z;
		}

		positions[particle_id] = new_position;
		velocities[particle_id] = new_velocity;
	}
}