

#pragma once

#include <string>
#include <memory>

namespace render {
    class ImageData;
    using ImageDataPtr = std::shared_ptr<ImageData>;
}

render::ImageDataPtr ImageLoadFromFile(const std::string &path);
render::ImageDataPtr ImageLoadFromFileInMemory(const uint8_t *data, size_t size);
