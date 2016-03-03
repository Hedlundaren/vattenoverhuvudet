#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

#include "rendering/ShaderProgram.hpp"
#include "GLFW/glfw3.h"

#include "glm/glm.hpp"

typedef unsigned int uint;

// forward declaration of classes
//class ShaderProgram;

class HeightMap {
public:
    HeightMap();
    ~HeightMap();

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

    void debug_print(int print_count = -1);

    // first argument is functor that takes in vector of distances, the maximum radius (aka kernel size) and returns the
    // density contribution (arbitrary units)
    void calcVoxelSamplers(std::function<float(std::vector<float>, float)> density_func, float density_radius_norm = 0.01f);

    void initGL(glm::vec3 origin, glm::vec3 dimensions);

    void render(glm::mat4 P, glm::mat4 MV);

private:
    uint width, height;

    std::vector<float> heightmap;
    std::vector<glm::vec3> normalmap;

    static glm::uvec3 MAX_VOXEL_SAMPLER_SIZE;

    std::vector<float> density_voxel_sampler;
    std::vector<glm::vec3> normal_voxel_sampler;

    void generateHeightMap(const std::vector<unsigned char> &hmap_src);
    void generateNormalMap(const std::vector<unsigned char> &nmap_src);

    glm::vec3 unbakeNormal(float x, float z);

    //////////////////////
    // OpenGL variables //
    //////////////////////

    std::shared_ptr<ShaderProgram> shader;
    GLint MV_loc, P_loc;

    GLuint VAO;
    GLuint VBO_positions, VBO_normals, VEO_indices;

    uint vertex_indices_count;
};