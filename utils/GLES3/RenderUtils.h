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

#include <glm/glm.hpp>
#include <glm/ext.hpp>


//#include "esUtil.h"

//tmp
//#include <OpenGLES/ES1/glext.h>
//#include <GLES/gl.h>
//#include <GLES/glext.h>

//tmp




#include "GlErrors.h"
#include "Queue.h"

#include "ModizerTypes.h"


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
    : Ax((float)_Ax*2.0/(float)width-1.0), Ay((float)_Ay*2.0/(float)height-1.0),Bx((float)_Bx*2.0/(float)width-1.0), By((float)_By*2.0/(float)height-1.0) {
    }
    
    GLfloat Ax,Ay;
    GLfloat Bx,By;
};


struct ColorDataF {
    
    ColorDataF() {}
    ColorDataF(GLfloat _Ar,GLfloat _Ag,GLfloat _Ab,GLfloat _Aa)
    : r(_Ar), g(_Ag), b(_Ab), a(_Aa) {
    }
    
    GLfloat r,g,b,a;
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
    : x((float)_x*2.0/(float)width-1.0), y((float)_y*2.0/(float)height-1.0), z(0), r((float)_r/255.0), g((float)_g/255.0), b((float)_b/255.0), a((float)_a/255.0) {
    }
    
    GLfloat x, y, z;
    GLfloat r, g, b, a;
};

struct VertexData {
    GLfloat x;             // OpenGL X Coordinate
    GLfloat y;             // OpenGL Y Coordinate
    GLfloat z;             // OpenGL Z Coordinate
    GLfloat s;             // Texture S Coordinate
    GLfloat t;             // Texture T Coordinate
    GLfloat r,g,b,a;
};

struct VertexCData {
    VertexCData() {}
    VertexCData(GLfloat _x,GLfloat _y,GLfloat _z,GLfloat _r,GLfloat _g,GLfloat _b,GLfloat _a) :
    x(_x),y(_y),z(_z),r(_r),g(_g),b(_b) {
    }
    
    GLfloat x;             // OpenGL X Coordinate
    GLfloat y;             // OpenGL Y Coordinate
    GLfloat z;             // OpenGL Z Coordinate
    GLfloat r,g,b,a;
};


struct VertexNData {
    VertexNData() {}
    VertexNData(GLfloat _x,GLfloat _y,GLfloat _z,GLfloat _Nx,GLfloat _Ny,GLfloat _Nz,GLfloat _r,GLfloat _g,GLfloat _b,GLfloat _a) :
    x(_x),y(_y),z(_z),Nx(_Nx),Ny(_Ny),Nz(_Nz),r(_r),g(_g),b(_b) {
        
    }
    
    
    GLfloat x;             // OpenGL X Coordinate
    GLfloat y;             // OpenGL Y Coordinate
    GLfloat z;             // OpenGL Z Coordinate
    GLfloat Nx;             // OpenGL Normal X
    GLfloat Ny;             // OpenGL Normal Y
    GLfloat Nz;             // OpenGL Normal Z
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
    GLint  modelLoc;
    GLint  viewLoc;
    GLint  projectionLoc;
    
