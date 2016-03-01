#include "HeightMap.hpp"
#include "HeightMapData.hpp"

#include <cmath>
#include <algorithm>

#include "glm/gtx/string_cast.hpp"
#include "common/image_util.hpp"
#include "rendering/ShaderProgram.hpp"

// LodePNG for PNG loading
#include "lodepng.h"
#include "lodepng_util.h"

// nanoflann for kd-tree nearest neighbour calculation, used for voxel grid sampler calc
#include "nanoflann.hpp"

#define ALWAYS_CALCULATE_CLOSEST_NORMAL true

using std::cout;
using std::cerr;
using std::endl;

glm::uvec3 HeightMap::MAX_VOXEL_SAMPLER_SIZE = glm::uvec3(32, 32, 32);

float uchar_to_float(unsigned char a) {
    return static_cast<float>(a) / 255.0f;
}

HeightMap::HeightMap() { }

bool HeightMap::initFromPNGs(std::string map_name) {
    bool success = true;

    // load heightmap
    const std::string hmap_name = map_name + ".hmap.png";
    std::vector<unsigned char> hmap;
    uint hmap_width = 0, hmap_height = 0;
    success &= loadPNGfromImagesFolder(hmap, hmap_width, hmap_height, hmap_name);

    if (!success) {
        cerr << "Failed to load heightmap \"" << hmap_name << "\"" << endl;
        return false;
    }

    // load normalmap
    const std::string nmap_name = map_name + ".nmap.png";
    std::vector<unsigned char> nmap;
    uint nmap_width = 0, nmap_height = 0;
    success &= loadPNGfromImagesFolder(nmap, nmap_width, nmap_height, nmap_name);

    if (!success) {
        cerr << "Failed to load normalmap \"" << nmap_name << "\"" << endl;
        return false;
    }

    // check if normalmap and heightmap are equal size
    success &= (hmap_width == nmap_width) && (hmap_height == nmap_height);
    if (!success) {
        cerr << "\"" << hmap_name << "\" and \"" << nmap_name << "\" differ in size." << endl
        << "hmap_width=" << hmap_width << ", hmap_height" << hmap_height
        << ", nmap_width=" << nmap_width << ", nmap_height=" << nmap_height << endl;
        return false;
    }

    width = hmap_width;
    height = hmap_height;

    generateHeightMap(hmap);
    generateNormalMap(nmap);

    return true;
}

void HeightMap::generateHeightMap(const std::vector<unsigned char> &hmap_src) {
    uint CHANNEL_COUNT = 0;

    auto info = lodepng::getPNGHeaderInfo(hmap_src);
    switch (info.color.colortype) {
        case LodePNGColorType::LCT_GREY:
            cerr << "Cannot create normalmap from greyscale image." << endl;
            std::exit(EXIT_FAILURE);
        case LodePNGColorType::LCT_RGB:
            CHANNEL_COUNT = 3;
            break;
        case LodePNGColorType::LCT_GREY_ALPHA:
            CHANNEL_COUNT = 2;
            break;
        case LodePNGColorType::LCT_RGBA:
            CHANNEL_COUNT = 4;
            break;
    }

    heightmap.resize(width * height);

    for (uint y = 0; y < height; ++y) {
        for (uint x = 0; x < width; ++x) {
            heightmap.at(width * y + x) = std::min(1.0f, uchar_to_float(hmap_src[CHANNEL_COUNT * (width * y + x)]));
        }
    }
}

void HeightMap::generateNormalMap(const std::vector<unsigned char> &nmap_src) {
    uint CHANNEL_COUNT = 0;

    auto info = lodepng::getPNGHeaderInfo(nmap_src);
    switch (info.color.colortype) {
        case LodePNGColorType::LCT_GREY:
            cerr << "Cannot create normalmap from greyscale image." << endl;
            std::exit(EXIT_FAILURE);
        case LodePNGColorType::LCT_RGB:
            CHANNEL_COUNT = 3;
            break;
        case LodePNGColorType::LCT_GREY_ALPHA:
            CHANNEL_COUNT = 2;
            break;
        case LodePNGColorType::LCT_RGBA:
            CHANNEL_COUNT = 4;
            break;
    }

    normalmap.resize(width * height);

    for (uint y = 0; y < height; ++y) {
        for (uint x = 0; x < width; ++x) {
            // red channel contains x-component of normal
            uint channel = 0; // red
            const float red = uchar_to_float(nmap_src[CHANNEL_COUNT * width * y + CHANNEL_COUNT * x + channel]);

            // green channel contains z-component of normal
            channel = 1; // green
            const float green = uchar_to_float(nmap_src[CHANNEL_COUNT * width * y + CHANNEL_COUNT * x + channel]);

            // extract y-component and store in normalmap
            normalmap.at(width * y + x) = unbakeNormal(red, green);
        }
    }
}

glm::vec3 HeightMap::unbakeNormal(float x, float z) {
    // todo should this be done before or after calculating y?
    x = 2 * x - 1;
    z = 2 * z - 1;

    const float y = sqrtf(1 - x * x - z * z);

    return glm::vec3(x, y, z);
}

void HeightMap::debug_print() {
    cout << "Height map:" << endl;
    for (const auto value : heightmap) {
        cout << value << ", ";
    }
    cout << endl;

    cout << "Normal map:" << endl;
    for (const auto value : normalmap) {
        cout << glm::to_string(value) << ", ";
    }
    cout << endl;
}

