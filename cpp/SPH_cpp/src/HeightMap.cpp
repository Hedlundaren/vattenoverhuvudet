#include "HeightMap.hpp"

#include "glm/gtx/string_cast.hpp"
#include "common/image_util.hpp"

using std::cout;
using std::cerr;
using std::endl;

float uchar_to_float(unsigned char a) {
    return static_cast<float>(a) / 255.0f;
}

HeightMap::HeightMap() { }

bool HeightMap::initFromPNGs(std::string map_name) {
    bool success = true;

    // load heightmap
    const std::string hmap_name = map_name + ".hmap.png";
    std::vector<unsigned char> hmap;
    unsigned int hmap_width = 0, hmap_height = 0;
    success &= loadPNGfromImagesFolder(hmap, hmap_width, hmap_height, hmap_name);

    if (!success) {
        cerr << "Failed to load heightmap \"" << hmap_name << "\"" << endl;
        return false;
    }

    // load normalmap
    const std::string nmap_name = map_name + ".nmap.png";
    std::vector<unsigned char> nmap;
    unsigned int nmap_width = 0, nmap_height = 0;
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
}

void HeightMap::generateHeightMap(const std::vector<unsigned char> &hmap_src) {
    auto info = lodepng::getPNGHeaderInfo(hmap_src);
    if (info.color.colortype != LodePNGColorType::LCT_GREY) {
        cerr << "Heightmap must be a greyscale image." << endl;
        std::exit(EXIT_FAILURE);
    }

    heightmap.reserve(hmap_src.size());

    for (unsigned int idx = 0; idx < hmap_src.size(); ++idx) {
        heightmap[idx] = uchar_to_float(hmap_src[idx]);
    }
}

void HeightMap::generateNormalMap(const std::vector<unsigned char> &nmap_src) {
    unsigned int CHANNEL_COUNT = 0;

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

    normalmap.reserve(nmap_src.size());

    for (unsigned int y = 0; y < height; ++y) {
        for (unsigned int x = 0; x < width; ++x) {
            // red channel contains x-component of normal
            unsigned int channel = 0; // red
            const float red = uchar_to_float(nmap_src[CHANNEL_COUNT * width * y + CHANNEL_COUNT * x + channel]);

            // green channel contains z-component of normal
            channel = 1; // green
            const float green = uchar_to_float(nmap_src[CHANNEL_COUNT * width * y + CHANNEL_COUNT * x + channel]);

            // extract y-component and store in normalmap
            normalmap[width * y + x] = unbakeNormal(red, green);
        }
    }
}

glm::vec3 HeightMap::unbakeNormal(float x, float z) {
    const float y = sqrtf(1 - x * x - z * z);

    // todo investigate this normalization
    x = 2 * x - 1;
    z = 2 * z - 1;

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