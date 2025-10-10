/*
 *  TextureUtils.h
 *  modizer
 *
 *  Created by Yohann Magnien on 23/08/10.
 *  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
 *
 */

#ifndef st_TextureUtils_h_
#define st_TextureUtils_h_

#include <types.h>
#include <GLES3/gl3.h>

bool LoadTextureFromMemory(const void* data, size_t data_size, GLuint* out_texture, int* out_width, int* out_height);
bool LoadTextureFromFile(const char* file_name, GLuint* out_texture, int* out_width, int* out_height);

#endif

