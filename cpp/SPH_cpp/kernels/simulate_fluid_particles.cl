#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable

#define MAX_CELL_PARTICLE_COUNT 16

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

typedef struct def_FluidInfo {
	// The mass of each fluid particle
	float mass;
} FluidInfo;

// Calculates the euclidean length of the vector r
float euclidean_distance(const float3 r);

// Calculates the squared euclidean length of the vector r (x^2 + y^2 + z^2)
float euclidean_distance2(const float3 r);

// The SPH kernel used in most SPH quantity calculations
float W_poly6(const float3 r, const float h);

// Map a particle index inside a voxel cell to its global buffer index
uint get_particle_buffer_index(const uint voxel_cell_index, 
							   const uint voxel_particle_index, 
					 		   const uint max_cell_particle_count,
					 		   __global const uint* restrict indices);

// Get the position of a particle based on its cell index and particle index inside the given cell
float3 get_particle_position(const uint voxel_cell_index, 
							 const uint voxel_particle_index, 
					 		 const uint max_cell_particle_count,
					 		 __global const uint* restrict indices, 
					 		 __global const float* restrict positions);

// Calculate the 1D-mapped voxel cell index for the given 3D voxel cell indices (x/y/z)
uint calculate_voxel_cell_index(const uint3 voxel_cell_indices, const VoxelGridInfo grid_info);

__kernel void calculate_particle_densities(__global const float* restrict positions, // The position of each particle
									 __global float* restrict out_densities, // The density of each particle. Is [max_cell_particle_count * total_grid_cells] long, since
									 								// it does NOT need to match up with the particle's global positions/velocities buffers 
								   	 __global const uint* restrict indices, // Indices from each voxel cell to each particle. Is [max_cell_particle_count * total_grid_cells] long
								   	 __global const uint* restrict cell_particle_count, // Particle counter for each voxel cell. Is [total_grid_cells] long
								   	 const VoxelGridInfo grid_info,
								   	 const FluidInfo fluid_info) {
	
	const uint3 voxel_cell_indices = (uint3)(get_global_id(0), get_global_id(1), get_global_id(2));
	const uint voxel_cell_index = calculate_voxel_cell_index(voxel_cell_indices, grid_info);
	const uint particle_count = cell_particle_count[voxel_cell_index];

	// Store the densities locally (in private kernel memory) during calculation
	float processed_particle_densities[MAX_CELL_PARTICLE_COUNT];

	// Pre-store the positions of the particles being processed locally (in private memory)
	float3 processed_particle_positions[MAX_CELL_PARTICLE_COUNT];
	for (uint idp = 0; idp < particle_count; ++idp) {
		processed_particle_positions[idp] = get_particle_position(voxel_cell_index, 
																  idp, 
																  grid_info.max_cell_particle_count,
																  indices,
																  positions);
		processed_particle_densities[idp] = 0.0f;
	}

	// Pre-define this before x*y*z loop
	const int3 max_cell_indices = convert_int3(grid_info.grid_dimensions) - (int3)(1, 1, 1);

	// Loop through all voxel cells around the currently processed voxel cell
	// todo optimize these for-loops and voxel cell index generation
	for (int d_idx = -1; d_idx <= 1; ++d_idx) {
		// Check if the x-index lies outside the voxel grid
		const int idx = convert_int(voxel_cell_indices.x) + d_idx;
		if (idx != clamp(idx, 0, max_cell_indices.x)) {
			continue;
		}

		for (int d_idy = -1; d_idy <= 1; ++d_idy) {
			// Check if the x-index lies outside the voxel grid
			const int idy = convert_int(voxel_cell_indices.y) + d_idy;
			if (idy != clamp(idy, 0, max_cell_indices.y)) {
				continue;
			}

			for (int d_idz = -1; d_idz <= 1; ++d_idz) {
				// Check if the x-index lies outside the voxel grid
				const int idz = convert_int(voxel_cell_indices.z) + d_idz;
				if (idz != clamp(idz, 0, max_cell_indices.z)) {
					continue;
				}

				const uint current_voxel_cell_index = calculate_voxel_cell_index((uint3)(idx, idy, idz), grid_info);
				const uint current_voxel_particle_count = cell_particle_count[current_voxel_cell_index];

				// Iterate through this cell's particles
				for (uint idp = 0; idp < current_voxel_particle_count; ++idp) {
					// LOOK HERE:
					// The below line is the reason the nested loops are the way they are:
					//
					// Since the position of a particle is NOT located within the grid cell, each time we want to use the position
					// of a particle we have to calculate its global buffer index and retrieve it that way. This is a slow operation.
					// So instead of having the outer-most loop be over each particle in the current voxel we loop through the voxels
					// This way we only need to fetch the position of each particle in the neighbouring cells ONCE. :D
					const float3 position = get_particle_position(current_voxel_cell_index, 
																  idp,
																  grid_info.max_cell_particle_count,
																  indices,
																  positions);

					for (uint processed_particle_id = 0; processed_particle_id < particle_count; ++processed_particle_id) {
						// Calculate and apply the processed particle's density based on the 'idp'
						processed_particle_densities[processed_particle_id] = processed_particle_densities[processed_particle_id] 
							+ fluid_info.mass * W_poly6(processed_particle_positions[processed_particle_id] - position, grid_info.grid_cell_size);
					}
				}
			}
		}
	}

	// Move the privately stored densities to global memory
	for (uint idp = 0; idp < particle_count; ++idp) {
		// The global density buffer array is simply linear with the particles in no particular order
		// To retrieve the correct index for a particle in a particular voxel cell we have to call our special function :)
		processed_particle_densities[get_particle_buffer_index(voxel_cell_index, idp, grid_info.max_cell_particle_count, indices)]
			= processed_particle_densities[idp];
	}
}

float euclidean_distance2(const float3 r) {
	return r.x * r.x + r.y * r.y + r.z * r.z;
}

float euclidean_distance(const float3 r) {
	return sqrt(r.x * r.x + r.y * r.y + r.z * r.z);
}

float W_poly6(const float3 r, const float h) {
	return (315.0f/(64.0f*convert_float(M_PI)*pow(h,9))) * pow((h*h - euclidean_distance2(r)), 3);
}

uint calculate_voxel_cell_index(const uint3 voxel_cell_indices, const VoxelGridInfo grid_info) {
	return voxel_cell_indices.x + grid_info.grid_dimensions.x * (voxel_cell_indices.y + grid_info.grid_dimensions.y * voxel_cell_indices.z);
}

uint get_particle_buffer_index(const uint voxel_cell_index, 
							   const uint voxel_particle_index, 
					 		   const uint max_cell_particle_count,
					 		   __global const uint* restrict indices) {
	return indices[voxel_cell_index * max_cell_particle_count + voxel_particle_index];
}

float3 get_particle_position(const uint voxel_cell_index, 
							 const uint voxel_particle_index, 
					 		 const uint max_cell_particle_count,
					 		 __global const uint* restrict indices, 
					 		 __global const float* restrict positions) {
	const uint particle_position_index = 3 * get_particle_buffer_index(voxel_cell_index, 
																  voxel_particle_index, 
																  max_cell_particle_count, 
																  indices);

	return (float3)(positions[particle_position_index], 
					positions[particle_position_index + 1], 
					positions[particle_position_index + 2]);
}