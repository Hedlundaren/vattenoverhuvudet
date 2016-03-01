#pragma once

#include <string>
#include <vector>

#include "glm/glm.hpp"

#include "lodepng.h"
#include "lodepng_util.h"
#include "nanoflann.hpp"

typedef unsigned int uint;

class HeightMap {
public:
    HeightMap();

    inline static void SetMaxVoxelSamplerSize(uint max_size) {
        SetMaxVoxelSamplerSize(max_size, max_size, max_size);
    }

    static void SetMaxVoxelSamplerSize(uint max_x, uint max_y, uint max_z) {
        MAX_VOXEL_SAMPLER_SIZE.x = max_x;
        MAX_VOXEL_SAMPLER_SIZE.y = max_y;
        MAX_VOXEL_SAMPLER_SIZE.z = max_z;
    }

    // Initialize the HeightMap with the provided PNGs located in the images/ folder
    // Returns true if it succeeded
    bool initFromPNGs(std::string map_name = "simple");

    void debug_print();

    void calcDensityVoxelSampler();

    void calcBakedNormalVoxelSampler();

    void calcNormalVoxelSampler();

private:
    uint width, height;

    std::vector<float> heightmap;
    std::vector<glm::vec3> normalmap;

    static glm::uvec3 MAX_VOXEL_SAMPLER_SIZE;

    std::vector<float> density_voxel_sampler;
    std::vector<glm::vec2> baked_normal_voxel_sampler;
    std::vector<glm::vec3> normal_voxel_sampler;

    void generateHeightMap(const std::vector<unsigned char> &hmap_src);
    void generateNormalMap(const std::vector<unsigned char> &nmap_src);

    glm::vec3 unbakeNormal(float x, float z);
};