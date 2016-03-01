#pragma once

#include <string>

#include "glm/glm.hpp"

#include "lodepng.h"
#include "lodepng_util.h"
#include "nanoflann.hpp"

class HeightMap {
public:
    HeightMap();

    void initFromPNGs(std::string heightmap_name = "simple");

private:
    unsigned int width, height;

    std::vector<float> heightmap;
    std::vector<glm::vec3> normalmap;
};