__kernel void taskParallelIntegrateVelocity(__global float* positions,
                                            __global float* velocities) {
    int id = get_global_id(0);
    positions[id] = velocities[id];
    velocities[id] = positions[id];
}