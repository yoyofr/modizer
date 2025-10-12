/*
 *  RenderUtils.h
 *  modizer
 *
 *  Created by Yohann Magnien on 23/08/10.
 *  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
 *
 */

#ifndef st_RenderUtils_h_
#define st_RenderUtils_h_

#import "ModizerConstants.h"
//#include "Mesh.h"

#include <GLES3/gl3.h>

#include "esUtil.h"

//tmp
//#include <OpenGLES/ES1/glext.h>
//#include <GLES/gl.h>
//#include <GLES/glext.h>

//tmp




#include "GlErrors.h"

#include "Queue.h"


//struct Mesh;

namespace
{

struct LineVertex
{
    LineVertex() {}
    LineVertex(signed short _x, signed short _y, uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a)
    : x(_x), y(_y), r(_r), g(_g), b(_b), a(_a)
    {}
    signed short x, y;
    uint8_t r, g, b, a;
};

struct SimpleLineVertexF {
    
    SimpleLineVertexF() {}
    SimpleLineVertexF(GLfloat _Ax, GLfloat _Ay, GLfloat _Bx,GLfloat _By)
    : Ax(_Ax), Ay(_Ay), Bx(_Bx), By(_By) {
    }
    
    SimpleLineVertexF(int _Ax, int _Ay,int _Bx, int _By, int width, int height)
    : Ax((float)_Ax*2.0/width-1.0), Ay((float)_Ay*2.0/height-1.0),Bx((float)_Bx*2.0/width-1.0), By((float)_By*2.0/height-1.0) {
    }
    
    GLfloat Ax,Ay;
    GLfloat Bx,By;
};

struct LineVertexF
{
    LineVertexF() {}
    LineVertexF(GLfloat _x, GLfloat _y, GLfloat _z,GLfloat _r, GLfloat _g, GLfloat _b, GLfloat _a)
    : x(_x), y(_y), z(_z), r(_r), g(_g), b(_b), a(_a) {
    }
    
    LineVertexF(GLfloat _x, GLfloat _y,GLfloat _r, GLfloat _g, GLfloat _b, GLfloat _a)
    : x(_x), y(_y), z(0), r(_r), g(_g), b(_b), a(_a) {
    }
    
    LineVertexF(int _x, int _y,int _r, int _g, int _b, int _a,int width,int height)
    : x((float)_x*2.0/width-1.0), y((float)_y*2.0/height-1.0), z(0), r((float)_r/255.0), g((float)_g/255.0), b((float)_b/255.0), a((float)_a/255.0) {
    }
    
    GLfloat x, y, z;
    GLfloat r, g, b, a;
};

struct vertexData {
    GLfloat x;             // OpenGL X Coordinate
    GLfloat y;             // OpenGL Y Coordinate
    GLfloat z;             // OpenGL Z Coordinate
    GLfloat s;             // Texture S Coordinate
    GLfloat t;             // Texture T Coordinate
    GLfloat r,g,b,a;
};

struct coordData {
    GLfloat u;             // OpenGL X Coordinate
    GLfloat v;             // OpenGL Y Coordinate
};

typedef struct {
    // Handle to a program object
    GLuint programObject;
    
    // Uniform locations
    GLint  mvpLoc;
    
    // MVP matrix
    ESMatrix  mvpMatrix;
    
} GLUserData;

}


