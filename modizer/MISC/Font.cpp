/*
 *  Font.cpp
 *  FontTest
 *
 *  Created by George Sealy on 9/04/09.
 *
 */

#include <stdlib.h>
#include <string>

#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>

#include "Font.h"


CCharacterData::CCharacterData()
{
}

CCharacterData::~CCharacterData()
{
	delete [] buffer;
}

void CCharacterData::Load(FILE *fontFile)
{
	fread(&byteWidth, sizeof(int), 1, fontFile);
	fread(&byteHeight, sizeof(int), 1, fontFile);
	fread(&xOffset, sizeof(int), 1, fontFile);
	fread(&yOffset, sizeof(int), 1, fontFile);
	fread(&screenWidth, sizeof(int), 1, fontFile);
	fread(&screenHeight, sizeof(int), 1, fontFile);
	
	buffer = new unsigned char[byteWidth * byteHeight];
	fread(buffer, byteWidth * byteHeight, 1, fontFile);
}

// Returns the texture size that will fit all the character data, or 0 if unable
// to fit al of the characters
int CFont::GetHeightForTexture(int textureWidth, int charWidth, int charHeight)
{
	int texX = charWidth;
	int texY = charHeight;

	for (int i = 0; i < 256; ++i)
	{
		if (mCharacterData[i].isWanted)
		{
			if (texX + charWidth < textureWidth)
			{
				texX += charWidth;
			}
			else
			{
				texX = charWidth;
				texY += charHeight;
			}
		}
	}
	
	int nextPowerOfTwo = 64;
	
	while (nextPowerOfTwo < texY)
	{
		nextPowerOfTwo *= 2;
		
		// The iPhone can't handle textures larger than 1024 in size
		if (nextPowerOfTwo > 1024)
		{
			return 0;
		}
	}
	
	return nextPowerOfTwo;
}

CFont::CFont(const std::string &fontFilename)
{
	FILE *fontFile = fopen(fontFilename.c_str(), "rb");
	
	if (fontFile)
	{		
		mCharacterData = new CCharacterData[256];
		
		int i;
		int numChars = 0;
		int maxSizeX = 0, maxSizeY = 0;	// Find the largest character
		for (i = 0; i < 256; ++i)
		{
			mCharacterData[i].Load(fontFile);

			// NOTE : Should allow user to specify...
			// Currently just gets all the ascii characters
			mCharacterData[i].isWanted = (i >= 32 && i <= 127);
			
			// Calculate maximum size of a letter, and number of characters we need...
			if (!mCharacterData[i].isWanted)
			{
				continue;
			}
			
			++numChars;
			if (maxSizeX < mCharacterData[i].byteWidth)
			{
				maxSizeX = mCharacterData[i].byteWidth;
			}
			if (maxSizeY < mCharacterData[i].byteHeight)
			{
				maxSizeY = mCharacterData[i].byteHeight;
			}
		}

		// We add a little extra space around all characters to avoid one
		// character bleeding into another.
		const int cCharacterPadding = 4;
		maxSizeX += cCharacterPadding;
		maxSizeY += cCharacterPadding;
		
		// Find the size of texture we need
		int textureWidth = 256;
		int textureHeight = GetHeightForTexture(textureWidth, maxSizeX, maxSizeY);
		
		while (textureHeight <= 0)
		{
			textureWidth *= 2;
			textureHeight = GetHeightForTexture(textureWidth, maxSizeX, maxSizeY);
		}
		
		// Allocate texture data
		GLubyte *textureData = new GLubyte[textureWidth * textureHeight * 4];
		
		// Should really clear out data array...
		int texX = 0;
		int texY = 0;
		
		for (i = 0; i < 256; ++i)
		{
			if (!mCharacterData[i].isWanted)
			{
				continue;
			}
			
			if (texX + maxSizeX >= textureWidth)
			{
				texX = 0;
				texY += maxSizeY;
			}
			
			int offX = texX + cCharacterPadding / 2;
			int offY = textureHeight - 1 - (texY + cCharacterPadding / 2);
			
			// Calculate texture coordinates for this character
			float widthScale = 1.0f / (float)(textureWidth - 0);
			float heightScale = 1.0f / (float)(textureHeight - 0);
			
			float left = (float)offX * widthScale;
			float right = 
				left + (float)mCharacterData[i].byteWidth * widthScale;
			
			float top = (float)offY * heightScale;
			float bottom = 
				top - (float)mCharacterData[i].byteHeight * heightScale;
			
			mCharacterData[i].texCoords[0] = left;
			mCharacterData[i].texCoords[1] = bottom;
			
			mCharacterData[i].texCoords[2] = left;
			mCharacterData[i].texCoords[3] = top;
			
			mCharacterData[i].texCoords[4] = right;
			mCharacterData[i].texCoords[5] = bottom;
			
			mCharacterData[i].texCoords[6] = right;
			mCharacterData[i].texCoords[7] = top;
			
			// Store the pixel data in the texture
			// Should investigate using a single channel texture
			for (int x = 0; x < mCharacterData[i].byteWidth; ++x)
			{
				for (int y = 0; y < mCharacterData[i].byteHeight; ++y)
				{						
					int texIndex = ((offY - y-1) * textureWidth) + (offX + x);
					int charIndex = y * mCharacterData[i].byteWidth + x;
					
					textureData[4 * texIndex + 0] = 255;
					textureData[4 * texIndex + 1] = 255;
					textureData[4 * texIndex + 2] = 255;
					textureData[4 * texIndex + 3] = mCharacterData[i].buffer[charIndex];
				}
			}
			
			texX += maxSizeX;
		}
		
		// Create the texture itself.
		glEnable(GL_TEXTURE_2D);
		
		glGenTextures(1, &mTexId);
		glBindTexture(GL_TEXTURE_2D, mTexId);

		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		
		// Copy texture data
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureWidth, textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
		
		glBindTexture(GL_TEXTURE_2D, 0);
		
		delete [] textureData;
	}
}

CFont::~CFont()
{
	delete [] mCharacterData;
}
