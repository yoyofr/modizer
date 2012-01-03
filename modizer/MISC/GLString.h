/*
 *  GLString.h
 *  FontTest
 *
 *  Created by George Sealy on 15/04/09.
 *
 */

// Forward declaration
class CFont;

struct colorData {
	unsigned char r,g,b,a;
};

class CGLString
{
public:
	CGLString(const char *text, const CFont *font);
	~CGLString();
	
	void Render(int msg_type);
	char *mText;
private:
	
	const CFont *mFont;
	GLfloat *mVertices;
	GLfloat *mUVs;
	GLshort *mIndices;
	colorData *mColors;
	int mNumberOfQuads;
	
	void BuildString(int msg_type);
};