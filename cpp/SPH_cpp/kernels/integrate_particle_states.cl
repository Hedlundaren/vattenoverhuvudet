#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable

__constant float PI = 3.1415926535f;
__constant float3 VELOCITY_CLAMP = (float3)(10.0f, 10.0f, 10.0f);

typedef struct def_FluidInfo {
	// The mass of each fluid particle
	float mass;

	float k_gas;
	float k_viscosity;
	float rest_density;
	float sigma;
	float k_threshold;
	float k_wall_damper;
	float k_wall_friction;

	float3 gravity;
} FluidInfo;

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

__kernel void integrate_particle_states(__global float* restrict positions,
										__global float* restrict velocities,
										__global const float3* restrict forces,
										const VoxelGridInfo grid_info,
										const FluidInfo fluid_info,
										const float dt) {
	const uint particle_id = get_global_id(0);
	const uint particle_position_id = 3 * particle_id;

	const float3 force = forces[particle_id];
	float3 position = (float3)(positions[particle_position_id],
									 positions[particle_position_id + 1],
									 positions[particle_position_id + 2]);
	float3 velocity = (float3)(velocities[particle_position_id],
									 velocities[particle_position_id + 1],
									 velocities[particle_position_id + 2]);

    // x-boundaries
    if (position.x < grid_info.grid_origin.x){
        position.x = grid_info.grid_origin.x;
        velocity.x =  - velocity.x * fluid_info.k_wall_damper;
        //velocity.y = -0.3f;
    } else if (position.x > grid_info.grid_origin.x + grid_info.grid_dimensions.x * grid_info.grid_cell_size) {
        position.x = grid_info.grid_origin.x + grid_info.grid_dimensions.x * grid_info.grid_cell_size;

        velocity.x = - velocity.x * fluid_info.k_wall_damper;
        //velocity.y = -0.3f;
    }
    // y-boundaries
    if (position.y < grid_info.grid_origin.y){
        position.y = grid_info.grid_origin.y + 0.0001f;

        velocity.y = -velocity.y* fluid_info.k_wall_damper;
    } else if (position.y > grid_info.grid_origin.y + grid_info.grid_dimensions.y * grid_info.grid_cell_size){
        position.y = grid_info.grid_origin.y + grid_info.grid_dimensions.y * grid_info.grid_cell_size;
        velocity.y = -velocity.y* fluid_info.k_wall_damper;
    }

    // z-boundaries
    if (position.z < grid_info.grid_origin.z){
        position.z = grid_info.grid_origin.z;

        //velocity.y = -0.3f;
        velocity.z = - velocity.z * fluid_info.k_wall_damper;
    } else if (position.z > grid_info.grid_origin.z + grid_info.grid_dimensions.z * grid_info.grid_cell_size) {
        position.z = grid_info.grid_origin.z + grid_info.grid_dimensions.z * grid_info.grid_cell_size;

        //velocity.y = -0.3f;
        velocity.z = - velocity.z * fluid_info.k_wall_damper;
    }
	// Acceleration according to Newton's law: a = F / m
	const float3 acceleration = force / fluid_info.mass + fluid_info.gravity;

	// Integrate to new state using simple Euler integration
	// Todo investigate other methods such as velocity verlet or leap-frog
	//velocity = velocity + acceleration * dt;
	velocity = clamp(velocity + acceleration * dt, -VELOCITY_CLAMP, VELOCITY_CLAMP);
	position = position + velocity * dt;

	// Write new position and velocity
	positions[particle_position_id] = position.x;
	positions[particle_position_id + 1] = position.y;
	positions[particle_position_id + 2] = position.z;
	velocities[particle_position_id] = velocity.x;
	velocities[particle_position_id + 1] = velocity.y;
	velocities[particle_position_id + 2] = velocity.z;
}