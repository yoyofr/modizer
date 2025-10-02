
//#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../external/stb/stb_image_write.h"

#include "ImageWriter.h"

#include <string>

#include "path.h"

bool ImageWriteToPNG(const std::string &path, const uint32_t *data, int width, int height)
{
    int pitch = width * sizeof(data[0]);


//    stbi_write_force_png_filter = 0;
//    stbi_write_png_compression_level = 1;
    
    return stbi_write_png( path.c_str(), width, height, 4, data, pitch ) != 0;
}

bool ImageWriteToBMP(const std::string &path, const uint32_t *data, int width, int height)
{
//    int pitch = width * sizeof(data[0]);
    
    return stbi_write_bmp( path.c_str(), width, height, 4, data  ) != 0;
}


bool ImageWriteToTGA(const std::string &path, const uint32_t *data, int width, int height)
{
    return stbi_write_tga( path.c_str(), width, height, 4, data  ) != 0;
}

bool ImageWriteToHDR(const std::string &path, const float *data, int width, int height)
{
    return stbi_write_hdr( path.c_str(), width, height, 4, data  ) != 0;
}




bool ImageWriteToFile(const std::string &path, const uint32_t *data, int width, int height)
{
    std::string ext = PathGetExtensionLower(path);
    if (ext == ".tga") {
        return ImageWriteToTGA(path, data, width, height);
    }
    if (ext == ".png") {
        return ImageWriteToPNG(path, data, width, height);
    }
    if (ext == ".bmp") {
        return ImageWriteToBMP(path, data, width, height);
    }
    
    return false;
}

