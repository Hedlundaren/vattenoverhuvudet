#pragma once

#include <vector>
#include <random>

#include "glm/glm.hpp"

std::random_device random_device;

/// Generates a vector of floats uniformly distributed in the specified range
/// The vector of random floats
inline std::vector<float> generate_uniform_floats(int N,
                                                  float lower_bound_inclusive = 0.0f,
                                                  float upper_bound_exclusive = 1.0f) {
    std::vector<float> randoms(N);

    std::mt19937 mt(random_device());
    std::uniform_real_distribution<float> distribution(lower_bound_inclusive, upper_bound_exclusive);

    int counter = 0;
    while (counter < N) {
        randoms[counter] = distribution(mt);
        ++counter;
    }

    return randoms;
}

/// Generates a vector of glm::vec3's with each x/y/z uniformly distributed in the specified range
/// The vector of random glm::vec3's
inline std::vector<glm::vec3> generate_uniform_vec3s(int N,
                                                     float x_lower_bound_inclusive = 0.0f,
                                                     float x_upper_bound_exclusive = 1.0f,
                                                     float y_lower_bound_inclusive = 0.0f,
                                                     float y_upper_bound_eyclusive = 1.0f,
                                                     float z_lower_bound_inclusive = 0.0f,
                                                     float z_upper_bound_ezclusive = 1.0f) {
    std::vector<glm::vec3> randoms(N);

    const std::vector<float> random_x = generate_uniform_floats(N, x_lower_bound_inclusive, x_upper_bound_exclusive);
    const std::vector<float> random_y = generate_uniform_floats(N, y_lower_bound_inclusive, y_upper_bound_eyclusive);
    const std::vector<float> random_z = generate_uniform_floats(N, z_lower_bound_inclusive, z_upper_bound_ezclusive);

    int counter = 0;
    while (counter < N) {
        randoms[counter] = glm::vec3(random_x[counter], random_y[counter], random_z[counter]);
        ++counter;
    }

    return randoms;
}

inline std::vector<float> generate_linear_floats(int N,
                                                 float lower_bound_inclusive = 0.0f,
                                                 float upper_bound_inclusive = 1.0f) {
    std::vector<float> linears(N);

    const float range = upper_bound_inclusive - lower_bound_inclusive;
    const float delta_range = range / N;

    for (unsigned int i = 0; i < N; ++i) {
        linears[i] = lower_bound_inclusive + delta_range * i;
    }
}

inline std::vector<glm::vec3> generate_linear_vec3s(int N,
                                                float x_lower_bound_inclusive = 0.0f,
                                                float x_upper_bound_inclusive = 1.0f,
                                                float y_lower_bound_inclusive = 0.0f,
                                                float y_upper_bound_inclusive = 1.0f,
                                                float z_lower_bound_inclusive = 0.0f,
                                                float z_upper_bound_inclusive = 1.0f) {
    std::vector<glm::vec3> linears(N);

    const int N_dim = static_cast<int>(ceil(pow(N, 1.0f / 3)));

    const float range_x = x_upper_bound_inclusive - x_lower_bound_inclusive;
    const float range_y = y_upper_bound_inclusive - y_lower_bound_inclusive;
    const float range_z = z_upper_bound_inclusive - z_lower_bound_inclusive;
    
    const float delta_range_x = range_x / N_dim;
    const float delta_range_y = range_y / N_dim;
    const float delta_range_z = range_z / N_dim;

    int counter = 0;
    for (unsigned int ix = 0; ix < N_dim; ++ix) {
        for (unsigned iy = 0; iy < N_dim; ++iy) {
            for (unsigned int iz = 0; iz < N_dim; ++iz) {
                linears[counter++] = glm::vec3(
                        x_lower_bound_inclusive + ix * delta_range_x,
                        y_lower_bound_inclusive + iy * delta_range_y,
                        z_lower_bound_inclusive + iz * delta_range_z);

                if (counter >= N) {
                    return linears;
                }
            }
        }
    }

    return linears;
}