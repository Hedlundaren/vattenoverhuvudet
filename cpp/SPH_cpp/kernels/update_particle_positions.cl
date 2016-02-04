__kernel void taskParallelIntegrateVelocity(__global float3* positions,
                                            __global float3* velocities) {
    int id = get_global_id(0);
    positions[id] = positions[id] + 0.016 * velocities[id];
}