    // MVP matrix
    glm::mat4 Model;
    glm::mat4 View;
    glm::mat4 Projection;
    glm::mat4 mvpMatrix;
    
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
void drawbarF(float x,float y,float z,float sx,float sy,float sz,float crt,float cgt,float cbt);
void drawbar2(float x,float y,float z,float sx,float sy,float sz,float crt,float cgt,float cbt);
void drawbar3(float x,float y,float z,float sx,float sy,float sz,float crt,float cgt,float cbt);

int DrawBox(LineVertexF *ptsB,int index,float x,float y,float width,float height,float border_size,int crt,int cgt,int cbt,int ca,int subnote,int ww,int hh);

int DrawKeyW(LineVertexF *ptsB,int index,float x,float y,float width,float height,float border_size,int crt,int cgt,int cbt,int ca,int subnote,int note_idx,int channel,int ww,int hh);
int DrawKeyB(LineVertexF *ptsB,int index,float x,float y,float width,float height,float border_size,int crt,int cgt,int cbt,int ca,int subnote,int note_idx,int channel,int ww,int hh);

void DrawChanLayout(float ox,float oy,float ww,float hh,int display_note_mode,int chanNb,float pixOfs,float char_width,float char_height,float mScaleFactor);
void DrawChanLayoutAfter(float ox,float oy,float ww,float hh,int display_note_mode,int *volumeData,int chanNb,float pixOfs,float char_width,float char_height,float char_yOfs,int rowToHighlight,float mScaleFactor);

void ReduceToUnit(GLfloat vector[3]);
void calcNormal(GLfloat v[3][3], GLfloat out[3]);

void DrawOscilloMultiple(float ox,float oy,float ww,float hh,signed char **snd_data,int snd_data_idx,int num_voices,uint color_mode,float mScaleFactor,bool isfullscreen,bool bloom,char *voices_label=NULL,bool draw_frame=true,bool flag_direct_stereo=false);

void DrawSpectrum3D(float ox,float oy,uint ww,uint hh,short int *spectrumDataL,short int *spectrumDataR,float angle,int mode,int nb_spectrum_bands,int bloom,float mScaleFactor);
void DrawSpectrumLandscape3D(float ox,float oy,uint ww,uint hh,short int *spectrumDataL,short int *spectrumDataR,float angle,int mode,int nb_spectrum_bands,int bloom,float mScaleFactor);
void DrawSpectrum3DMorph(float ox,float oy,uint ww,uint hh,short int *spectrumDataL,short int *spectrumDataR,float angle,int mode,int nb_spectrum_bands,int bloom,float mScaleFactor);

void DrawSpectrum3DBar(float ox,float oy,uint ww,uint hh,short int *spectrumDataL,short int *spectrumDataR,float angle,int mode,int nb_spectrum_bands,int mirror,float mScaleFactor,int bloom,float rotx,float roty,float posx,float posy,float posz);
void DrawSpectrum2D(float ox,float oy,uint ww,uint hh,short int *spectrumDataL,short int *spectrumDataR,int mode,int nb_spectrum_bands,float mScaleFactor,bool bloom);

void UpdateDataMidiFX(unsigned int *data,bool clearBuffer,bool paused);
void UpdateDataPiano(unsigned int *data,bool clearbuffer,bool paused);

int DrawBox(LineVertexF *ptsB,int index,float x,float y,float width,float height,float border_size,int crt,int cgt,int cbt,int ca,int subnote,int ww,int hh);

void DrawMidiFX(float ox,float oy,float ww,float hh,int horiz_vert,float note_display_range, float note_display_offset,int fx_len,int color_mode,float mScaleFactor,float *scaleInfo);
void DrawPianoRollFX(float ox,float oy,float ww,float hh,int horiz_vert,float note_display_range, float note_display_offset,int fx_len,int color_mode,float mScaleFactor,char *voices_label,float *scaleInfo);
void DrawPianoRollSynthesiaFX(float ox,float oy,float ww,float hh,int horiz_vert,float note_display_range, float note_display_offset,int fx_len,int color_mode,float mScaleFactor,char *voices_label,float *scaleInfo);
void DrawPiano3D(float ox,float oy,float ww,float hh,int automove,float posx,float posy,float posz,float rotx,float roty,int color_mode);
void DrawPiano3DWithNotesWall(float ox,float oy,float ww,float hh,int automove,float posx,float posy,float posz,float rotx,float roty,int color_mode, int fxquality);

void DrawTexture(uint ww,uint hh,GLuint textureIdx,float alpha,bool reversed=false,float aspect_ratio=0);
void DrawTextureBlur(uint ww,uint hh,GLuint textureIdx,int hori,float min_brightness,float blurDiv);
void DrawTextureBlend(uint ww,uint hh,GLuint textOrigIdx,GLuint textBlurIdx);
void DrawTextureBasic(uint ww,uint hh,GLuint textureIdx,float alpha,bool reversed);

bool initRenderToTexture(int width,int height);
void shutdownRenderToTexture();
void startRenderToTexture(int width,int height);
void endRenderToTexture(int width,int height,int bloomIntensity);
void endRenderToTextureBasic(int width,int height,float alpha);

int build3DQuad(VertexCData *vert,float x1,float y1,float z1,float cr1,float cg1,float cb1,float ca1,
                                 float x2,float y2,float z2,float cr2,float cg2,float cb2,float ca2,
                                 float x3,float y3,float z3,float cr3,float cg3,float cb3,float ca3,
                                 float x4,float y4,float z4,float cr4,float cg4,float cb4,float ca4);

int buildQuad(LineVertexF *pts,
              int x1,int y1,
                  int x2,int y2,
                  int x3,int y3,
                  int x4,int y4,
                  int r1,int g1,int b1,int a1,
                  int r2,int g2,int b2,int a2,
                  int r3,int g3,int b3,int a3,
                  int r4,int g4,int b4,int a4,int ww,int hh);

void releaseProgram(int prgId);

void FillArea(float ox,float oy,float ww,float hh,float winWidth,float winHeight,float mScaleFactor,int a, int r=0,int g=0,int b=0);

void FillAreaPattern(float ox,float oy,float ww,float hh,float winWidth,float winHeight,float mScaleFactor,int mode,int a, int r=0,int g=0,int b=0);

}


#endif
