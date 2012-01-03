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
	CGImage* image = uiimage.CGImage;
	assert(image != NULL);
	const int width = CGImageGetWidth(image);
	const int height = CGImageGetHeight(image);
	const int dataSize = width * height * 4;
	uint handle;
	
	uint8_t* textureData = (uint8_t*)malloc(dataSize);
	CGContext* textureContext = CGBitmapContextCreate(textureData, width, height, 8, width * 4, CGImageGetColorSpace(image), kCGImageAlphaPremultipliedLast);
	CGContextDrawImage(textureContext, CGRectMake(0.0, 0.0, (CGFloat)width, (CGFloat)height), image);
	CGContextRelease(textureContext);
	

	glGenTextures(1, &handle);
	glBindTexture(GL_TEXTURE_2D, handle);
	
	glTexParameterf(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
	
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	glBindTexture(GL_TEXTURE_2D, 0);
	
	free(textureData);
	return handle;
}

