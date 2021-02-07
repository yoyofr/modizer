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


namespace RenderUtils
{
void drawbar(float x,float y,float z,float sx,float sy,float sz,float crt,float cgt,float cbt);
void drawbar2(float x,float y,float z,float sx,float sy,float sz,float crt,float cgt,float cbt);
void drawbar3(float x,float y,float z,float sx,float sy,float sz,float crt,float cgt,float cbt);

void SetUpOrtho(float rotation,uint width,uint height);
	
void DrawChanLayout(uint ww,uint hh,int display_note_mode,int chanNb,int pixOfs);
void DrawChanLayoutAfter(uint _ww,uint _hh,int display_note_mode,int *volumeData,int chanNb,int pixOfs);

void ReduceToUnit(GLfloat vector[3]);
void calcNormal(GLfloat v[3][3], GLfloat out[3]);
	
void DrawOscillo(short int *snd_data,int numval,uint ww,uint hh,uint bg,uint type_oscillo,uint pos);
void DrawSpectrum(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,uint bg,uint peaks,uint _pos,int nb_spectrum_bands);

void DrawSpectrum3D(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int nb_spectrum_bands);
void DrawSpectrumLandscape3D(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int nb_spectrum_bands);
void DrawSpectrum3DBar(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int nb_spectrum_bands,int mirror);
void DrawSpectrum3DBarFlat(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,int mode,int nb_spectrum_bands);
void DrawSpectrum3DSphere(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int nb_spectrum_bands);
void DrawSpectrum3DMorph(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int nb_spectrum_bands);

void DrawBeat(unsigned char *beatDataL,unsigned char *beatDataR,uint ww,uint hh,uint bg,uint _pos,int nb_spectrum_bands);

void DrawFXTouchGrid(uint _ww,uint _hh,int fade_level,int min_level,int active_idx,int cpt);
	
void DrawMidiFX(int *data,uint ww,uint hh,int horiz_vert,int note_display_range, int note_display_offset,int fx_len,int color_mode);
void DrawPiano3D(int *data,uint ww,uint hh,int fx_len,int automove,float posx,float posy,float posz,float rotx,float roty,int color_mode);
void DrawPiano3DWithNotesWall(int *data,uint ww,uint hh,int fx_len,int automove,float posx,float posy,float posz,float rotx,float roty,int color_mode, int fxquality);
}

#endif
