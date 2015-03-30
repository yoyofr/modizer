/*
 *  GLString.cpp
 *  FontTest
 *
 *  Created by George Sealy on 15/04/09.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>

#include "GLString.h"
#include "Font.h"

CGLString::CGLString(const char *text, const CFont *font,float mScaleFactor) :
	mText(NULL),
	mFont(font),
	mVertices(NULL),
	mUVs(NULL),
	mIndices(NULL),
	mColors(NULL)
{
	mText = strdup(text);
    
    scaleFactor=mScaleFactor;
	
	// We figure out the number of texured quads we'll be rendering later
	// (the number of characters minus any spaces)
	mNumberOfQuads = 0;
	
	int i, length = strlen(mText);
	for (i = 0; i < length; ++i)
	{
		if (mText[i] != ' ')
		{
			++mNumberOfQuads;
		}
	}
}

CGLString::~CGLString()
{
	free(mText);
	if (mVertices) {
		delete [] mVertices;
		mVertices=NULL;
	}
	if (mUVs) {
		delete [] mUVs;
		mUVs=NULL;
	}
	if (mIndices) {
		delete [] mIndices;
		mIndices=NULL;
	}
	if (mColors) {
		delete [] mColors;
		mColors=NULL;
	}
}

void CGLString::Render(int msg_type)
{
	// The first time we render this text, build the set of textured quads
	if (!mVertices) {
		BuildString(msg_type);
	} else if (msg_type&128) {
		unsigned char valr,valg,valb,vala;
		valr=valg=valb=0xFF;
		vala=(msg_type&127)*2;
		for (int i = 0; i < mNumberOfQuads; ++i) {
			mColors[i*4 + 0].r=mColors[i*4 + 1].r=mColors[i*4 + 2].r=mColors[i*4 + 3].r=valr;
			mColors[i*4 + 0].g=mColors[i*4 + 1].g=mColors[i*4 + 2].g=mColors[i*4 + 3].g=valg;
			mColors[i*4 + 0].b=mColors[i*4 + 1].b=mColors[i*4 + 2].b=mColors[i*4 + 3].b=valb;
			mColors[i*4 + 0].a=mColors[i*4 + 1].a=mColors[i*4 + 2].a=mColors[i*4 + 3].a=vala;
		}
	}
	
	// Enable texturing, bind the font's texture and set up blending
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, mFont->mTexId);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);    
	glEnableClientState(GL_COLOR_ARRAY);
	

	// Bind our vertex data
    glVertexPointer(2, GL_FLOAT, 0, mVertices);
	glTexCoordPointer(2, GL_FLOAT, 0, mUVs);
	glColorPointer(4, GL_UNSIGNED_BYTE, 0, mColors);

	// Draw the text
	glDrawElements(GL_TRIANGLES, 6 * mNumberOfQuads, GL_UNSIGNED_SHORT, mIndices);
	
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
    
}

void CGLString::BuildString(int msg_type)
{
	mVertices = new GLfloat[8 * mNumberOfQuads];
	mUVs = new GLfloat[8 * mNumberOfQuads];
	
	// Using bytes for indices limits us to 256 / 4 = 64 characters
	// If more are needed, we could use unsigned shorts instead
	mIndices = new GLshort[6 * mNumberOfQuads];
	
	mColors = new colorData[8 * mNumberOfQuads];
	
	float baseX = 0.0f;
	float baseY = 0.0f;
	float x = 0.0f;
	float y;
	unsigned char valr,valg,valb,vala;
	vala=0xFF;
	
	if (msg_type==0) {
		valr=0xF0;
		valg=0xFF;
		valb=0xF0;
	} else if (msg_type==1) {
		valr=0xFF;
		valg=0xFF;
		valb=0xF;
	} else if (msg_type==2) {
		valr=0xFF;
		valg=0xFF;
		valb=0xFF;
	} 
	else if (msg_type&128) {
		valr=valg=valb=0xFF;
		vala=(msg_type&127)*2;
	}    
	
	// Set up vertex data...
	int length = strlen(mText);
	int vertIndex = 0;
	for (int i = 0; i < length; ++i)
	{
		if (mText[i] == ' ')
		{
			// Simple hack to generate spaces, could be handled better though...
			baseX += mFont->mCharacterData['i'].screenWidth/scaleFactor;
		}
		else
		{
			// Build a quad (two triangles) for the character
			CCharacterData &data = mFont->mCharacterData[mText[i]];
			
            x = baseX;
            y = baseY;
            
			x += data.xOffset/scaleFactor;
			y -= data.yOffset/scaleFactor;
			
			mVertices[vertIndex + 0] = x;
			mVertices[vertIndex + 1] = y - data.byteHeight/scaleFactor;
			mVertices[vertIndex + 2] = x;
			mVertices[vertIndex + 3] = y;
			
			x += data.byteWidth/scaleFactor;
			
			mVertices[vertIndex + 4] = x;
			mVertices[vertIndex + 5] = y - data.byteHeight/scaleFactor;
			mVertices[vertIndex + 6] = x;
			mVertices[vertIndex + 7] = y;
			
			memcpy(&mUVs[vertIndex], data.texCoords, sizeof(float) * 8);
			
			vertIndex += 8;
            
            baseX += data.screenWidth/scaleFactor;
		}
	}
	
	// And now set up an array of index data
	int indIndex = 0;
	for (int i = 0; i < mNumberOfQuads; ++i)
	{
		if (msg_type==3+0) {
			switch (i%10) {
				//note
				case 0:
				case 1:
				case 2:valr=0xFF;valg=0xFF;valb=0xFF;break;
				//instr
				case 3:
				case 4:valr=0x80;valg=0xE0;valb=0xFF;break;
				//vol
				case 5:
				case 6:valr=0x80;valg=0xFF;valb=0x80;break;
				//eff
				case 7:valr=0xFF;valg=0x80;valb=0xE0;break;
				//param
				case 8:
				case 9:valr=0xFF;valg=0xE0;valb=0x80;break;
			}
		} else if (msg_type==10+0) {
			switch (i%10) {
					//note
				case 0:
				case 1:
				case 2:valr=0xFF/3;valg=0xFF/3;valb=0xFF/3;break;
					//instr
				case 3:
				case 4:valr=0x80/3;valg=0xE0/3;valb=0xFF/3;break;
					//vol
				case 5:
				case 6:valr=0x80/3;valg=0xFF/3;valb=0x80/3;break;
					//eff
				case 7:valr=0xFF/3;valg=0x80/3;valb=0xE0/3;break;
					//param
				case 8:
				case 9:valr=0xFF/3;valg=0xE0/3;valb=0x80/3;break;
			}
		} else if (msg_type==3+1) {
			switch (i%5) {
					//note
				case 0:
				case 1:
				case 2:valr=0xFF;valg=0xFF;valb=0xFF;break;
					//instr
				case 3:
				case 4:valr=0x80;valg=0xE0;valb=0xFF;break;
			}
		} else if (msg_type==10+1) {
			switch (i%5) {
					//note
				case 0:
				case 1:
				case 2:valr=0xFF/3;valg=0xFF/3;valb=0xFF/3;break;
					//instr
				case 3:
				case 4:valr=0x80/3;valg=0xE0/3;valb=0xFF/3;break;
			}
		} else if (msg_type==3+2) {
			switch (i%3) {
					//note
				case 0:
				case 1:
				case 2:valr=0xFF;valg=0xFF;valb=0xFF;break;
			}
		} else if (msg_type==10+2) {
			switch (i%3) {
					//note
				case 0:
				case 1:
				case 2:valr=0xFF/3;valg=0xFF/3;valb=0xFF/3;break;
			}
		}
		mColors[i*4 + 0].r=mColors[i*4 + 1].r=mColors[i*4 + 2].r=mColors[i*4 + 3].r=valr;
		mColors[i*4 + 0].g=mColors[i*4 + 1].g=mColors[i*4 + 2].g=mColors[i*4 + 3].g=valg;
		mColors[i*4 + 0].b=mColors[i*4 + 1].b=mColors[i*4 + 2].b=mColors[i*4 + 3].b=valb;
		mColors[i*4 + 0].a=mColors[i*4 + 1].a=mColors[i*4 + 2].a=mColors[i*4 + 3].a=vala;
		
		
		mIndices[indIndex + 0] = (GLshort)(4 * i + 0);
		mIndices[indIndex + 1] = (GLshort)(4 * i + 1);
		mIndices[indIndex + 2] = (GLshort)(4 * i + 2);
		
		mIndices[indIndex + 3] = (GLshort)(4 * i + 2);
		mIndices[indIndex + 4] = (GLshort)(4 * i + 1);
		mIndices[indIndex + 5] = (GLshort)(4 * i + 3);
		
		indIndex += 6;
	}
}