void HeightMap::calcVoxelSamplers(std::function<float(const std::vector<float>, const float)> density_func,
                                  float density_radius_norm) {
    using namespace nanoflann;
    typedef KDTreeSingleIndexAdaptor<
            L2_Simple_Adaptor<float, HeightMapData>,
            HeightMapData,
            1> kdTree;

    const glm::uvec3 size(std::min(MAX_VOXEL_SAMPLER_SIZE.x, width),
                          std::min(MAX_VOXEL_SAMPLER_SIZE.y, std::max(width, height)),
                          std::min(MAX_VOXEL_SAMPLER_SIZE.z, height));
    auto get_flat_index = [&](uint x, uint y, uint z) {
        return x + size.x * (y + size.y * z);
    };

    density_voxel_sampler.resize(size.x * size.y * size.z);
    normal_voxel_sampler.resize(size.x * size.y * size.z);

    // Create heightmap data set for kdTree nearest neighbour algorithm
    std::shared_ptr<std::vector<glm::vec3>> tmp_map(new std::vector<glm::vec3>);
    tmp_map->resize(width * height);

    for (uint imx = 0; imx < width; ++imx) {
        for (uint imy = 0; imy < height; ++imy) {
            const float x = static_cast<float>(imx) / width;
            const float z = static_cast<float>(imy) / height;
            const float y = heightmap[imx + imy * width];

            tmp_map->at(imx + imy * width) = glm::vec3(x, y, z);
        }
    }

    const HeightMapData heightMapData(tmp_map);

    kdTree index(1, heightMapData, KDTreeSingleIndexAdaptorParams(4));
    index.buildIndex();

    uint counter = 0;

    for (uint vox_idx = 0; vox_idx < size.x; ++vox_idx) {
        for (uint vox_idy = 0; vox_idy < size.y; ++vox_idy) {
            for (uint vox_idz = 0; vox_idz < size.z; ++vox_idz) {
                const uint flat_index = get_flat_index(vox_idx, vox_idy, vox_idz);

                // knnSearch: perform a search for the N closest points
                const glm::vec3 query(static_cast<float>(vox_idx) / size.x,
                                      static_cast<float>(vox_idy) / size.y,
                                      static_cast<float>(vox_idz) / size.z);

                // Find neighbours within "density-contribution-range"
                std::vector<std::pair<size_t, float>> indices_distances;
                index.radiusSearch(&(query[0]), density_radius_norm, indices_distances, SearchParams());

                // Extract distances and calculate the total density contribution
                std::vector<float> distances;
                std::transform(indices_distances.begin(), indices_distances.end(), std::back_inserter(distances),
                               [](const std::pair<size_t, float> &p) { return p.second; });

                const float density_contribution = density_func(distances, density_radius_norm);
                density_voxel_sampler[flat_index] = density_contribution;

#ifdef MY_DEBUG
                cout << "radiusSearch(): query=" << glm::to_string(query) << ", result_count=" <<
                indices_distances.size() << endl;
                for (uint result = 0; result < indices_distances.size(); ++result) {
                    cout << "idx[" << result << "]=" << indices_distances[result].first << " dist[" << result << "]=" <<
                    indices_distances[result].second << endl;
                }
                cout << endl;
#endif

#if !ALWAYS_CALCULATE_CLOSEST_NORMAL
                if (indices_distances.empty() ) {
#endif
                // Find the closest neighbour
                size_t closest_index = 0;
                float distance = INFINITY;
                index.knnSearch(&(query[0]), 1, &closest_index, &distance);

                // todo investigate indexing here
                normal_voxel_sampler[flat_index] = normalmap[closest_index];

#if !ALWAYS_CALCULATE_CLOSEST_NORMAL
                } else {
                    normal_voxel_sampler[flat_index] = glm::vec3(0.0f, 1.0f, 0.0f);
                }
#endif

#ifdef MY_DEBUG
                cout << "knnSearch(): query=" << glm::to_string(query) << endl;
                cout << "closest index=" << closest_index << ", distance=" << distance << endl;
#endif
            }
        }
    }
}

void HeightMap::initGL(glm::vec3 origin, glm::vec3 dimensions) {
    std::vector<glm::vec3> positions(width * height);

    glm::vec3 position;
    for (uint imx = 0; imx < width; ++imx) {
        for (uint imy = 0; imy < height; ++imy) {
            // x/z-coords are simply generated from for-loop indices
            position.x = (static_cast<float>(imx) / width) * dimensions.x - origin.x;
            position.z = (static_cast<float>(imy) / height) * dimensions.z - origin.z;

            // Read y-coord of vertex from heightmap
            position.y = heightmap[imx + width * imy] * dimensions.y - origin.y;

            positions[imx + width * imy] = position;
        }
    }

    /// setup shader
    shader = std::shared_ptr<ShaderProgram>(new ShaderProgram("../shaders/heightmap.vert",
                                                              "", "", "", // no tesselation or geometry
                                                              "../shaders/heightmap.frag"));
    MVP_loc = glGetUniformLocation(*shader, "MVP");

    // generate vertex buffer for positions
    glGenBuffers(1, &VBO_positions);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_positions);
    glBufferData(GL_ARRAY_BUFFER, 3 * width * height * sizeof(float), positions.data(), GL_STATIC_DRAW);

    // generate vertex buffer for normals
    glGenBuffers(1, &VBO_normals);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_normals);
    glBufferData(GL_ARRAY_BUFFER, 3 * width * height * sizeof(float), normalmap.data(), GL_STATIC_DRAW);

    // setup vertex triangle indices


    /// setup Vertex Array Object
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_positions);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL); //position
    glBindBuffer(GL_ARRAY_BUFFER, VBO_normals);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL); //normal

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
}

void HeightMap::render(glm::mat4 MVP) {
    glUseProgram(*shader);
    glUniformMatrix4fv(MVP_loc, 1, GL_FALSE, &MVP[0][0]);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, width * height);
    glBindVertexArray(0);
}