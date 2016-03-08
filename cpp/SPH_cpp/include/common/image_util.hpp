#pragma once

#include <iostream>
#include <string>
#include "lodepng.h"
#include "lodepng_util.h"

// Loads a PNG image file from the "images" folder
// Returns true if success, false otherwise
inline bool loadPNGfromImagesFolder(std::vector<unsigned char> &img_out,
                                    unsigned int &img_width_out,
                                    unsigned int &img_height_out,
                                    std::string image_name) {
    const std::string relative_file_path = "../images/" + image_name;

    const unsigned int error = lodepng::decode(img_out,
                                               img_width_out,
                                               img_height_out,
                                               relative_file_path);

    if (error != 0) {
        std::cerr << "LodePNG error " << error << ": " << lodepng_error_text(error) << std::endl;
        return false;
    }

#ifdef MY_DEBUG
    auto info = lodepng::getPNGHeaderInfo(img_out);
    std::string colortype;
    switch (info.color.colortype) {
        case LodePNGColorType::LCT_GREY:
            colortype = "greyscale";
            break;
        case LodePNGColorType::LCT_PALETTE:
            colortype = "palette";
            break;
        case LodePNGColorType::LCT_RGB:
            colortype = "RGB";
            break;
        case LodePNGColorType::LCT_GREY_ALPHA:
            colortype = "greyscale with alpha";
            break;
        case LodePNGColorType::LCT_RGBA:
            colortype = "RGBA";
            break;
        default:
            colortype = "";
            break;
    }
    std::cout << "bitdepth=" << info.color.bitdepth << ", color type =\"" << colortype << "\"" << std::endl;

#endif

    return true;
}