#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable

__constant float PI = 3.1415926535f;

typedef struct def_FluidInfo {
	// The mass of each fluid particle
	float mass;

	float k_gas;
	float k_viscosity;
	float rest_density;
	float sigma;
	float k_threshold;
	float k_wall_damper;

	float3 gravity;
} FluidInfo;

__kernel void integrate_particle_states(__global float* restrict positions,
										__global float* restrict velocities,
										__global const float3* restrict forces,
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

	// Acceleration according to Newton's law: a = F / m
	const float3 acceleration = force / fluid_info.mass;

	// Integrate to new state using simple Euler integration
	// Todo investigate other methods such as velocity verlet or leap-frog
	velocity = velocity + acceleration * dt;
	position = position + velocity * dt;

	// Simple bounds-checking
	// Todo replace with actual boundary-force calculations

	// x-boundaries
	if (position.x != clamp(position.x, -0.9f, 0.9f)) {
		position.x = clamp(position.x, -0.9f, 0.9f);
		velocity.x = - velocity.x * fluid_info.k_wall_damper;
	}

	// y-boundaries
	if (position.y != clamp(position.y, -0.9f, 0.9f)) {
		position.y = clamp(position.y, -0.9f, 0.9f);
		velocity.y = - velocity.y * fluid_info.k_wall_damper;
	}

	// z-boundaries
	if (position.z != clamp(position.z, -0.9f, 0.9f)) {
		position.z = clamp(position.z, -0.9f, 0.9f);
		velocity.z = - velocity.z * fluid_info.k_wall_damper;
	}

	// Write new position and velocity
	positions[particle_position_id] = position.x;
	positions[particle_position_id + 1] = position.y;
	positions[particle_position_id + 2] = position.z;
	velocities[particle_position_id] = velocity.x;
	velocities[particle_position_id + 1] = velocity.y;
	velocities[particle_position_id + 2] = velocity.z;
}