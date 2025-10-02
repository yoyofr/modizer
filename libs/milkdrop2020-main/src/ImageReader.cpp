
//#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb/stb_image.h"

#include "ImageReader.h"

#include <string>

#include "path.h"
#include "platform.h"

using namespace render;

render::ImageDataPtr ImageLoadFromFileInMemory(const uint8_t *data, size_t size)
{
//    STBIDEF stbi_uc *stbi_load_from_memory   (buffer.data(), (int)buffer.size(), int *x, int *y, int *channels_in_file, int desired_channels);
    
    stbi_set_flip_vertically_on_load(1);
    
    int x,y,channels_in_file;
    int desired_channels = 4;
    stbi_uc *image_data = stbi_load_from_memory   (data, (int)size, &x, &y, &channels_in_file, desired_channels);
    
    if (!image_data) {
        return nullptr;
    }
    
    auto ptr = std::make_shared<ImageData>(x, y, PixelFormat::RGBA8Unorm);
    
    memcpy(ptr->data, image_data, ptr->size);
    
    STBI_FREE(image_data);
    return ptr;
}



render::ImageDataPtr ImageLoadFromFile(const std::string &path)
{
    std::vector<uint8_t> buffer;
    if (!FileReadAllBytes(path, buffer)) {
        return nullptr;
    }
    return ImageLoadFromFileInMemory(buffer.data(), buffer.size());
}
