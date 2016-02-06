__kernel void taskParallelIntegrateVelocity(__global float3* positions,
                                            __global float3* velocities,
                                            const float dt) {
    int id = get_global_id(0);

    positions[id].x = positions[id].x + dt * velocities[id].x;
    positions[id].y = positions[id].y + dt * velocities[id].y;
    positions[id].z = positions[id].z + dt * velocities[id].z;
}