namespace RenderUtils
{

GLuint LoadShaderFromFile ( GLenum type, const char *shaderFile );
GLuint LoadShader ( GLenum type, const GLchar *shaderSrc );

int RenderInit();
GLUserData* InitProgram(char *vsfile,char *fsfile);
void ShutdownProgram(GLUserData *userData);


void drawbar(float x,float y,float z,float sx,float sy,float sz,float crt,float cgt,float cbt);
void drawbar2(float x,float y,float z,float sx,float sy,float sz,float crt,float cgt,float cbt);
void drawbar3(float x,float y,float z,float sx,float sy,float sz,float crt,float cgt,float cbt);

int DrawBox(LineVertexF *ptsB,int index,float x,float y,float width,float height,float border_size,int crt,int cgt,int cbt,int ca,int subnote);

int DrawKeyW(LineVertexF *ptsB,int index,float x,float y,float width,float height,float border_size,int crt,int cgt,int cbt,int ca,int subnote,int note_idx,int channel);
int DrawKeyB(LineVertexF *ptsB,int index,float x,float y,float width,float height,float border_size,int crt,int cgt,int cbt,int ca,int subnote,int note_idx,int channel);

void SetUpOrtho(float rotation,uint width,uint height);

void DrawChanLayout(uint ww,uint hh,int display_note_mode,int chanNb,float pixOfs,float char_width,float char_height,float mScaleFactor);
void DrawChanLayoutAfter(uint _ww,uint _hh,int display_note_mode,int *volumeData,int chanNb,float pixOfs,float char_width,float char_height,float char_yOfs,int rowToHighlight,float mScaleFactor);

void ReduceToUnit(GLfloat vector[3]);
void calcNormal(GLfloat v[3][3], GLfloat out[3]);

// Clear the top & bottom part of the UI when opengl window is not fully opaque & in fullscreen
void ClearUI(uint width,uint height,uint top_size,uint bottom_size);
	
void DrawOscillo(short int *snd_data,int numval,uint ww,uint hh,uint bg,uint type_oscillo,uint pos,float mScaleFactor);
void DrawOscilloMultiple(signed char **snd_data,int snd_data_idx,int num_voices,uint ww,uint hh,uint color_mode,float mScaleFactor,bool isfullscreen,char *voices_label=NULL,bool draw_frame=true,bool flag_direct_stereo=false);
void DrawOscilloStereo(short int **snd_data,int snd_data_idx,uint ww,uint hh,uint color_mode,float mScaleFactor,bool isfullscreen,bool draw_frame);
void DrawSpectrum(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,uint bg,uint peaks,uint _pos,int nb_spectrum_bands,float mScaleFactor);

void DrawSpectrum3D(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int nb_spectrum_bands);
void DrawSpectrumLandscape3D(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int nb_spectrum_bands);
void DrawSpectrum3DBar(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int nb_spectrum_bands,int mirror);
void DrawSpectrum3DBarFlat(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,int mode,int nb_spectrum_bands);
void DrawSpectrum3DMorph(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int nb_spectrum_bands);

void DrawFXTouchGrid(uint _ww,uint _hh,float fade_level,float min_level,int active_idx,int cpt,float mScaleFactor);
	

void UpdateDataMidiFX(unsigned int *data,bool clearBuffer,bool paused);
void UpdateDataPiano(unsigned int *data,bool clearbuffer,bool paused);

int DrawBox(LineVertexF *ptsB,int index,float x,float y,float width,float height,float border_size,int crt,int cgt,int cbt,int ca,int subnote);

void DrawMidiFX(uint ww,uint hh,int horiz_vert,float note_display_range, float note_display_offset,int fx_len,int color_mode,float mScaleFactor);
void DrawPianoRollFX(uint ww,uint hh,int horiz_vert,float note_display_range, float note_display_offset,int fx_len,int color_mode,float mScaleFactor,char *voices_label);
void DrawPianoRollSynthesiaFX(uint ww,uint hh,int horiz_vert,float note_display_range, float note_display_offset,int fx_len,int color_mode,float mScaleFactor,char *voices_label);
void DrawPiano3D(uint ww,uint hh,int automove,float posx,float posy,float posz,float rotx,float roty,int color_mode);
void DrawPiano3DWithNotesWall(uint ww,uint hh,int automove,float posx,float posy,float posz,float rotx,float roty,int color_mode, int fxquality);

void DrawTexture(uint ww,uint hh,GLuint textureIdx,float alpha);
}

#endif
