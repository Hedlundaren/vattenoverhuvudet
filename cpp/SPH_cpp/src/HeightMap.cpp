#include "HeightMap.hpp"
#include "HeightMapData.hpp"

#include <cmath>
#include <algorithm>

#include "glm/gtx/string_cast.hpp"
#include "common/image_util.hpp"


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

HeightMap::~HeightMap() {
    glDeleteBuffers(1, &VBO_positions);
    glDeleteBuffers(1, &VBO_normals);
    glDeleteBuffers(1, &VEO_indices);
    glDeleteVertexArrays(1, &VAO);

    // ShaderProgram is automatically deleted since it is a smart pointer
}

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
        case LodePNGColorType::LCT_RGB:
            CHANNEL_COUNT = 3;
            break;
        case LodePNGColorType::LCT_RGBA:
            CHANNEL_COUNT = 4;
            break;
        default:
            cerr << "Normalmap image must have at least 3 channels." << endl;
            std::exit(EXIT_FAILURE);
    }

    normalmap.resize(width * height);

    for (uint y = 0; y < height; ++y) {
        for (uint x = 0; x < width; ++x) {
            // red channel contains x-component of normal
            uint channel = 0; // red
            const float red = uchar_to_float(nmap_src[CHANNEL_COUNT * width * y + CHANNEL_COUNT * x + channel]);

            // green channel contains y-component of normal
            channel = 1; // green
            const float green = uchar_to_float(nmap_src[CHANNEL_COUNT * width * y + CHANNEL_COUNT * x + channel]);

            // blue channel contains z-component of normal
            channel = 2; // green
            const float blue = uchar_to_float(nmap_src[CHANNEL_COUNT * width * y + CHANNEL_COUNT * x + channel]);

            // extract y-component and store in normalmap
            normalmap[width * y + x] = glm::vec3(red, green, blue);
        }
    }
}

glm::vec3 HeightMap::unbakeNormal(float x, float z) {
    const float y = 2 * sqrtf(1 - x * x - z * z) - 1;

    // todo should this be done before or after calculating y?
    x = 2 * x - 1;
    z = 2 * z - 1;

    return glm::normalize(glm::vec3(x, y, z));
}

void HeightMap::debug_print(int print_count) {
    if (print_count == -1) {
        print_count = height * width;
    }

    uint counter = 0;
    cout << "Height map:" << endl;
    for (const auto value : heightmap) {
        cout << value << ", ";

        if (++counter >= print_count) {
            counter = 0;
            break;
        }
    }
    cout << endl;

    cout << "Normal map:" << endl;
    for (const auto value : normalmap) {
        cout << glm::to_string(value) << ", ";

        if (++counter >= print_count) {
            counter = 0;
            break;
        }
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

    voxel_sampler_size = glm::uvec3(std::min(MAX_VOXEL_SAMPLER_SIZE.x, width),
                          std::min(MAX_VOXEL_SAMPLER_SIZE.y, std::max(width, height)),
                          std::min(MAX_VOXEL_SAMPLER_SIZE.z, height));
    auto get_flat_index = [&](uint x, uint y, uint z) {
        return x + voxel_sampler_size.x * (y + voxel_sampler_size.y * z);
    };

    density_voxel_sampler.resize(voxel_sampler_size.x * voxel_sampler_size.y * voxel_sampler_size.z);
    normal_voxel_sampler.resize(voxel_sampler_size.x * voxel_sampler_size.y * voxel_sampler_size.z);

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

    for (uint vox_idx = 0; vox_idx < voxel_sampler_size.x; ++vox_idx) {
        for (uint vox_idy = 0; vox_idy < voxel_sampler_size.y; ++vox_idy) {
            for (uint vox_idz = 0; vox_idz < voxel_sampler_size.z; ++vox_idz) {
                const uint flat_index = get_flat_index(vox_idx, vox_idy, vox_idz);

                // knnSearch: perform a search for the N closest points
                const glm::vec3 query(static_cast<float>(vox_idx) / voxel_sampler_size.x,
                                      static_cast<float>(vox_idy) / voxel_sampler_size.y,
                                      static_cast<float>(vox_idz) / voxel_sampler_size.z);

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
    std::vector<uint> indices((width - 1) * (height - 1) * 3 * 2);

    glm::vec3 position;
    for (uint imx = 0; imx < width; ++imx) {
        for (uint imy = 0; imy < height; ++imy) {
            // x/z-coords are simply generated from for-loop indices
            position.x =  origin.x + (static_cast<float>(imx) / width) * dimensions.x;
            position.z =  origin.z + (static_cast<float>(imy) / height) * dimensions.z;

            // Read y-coord of vertex from heightmap
            position.y = origin.y + heightmap[imx + width * imy] * dimensions.y;

            positions[imx + width * imy] = position;
        }
    }

    /// setup shader
    shader = std::shared_ptr<ShaderProgram>(new ShaderProgram("../shaders/heightmap.vert",
                                                              "", "", "", // no tesselation or geometry
                                                              "../shaders/heightmap.frag"));
    MV_loc = glGetUniformLocation(*shader, "MV");
    P_loc = glGetUniformLocation(*shader, "P");

    // generate vertex buffer for positions
    glGenBuffers(1, &VBO_positions);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_positions);
    glBufferData(GL_ARRAY_BUFFER, 3 * width * height * sizeof(float), positions.data(), GL_STATIC_DRAW);

    // generate vertex buffer for normals
    glGenBuffers(1, &VBO_normals);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_normals);
    glBufferData(GL_ARRAY_BUFFER, 3 * width * height * sizeof(float), normalmap.data(), GL_STATIC_DRAW);

    const auto calc_flat_index = [&](uint imx, uint imy) {
        return imx + width * imy;
    };

    // setup vertex triangle indices
    for (uint imx = 0; imx + 1 < width; ++imx) {
        for (uint imy = 0; imy + 1 < width; ++imy) {
//              0      1
//            3 x------x
//              |\     |
//              | \  A |
//              |  \   |
//              |   \  |
//              |  B \ |
//              |     \|
//            5 x------x 2
//                     4

            const uint flat_index = calc_flat_index(imx, imy);

            // triangle A
            indices[6 * flat_index + 0] = flat_index;
            indices[6 * flat_index + 1] = calc_flat_index(imx + 1, imy);
            indices[6 * flat_index + 2] = calc_flat_index(imx + 1, imy + 1);

            // triangle B
            indices[6 * flat_index + 3] = flat_index;
            indices[6 * flat_index + 4] = calc_flat_index(imx + 1, imy + 1);
            indices[6 * flat_index + 5] = calc_flat_index(imx, imy + 1);
        }
    }

    glGenBuffers(1, &VEO_indices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VEO_indices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint), indices.data(), GL_STATIC_DRAW);

    vertex_indices_count = indices.size();

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

void HeightMap::render(glm::mat4 P, glm::mat4 MV) {
    glUseProgram(*shader);
    glUniformMatrix4fv(MV_loc, 1, GL_FALSE, &MV[0][0]);
    glUniformMatrix4fv(P_loc, 1, GL_FALSE, &P[0][0]);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VEO_indices);
    //glDrawArrays(GL_TRIANGLES, 0, width * height);
    glCullFace(GL_FRONT);
    glDrawElements(GL_TRIANGLES, vertex_indices_count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}