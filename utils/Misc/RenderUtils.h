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
#include "GlErrors.h"

#include <OpenGLES/ES1/glext.h>
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

struct LineVertexF
{
    LineVertexF() {}
    LineVertexF(float _x, float _y, uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a)
    : x(_x), y(_y), r(_r), g(_g), b(_b), a(_a)
    {}
    float x, y;
    uint8_t r, g, b, a;
};

struct vertexData {
    GLfloat x;             // OpenGL X Coordinate
    GLfloat y;             // OpenGL Y Coordinate
    GLfloat z;             // OpenGL Z Coordinate
    GLfloat s;             // Texture S Coordinate
    GLfloat t;             // Texture T Coordinate
    GLfloat r,g,b,a;
};


}


namespace RenderUtils
{
void drawbar(float x,float y,float z,float sx,float sy,float sz,float crt,float cgt,float cbt);
void drawbar2(float x,float y,float z,float sx,float sy,float sz,float crt,float cgt,float cbt);
void drawbar3(float x,float y,float z,float sx,float sy,float sz,float crt,float cgt,float cbt);

void SetUpOrtho(float rotation,uint width,uint height);
	
void DrawChanLayout(uint ww,uint hh,int display_note_mode,int chanNb,float pixOfs,float char_width,float char_height,float mScaleFactor);
void DrawChanLayoutAfter(uint _ww,uint _hh,int display_note_mode,int *volumeData,int chanNb,float pixOfs,float char_width,float char_height,float char_yOfs,int rowToHighlight,float mScaleFactor);

void ReduceToUnit(GLfloat vector[3]);
void calcNormal(GLfloat v[3][3], GLfloat out[3]);
	
void DrawOscillo(short int *snd_data,int numval,uint ww,uint hh,uint bg,uint type_oscillo,uint pos,float mScaleFactor);
void DrawOscilloMultiple(signed char **snd_data,int snd_data_idx,int num_voices,uint ww,uint hh,uint color_mode,float mScaleFactor,bool isfullscreen,char *voices_label=NULL,bool draw_frame=true);
void DrawOscilloStereo(short int **snd_data,int snd_data_idx,uint ww,uint hh,uint color_mode,float mScaleFactor,bool isfullscreen,bool draw_frame);
void DrawSpectrum(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,uint bg,uint peaks,uint _pos,int nb_spectrum_bands,float mScaleFactor);

void DrawSpectrum3D(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int nb_spectrum_bands);
void DrawSpectrumLandscape3D(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int nb_spectrum_bands);
void DrawSpectrum3DBar(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int nb_spectrum_bands,int mirror);
void DrawSpectrum3DBarFlat(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,int mode,int nb_spectrum_bands);
void DrawSpectrum3DSphere(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int nb_spectrum_bands);
void DrawSpectrum3DMorph(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int nb_spectrum_bands);

void DrawBeat(unsigned char *beatDataL,unsigned char *beatDataR,uint ww,uint hh,uint bg,uint _pos,int nb_spectrum_bands);

void DrawFXTouchGrid(uint _ww,uint _hh,int fade_level,int min_level,int active_idx,int cpt,float mScaleFactor);
	

void UpdateDataMidiFX(unsigned int *data,bool clearBuffer,bool paused);
void UpdateDataPiano(unsigned int *data,bool clearbuffer,bool paused);

int DrawBox(LineVertexF *ptsB,int index,float x,float y,float width,float height,float border_size,int crt,int cgt,int cbt,int ca,int subnote);

void DrawMidiFX(uint ww,uint hh,int horiz_vert,float note_display_range, float note_display_offset,int fx_len,int color_mode,float mScaleFactor);
void DrawPianoRollFX(uint ww,uint hh,int horiz_vert,float note_display_range, float note_display_offset,int fx_len,int color_mode,float mScaleFactor,char *voices_label);
void DrawPiano3D(uint ww,uint hh,int automove,float posx,float posy,float posz,float rotx,float roty,int color_mode);
void DrawPiano3DWithNotesWall(uint ww,uint hh,int automove,float posx,float posy,float posz,float rotx,float roty,int color_mode, int fxquality);
}

#endif
