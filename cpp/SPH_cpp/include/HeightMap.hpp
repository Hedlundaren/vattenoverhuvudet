#pragma once

#include <string>
#include <vector>

#include "glm/glm.hpp"

#include "lodepng.h"
#include "lodepng_util.h"
#include "nanoflann.hpp"

class HeightMap {
public:
    HeightMap();

    // Initialize the HeightMap with the provided PNGs located in the images/ folder
    // Returns true if it succeeded
    bool initFromPNGs(std::string map_name = "simple");

    void debug_print();

private:
    unsigned int width, height;

    std::vector<float> heightmap;
    std::vector<glm::vec3> normalmap;

    void generateHeightMap(const std::vector<unsigned char> &hmap_src);
    void generateNormalMap(const std::vector<unsigned char> &nmap_src);

    glm::vec3 unbakeNormal(float x, float z);
};