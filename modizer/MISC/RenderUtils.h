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


void SetUpOrtho(float rotation,uint width,uint height);
	
void DrawChanLayout(uint ww,uint hh,int display_note_mode,int chanNb,int pixOfs);
void DrawChanLayoutAfter(uint ww,uint hh,int display_note_mode);

void ReduceToUnit(GLfloat vector[3]);
void calcNormal(GLfloat v[3][3], GLfloat out[3]);
	
void DrawOscillo(short int *snd_data,int numval,uint ww,uint hh,uint bg,uint type_oscillo,uint pos,int deviceType);
void DrawSpectrum(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,uint bg,uint peaks,uint _pos,int deviceType,int nb_spectrum_bands);

void DrawSpectrum3D(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int deviceType,int nb_spectrum_bands);
void DrawSpectrum3DSphere(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int deviceType,int nb_spectrum_bands);
void DrawSpectrum3DMorph(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int deviceType,int nb_spectrum_bands);

void DrawBeat(unsigned char *beatDataL,unsigned char *beatDataR,uint ww,uint hh,uint bg,uint _pos,int deviceType,int nb_spectrum_bands);

void DrawFXTouchGrid(uint _ww,uint _hh,int fade_level);
	
void DrawMidiFX(int *data,uint ww,uint hh,int deviceType,int horiz_vert,int note_display_range, int note_display_offset,int fx_len);
void DrawPiano3D(int *data,uint ww,uint hh,int deviceType,int note_display_range, int note_display_offset,int fx_len);
}

#endif
