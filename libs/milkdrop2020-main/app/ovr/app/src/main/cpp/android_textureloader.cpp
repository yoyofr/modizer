#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <jni.h>
#include <stdlib.h>
#include <string.h>
#include <android/log.h>
#include <GLES3/gl3.h>

#define STB_IMAGE_IMPLEMENTATION
//#define STBI_ONLY_PNG
#include "stb_image.h"

#include "platform.h"
#include "render/context.h"
#include "render/context_gles.h"

bool STB_LoadTextureFromFile(const char *path, render::gles::GLTextureInfo &ti)
{
    std::vector<uint8_t> fileBits;
    if (!FileReadAllBytes(path, fileBits))
        return false;


    int width, height;
    int channelCount;

    stbi_set_flip_vertically_on_load(1);

    uint8_t* data = stbi_load_from_memory(
            fileBits.data(), fileBits.size(),
            &width, &height, &channelCount, 4);
    if (!data) {
        return false;
    }

    GLuint name;
    glGenTextures(1, &name);
    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D, name);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);

    stbi_image_free(data);

    ti.name = name;
    ti.width = width;
    ti.height = height;
    return true;
}

