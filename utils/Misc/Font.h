/*
 *  Font.h
 *  FontTest
 *
 *  Created by George Sealy on 9/04/09.
 *
 */


#include <string>
#include <vector>

// A store for each character's data
class CCharacterData
{
public:
		
	CCharacterData();
	~CCharacterData();
		
	// Given an open file, load a character's worth of data
	void Load(FILE *fontFile);
    
    void Unalloc();
	
	// Refer to the README file that comes with Paul Nettle's Font generator
	// for details of these values.
	int byteWidth;
	int byteHeight;
	int xOffset;
	int yOffset;
	int screenWidth;
	int screenHeight;
	unsigned char *buffer;

	// Mark whether we want this character or not
	bool isWanted;
	float texCoords[8];
};

class CFont 
{
public:
	CFont(const std::string &fontFilename);
	~CFont();
	
	GLuint mTexId;
	CCharacterData *mCharacterData;
    int maxCharWidth,maxCharHeight,xCharOffset,yCharOffset;
private:
	
	// Determine what size of texture we need for a given width of texture
	int GetHeightForTexture(int textureWidth, int charWidth, int charHeight);
};
