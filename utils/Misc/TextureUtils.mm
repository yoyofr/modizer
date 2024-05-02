/*
 *  TextureUtils.mm
 *  modizer
 *
 *  Created by Yohann Magnien on 23/08/10.
 *  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
 *
 */

#include "TextureUtils.h"

#include <assert.h>
#include <UIKit/UIKit.h>
#include <OpenGLES/ES1/glext.h>


uint TextureUtils::Create(UIImage* uiimage)
{
    if (uiimage==NULL) return 0;
	CGImage* image = uiimage.CGImage;
	assert(image != NULL);
	const int width = CGImageGetWidth(image);
	const int height = CGImageGetHeight(image);
	const int dataSize = width * height * 4;
	uint handle;
    
    CFDataRef data = CGDataProviderCopyData(CGImageGetDataProvider(image));
    GLubyte *textureData = (GLubyte *)CFDataGetBytePtr(data);
	
	glGenTextures(1, &handle);
	glBindTexture(GL_TEXTURE_2D, handle);
	
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);//TRUE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
	
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	glBindTexture(GL_TEXTURE_2D, 0);
	
	//free(textureData);
    CFRelease(data);
	return handle;
}

