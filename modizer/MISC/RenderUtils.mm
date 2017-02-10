/*
 *  RenderUtils.mm
 *  modizer
 *
 *  Created by Yohann Magnien on 23/08/10.
 *  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
 *
 */


extern BOOL is_retina;

#include "RenderUtils.h"
#include "TextureUtils.h"

#define MAX_VISIBLE_CHAN 64

#define SPECTRUM_DEPTH 32
#define SPECTRUM_ZSIZE 12
#define SPECTRUM_Y 12.0f
#define SPECTR_XSIZE_FACTOR 0.95f
#define SPECTRUM_DECREASE_FACTOR 0.96f
#define SPECTR_XSIZE 38.0f
static float oldSpectrumDataL[SPECTRUM_DEPTH*4][SPECTRUM_BANDS];
static float oldSpectrumDataR[SPECTRUM_DEPTH*4][SPECTRUM_BANDS];
static GLfloat vertices[4][3];  /* Holds Float Info For 4 Sets Of Vertices */
static GLfloat normals[4][3];  /* Holds Float Info For 4 Sets Of Vertices */
static GLfloat vertColor[4][4];  /* Holds Float Info For 4 Sets Of Vertices */


#define MAX_BARS 2048
typedef struct {
    unsigned char startidx,note,instr,size;
} t_data_bar2draw;
static t_data_bar2draw data_bar2draw[MAX_BARS];


static int piano_note_type[128];
static float piano_note_posx[128];
static float piano_note_posy[128];
static float piano_note_posz[128];


static GLfloat verticesBAR[MAX_BARS*24][3];  /* Holds Float Info For 4 Sets Of Vertices */
static GLfloat vertColorBAR[MAX_BARS*24][4];  /* Holds Float Info For 4 Sets Of Vertices */
static GLushort vertBARindices[MAX_BARS*6*6]; //6 faces, 6 indices/face
//1,2,3,4,4,5,5,6,7,8,8,9,9,10,11,12,

float ambientLight[3][4] = {
    {0.1f, 0.1f, 0.2f, 1.0f},
    {0.2f, 0.1f, 0.1f, 1.0f},
    {0.1f, 0.1f, 0.1f, 1.0f }
};	// �wiat�o otoczenia
float diffuseLight[3][4] = {
    {0.5f, 0.5f, 0.9f, 1.0f },
    {0.9f, 0.5f, 0.5f, 1.0f },
    {1.0f, 1.0f, 1.0f, 1.0f }
};	// �wiat�o rozproszone
float specularLight[3][4] = {
    {1.0f, 1.0f, 1.0f, 1.0f },
    {1.0f, 1.0f, 1.0f, 1.0f },
    {1.0f, 1.0f, 1.0f, 1.0f }
};	// �wiat�o odbicia
float position[] = { 0, 0, 8, 1 };


namespace
{
    
    struct LineVertex
    {
        LineVertex() {}
        LineVertex(uint16_t _x, uint16_t _y, uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a)
        : x(_x), y(_y), r(_r), g(_g), b(_b), a(_a)
        {}
        uint16_t x, y;
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


void RenderUtils::SetUpOrtho(float rotation,uint width,uint height)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glRotatef(rotation, 0, 0, 1);
    glOrthof(0, width, 0, height, 0, 200);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
}

void RenderUtils::DrawOscillo(short int *snd_data,int numval,uint ww,uint hh,uint bg,uint type_oscillo,uint pos) {
    LineVertex *pts,*ptsB;
    int mulfactor;
    int dval,valL,valR,ovalL,ovalR,ospl,ospr,spl,spr,colR1,colL1,colR2,colL2,ypos;
    int count;
    
    if (numval>=128) {
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        
        
        
        pts=(LineVertex*)malloc(sizeof(LineVertex)*128*6);
        ptsB=(LineVertex*)malloc(sizeof(LineVertex)*4);
        count=0;
        
        
        
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        
        if (type_oscillo==1) {
            int wd=(ww/2-10)/64;
            if (pos) {
                ypos=hh/4;
                mulfactor=hh*1/4;
            } else {
                ypos=hh/2;
                mulfactor=hh*1/4;
            }
            
            if (bg) {
                if (pos) ypos=40;
                else ypos=hh/2;
                ptsB[0] = LineVertex((ww/2+(64*wd))/2, ypos-32,		0,0,16,192);
                ptsB[1] = LineVertex((ww/2-(64*wd))/2, ypos-32,		0,0,16,192);
                ptsB[2] = LineVertex((ww/2+(64*wd))/2, ypos+32,		0,0,16,192);
                ptsB[3] = LineVertex((ww/2-(64*wd))/2, ypos+32,		0,0,16,192);
                glVertexPointer(2, GL_SHORT, sizeof(LineVertex), &ptsB[0].x);
                glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertex), &ptsB[0].r);
                /* Render The Quad */
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                
                ptsB[0] = LineVertex(ww/2+(ww/2+(64*wd))/2, ypos-32,		0,0,16,192);
                ptsB[1] = LineVertex(ww/2+(ww/2-(64*wd))/2, ypos-32,		0,0,16,192);
                ptsB[2] = LineVertex(ww/2+(ww/2+(64*wd))/2, ypos+32,		0,0,16,192);
                ptsB[3] = LineVertex(ww/2+(ww/2-(64*wd))/2, ypos+32,		0,0,16,192);
                glVertexPointer(2, GL_SHORT, sizeof(LineVertex), &ptsB[0].x);
                glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertex), &ptsB[0].r);
                /* Render The Quad */
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
            valL=snd_data[0]*mulfactor>>6;
            valR=snd_data[1]*mulfactor>>6;
            spl=(valL)>>(15-5); if(spl>mulfactor) spl=mulfactor; if (spl<-mulfactor) spl=-mulfactor;
            spr=(valR)>>(15-5); if(spr>mulfactor) spr=mulfactor; if (spr<-mulfactor) spr=-mulfactor;
            colR1=150;
            colL1=150;
            colR2=75;
            colL2=75;
            
            for (int i=1; i<64; i++) {
                ovalL=valL;ovalR=valR;
                valL=snd_data[(i*numval>>6)*2]*mulfactor>>6;
                valR=snd_data[(i*numval>>6)*2+1]*mulfactor>>6;
                ospl=spl;ospr=spr;
                spl=(valL)>>(15-5); if(spl>mulfactor) spl=mulfactor; if (spl<-mulfactor) spl=-mulfactor;
                spr=(valR)>>(15-5); if(spr>mulfactor) spr=mulfactor; if (spr<-mulfactor) spr=-mulfactor;
                pts[count++] = LineVertex((ww/2-(64*wd))/2+i*wd-wd, ypos+ospl,colL2,colL1,colL2,205);
                colL1=(((valL-ovalL)*1024)>>15)+180;
                colL2=(((valL-ovalL)*128)>>15)+32;
                if (colL1<32) colL1=32;if (colL1>255) colL1=255;
                if (colL2<32) colL2=32;if (colL2>255) colL2=255;
                pts[count++] = LineVertex((ww/2-(64*wd))/2+i*wd, ypos+spl,colL2,colL1,colL2,205);
                
                pts[count++] = LineVertex(ww/2+(ww/2-(64*wd))/2+i*wd-wd, ypos+ospr,colR2,colR1,colR2,205);
                colR1=(((valR-ovalR)*1024)>>15)+180;
                colR2=(((valR-ovalR)*128)>>15)+32;
                if (colR1<32) colR1=32;if (colR1>255) colR1=255;
                if (colR2<32) colR2=32;if (colR2>255) colR2=255;
                pts[count++] = LineVertex(ww/2+(ww/2-(64*wd))/2+i*wd, ypos+spr,colR2,colR1,colR2,205);
            }
            glLineWidth(2.0f);
            glVertexPointer(2, GL_SHORT, sizeof(LineVertex), &pts[0].x);
            glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertex), &pts[0].r);
            glDrawArrays(GL_LINES, 0, count);
            
        }
        if (type_oscillo==2) {
            int wd=(ww-10)/128;
            
            valL=snd_data[0]*mulfactor>>6;
            valR=snd_data[1]*mulfactor>>6;
            spl=(valL)>>(15-5); if(spl>mulfactor) spl=mulfactor; if (spl<-mulfactor) spl=-mulfactor;
            spr=(valR)>>(15-5); if(spr>mulfactor) spr=mulfactor; if (spr<-mulfactor) spr=-mulfactor;
            colR1=150;
            colL1=150;
            colR2=75;
            colL2=75;
            
            if (pos) {
                ypos=hh/4;
                mulfactor=hh*1/4;
            } else {
                ypos=hh/2;
                mulfactor=hh*1/4;
            }
            
            if (bg) {
                if (pos) ypos=40;
                else ypos=hh/2;
                ptsB[0] = LineVertex((ww+128*wd)/2, ypos-32,		0,0,16,192);
                ptsB[1] = LineVertex((ww-128*wd)/2, ypos-32,		0,0,16,192);
                ptsB[2] = LineVertex((ww+128*wd)/2, ypos+32,		0,0,16,192);
                ptsB[3] = LineVertex((ww-128*wd)/2, ypos+32,		0,0,16,192);
                glVertexPointer(2, GL_SHORT, sizeof(LineVertex), &ptsB[0].x);
                glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertex), &ptsB[0].r);
                /* Render The Quad */
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
            for (int i=1; i<128; i++) {
                valL=snd_data[((i*numval)>>7)*2]*mulfactor>>6;
                valR=snd_data[((i*numval)>>7)*2+1]*mulfactor>>6;
                spl=(valL)>>(15-5); if(spl>mulfactor) spl=mulfactor; if (spl<-mulfactor) spl=-mulfactor;
                spr=(valR)>>(15-5); if(spr>mulfactor) spr=mulfactor; if (spr<-mulfactor) spr=-mulfactor;
                dval=valL-valR; if (dval<0) dval=-dval;
                colL1=((dval*512)>>15)+164;
                colL2=((dval*256)>>15)+48;
                if (colL1<48) colL1=48;if (colL1>255) colL1=255;
                if (colL2<48) colL2=48;if (colL2>255) colL2=255;
                
                pts[count++] = LineVertex((ww-128*wd)/2+i*wd, ypos+spl,colL2,colL1,colL2,192);
                pts[count++] = LineVertex((ww-128*wd)/2+i*wd, ypos+spr,colL2,colL1,colL2,192);
                
            }
            glLineWidth(1.0f);
            glVertexPointer(2, GL_SHORT, sizeof(LineVertex), &pts[0].x);
            glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertex), &pts[0].r);
            glDrawArrays(GL_LINES, 0, count);
            
            
            count=0;
            valL=snd_data[0]*mulfactor>>6;
            valR=snd_data[1]*mulfactor>>6;
            spl=(valL)>>(15-5); if(spl>mulfactor) spl=mulfactor; if (spl<-mulfactor) spl=-mulfactor;
            spr=(valR)>>(15-5); if(spr>mulfactor) spr=mulfactor; if (spr<-mulfactor) spr=-mulfactor;
            colR1=150;
            colL1=150;
            colR2=75;
            colL2=75;
            for (int i=1; i<128; i++) {
                ovalL=valL;ovalR=valR;
                valL=snd_data[((i*numval)>>7)*2]*mulfactor>>6;
                valR=snd_data[((i*numval)>>7)*2+1]*mulfactor>>6;
                ospl=spl;ospr=spr;
                spl=(valL)>>(15-5); if(spl>mulfactor) spl=mulfactor; if (spl<-mulfactor) spl=-mulfactor;
                spr=(valR)>>(15-5); if(spr>mulfactor) spr=mulfactor; if (spr<-mulfactor) spr=-mulfactor;
                pts[count++] = LineVertex((ww-128*wd)/2+i*wd-wd, ypos+ospl,colL2,colL1,colL2,205);
                colL1=(((ovalL-valL)*1024)>>15)+164;
                colL2=(((ovalL-valL)*256)>>15)+64;
                if (colL1<48) colL1=48;if (colL1>255) colL1=255;
                if (colL2<48) colL2=48;if (colL2>255) colL2=255;
                pts[count++] = LineVertex((ww-128*wd)/2+i*wd, ypos+spl,colL2,colL1,colL2,205);
                
                pts[count++] = LineVertex((ww-128*wd)/2+i*wd-wd, ypos+ospr,colR2,colR1,colR2,205);
                colR1=(((ovalR-valR)*1024)>>15)+164;
                colR2=(((ovalR-valR)*256)>>15)+64;
                if (colR1<48) colR1=48;if (colR1>255) colR1=255;
                if (colR2<48) colR2=48;if (colR2>255) colR2=255;
                pts[count++] = LineVertex((ww-128*wd)/2+i*wd, ypos+spr,colR2,colR1,colR2,205);
            }
            glLineWidth(2.0f);
            glVertexPointer(2, GL_SHORT, sizeof(LineVertex), &pts[0].x);
            glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertex), &pts[0].r);
            glDrawArrays(GL_LINES, 0, count);
            
        }
        
        
        
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
        glDisable(GL_BLEND);
        free(pts);
        free(ptsB);
    }
    
}

static int DrawSpectrum_first_call=1;
static int spectrumPeakValueL[SPECTRUM_BANDS];
static int spectrumPeakValueR[SPECTRUM_BANDS];
static int spectrumPeakValueL_index[SPECTRUM_BANDS];
static int spectrumPeakValueR_index[SPECTRUM_BANDS];


void RenderUtils::DrawSpectrum(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,uint bg,uint peaks,uint _pos,int nb_spectrum_bands) {
    LineVertex *pts,*ptsB,*ptsC;
    float x,y;
    int spl,spr,mulfactor,cr,cg,cb;
    int pr,pl;
    int count,yposL,yposR,maxsp,xshift;
    float band_width;
    
    band_width=(ww-32)*1.0f/(float)(nb_spectrum_bands);
    pts=(LineVertex*)malloc(sizeof(LineVertex)*nb_spectrum_bands*2*2*2);
    ptsB=(LineVertex*)malloc(sizeof(LineVertex)*4);
    if (peaks) ptsC=(LineVertex*)malloc(sizeof(LineVertex)*nb_spectrum_bands*2*2*2);
    
    if (DrawSpectrum_first_call) {
        DrawSpectrum_first_call=0;
        for (int i=0;i<nb_spectrum_bands;i++) {
            spectrumPeakValueL[i]=0;
            spectrumPeakValueL_index[i]=0;
            spectrumPeakValueR[i]=0;
            spectrumPeakValueR_index[i]=0;
        }
    }
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    if (_pos) {
        mulfactor=hh/3;
        maxsp=hh/3;
        yposL=hh-hh/2-hh/4;
        yposR=hh-hh/2+hh/4;
        
    } else {
        //ypos=hh/2-hh/2/2;
        mulfactor=hh/2;
        maxsp=hh/2;
        yposL=hh/2-hh/4;
        yposR=hh/2+hh/4;
        
    }
    
    xshift=maxsp/10;
    
    if (bg) {
        /*ptsB[0] = LineVertex(xshift+(ww/2+(nb_spectrum_bands*band_width))/2,  ypos-maxsp/2,		0,0,16,192);
         ptsB[1] = LineVertex(xshift+(ww/2-(nb_spectrum_bands*band_width))/2-maxsp/4,  ypos-maxsp/2,		0,0,16,192);
         ptsB[2] = LineVertex(xshift+(ww/2+(nb_spectrum_bands*band_width))/2,  ypos+maxsp,		0,0,16,192);
         ptsB[3] = LineVertex(xshift+(ww/2-(nb_spectrum_bands*band_width))/2-maxsp/4, ypos+maxsp,		0,0,16,192);
         glVertexPointer(2, GL_SHORT, sizeof(LineVertex), &ptsB[0].x);
         glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertex), &ptsB[0].r);
         // Render The Quad
         glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
         
         ptsB[0] = LineVertex(xshift+ww/2+(ww/2+(nb_spectrum_bands*band_width))/2,  ypos-maxsp/2,		0,0,16,192);
         ptsB[1] = LineVertex(xshift+ww/2+(ww/2-(nb_spectrum_bands*band_width))/2-maxsp/4,  ypos-maxsp/2,		0,0,16,192);
         ptsB[2] = LineVertex(xshift+ww/2+(ww/2+(nb_spectrum_bands*band_width))/2,  ypos+maxsp,		0,0,16,192);
         ptsB[3] = LineVertex(xshift+ww/2+(ww/2-(nb_spectrum_bands*band_width))/2-maxsp/4, ypos+maxsp,		0,0,16,192);
         glVertexPointer(2, GL_SHORT, sizeof(LineVertex), &ptsB[0].x);
         glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertex), &ptsB[0].r);
         // Render The Quad
         glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
         */
    }
    //	NSLog(@"%d %d",hh,ypos);
    glDisable(GL_BLEND);
    count=0;
    for (int i=0; i<nb_spectrum_bands; i++)
    {
        spl=((int)spectrumDataL[i]*maxsp)>>13;
        spr=((int)spectrumDataR[i]*maxsp)>>13;
        if (spl>maxsp) spl=maxsp;
        if (spr>maxsp) spr=maxsp;
        
        
        if (spectrumPeakValueL_index[i]) {
            pl=spectrumPeakValueL[i]*sin(spectrumPeakValueL_index[i]*3.14159f/180)*sin(spectrumPeakValueL_index[i]*3.14159f/180);
            spectrumPeakValueL_index[i]-=2;
        } else pl=0;
        if (spectrumPeakValueR_index[i]) {
            pr=spectrumPeakValueR[i]*sin(spectrumPeakValueR_index[i]*3.14159f/180)*sin(spectrumPeakValueR_index[i]*3.14159f/180);
            spectrumPeakValueR_index[i]-=2;
        } else pr=0;
        
        if (pl<spl) {
            spectrumPeakValueL[i]=spl;
            pl=spl;
            spectrumPeakValueL_index[i]=90;
        }
        if (pr<spr) {
            spectrumPeakValueR[i]=spr;
            pr=spr;
            spectrumPeakValueR_index[i]=90;
        }
        if (spl>=1) {
            cg=(spl*2*256)/maxsp; if (cg<0) cg=0; if (cg>255) cg=255;
            cb=(spl*1*256)/maxsp; if (cb<0) cb=0; if (cb>255) cb=255;
            cr=(spl*3*256)/maxsp; if (cr<0) cr=0; if (cr>255) cr=255;
            cr=cr-(cg+cb)/2;if (cr<0) cr=0;
            cr=cg=cb=255;
            
            x=xshift+16+i*band_width+band_width/2;
            pts[count++] = LineVertex(x, yposL,	cb/3,cg/3,cr,255);
            pts[count++] = LineVertex(x, yposL+spl,	cb,cr/3,cg,255);
            
            if (spl>=2) {
                pts[count++] = LineVertex(x, yposL,	cb/3/3,cg/3/3,cr/3,255);
                y=yposL-(int)(spl/2);
                x=x-(int)(spl/4);
                if (x<0) {y-=x*2;x=0;}
                pts[count++] = LineVertex(x, y,	cb/3,cr/3/3,cg/3,255);
            }
        }
        
        if (spr>=1) {
            cg=(spr*2*256)/maxsp; if (cg<0) cg=0; if (cg>255) cg=255;
            cb=(spr*1*256)/maxsp; if (cb<0) cb=0; if (cb>255) cb=255;
            cr=(spr*3*256)/maxsp; if (cr<0) cr=0; if (cr>255) cr=255;
            cr=cr-(cg+cb)/2;if (cr<0) cr=0;
            cr=cg=cb=255;
            
            
            x=xshift+16+i*band_width+band_width/2;
            pts[count++] = LineVertex(x, yposR,	cg/3,cr/3,cb,255);
            pts[count++] = LineVertex(x, yposR+spr,	cg,cb,cr/3,255);
            
            if (spr>=2) {
                pts[count++] = LineVertex(x, yposR,	cg/3/3,cr/3/3,cb/3,255);
                y=yposR-(int)(spr/2);
                x=x-(int)(spr/4);
                if (x<0) {y-=x*2;x=0;}
                
                pts[count++] = LineVertex(x, y,	cg/3,cb/3,cr/3/3,255);
            }
        }
        
        if (peaks) {
            ptsC[i*8+0] = LineVertex(xshift+16+i*band_width, yposL+pl,	255,255/3,255,255);
            ptsC[i*8+1] = LineVertex(xshift+16+i*band_width+band_width, yposL+pl,	255,255/3,255,255);
            ptsC[i*8+2] = LineVertex(xshift+16+i*band_width, yposR+pr,	255,255,255/3,255);
            ptsC[i*8+3] = LineVertex(xshift+16+i*band_width+band_width, yposR+pr,	255,255,255/3,255);
            
            x=xshift+16+i*band_width-pl/4;if (x<0) x=0;
            ptsC[i*8+4] = LineVertex(x, yposL-pl/2,	255/3,255/3/3,255/3,255);
            x=xshift+16+i*band_width+band_width-pl/4;if (x<0) x=0;
            ptsC[i*8+5] = LineVertex(x, yposL-pl/2,	255/3,255/3/3,255/3,255);
            ptsC[i*8+6] = LineVertex(xshift+16+i*band_width-pr/4, yposR-pr/2,	255/3,255/3,255/3/3,255);
            ptsC[i*8+7] = LineVertex(xshift+16+i*band_width+band_width-pr/4, yposR-pr/2,	255/3,255/3,255/3/3,255);
        }
    }
    
    glLineWidth((band_width-1)*(is_retina?2:1));
    glVertexPointer(2, GL_SHORT, sizeof(LineVertex), &pts[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertex), &pts[0].r);
    glDrawArrays(GL_LINES, 0, count);
    
    if (peaks) {
        glLineWidth(1);
        glVertexPointer(2, GL_SHORT, sizeof(LineVertex), &ptsC[0].x);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertex), &ptsC[0].r);
        glDrawArrays(GL_LINES, 0, nb_spectrum_bands*4*2);
    }
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    
    free(pts);
    free(ptsB);
    if (peaks) free(ptsC);
}

static int DrawBeat_first_call=1;
static int beatValueL_index[SPECTRUM_BANDS];
static int beatValueR_index[SPECTRUM_BANDS];


void RenderUtils::DrawBeat(unsigned char *beatDataL,unsigned char *beatDataR,uint ww,uint hh,uint bg,uint _pos,int nb_spectrum_bands) {
    LineVertex *ptsB;
    float pr,pl,cr,cg,cb;
    int band_width,ypos;
    
    band_width=(ww/2-32)/nb_spectrum_bands;
    ptsB=(LineVertex*)malloc(sizeof(LineVertex)*4);
    
    if (DrawBeat_first_call) {
        DrawBeat_first_call=0;
        for (int i=0;i<nb_spectrum_bands;i++) {
            beatValueL_index[i]=0;
            beatValueR_index[i]=0;
        }
    }
    
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    if (_pos) {
        ypos=hh-band_width-10;
        
    } else {
        ypos=hh/2;
    }
    
    /*if (bg) {
     
     
     ptsB[0] = LineVertex((ww/2+(nb_spectrum_bands*band_width))/2,  ypos-16,		0,0,16,192);
     ptsB[1] = LineVertex((ww/2-(nb_spectrum_bands*band_width))/2,  ypos-16,		0,0,16,192);
     ptsB[2] = LineVertex((ww/2+(nb_spectrum_bands*band_width))/2,  ypos+16,		0,0,16,192);
     ptsB[3] = LineVertex((ww/2-(nb_spectrum_bands*band_width))/2, ypos+16,		0,0,16,192);
     glVertexPointer(2, GL_SHORT, sizeof(LineVertex), &ptsB[0].x);
     glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertex), &ptsB[0].r);
     // Render The Quad
     glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
     
     ptsB[0] = LineVertex(ww/2+(ww/2+(nb_spectrum_bands*band_width))/2,  ypos-16,		0,0,16,192);
     ptsB[1] = LineVertex(ww/2+(ww/2-(nb_spectrum_bands*band_width))/2,  ypos-16,		0,0,16,192);
     ptsB[2] = LineVertex(ww/2+(ww/2+(nb_spectrum_bands*band_width))/2,  ypos+16,		0,0,16,192);
     ptsB[3] = LineVertex(ww/2+(ww/2-(nb_spectrum_bands*band_width))/2, ypos+16,		0,0,16,192);
     glVertexPointer(2, GL_SHORT, sizeof(LineVertex), &ptsB[0].x);
     glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertex), &ptsB[0].r);
     // Render The Quad
     glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
     }*/
    //	NSLog(@"%d %d",hh,ypos);
    glDisable(GL_BLEND);
    for (int i=0; i<nb_spectrum_bands; i++)
    {
        if (beatValueL_index[i]) {
            pl=band_width*sin(beatValueL_index[i]*3.14159/80)*0.7f+band_width*0.2f;
            if (pl>band_width/2-1) pl=band_width/2-1;
            beatValueL_index[i]-=4;
        } else pl=band_width*0.1f;
        if (beatValueR_index[i]) {
            pr=band_width*sin(beatValueR_index[i]*3.14159/80)*0.7f+band_width*0.2f;
            if (pr>band_width/2-1) pr=band_width/2-1;
            beatValueR_index[i]-=4;
        } else pr=band_width*0.1f;
        
        if (beatDataL[i]) {
            pl=band_width/2;
            beatValueL_index[i]=40;
        }
        if (beatDataR[i]) {
            pr=band_width/2;
            beatValueR_index[i]=40;
        }
        cg=beatValueL_index[i]*2*256/40; if (cg<32) cg=32; if (cg>255) cg=255;
        cb=beatValueL_index[i]*1*256/40; if (cb<24) cb=24; if (cb>255) cb=255;
        cr=beatValueL_index[i]*3*256/40; if (cr<16) cr=16; if (cr>255) cr=255;
        cr=cr-(cg+cb)/2;if (cr<24) cr=24;
        
        ptsB[0] = LineVertex((ww/2-(nb_spectrum_bands*band_width))/2+i*band_width+band_width/2-pl, ypos+pl,cb,cg,cr/3,255);
        ptsB[1] = LineVertex((ww/2-(nb_spectrum_bands*band_width))/2+i*band_width+band_width/2+pl, ypos+pl,cb,cg,cr/3,255);
        
        ptsB[2] = LineVertex((ww/2-(nb_spectrum_bands*band_width))/2+i*band_width+band_width/2-pl, ypos-pl,cb,cg,cr/3,255);
        ptsB[3] = LineVertex((ww/2-(nb_spectrum_bands*band_width))/2+i*band_width+band_width/2+pl, ypos-pl,cb,cg,cr/3,255);
        
        glVertexPointer(2, GL_SHORT, sizeof(LineVertex), &ptsB[0].x);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertex), &ptsB[0].r);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        
        cg=beatValueR_index[i]*2*256/band_width; if (cg<32) cg=32; if (cg>255) cg=255;
        cb=beatValueR_index[i]*1*256/band_width; if (cb<24) cb=24; if (cb>255) cb=255;
        cr=beatValueR_index[i]*3*256/band_width; if (cr<16) cr=16; if (cr>255) cr=255;
        cr=cr-(cg+cb)/2;if (cr<24) cr=24;
        
        ptsB[0] = LineVertex(ww/2+(ww/2-(nb_spectrum_bands*band_width))/2+i*band_width+band_width/2-pr, ypos+pr,cg,cb,cr/3,255);
        ptsB[1] = LineVertex(ww/2+(ww/2-(nb_spectrum_bands*band_width))/2+i*band_width+band_width/2+pr, ypos+pr,cg,cb,cr/3,255);
        
        ptsB[2] = LineVertex(ww/2+(ww/2-(nb_spectrum_bands*band_width))/2+i*band_width+band_width/2-pr, ypos-pr,cg,cb,cr/3,255);
        ptsB[3] = LineVertex(ww/2+(ww/2-(nb_spectrum_bands*band_width))/2+i*band_width+band_width/2+pr, ypos-pr,cg,cb,cr/3,255);
        
        glVertexPointer(2, GL_SHORT, sizeof(LineVertex), &ptsB[0].x);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertex), &ptsB[0].r);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    
    free(ptsB);
}

void RenderUtils::DrawFXTouchGrid(uint _ww,uint _hh,int fade_level,int min_level,int active_idx,int cpt) {
    LineVertex pts[24];
    //set the opengl state
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glVertexPointer(2, GL_SHORT, sizeof(LineVertex), &pts[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertex), &pts[0].r);
    
    int fade_lev=fade_level*0.75;
    if (fade_lev<+min_level) fade_lev=min_level;
    if (fade_lev>255*0.8) fade_lev=255*0.8;
    pts[0] = LineVertex(0, 0,		0,0,0,fade_lev);
    pts[1] = LineVertex(_ww, 0,		0,0,0,fade_lev);
    pts[2] = LineVertex(0, _hh,		0,0,0,fade_lev);
    pts[3] = LineVertex(_ww, _hh,	0,0,0,fade_lev);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    
    
    pts[0] = LineVertex(_ww*1/4-1, 0,		255,255,255,fade_level);
    pts[1] = LineVertex(_ww*1/4-1, _hh,		55,55,155,fade_level);
    pts[2] = LineVertex(_ww*2/4-1, 0,		55,55,155,fade_level);
    pts[3] = LineVertex(_ww*2/4-1, _hh,		255,255,255,fade_level);
    pts[4] = LineVertex(_ww*3/4-1, 0,		55,55,155,fade_level);
    pts[5] = LineVertex(_ww*3/4-1, _hh,		255,255,255,fade_level);
    pts[6] = LineVertex(_ww*1/4+1, 0,		255,255,255,fade_level/4);
    pts[7] = LineVertex(_ww*1/4+1, _hh,		55,55,155,fade_level/4);
    pts[8] = LineVertex(_ww*2/4+1, 0,		55,55,155,fade_level/4);
    pts[9] = LineVertex(_ww*2/4+1, _hh,		255,255,255,fade_level/4);
    pts[10] = LineVertex(_ww*3/4+1, 0,		55,55,155,fade_level/4);
    pts[11] = LineVertex(_ww*3/4+1, _hh,		255,255,255,fade_level/4);
    
    pts[12] = LineVertex(0,	_hh*1/4-1, 	55,55,155,fade_level);
    pts[13] = LineVertex(_ww,	_hh*1/4-1, 	255,255,255,fade_level);
    pts[14] = LineVertex(0,	_hh*2/4-1, 	255,255,255,fade_level);
    pts[15] = LineVertex(_ww,	_hh*2/4-1, 	55,55,155,fade_level);
    pts[16] = LineVertex(0,	_hh*3/4-1, 	255,255,255,fade_level);
    pts[17] = LineVertex(_ww,	_hh*3/4-1, 	55,55,155,fade_level);
    pts[18] = LineVertex(0,	_hh*1/4+1, 	55,55,155,fade_level/4);
    pts[19] = LineVertex(_ww,	_hh*1/4+1, 	255,255,255,fade_level/4);
    pts[20] = LineVertex(0,	_hh*2/4+1, 	255,255,255,fade_level/4);
    pts[21] = LineVertex(_ww,	_hh*2/4+1, 	55,55,155,fade_level/4);
    pts[22] = LineVertex(0,	_hh*3/4+1, 	255,255,255,fade_level/4);
    pts[23] = LineVertex(_ww,	_hh*3/4+1, 	55,55,155,fade_level/4);
    
    glLineWidth(1.0f*(is_retina?2:1));
    glDrawArrays(GL_LINES, 0, 24);
    
    
    pts[0] = LineVertex(_ww*1/4, 0,		255,255,255,fade_level/2);
    pts[1] = LineVertex(_ww*1/4, _hh,		55,55,155,fade_level/2);
    pts[2] = LineVertex(_ww*2/4, 0,		55,55,155,fade_level/2);
    pts[3] = LineVertex(_ww*2/4, _hh,		255,255,255,fade_level/2);
    pts[4] = LineVertex(_ww*3/4, 0,		55,55,155,fade_level/2);
    pts[5] = LineVertex(_ww*3/4, _hh,		255,255,255,fade_level/2);
    
    pts[6] = LineVertex(0,	_hh*1/4, 	55,55,155,fade_level/2);
    pts[7] = LineVertex(_ww,	_hh*1/4, 	255,255,255,fade_level/2);
    pts[8] = LineVertex(0,	_hh*2/4, 	255,255,255,fade_level/2);
    pts[9] = LineVertex(_ww,	_hh*2/4, 	55,55,155,fade_level/2);
    pts[10] = LineVertex(0,	_hh*3/4, 	255,255,255,fade_level/2);
    pts[11] = LineVertex(_ww,	_hh*3/4, 	55,55,155,fade_level/2);
    glLineWidth(2.0f*(is_retina?2:1));
    glDrawArrays(GL_LINES, 0, 12);
    
    int factA,factB;
    factA=230;
    factB=16;
    int colbgAR=factA+factB*(0.3*sin(cpt*7*3.1459/1024)+1.2*sin(cpt*17*8*3.1459/1024)+0.7*sin(cpt*31*8*3.1459/1024));
    int colbgAG=factA+factB*(0.3*sin(cpt*5*3.1459/1024)+1.2*sin(cpt*11*8*3.1459/1024)-0.7*sin(cpt*27*8*3.1459/1024));
    int colbgAB=factA+factB*(1.2*sin(cpt*7*3.1459/1024)-0.5*sin(cpt*13*8*3.1459/1024)+1.5*sin(cpt*57*8*3.1459/1024));
    cpt+=16;
    int colbgBR=factA+factB*(0.3*sin(cpt*7*3.1459/1024)+1.2*sin(cpt*17*8*3.1459/1024)+0.7*sin(cpt*31*8*3.1459/1024));
    int colbgBG=factA+factB*(0.3*sin(cpt*5*3.1459/1024)+1.2*sin(cpt*11*8*3.1459/1024)-0.7*sin(cpt*27*8*3.1459/1024));
    int colbgBB=factA+factB*(1.2*sin(cpt*7*3.1459/1024)-0.5*sin(cpt*13*8*3.1459/1024)+1.5*sin(cpt*57*8*3.1459/1024));
    
    if (colbgAR<0) colbgAR=0; if (colbgAR>255) colbgAR=255;
    if (colbgAG<0) colbgAG=0; if (colbgAG>255) colbgAG=255;
    if (colbgAB<0) colbgAB=0; if (colbgAB>255) colbgAB=255;
    if (colbgBR<0) colbgBR=0; if (colbgBR>255) colbgBR=255;
    if (colbgBG<0) colbgBG=0; if (colbgBG>255) colbgBG=255;
    if (colbgBB<0) colbgBB=0; if (colbgBB>255) colbgBB=255;
    glLineWidth(2.0f*(is_retina?2:1));
    fade_lev=255;
    glDisable(GL_BLEND);
    for (int y=0;y<4;y++)
        for (int x=0;x<4;x++) {
            if (active_idx&(1<<((3-y)*4+x))) {
                pts[0] = LineVertex(x*_ww/4+3, y*_hh/4+3,		colbgAR,colbgAG,colbgAB,fade_lev);
                pts[1] = LineVertex((x+1)*_ww/4-3, y*_hh/4+3,		colbgBR,colbgBG,colbgBB,fade_lev);
                pts[2] = LineVertex((x+1)*_ww/4-3, (y+1)*_hh/4-3,	colbgAR,colbgAG,colbgAB,fade_lev);
                pts[3] = LineVertex(x*_ww/4+3, (y+1)*_hh/4-3,		colbgBR,colbgBG,colbgBB,fade_lev);
                glDrawArrays(GL_LINE_LOOP, 0, 4);
            }
        }
    
    
    
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
}

void RenderUtils::DrawChanLayout(uint _ww,uint _hh,int display_note_mode,int chanNb,int pixOfs) {
    int count=0;
    int col_size,col_ofs;
    LineVertex pts[10*MAX_VISIBLE_CHAN+10],ptsD[4*2];
    //set the opengl state
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.0f*(is_retina?2:1));
    
    
    switch (display_note_mode){
        case 0:col_size=12*6;col_ofs=25;break;
        case 1:col_size=6*6;col_ofs=27;break;
        case 2:col_size=4*6;col_ofs=27;break;
    }
    
    
    //then draw channels frame
    
    for (int i=0; i<chanNb; i++) {
        if (col_size*i+col_ofs-2.0f>_ww) break;
        pts[count++] = LineVertex(pixOfs+col_size*i+col_ofs-2.0f, (i&1?_hh:0),	140,160,255,255);
        pts[count++] = LineVertex(pixOfs+col_size*i+col_ofs-2.0f,	(i&1?0:_hh),	60,100,255,255);
        pts[count++] = LineVertex(pixOfs+col_size*i+col_ofs-1, (i&1?_hh:0),	140/3,160/3,255/3,255);
        pts[count++] = LineVertex(pixOfs+col_size*i+col_ofs-1, (i&1?0:_hh),	60/3,100/3,255/3,255);
        pts[count++] = LineVertex(pixOfs+col_size*i+col_ofs, (i&1?_hh:0),		140/3,160/3,255/3,255);
        pts[count++] = LineVertex(pixOfs+col_size*i+col_ofs, (i&1?0:_hh),		60/3,100/3,255/3,255);
        pts[count++] = LineVertex(pixOfs+col_size*i+col_ofs+1, (i&1?_hh:0),	140/3,160/3,255/3,255);
        pts[count++] = LineVertex(pixOfs+col_size*i+col_ofs+1, (i&1?0:_hh),	60/3,100/3,255/3,255);
        
        pts[count++] = LineVertex(pixOfs+col_size*i+col_ofs+2.0f, (i&1?_hh:0),	140/6,160/6,255/6,255);
        pts[count++] = LineVertex(pixOfs+col_size*i+col_ofs+2.0f, (i&1?0:_hh),	60/6,100/6,255/6,255);
    }
    pts[count++] = LineVertex(1, _hh-20+2,			140,160,255,255);
    pts[count++] = LineVertex(_ww-1, _hh-20+2,		60,100,255,255);
    pts[count++] = LineVertex(1, _hh-20+1,		140/3,160/3,255/3,255);
    pts[count++] = LineVertex(_ww-1, _hh-20+1,	60/3,100/3,255/3,255);
    pts[count++] = LineVertex(1, _hh-20,		140/3,160/3,255/3,255);
    pts[count++] = LineVertex(_ww-1, _hh-20,		60/3,100/3,255/3,255);
    pts[count++] = LineVertex(1, _hh-20-1,		140/3,160/3,255/3,255);
    pts[count++] = LineVertex(_ww-1, _hh-20-1,	60/3,100/3,255/3,255);
    pts[count++] = LineVertex(1, _hh-20-2,		140/6,160/6,255/6,255);
    pts[count++] = LineVertex(_ww-1, _hh-20-2,	60/6,100/6,255/6,255);
    
    
    glVertexPointer(2, GL_SHORT, sizeof(LineVertex), &pts[0].x);
    
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertex), &pts[0].r);
    
    glDrawArrays(GL_LINES, 0, count);
    
    
    //3D border effect
    ptsD[0] = LineVertex(0, 1,		80,80,80,255);
    ptsD[1] = LineVertex(_ww, 1,	140,140,140,255);
    ptsD[2] = LineVertex(0, _hh-1,	20,20,20,255);
    ptsD[3] = LineVertex(_ww, _hh-1,80,80,80,255);
    ptsD[4] = LineVertex(_ww-1, 0,	140,140,140,255);
    ptsD[5] = LineVertex(_ww-1, _hh,80,80,80,255);
    ptsD[6] = LineVertex(1, 0,		80,80,80,255);
    ptsD[7] = LineVertex(1, _hh,	20,20,20,255);
    glLineWidth(2.0f*(is_retina?2:1));
    glVertexPointer(2, GL_SHORT, sizeof(LineVertex), &ptsD[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertex), &ptsD[0].r);
    glDrawArrays(GL_LINES, 0, 8);
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisable(GL_BLEND);
    
}

void RenderUtils::DrawChanLayoutAfter(uint _ww,uint _hh,int display_note_mode,int *volumeData,int chanNb,int pixOfs) {
    LineVertex pts[64*2];
    int ii;
    int count=0;
    int col_size,col_ofs;
    
    //set the opengl state
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnable(GL_BLEND);
    glLineWidth(2.0f*(is_retina?2:1));
    glVertexPointer(2, GL_SHORT, sizeof(LineVertex), &pts[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertex), &pts[0].r);
    
    
    //current playing line
    ii=(_hh-30+11)/12;
    pts[0] = LineVertex(0,     _hh-30-12*(ii/2)+3-8,		230,76,153,120);
    pts[1] = LineVertex(_ww-1, _hh-30-12*(ii/2)+3-8,		230,76,153,120);
    pts[2] = LineVertex(0,     _hh-30-12*(ii/2)+3+8,		230,76,153,120);
    pts[3] = LineVertex(_ww-1, _hh-30-12*(ii/2)+3+8,		230,76,153,120);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    pts[0] = LineVertex(0,     _hh-30-12*(ii/2)+3-9.0f,     230/2,76/2,153/2,120);
    pts[1] = LineVertex(_ww-1, _hh-30-12*(ii/2)+3-9.0f,     230/2,76/2,153/2,120);
    pts[2] = LineVertex(0,     _hh-30-12*(ii/2)+3+9.0f,     250,96,183,190);
    pts[3] = LineVertex(_ww-1, _hh-30-12*(ii/2)+3+9.0f,     250,96,183,190);
    glDrawArrays(GL_LINES, 0, 4);
    
    
    switch (display_note_mode){
        case 0:col_size=12*6;col_ofs=25;break;
        case 1:col_size=6*6;col_ofs=27;break;
        case 2:col_size=4*6;col_ofs=27;break;
    }
    
    
    count=0;
    if (volumeData) {
        for (int i=0; i<chanNb; i++) {
            if (col_size*i+col_ofs-2.0f>_ww) break;
            
            int cr,cg,cb,crb,cgb,cbb;
            crb=100;
            cgb=50;
            cbb=150;
            cr=crb+volumeData[i]*2; if (cr<0) cr=0; if (cr>255) cr=255;
            cg=cgb+volumeData[i]/4; if (cg<0) cg=0; if (cg>255) cg=255;
            cb=cbb+volumeData[i]; if (cb<0) cb=0; if (cb>255) cb=255;
            
            
            
            pts[0] = LineVertex(pixOfs+col_size*i+col_ofs+col_size*1/5, 0,	crb,cgb,cbb,125);
            pts[1] = LineVertex(pixOfs+col_size*i+col_ofs+col_size*4/5,	0,	crb,cgb,cbb,125);
            pts[2] = LineVertex(pixOfs+col_size*i+col_ofs+col_size*1/5,	volumeData[i]*_hh/256/5,cr,cg,cb,125);
            pts[3] = LineVertex(pixOfs+col_size*i+col_ofs+col_size*4/5,	volumeData[i]*_hh/256/5,cr,cg,cb,125);
            glDrawArrays(GL_TRIANGLE_STRIP,0,4);
        }
    }
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisable(GL_BLEND);
    
}

/* Reduces A Normal Vector (3 Coordinates)       */
/* To A Unit Normal Vector With A Length Of One. */
void RenderUtils::ReduceToUnit(GLfloat vector[3]) {
    /* Holds Unit Length */
    GLfloat length;
    
    /* Calculates The Length Of The Vector */
    length=(GLfloat)sqrt((vector[0]*vector[0])+(vector[1]*vector[1])+(vector[2]*vector[2]));
    
    /* Prevents Divide By 0 Error By Providing */
    if (length==0.0f)
    {
        /* An Acceptable Value For Vectors To Close To 0. */
        length=1.0f;
    }
    
    vector[0]/=length;  /* Dividing Each Element By */
    vector[1]/=length;  /* The Length Results In A  */
    vector[2]/=length;  /* Unit Normal Vector.      */
}

/* Calculates Normal For A Quad Using 3 Points */
void RenderUtils::calcNormal(GLfloat v[3][3], GLfloat out[3]) {
    /* Vector 1 (x,y,z) & Vector 2 (x,y,z) */
    GLfloat v1[3], v2[3];
    /* Define X Coord */
    static const int x=0;
    /* Define Y Coord */
    static const int y=1;
    /* Define Z Coord */
    static const int z=2;
    
    /* Finds The Vector Between 2 Points By Subtracting */
    /* The x,y,z Coordinates From One Point To Another. */
    
    /* Calculate The Vector From Point 1 To Point 0 */
    v1[x]=v[0][x]-v[1][x];      /* Vector 1.x=Vertex[0].x-Vertex[1].x */
    v1[y]=v[0][y]-v[1][y];      /* Vector 1.y=Vertex[0].y-Vertex[1].y */
    v1[z]=v[0][z]-v[1][z];      /* Vector 1.z=Vertex[0].y-Vertex[1].z */
    
    /* Calculate The Vector From Point 2 To Point 1 */
    v2[x]=v[1][x]-v[2][x];      /* Vector 2.x=Vertex[0].x-Vertex[1].x */
    v2[y]=v[1][y]-v[2][y];      /* Vector 2.y=Vertex[0].y-Vertex[1].y */
    v2[z]=v[1][z]-v[2][z];      /* Vector 2.z=Vertex[0].z-Vertex[1].z */
    
    /* Compute The Cross Product To Give Us A Surface Normal */
    out[x]=v1[y]*v2[z]-v1[z]*v2[y];     /* Cross Product For Y - Z */
    out[y]=v1[z]*v2[x]-v1[x]*v2[z];     /* Cross Product For X - Z */
    out[z]=v1[x]*v2[y]-v1[y]*v2[x];     /* Cross Product For X - Y */
    
    ReduceToUnit(out);          /* Normalize The Vectors */
}


void RenderUtils::drawbar(float x,float y,float z,float sx,float sy,float sz,float crt,float cgt,float cbt) {
    float cr,cg,cb;
    //top
    cr=crt;cg=cgt;cb=cbt;
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x;
    vertices[0][1]=y;
    vertices[0][2]=z+sz;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x+sx;
    vertices[1][1]=y;
    vertices[1][2]=z+sz;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x;
    vertices[2][1]=y+sy;
    vertices[2][2]=z+sz;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x+sx;
    vertices[3][1]=y+sy;
    vertices[3][2]=z+sz;
    
    normals[0][0]=0;
    normals[0][1]=0;
    normals[0][2]=1;
    normals[1][0]=0;
    normals[1][1]=0;
    normals[1][2]=1;
    normals[2][0]=0;
    normals[2][1]=0;
    normals[2][2]=1;
    normals[3][0]=0;
    normals[3][1]=0;
    normals[3][2]=1;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //left
    cr=crt/2;cg=cgt/2;cb=cbt/2;
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x;
    vertices[0][1]=y;
    vertices[0][2]=z;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x;
    vertices[1][1]=y+sy;
    vertices[1][2]=z;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x;
    vertices[2][1]=y;
    vertices[2][2]=z+sz;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x;
    vertices[3][1]=y+sy;
    vertices[3][2]=z+sz;
    
    normals[0][0]=-1;
    normals[0][1]=0;
    normals[0][2]=0;
    normals[1][0]=-1;
    normals[1][1]=0;
    normals[1][2]=0;
    normals[2][0]=-1;
    normals[2][1]=0;
    normals[2][2]=0;
    normals[3][0]=-1;
    normals[3][1]=0;
    normals[3][2]=0;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //right
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x+sx;
    vertices[0][1]=y;
    vertices[0][2]=z;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x+sx;
    vertices[1][1]=y+sy;
    vertices[1][2]=z;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x+sx;
    vertices[2][1]=y;
    vertices[2][2]=z+sz;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x+sx;
    vertices[3][1]=y+sy;
    vertices[3][2]=z+sz;
    
    normals[0][0]=1;
    normals[0][1]=0;
    normals[0][2]=0;
    normals[1][0]=1;
    normals[1][1]=0;
    normals[1][2]=0;
    normals[2][0]=1;
    normals[2][1]=0;
    normals[2][2]=0;
    normals[3][0]=1;
    normals[3][1]=0;
    normals[3][2]=0;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //up
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x;
    vertices[0][1]=y+sy;
    vertices[0][2]=z;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x+sx;
    vertices[1][1]=y+sy;
    vertices[1][2]=z;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x;
    vertices[2][1]=y+sy;
    vertices[2][2]=z+sz;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x+sx;
    vertices[3][1]=y+sy;
    vertices[3][2]=z+sz;
    
    normals[0][0]=0;
    normals[0][1]=1;
    normals[0][2]=0;
    normals[1][0]=0;
    normals[1][1]=1;
    normals[1][2]=0;
    normals[2][0]=0;
    normals[2][1]=1;
    normals[2][2]=0;
    normals[3][0]=0;
    normals[3][1]=1;
    normals[3][2]=0;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //down
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x;
    vertices[0][1]=y;
    vertices[0][2]=z;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x+sx;
    vertices[1][1]=y;
    vertices[1][2]=z;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x;
    vertices[2][1]=y;
    vertices[2][2]=z+sz;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x+sx;
    vertices[3][1]=y;
    vertices[3][2]=z+sz;
    
    normals[0][0]=0;
    normals[0][1]=-1;
    normals[0][2]=0;
    normals[1][0]=0;
    normals[1][1]=-1;
    normals[1][2]=0;
    normals[2][0]=0;
    normals[2][1]=-1;
    normals[2][2]=0;
    normals[3][0]=0;
    normals[3][1]=-1;
    normals[3][2]=0;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //back
    cr=crt/4;cg=cgt/4;cb=cbt/4;
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x;
    vertices[0][1]=y;
    vertices[0][2]=z;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x+sx;
    vertices[1][1]=y;
    vertices[1][2]=z;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x;
    vertices[2][1]=y+sy;
    vertices[2][2]=z;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x+sx;
    vertices[3][1]=y+sy;
    vertices[3][2]=z;
    
    normals[0][0]=0;
    normals[0][1]=0;
    normals[0][2]=-1;
    normals[1][0]=0;
    normals[1][1]=0;
    normals[1][2]=-1;
    normals[2][0]=0;
    normals[2][1]=0;
    normals[2][2]=-1;
    normals[3][0]=0;
    normals[3][1]=0;
    normals[3][2]=-1;
    
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
}

void RenderUtils::drawbar3(float x,float y,float z,float sx,float sy,float sz,float crt,float cgt,float cbt) {
    float cr,cg,cb;
    cr=crt;cg=cgt;cb=cbt;
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x+sx;
    vertices[0][1]=y;
    vertices[0][2]=z;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x+sx;
    vertices[1][1]=y+sy;
    vertices[1][2]=z;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x+sx;
    vertices[2][1]=y;
    vertices[2][2]=z+sz;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x+sx;
    vertices[3][1]=y+sy;
    vertices[3][2]=z+sz;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void RenderUtils::drawbar2(float x,float y,float z,float sx,float sy,float sz,float crt,float cgt,float cbt) {
    float cr,cg,cb;
    //top
    cr=crt;cg=cgt;cb=cbt;
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x;
    vertices[0][1]=y;
    vertices[0][2]=z+sz;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x+sx;
    vertices[1][1]=y;
    vertices[1][2]=z+sz;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x;
    vertices[2][1]=y+sy;
    vertices[2][2]=z+sz;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x+sx;
    vertices[3][1]=y+sy;
    vertices[3][2]=z+sz;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //left
    cr=crt/2;cg=cgt/2;cb=cbt/2;
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x;
    vertices[0][1]=y;
    vertices[0][2]=z;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x;
    vertices[1][1]=y+sy;
    vertices[1][2]=z;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x;
    vertices[2][1]=y;
    vertices[2][2]=z+sz;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x;
    vertices[3][1]=y+sy;
    vertices[3][2]=z+sz;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //right
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x+sx;
    vertices[0][1]=y;
    vertices[0][2]=z;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x+sx;
    vertices[1][1]=y+sy;
    vertices[1][2]=z;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x+sx;
    vertices[2][1]=y;
    vertices[2][2]=z+sz;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x+sx;
    vertices[3][1]=y+sy;
    vertices[3][2]=z+sz;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //up
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x;
    vertices[0][1]=y+sy;
    vertices[0][2]=z;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x+sx;
    vertices[1][1]=y+sy;
    vertices[1][2]=z;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x;
    vertices[2][1]=y+sy;
    vertices[2][2]=z+sz;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x+sx;
    vertices[3][1]=y+sy;
    vertices[3][2]=z+sz;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //down
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x;
    vertices[0][1]=y;
    vertices[0][2]=z;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x+sx;
    vertices[1][1]=y;
    vertices[1][2]=z;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x;
    vertices[2][1]=y;
    vertices[2][2]=z+sz;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x+sx;
    vertices[3][1]=y;
    vertices[3][2]=z+sz;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //back
    cr=crt/4;cg=cgt/4;cb=cbt/4;
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x;
    vertices[0][1]=y;
    vertices[0][2]=z;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x+sx;
    vertices[1][1]=y;
    vertices[1][2]=z;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x;
    vertices[2][1]=y+sy;
    vertices[2][2]=z;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x+sx;
    vertices[3][1]=y+sy;
    vertices[3][2]=z;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
}

float barSpectrumDataL[SPECTRUM_BANDS];
float barSpectrumDataR[SPECTRUM_BANDS];

void RenderUtils::DrawSpectrum3DBarFlat(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,int mode,int nb_spectrum_bands) {
    GLfloat spL,spR;
    GLfloat crt,cgt,cbt;
    GLfloat x,y,z,sx,sy,sz;
    
    for (int i=0;i<nb_spectrum_bands;i++) {
        barSpectrumDataL[i]=(float)1.f*spectrumDataL[i]/512.0f;
        barSpectrumDataR[i]=(float)1.f*spectrumDataR[i]/512.0f;
    }
    
    //Ortho view
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    float aspectRatio = (float)ww/(float)hh;
    float _hw;// = 16*1.0/2;//0.2f;
    float _hh;// = _hw/aspectRatio;
    
    
    _hw = (float)(nb_spectrum_bands)*1.4f/2;
    _hh = _hw/aspectRatio*SPECTRUM_BANDS/nb_spectrum_bands;
    
    glOrthof(-_hw, _hw, -_hh, _hh, 100.0f, 10000.0f);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    //
    
    glPushMatrix();                     /* Push The Modelview Matrix */
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glColorPointer(4, GL_FLOAT, 0, vertColor);
    
    
    glTranslatef(0.0, 0.0, -120.0);
    
    glRotatef(-90,0,0,1);
    
    
    glRotatef(-90, 0, 1, 0);
    
    
    
    vertColor[0][3]=vertColor[1][3]=vertColor[2][3]=vertColor[3][3]=1;
    crt=0;
    cgt=0;
    cbt=0;
    
    x=-0.5;y=0;z=0;
    sx=sy=24.0/(float)nb_spectrum_bands;
    
    for (int i=0; i<nb_spectrum_bands; i++) {
        /////////////////
        //LEFT
        spL=barSpectrumDataL[i];
        
        if (i<nb_spectrum_bands*2/3) {
            cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
        } else {
            cbt=0;
        }
        if (i>nb_spectrum_bands/3) {
            cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3);
        } else {
            cgt=0;
        }
        crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
        
        if (spL>2) crt+=(spL-2)*0.05f;
        if (spL>2) cgt+=(spL-2)*0.05f;
        if (spL>2) cbt+=(spL-2)*0.05f;
        
        crt*=0.5f+(1*spL);
        if (crt>1) crt=1;
        cgt*=0.5f+(1*spL);
        if (cgt>1) cgt=1;
        cbt*=0.5f+(1*spL);
        if (cbt>1) cbt=1;
        
        sx=1;
        sy=1;
        sz=spL*2+0.1f;
        x=0-sx/2;
        y=(i-nb_spectrum_bands/2)*sy*1.2;
        
        if (mode==1) {
            z=8+0.1f;//+spL/2;
            drawbar3(x,y,z,sx,sy,sz,crt,cgt,cbt);
        } else if (mode==2) {
            z=0.1f;
            drawbar3(x,y,z,sx,sy,sz,crt,cgt,cbt);
        }
        
        /////////////////
        //RIGHT
        spR=barSpectrumDataR[i];
        
        if (i<nb_spectrum_bands*2/3) {
            cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
        } else {
            cbt=0.1;
        }
        if (i>nb_spectrum_bands/3) {
            cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3)+0.1;
        } else {
            cgt=0.1;
        }
        crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
        
        if (spR>2) crt+=(spR-2)*0.05f;
        if (spR>2) cgt+=(spR-2)*0.05f;
        if (spR>2) cbt+=(spR-2)*0.05f;
        
        crt*=0.5+(spR);
        if (crt>1) crt=1;
        cgt*=0.5+(spR);
        if (cgt>1) cgt=1;
        cbt*=0.5+(spR);
        if (cbt>1) cbt=1;
        
        
        sx=1;
        sy=1;
        sz=spR*2+0.1f;
        x=0-sx/2;
        y=(i-nb_spectrum_bands/2)*sy*1.2;
        
        if (mode==1) {
            z=-16+0.1f;
            drawbar3(x,y,z,sx,sy,sz,crt,cgt,cbt);
        } else if (mode==2) {
            z=0.1f;
            glRotatef(180, 0, 1, 0);
            drawbar3(x,y,z,sx,sy,sz,crt,cgt,cbt);
            glRotatef(180, 0, 1, 0);
        }
    }
    
    
    /* Disable Vertex Pointer */
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    
    
    
    //    glDisable(GL_BLEND);
    
    /* Pop The Matrix */
    glPopMatrix();
}


void RenderUtils::DrawSpectrum3DBar(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int nb_spectrum_bands,int mirror) {
    GLfloat spL,spR;
    GLfloat crt,cgt,cbt;
    GLfloat x,y,z,sx,sy,sz;
    static int frameCpt=0;
    
    if (frameCpt==0) {
        frameCpt=arc4random()&32767;
        memset(barSpectrumDataL,0,sizeof(float)*SPECTRUM_BANDS);
        memset(barSpectrumDataR,0,sizeof(float)*SPECTRUM_BANDS);
    }
    for (int i=0;i<nb_spectrum_bands;i++) {
        /*barSpectrumDataL[i]=barSpectrumDataL[i]*0.8;
         barSpectrumDataR[i]=barSpectrumDataR[i]*0.8;
         if (barSpectrumDataL[i]<(float)spectrumDataL[i]/512.0f)
         */
        barSpectrumDataL[i]=(float)spectrumDataL[i]/512.0f;
        /*if (barSpectrumDataR[i]<(float)spectrumDataR[i]/512.0f) */
        barSpectrumDataR[i]=(float)spectrumDataR[i]/512.0f;
    }
    
    //////////////////////////////
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspectRatio = (float)ww/(float)hh;
    float _hw;// = 16*1.0/2;//0.2f;
    float _hh;// = _hw/aspectRatio;
    //glFrustumf(-_hw, _hw, -_hh, _hh, 100.0f, 10000.0f);
    
    
    switch (mode) {
        case 1:
            _hw = (float)nb_spectrum_bands*0.99/2;
            _hh = _hw/aspectRatio;
            break;
        case 2:
            _hw = 16*1.0/2;
            _hh = _hw/aspectRatio;
            break;
        case 3:
            _hw = (float)nb_spectrum_bands*0.99/2;
            _hh = _hw/aspectRatio;
            break;
        case 4:
            _hw = (float)nb_spectrum_bands*0.99/2;
            _hh = _hw/aspectRatio;
            break;
    }
    
    glFrustumf(-_hw, _hw, -_hh, _hh, 100.0f, 10000.0f);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPushMatrix();                     /* Push The Modelview Matrix */
    glEnable(GL_COLOR_MATERIAL);
    glEnable( GL_LIGHTING );
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, position );
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 90);
    
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight[2]);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight[2]);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight[2] );
    
    
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glColorPointer(4, GL_FLOAT, 0, vertColor);
    glNormalPointer(GL_FLOAT, 0, normals);
    
    frameCpt++;
    
    switch (mode) {
        case 1:
            glTranslatef(0.0, 0.0, -150.0+
                         15*(0.8f*sin((float)frameCpt*3.14159f/991)+
                             1.7f*sin((float)frameCpt*3.14159f/3065)-
                             0.3f*sin((float)frameCpt*3.14159f/5009)));
            
            glRotatef(-90+10.0f*(0.8f*sin((float)frameCpt*3.14159f/2691)+
                                 0.7f*sin((float)frameCpt*3.14159f/3113)-
                                 0.3f*sin((float)frameCpt*3.14159f/7409)),0,0,1);
            
            
            glRotatef(3*360.0f*(0.5f*sin((float)frameCpt*3.14159f/761)-
                                0.7f*sin((float)frameCpt*3.14159f/1211)-
                                0.9f*sin((float)frameCpt*3.14159f/2213)), 0, 1, 0);
            
            
            glRotatef(10.0f*(0.8f*sin((float)frameCpt*3.14159f/891)-
                             0.2f*sin((float)frameCpt*3.14159f/211)-
                             0.4f*sin((float)frameCpt*3.14159f/5213)),0,0,1);
            
            break;
        case 2:
            glTranslatef(0.0, 0.0, -190.0+
                         15*(0.8f*sin((float)frameCpt*3.14159f/991)+
                             1.7f*sin((float)frameCpt*3.14159f/3065)-
                             0.3f*sin((float)frameCpt*3.14159f/5009)));
            
            
            
            glRotatef(20+10.0f*(0.8f*sin((float)frameCpt*3.14159f/2691)+
                                0.7f*sin((float)frameCpt*3.14159f/3113)-
                                0.3f*sin((float)frameCpt*3.14159f/7409)),1,0,0);
            
            glRotatef(20.0f*(0.8f*sin((float)frameCpt*3.14159f/891)-
                             0.2f*sin((float)frameCpt*3.14159f/211)-
                             0.4f*sin((float)frameCpt*3.14159f/5213)),0,0,1);
            
            
            glRotatef(360.0f*(0.5f*sin((float)frameCpt*3.14159f/761)-
                              0.7f*sin((float)frameCpt*3.14159f/1211)-
                              0.9f*sin((float)frameCpt*3.14159f/2213)), 0, 1, 0);
            
            break;
        case 3:
            glTranslatef(0.0, 0.0, -150.0+
                         15*(0.8f*sin((float)frameCpt*3.14159f/991)+
                             1.7f*sin((float)frameCpt*3.14159f/3065)-
                             0.3f*sin((float)frameCpt*3.14159f/5009)));
            
            glRotatef(-90+10.0f*(0.8f*sin((float)frameCpt*3.14159f/2691)+
                                 0.7f*sin((float)frameCpt*3.14159f/3113)-
                                 0.3f*sin((float)frameCpt*3.14159f/7409)),0,0,1);
            
            
            glRotatef(3*360.0f*(0.5f*sin((float)frameCpt*3.14159f/761)-
                                0.7f*sin((float)frameCpt*3.14159f/1211)-
                                0.9f*sin((float)frameCpt*3.14159f/2213)), 0, 1, 0);
            
            
            glRotatef(10.0f*(0.8f*sin((float)frameCpt*3.14159f/891)-
                             0.2f*sin((float)frameCpt*3.14159f/211)-
                             0.4f*sin((float)frameCpt*3.14159f/5213)),0,0,1);
            
            break;
        case 4:
            glTranslatef(0.0, 0.0, -150.0+
                         0*(0.8f*sin((float)frameCpt*3.14159f/991)+
                            1.7f*sin((float)frameCpt*3.14159f/3065)-
                            0.3f*sin((float)frameCpt*3.14159f/5009)));
            
            glRotatef(-90+0.0f*(0.8f*sin((float)frameCpt*3.14159f/2691)+
                                0.7f*sin((float)frameCpt*3.14159f/3113)-
                                0.3f*sin((float)frameCpt*3.14159f/7409)),0,0,1);
            
            
            glRotatef(90+0*360.0f*(0.5f*sin((float)frameCpt*3.14159f/761)-
                                   0.7f*sin((float)frameCpt*3.14159f/1211)-
                                   0.9f*sin((float)frameCpt*3.14159f/2213)), 0, 1, 0);
            
            
            glRotatef(0.0f*(0.8f*sin((float)frameCpt*3.14159f/891)-
                            0.2f*sin((float)frameCpt*3.14159f/211)-
                            0.4f*sin((float)frameCpt*3.14159f/5213)),0,0,1);
            
            break;
    }
    
    vertColor[0][3]=vertColor[1][3]=vertColor[2][3]=vertColor[3][3]=1;
    crt=0;
    cgt=0;
    cbt=0;
    
    float ang=0;
    x=-0.5;y=0;z=0;
    sx=sy=24.0/(float)nb_spectrum_bands;
    float trans=14+sx;
    
    if (mode==2) {
        for (int i=0; i<nb_spectrum_bands; i++) {
            /////////////////
            //LEFT
            spL=barSpectrumDataL[i];
            
            if (i<nb_spectrum_bands*2/3) {
                cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
            } else {
                cbt=0;
            }
            if (i>nb_spectrum_bands/3) {
                cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3);
            } else {
                cgt=0;
            }
            crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
            crt*=0.5+(spL/1);
            if (crt>1) crt=1;
            cgt*=0.5+(spL/1);
            if (cgt>1) cgt=1;
            cbt*=0.5+(spL/1);
            if (cbt>1) cbt=1;
            
            sz=(spL+0.1f);
            x=0-sx/2;
            y=4+ang/10;
            z=-4-ang/10-spL/4;
            
            //y=(i-nb_spectrum_bands/2)*sy*1.2;
            //z=1+spL/4;
            
            glTranslatef(0,-2,trans);
            glRotatef(ang+270,1,0,0);
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            glRotatef(-(ang+270),1,0,0);
            glTranslatef(0,2,-trans);
            
            glRotatef(180,0,1,0);
            
            glTranslatef(0,-2,trans);
            glRotatef(ang+270,1,0,0);
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            glRotatef(-(ang+270),1,0,0);
            glTranslatef(0,2,-trans);
            
            
            glRotatef(180,0,1,0);
            
            
            
            /////////////////
            //RIGHT
            spR=barSpectrumDataR[i];
            /////////////////
            if (i<nb_spectrum_bands*2/3) {
                cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
            } else {
                cbt=0;
            }
            if (i>nb_spectrum_bands/3) {
                cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3);
            } else {
                cgt=0;
            }
            crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
            crt*=0.5+(spR/1);
            if (crt>1) crt=1;
            cgt*=0.5+(spR/1);
            if (cgt>1) cgt=1;
            cbt*=0.5+(spR/1);
            if (cbt>1) cbt=1;
            
            sz=(spR+0.1f);
            x=0-sx/2;
            y=4+ang/10;
            z=-4-ang/10-spR/4;
            
            //y=(i-nb_spectrum_bands/2)*sy*1.2;
            //z=1+spL/4;
            
            glRotatef(90,0,1,0);
            
            glTranslatef(0,-2,trans);
            glRotatef(ang+270,1,0,0);
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            glRotatef(-(ang+270),1,0,0);
            glTranslatef(0,2,-trans);
            
            glRotatef(180,0,1,0);
            
            glTranslatef(0,-2,trans);
            glRotatef(ang+270,1,0,0);
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            glRotatef(-(ang+270),1,0,0);
            glTranslatef(0,2,-trans);
            
            
            glRotatef(180+90,0,1,0);
            
            if (ang<90) ang+=(90.0/(float)nb_spectrum_bands)*1.1;
            if (ang>90) ang=90;
            
            
        }
        
        glRotatef(180,0,0,1);
        glTranslatef(0,14,0);
        ang=0;
        x=-0.5;y=0;z=0;
        sx=sy=24.0/(float)nb_spectrum_bands;
        trans=14+sx;
        if (mirror)
            for (int i=0; i<nb_spectrum_bands; i++) {
                /////////////////
                //LEFT
                spL=barSpectrumDataL[i];
                
                if (i<nb_spectrum_bands*2/3) {
                    cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
                } else {
                    cbt=0;
                }
                if (i>nb_spectrum_bands/3) {
                    cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3);
                } else {
                    cgt=0;
                }
                crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
                crt*=0.5+(spL/1);
                if (crt>1) crt=1;
                cgt*=0.5+(spL/1);
                if (cgt>1) cgt=1;
                cbt*=0.5+(spL/1);
                if (cbt>1) cbt=1;
                
                crt*=0.5;cgt*=0.5;cbt*=0.5;
                
                sz=(spL+0.1f);
                x=0-sx/2;
                y=4+ang/10;
                z=-4-ang/10-spL/4;
                
                //y=(i-nb_spectrum_bands/2)*sy*1.2;
                //z=1+spL/4;
                
                glTranslatef(0,-2,trans);
                glRotatef(ang+270,1,0,0);
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                glRotatef(-(ang+270),1,0,0);
                glTranslatef(0,2,-trans);
                
                glRotatef(180,0,1,0);
                
                glTranslatef(0,-2,trans);
                glRotatef(ang+270,1,0,0);
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                glRotatef(-(ang+270),1,0,0);
                glTranslatef(0,2,-trans);
                
                
                glRotatef(180,0,1,0);
                
                
                
                /////////////////
                //RIGHT
                spR=barSpectrumDataR[i];
                /////////////////
                if (i<nb_spectrum_bands*2/3) {
                    cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
                } else {
                    cbt=0;
                }
                if (i>nb_spectrum_bands/3) {
                    cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3);
                } else {
                    cgt=0;
                }
                crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
                crt*=0.5+(spR/1);
                if (crt>1) crt=1;
                cgt*=0.5+(spR/1);
                if (cgt>1) cgt=1;
                cbt*=0.5+(spR/1);
                if (cbt>1) cbt=1;
                
                crt*=0.5;cgt*=0.5;cbt*=0.5;
                
                sz=(spR+0.1f);
                x=0-sx/2;
                y=4+ang/10;
                z=-4-ang/10-spR/4;
                
                //y=(i-nb_spectrum_bands/2)*sy*1.2;
                //z=1+spL/4;
                
                glRotatef(90,0,1,0);
                
                glTranslatef(0,-2,trans);
                glRotatef(ang+270,1,0,0);
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                glRotatef(-(ang+270),1,0,0);
                glTranslatef(0,2,-trans);
                
                glRotatef(180,0,1,0);
                
                glTranslatef(0,-2,trans);
                glRotatef(ang+270,1,0,0);
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                glRotatef(-(ang+270),1,0,0);
                glTranslatef(0,2,-trans);
                
                
                glRotatef(180+90,0,1,0);
                
                if (ang<90) ang+=(90.0/(float)nb_spectrum_bands)*1.1;
                if (ang>90) ang=90;
                
                
            }
    }
    if (mode==1) {
        for (int i=0; i<nb_spectrum_bands; i++) {
            /////////////////
            //LEFT
            spL=barSpectrumDataL[i];
            
            if (i<nb_spectrum_bands*2/3) {
                cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
            } else {
                cbt=0;
            }
            if (i>nb_spectrum_bands/3) {
                cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3);
            } else {
                cgt=0;
            }
            crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
            crt*=0.5+(spL/1);
            if (crt>1) crt=1;
            cgt*=0.5+(spL/1);
            if (cgt>1) cgt=1;
            cbt*=0.5+(spL/1);
            if (cbt>1) cbt=1;
            
            
            sx=1;
            sy=1;
            sz=spL+0.1f;
            x=0-sx/2;
            y=(i-nb_spectrum_bands/2)*sy*1.2;
            z=1+spL/4;
            
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            glRotatef(180, 0, 1, 0);
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            
            /////////////////
            //RIGHT
            spR=barSpectrumDataR[i];
            
            if (i<nb_spectrum_bands*2/3) {
                cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
            } else {
                cbt=0.1;
            }
            if (i>nb_spectrum_bands/3) {
                cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3)+0.1;
            } else {
                cgt=0.1;
            }
            crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
            crt*=0.5+(spR/1);
            if (crt>1) crt=1;
            cgt*=0.5+(spR/1);
            if (cgt>1) cgt=1;
            cbt*=0.5+(spR/1);
            if (cbt>1) cbt=1;
            
            
            sx=1;
            sy=1;
            sz=spR+0.1f;
            x=0-sx/2;
            y=(i-nb_spectrum_bands/2)*sy*1.2;
            z=1+spR/4;
            
            glRotatef(90, 0, 1, 0);
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            glRotatef(180, 0, 1, 0);
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            
            glRotatef(180-90, 0, 1, 0);
        }
        
        glRotatef(-3*360.0f*(0.5f*sin((float)frameCpt*3.14159f/761)-
                             0.7f*sin((float)frameCpt*3.14159f/1211)-
                             0.9f*sin((float)frameCpt*3.14159f/2213)), 0, 1, 0);
        
        //glRotatef(180,0,0,1);
        glTranslatef(12,0,0);
        
        glRotatef(-3*360.0f*(0.5f*sin((float)frameCpt*3.14159f/761)-
                             0.7f*sin((float)frameCpt*3.14159f/1211)-
                             0.9f*sin((float)frameCpt*3.14159f/2213)), 0, 1, 0);
        
        ang=0;
        if (mirror)
            for (int i=0; i<nb_spectrum_bands; i++) {
                /////////////////
                //LEFT
                spL=barSpectrumDataL[i];
                
                if (i<nb_spectrum_bands*2/3) {
                    cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
                } else {
                    cbt=0;
                }
                if (i>nb_spectrum_bands/3) {
                    cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3);
                } else {
                    cgt=0;
                }
                crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
                crt*=0.5+(spL/1);
                if (crt>1) crt=1;
                cgt*=0.5+(spL/1);
                if (cgt>1) cgt=1;
                cbt*=0.5+(spL/1);
                if (cbt>1) cbt=1;
                
                crt*=0.5;cgt*=0.5;cbt*=0.5;
                
                sx=1;
                sy=1;
                sz=spL+0.1f;
                x=0-sx/2;
                y=(i-nb_spectrum_bands/2)*sy*1.2;
                z=1+spL/4;
                
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                glRotatef(180, 0, 1, 0);
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                
                /////////////////
                //RIGHT
                spR=barSpectrumDataR[i];
                
                if (i<nb_spectrum_bands*2/3) {
                    cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
                } else {
                    cbt=0.1;
                }
                if (i>nb_spectrum_bands/3) {
                    cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3)+0.1;
                } else {
                    cgt=0.1;
                }
                crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
                crt*=0.5+(spR/1);
                if (crt>1) crt=1;
                cgt*=0.5+(spR/1);
                if (cgt>1) cgt=1;
                cbt*=0.5+(spR/1);
                if (cbt>1) cbt=1;
                
                crt*=0.5;cgt*=0.5;cbt*=0.5;
                
                sx=1;
                sy=1;
                sz=spR+0.1f;
                x=0-sx/2;
                y=(i-nb_spectrum_bands/2)*sy*1.2;
                z=1+spR/4;
                
                glRotatef(90, 0, 1, 0);
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                glRotatef(180, 0, 1, 0);
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                
                glRotatef(180-90, 0, 1, 0);
            }
    }
    
    if (mode==4) {
        for (int i=0; i<nb_spectrum_bands; i++) {
            /////////////////
            //LEFT
            spL=barSpectrumDataL[i];
            
            if (i<nb_spectrum_bands*2/3) {
                cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
            } else {
                cbt=0;
            }
            if (i>nb_spectrum_bands/3) {
                cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3);
            } else {
                cgt=0;
            }
            crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
            crt*=0.5+(spL/1);
            if (crt>1) crt=1;
            cgt*=0.5+(spL/1);
            if (cgt>1) cgt=1;
            cbt*=0.5+(spL/1);
            if (cbt>1) cbt=1;
            
            
            sx=1;
            sy=1;
            sz=spL+0.1f;
            x=0-sx/2;
            y=(i-nb_spectrum_bands/2)*sy*1.2;
            z=0.1f;//+spL/2;
            
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            
            /////////////////
            //RIGHT
            spR=barSpectrumDataR[i];
            
            if (i<nb_spectrum_bands*2/3) {
                cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
            } else {
                cbt=0.1;
            }
            if (i>nb_spectrum_bands/3) {
                cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3)+0.1;
            } else {
                cgt=0.1;
            }
            crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
            crt*=0.5+(spR/1);
            if (crt>1) crt=1;
            cgt*=0.5+(spR/1);
            if (cgt>1) cgt=1;
            cbt*=0.5+(spR/1);
            if (cbt>1) cbt=1;
            
            
            sx=1;
            sy=1;
            sz=spR+0.1f;
            x=0-sx/2;
            y=(i-nb_spectrum_bands/2)*sy*1.2;
            z=0.1f;//+spR/2;
            
            //glRotatef(90, 0, 1, 0);
            //drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            glRotatef(180, 0, 1, 0);
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            
            glRotatef(180, 0, 1, 0);
        }
        
        /*glRotatef(-3*360.0f*(0.5f*sin((float)frameCpt*3.14159f/761)-
         0.7f*sin((float)frameCpt*3.14159f/1211)-
         0.9f*sin((float)frameCpt*3.14159f/2213)), 0, 1, 0);
         */
        //glRotatef(180,0,0,1);
        glTranslatef(0,0,12);
        
        glRotatef(-45,0,1,0);
        
        /*glRotatef(-3*360.0f*(0.5f*sin((float)frameCpt*3.14159f/761)-
         0.7f*sin((float)frameCpt*3.14159f/1211)-
         0.9f*sin((float)frameCpt*3.14159f/2213)), 0, 1, 0);
         */
        ang=0;
        if (mirror*0)
            for (int i=0; i<nb_spectrum_bands; i++) {
                /////////////////
                //LEFT
                spL=barSpectrumDataL[i];
                
                if (i<nb_spectrum_bands*2/3) {
                    cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
                } else {
                    cbt=0;
                }
                if (i>nb_spectrum_bands/3) {
                    cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3);
                } else {
                    cgt=0;
                }
                crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
                crt*=0.5+(spL/1);
                if (crt>1) crt=1;
                cgt*=0.5+(spL/1);
                if (cgt>1) cgt=1;
                cbt*=0.5+(spL/1);
                if (cbt>1) cbt=1;
                
                crt*=0.5;cgt*=0.5;cbt*=0.5;
                
                sx=1;
                sy=1;
                sz=spL+0.1f;
                x=0-sx/2;
                y=(i-nb_spectrum_bands/2)*sy*1.2;
                z=0.1f;//+spL/4;
                
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                
                /////////////////
                //RIGHT
                spR=barSpectrumDataR[i];
                
                if (i<nb_spectrum_bands*2/3) {
                    cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
                } else {
                    cbt=0.1;
                }
                if (i>nb_spectrum_bands/3) {
                    cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3)+0.1;
                } else {
                    cgt=0.1;
                }
                crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
                crt*=0.5+(spR/1);
                if (crt>1) crt=1;
                cgt*=0.5+(spR/1);
                if (cgt>1) cgt=1;
                cbt*=0.5+(spR/1);
                if (cbt>1) cbt=1;
                
                crt*=0.5;cgt*=0.5;cbt*=0.5;
                
                sx=1;
                sy=1;
                sz=spR+0.1f;
                x=0-sx/2;
                y=(i-nb_spectrum_bands/2)*sy*1.2;
                z=0.1f;//+spR/4;
                
                glRotatef(180, 0, 1, 0);
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                
                glRotatef(180, 0, 1, 0);
            }
    }
    
    if (mode==3) {
        float dsz,curve_rate;
#define absf(x) (x<0?x:-x)
        for (int i=0; i<nb_spectrum_bands; i++) {
            /////////////////
            //LEFT
            spL=barSpectrumDataL[i];
            
            if (i<nb_spectrum_bands*2/3) {
                cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
            } else {
                cbt=0;
            }
            if (i>nb_spectrum_bands/3) {
                cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3);
            } else {
                cgt=0;
            }
            crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
            crt*=0.5+(spL/1);
            if (crt>1) crt=1;
            cgt*=0.5+(spL/1);
            if (cgt>1) cgt=1;
            cbt*=0.5+(spL/1);
            if (cbt>1) cbt=1;
            
            dsz=2+3*(0.2f*sin(frameCpt*0.05f+i*0.1f)+0.5f*sin(frameCpt*0.07f-i*0.3f+0.01f)*sin(frameCpt*0.07f-i*0.3f+0.01f)+0.5f*sin(frameCpt*0.01f+i*0.03f+0.2f)*sin(frameCpt*0.01f+i*0.03f+0.2f)*sin(frameCpt*0.01f+i*0.03f+0.2f));
            curve_rate=i*2*360/nb_spectrum_bands+360*(0.2f*sin(frameCpt*0.02f+i*0.01f)+0.5f*sin(frameCpt*0.01f-i*0.03f+0.01f)*sin(frameCpt*0.01f-i*0.03f+0.01f)+0.5f*sin(frameCpt*0.008f+i*0.003f+0.2f)*sin(frameCpt*0.008f+i*0.003f+0.2f)*sin(frameCpt*0.008f+i*0.003f+0.2f));
            
            sx=1;
            sy=1;
            sz=spL*2+0.1f;
            x=-0.5f;
            z=dsz+spL/4;
            y=(i-nb_spectrum_bands/2)*sy*1.05f;
            
            glRotatef( curve_rate ,0,1,0);
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            glRotatef(90, 0, 1, 0);
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            glRotatef(90, 0, -1, 0);
            glRotatef(curve_rate,0,-1,0);
            
            /////////////////
            //RIGHT
            spR=barSpectrumDataR[i];
            
            if (i<nb_spectrum_bands*2/3) {
                cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
            } else {
                cbt=0.1;
            }
            if (i>nb_spectrum_bands/3) {
                cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3)+0.1;
            } else {
                cgt=0.1;
            }
            crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
            crt*=0.5+(spR/1);
            if (crt>1) crt=1;
            cgt*=0.5+(spR/1);
            if (cgt>1) cgt=1;
            cbt*=0.5+(spR/1);
            if (cbt>1) cbt=1;
            
            
            sx=1;
            sy=1;
            sz=spR*2+0.1f;
            x=-0.5f;
            z=dsz+spR/4;
            y=(i-nb_spectrum_bands/2)*sy*1.05f;
            
            glRotatef(180+curve_rate,0,1,0);
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            glRotatef(90, 0, 1, 0);
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            glRotatef(90, 0, -1, 0);
            glRotatef(180+curve_rate,0,-1,0);
            
        }
        
        glRotatef(-3*360.0f*(0.5f*sin((float)frameCpt*3.14159f/761)-
                             0.7f*sin((float)frameCpt*3.14159f/1211)-
                             0.9f*sin((float)frameCpt*3.14159f/2213)), 0, 1, 0);
        
        //glRotatef(180,0,0,1);
        glTranslatef(15,0,0);
        
        glRotatef(-3*360.0f*(0.5f*sin((float)frameCpt*3.14159f/761)-
                             0.7f*sin((float)frameCpt*3.14159f/1211)-
                             0.9f*sin((float)frameCpt*3.14159f/2213)), 0, 1, 0);
        
        ang=0;
        if (mirror)
            for (int i=0; i<nb_spectrum_bands; i++) {
                /////////////////
                //LEFT
                spL=barSpectrumDataL[i];
                
                if (i<nb_spectrum_bands*2/3) {
                    cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
                } else {
                    cbt=0;
                }
                if (i>nb_spectrum_bands/3) {
                    cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3);
                } else {
                    cgt=0;
                }
                crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
                crt*=0.5+(spL/1);
                if (crt>1) crt=1;
                cgt*=0.5+(spL/1);
                if (cgt>1) cgt=1;
                cbt*=0.5+(spL/1);
                if (cbt>1) cbt=1;
                
                crt*=0.5;cgt*=0.5;cbt*=0.5;
                
                dsz=2+3*(0.2f*sin(frameCpt*0.05f+i*0.1f)+0.5f*sin(frameCpt*0.07f-i*0.3f+0.01f)*sin(frameCpt*0.07f-i*0.3f+0.01f)+0.5f*sin(frameCpt*0.01f+i*0.03f+0.2f)*sin(frameCpt*0.01f+i*0.03f+0.2f)*sin(frameCpt*0.01f+i*0.03f+0.2f));
                curve_rate=-(i*2*360/nb_spectrum_bands+360*(0.2f*sin(frameCpt*0.02f+i*0.01f)+0.5f*sin(frameCpt*0.01f-i*0.03f+0.01f)*sin(frameCpt*0.01f-i*0.03f+0.01f)+0.5f*sin(frameCpt*0.008f+i*0.003f+0.2f)*sin(frameCpt*0.008f+i*0.003f+0.2f)*sin(frameCpt*0.008f+i*0.003f+0.2f)));
                
                
                
                sx=1;
                sy=1;
                sz=spL*2+0.1f;
                x=-0.5f;
                z=dsz+spL/4;
                y=(i-nb_spectrum_bands/2)*sy*1.05f;
                
                glRotatef( curve_rate ,0,1,0);
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                glRotatef(90, 0, 1, 0);
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                glRotatef(90, 0, -1, 0);
                glRotatef(curve_rate,0,-1,0);
                
                
                /////////////////
                //RIGHT
                spR=barSpectrumDataR[i];
                
                if (i<nb_spectrum_bands*2/3) {
                    cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
                } else {
                    cbt=0.1;
                }
                if (i>nb_spectrum_bands/3) {
                    cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3)+0.1;
                } else {
                    cgt=0.1;
                }
                crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
                crt*=0.5+(spR/1);
                if (crt>1) crt=1;
                cgt*=0.5+(spR/1);
                if (cgt>1) cgt=1;
                cbt*=0.5+(spR/1);
                if (cbt>1) cbt=1;
                
                crt*=0.5;cgt*=0.5;cbt*=0.5;
                
                sx=1;
                sy=1;
                sz=spR*2+0.1f;
                x=-0.5f;
                z=dsz+spR/4;
                y=(i-nb_spectrum_bands/2)*sy*1.05f;
                
                glRotatef(180+curve_rate,0,1,0);
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                glRotatef(90, 0, 1, 0);
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                glRotatef(90, 0, -1, 0);
                glRotatef(180+curve_rate,0,-1,0);
            }
    }
    
    
    /* Disable Vertex Pointer */
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    
    glDisable(GL_LIGHT0);
    glDisable( GL_LIGHTING );
    glDisable(GL_COLOR_MATERIAL);
    
    
    //    glDisable(GL_BLEND);
    
    /* Pop The Matrix */
    glPopMatrix();
}


void RenderUtils::DrawSpectrum3D(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int nb_spectrum_bands) {
    GLfloat y,z,z2,spL,spR;
    GLfloat cr,cg,cb,tr,tb,tg;
    
    //////////////////////////////
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const float aspectRatio = (float)ww/(float)hh;
    const float _hw = 0.1f;
    const float _hh = _hw/aspectRatio;
    glFrustumf(-_hw, _hw, -_hh, _hh, 1.0f, (SPECTRUM_DEPTH-1)*SPECTRUM_ZSIZE*2+120.0f);
    
    glPushMatrix();                     /* Push The Modelview Matrix */
    
    glTranslatef(0.0, 0.0, -120.0);      /* Translate 50 Units Into The Screen */
    if ((mode==3)||(mode==6)) glRotatef(angle/30.0f, 0, 0, 1);
    if ((mode==2)||(mode==5)) glRotatef(90.0f, 0, 0, 1);
    
    
    //	glEnable(GL_BLEND);
    //	glBlendFunc(GL_ONE, GL_ONE);
    
    /* Begin Drawing Quads, setup vertex array pointer */
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glColorPointer(4, GL_FLOAT, 0, vertColor);
    /* Enable Vertex Pointer */
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    for (int i=0;i<nb_spectrum_bands;i++) {
        oldSpectrumDataL[SPECTRUM_DEPTH-1][i]=((float)spectrumDataL[i]/128.0f<24?(float)spectrumDataL[i]/128.0f:24);
        oldSpectrumDataR[SPECTRUM_DEPTH-1][i]=((float)spectrumDataR[i]/128.0f<24?(float)spectrumDataR[i]/128.0f:24);
    }
    vertColor[0][3]=vertColor[1][3]=vertColor[2][3]=vertColor[3][3]=1;
    for (int j=1;j<SPECTRUM_DEPTH;j++) {
        for (int i=0; i<nb_spectrum_bands; i++) {
            oldSpectrumDataL[j-1][i]=oldSpectrumDataL[j][i]*SPECTRUM_DECREASE_FACTOR;
            oldSpectrumDataR[j-1][i]=oldSpectrumDataR[j][i]*SPECTRUM_DECREASE_FACTOR;
            
            z=-(j-1)*(SPECTRUM_ZSIZE);
            
            if (mode<=3) z2=z-(SPECTRUM_ZSIZE+j)*0.9f;
            else z2=z*0.9f;
            
            
            if (z>0) z=0;
            if (z2>0) z2=0;
            
            y=SPECTRUM_Y;
            spL=oldSpectrumDataL[j][i];
            spR=oldSpectrumDataR[j][i];
            
            tg=spL*2/8;
            tb=spL*1/8;
            tr=spL*3/8;
            tr=tr-(tg+tb)/2;
            cr=tb/3;
            cg=tg/3;
            cb=tr;
            
            
            
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+0;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y+0;
            vertices[1][2]=z+0.0f;
            
            
            spL*=0.5f;
            cr=tb;
            cg=tr/3;
            cb=tg;
            
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y-spL;
            vertices[2][2]=z+0.0f;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y-spL;
            vertices[3][2]=z+0.0f;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            cr*=0.5f;
            cg*=0.5f;
            cb*=0.5f;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y-spL;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y-spL;
            vertices[1][2]=z+0.0f;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y-spL;
            vertices[2][2]=z2;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y-spL;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            cr*=0.5f;
            cg*=0.5f;
            cb*=0.5f;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+0;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y-spL;
            vertices[1][2]=z+0.0f;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y+0;
            vertices[2][2]=z2;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y-spL;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+0;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y-spL;
            vertices[1][2]=z+0.0f;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y+0;
            vertices[2][2]=z2;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y-spL;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            tg=spR*2/8;
            tb=spR*1/8;
            tr=spR*3/8;
            tr=tr-(tg+tb)/2;
            cr=tg/3;
            cg=tr/3;
            cb=tb;
            
            y=-SPECTRUM_Y;
            
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+0;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y+0;
            vertices[1][2]=z+0.0f;
            
            spR*=0.5f;
            cr=tg;
            cg=tb;
            cb=tb/3;
            
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y+spR;
            vertices[2][2]=z+0.0f;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y+spR;
            vertices[3][2]=z+0.0f;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            cr*=0.5f;
            cg*=0.5f;
            cb*=0.5f;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+spR;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y+spR;
            vertices[1][2]=z+0.0f;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y+spR;
            vertices[2][2]=z2;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y+spR;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            cr*=0.5f;
            cg*=0.5f;
            cb*=0.5f;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+0;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y+spR;
            vertices[1][2]=z+0.0f;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y+0;
            vertices[2][2]=z2;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y+spR;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+0;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y+spR;
            vertices[1][2]=z+0.0f;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y+0;
            vertices[2][2]=z2;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y+spR;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
        }
    }
    /* Disable Vertex Pointer */
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    
    //    glDisable(GL_BLEND);
    
    /* Pop The Matrix */
    glPopMatrix();
}

void RenderUtils::DrawSpectrumLandscape3D(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int nb_spectrum_bands) {
    GLfloat y,z,z2,spL,spR;
    GLfloat cr,cg,cb,tr,tb,tg;
    
    //////////////////////////////
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const float aspectRatio = (float)ww/(float)hh;
    const float _hw = 0.1f;
    const float _hh = _hw/aspectRatio;
    glFrustumf(-_hw, _hw, -_hh, _hh, 1.0f, (SPECTRUM_DEPTH-1)*SPECTRUM_ZSIZE*2+120.0f);
    
    glPushMatrix();                     /* Push The Modelview Matrix */
    
    glTranslatef(0.0, 0.0, -80.0);      /* Translate Into The Screen */
    if ((mode==3)||(mode==6)) glRotatef(angle/30.0f, 0, 0, 1);
    if ((mode==2)||(mode==5)) glRotatef(90.0f, 0, 0, 1);
    
    
    //	glEnable(GL_BLEND);
    //	glBlendFunc(GL_ONE, GL_ONE);
    
    /* Begin Drawing Quads, setup vertex array pointer */
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glColorPointer(4, GL_FLOAT, 0, vertColor);
    /* Enable Vertex Pointer */
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    for (int i=0;i<nb_spectrum_bands;i++) {
        oldSpectrumDataL[SPECTRUM_DEPTH-1][i]=((float)spectrumDataL[i]/128.0f<24?(float)spectrumDataL[i]/128.0f:24);
        oldSpectrumDataR[SPECTRUM_DEPTH-1][i]=((float)spectrumDataR[i]/128.0f<24?(float)spectrumDataR[i]/128.0f:24);
    }
    vertColor[0][3]=vertColor[1][3]=vertColor[2][3]=vertColor[3][3]=1;
    for (int j=1;j<SPECTRUM_DEPTH;j++) {
        for (int i=0; i<nb_spectrum_bands; i++) {
            oldSpectrumDataL[j-1][i]=oldSpectrumDataL[j][i];//*SPECTRUM_DECREASE_FACTOR;
            oldSpectrumDataR[j-1][i]=oldSpectrumDataR[j][i];//*SPECTRUM_DECREASE_FACTOR;
            
            z=-(j-1)*(SPECTRUM_ZSIZE);
            
            if (mode<=3) z2=z-(SPECTRUM_ZSIZE+j)*0.9f;
            else z2=z*0.9f;
            
            
            if (z>0) z=0;
            if (z2>0) z2=0;
            
            y=SPECTRUM_Y;
            spL=oldSpectrumDataL[j][i];
            spR=oldSpectrumDataR[j][i];
            
            //***********************************************************************
            //***********************************************************************
            //LEFT Channel
            //***********************************************************************
            //***********************************************************************
            
            
            tg=spL*2/8;
            tb=spL*1/8;
            tr=spL*3/8;
            tr=tr-(tg+tb)/2;
            cr=tb/3;
            cg=tg/3;
            cb=tr;
            
            
            
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+0;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y+0;
            vertices[1][2]=z+0.0f;
            
            
            spL*=0.5f;
            cr=tb;
            cg=tr/3;
            cb=tg;
            
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y-spL;
            vertices[2][2]=z+0.0f;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y-spL;
            vertices[3][2]=z+0.0f;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            cr*=0.5f;
            cg*=0.5f;
            cb*=0.5f;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y-spL;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y-spL;
            vertices[1][2]=z+0.0f;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y-spL;
            vertices[2][2]=z2;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y-spL;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            cr*=0.5f;
            cg*=0.5f;
            cb*=0.5f;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+0;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y-spL;
            vertices[1][2]=z+0.0f;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y+0;
            vertices[2][2]=z2;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y-spL;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            
            
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+0;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y-spL;
            vertices[1][2]=z+0.0f;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y+0;
            vertices[2][2]=z2;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y-spL;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            //***********************************************************************
            //***********************************************************************
            //RIGHT Channel
            //***********************************************************************
            //***********************************************************************
            
            
            tg=spR*2/8;
            tb=spR*1/8;
            tr=spR*3/8;
            tr=tr-(tg+tb)/2;
            cr=tg/3;
            cg=tr/3;
            cb=tb;
            
            y=-SPECTRUM_Y;
            
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+0;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y+0;
            vertices[1][2]=z+0.0f;
            
            spR*=0.5f;
            cr=tg;
            cg=tb;
            cb=tb/3;
            
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y+spR;
            vertices[2][2]=z+0.0f;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y+spR;
            vertices[3][2]=z+0.0f;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            cr*=0.5f;
            cg*=0.5f;
            cb*=0.5f;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+spR;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y+spR;
            vertices[1][2]=z+0.0f;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y+spR;
            vertices[2][2]=z2;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y+spR;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            cr*=0.5f;
            cg*=0.5f;
            cb*=0.5f;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+0;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y+spR;
            vertices[1][2]=z+0.0f;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y+0;
            vertices[2][2]=z2;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y+spR;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+0;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y+spR;
            vertices[1][2]=z+0.0f;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y+0;
            vertices[2][2]=z2;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y+spR;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
        }
    }
    /* Disable Vertex Pointer */
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    
    //    glDisable(GL_BLEND);
    
    /* Pop The Matrix */
    glPopMatrix();
}


static int sphSize=0;
static int sphMode=0;
static GLfloat sphVert[(SPECTRUM_BANDS/2)*(SPECTRUM_BANDS/2)*4*5][3];  /* Holds Float Info For 4 Sets Of Vertices */
static GLfloat sphNorm[(SPECTRUM_BANDS/2)*(SPECTRUM_BANDS/2)*5][3];  /* Holds Float Info For 4 Sets Of Vertices */


void RenderUtils::DrawSpectrum3DSphere(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int nb_spectrum_bands) {
    GLfloat x1,y1,z1,x2,y2,z2,x3,y3,z3,x4,y4,z4,spL,spR,ra1,rb1,ra2,rb2,r0;
    GLfloat xn,yn,zn,v1x,v1y,v1z,v2x,v2y,v2z,nn;
    int idxNorm,idxVert;
    short int *data;
    //////////////////////////////
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const float aspectRatio = (float)ww/(float)hh;
    const float _hw = 0.1f;
    const float _hh = _hw/aspectRatio;
    glFrustumf(-_hw, _hw, -_hh, _hh, 1.0f, 200.0f);
    
    //    glTranslatef(0.0, 0.0, 120.0);      /* Translate 50 Units Into The Screen */
    
    glEnable(GL_COLOR_MATERIAL);
    glEnable( GL_LIGHTING );
    glEnable(GL_LIGHT0);
    
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPushMatrix();                     /* Push The Modelview Matrix */
    
    glTranslatef(0.0, 0.0, -120.0);      /* Translate 50 Units Into The Screen */
    
    //    glShadeModel(GL_FLAT);
    glLightfv(GL_LIGHT0, GL_POSITION, position );
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 90);
    
    glRotatef(6*(sin(0.11*angle*M_PI/180)+0.3*sin(0.19*angle*M_PI/180)+0.7*sin(0.37*angle*M_PI/180)), 0, 0, 1);
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    
    
    /* Begin Drawing Quads, setup vertex array pointer */
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glNormalPointer(GL_FLOAT, 0, normals);
    /* Enable Vertex Pointer */
    
    float divisor=1;
    if (mode==1) divisor=1;
    if (mode==2) divisor=3;
    
    if ((sphSize!=nb_spectrum_bands)||(sphMode!=mode)) { //change of size, recompute sphere
        sphSize=nb_spectrum_bands;
        sphMode=mode;
        idxNorm=idxVert=0;
        for (int j=0;j<nb_spectrum_bands;j++)
            for (int i=0; i<nb_spectrum_bands; i++) {
                //compute radius
                ra1=(float)i*M_PI*2/nb_spectrum_bands;
                rb1=((float)j*M_PI/nb_spectrum_bands-M_PI_2)/divisor;
                ra2=(float)(i+1)*M_PI*2/nb_spectrum_bands;
                rb2=((float)(j+1)*M_PI/nb_spectrum_bands-M_PI_2)/divisor;
                //compute coord
                //TOP
                x1=cos(rb1)*cos(ra1);
                y1=sin(rb1);
                z1=-cos(rb1)*sin(ra1);
                x2=cos(rb1)*cos(ra2);
                y2=sin(rb1);
                z2=-cos(rb1)*sin(ra2);
                x3=cos(rb2)*cos(ra1);
                y3=sin(rb2);
                z3=-cos(rb2)*sin(ra1);
                x4=cos(rb2)*cos(ra2);
                y4=sin(rb2);
                z4=-cos(rb2)*sin(ra2);
                
                /*                v1x=x1-x2;
                 v1y=y1-y2;
                 v1z=z1-z2;
                 v2x=x2-0;
                 v2y=y2-0;
                 v2z=z2-0;
                 xn=v1y*v2z-v1z*v2y;
                 yn=v1z*v2x-v1x*v2z;
                 zn=v1x*v2y-v1y*v2x;
                 nn=sqrt(xn*xn+yn*yn+zn*zn);
                 xn/=nn;
                 yn/=nn;
                 zn/=nn;*/
                sphNorm[idxNorm][0]=x1;
                sphNorm[idxNorm][1]=y1;
                sphNorm[idxNorm++][2]=z1;
                sphVert[idxVert][0]=x1;
                sphVert[idxVert][1]=y1;
                sphVert[idxVert++][2]=z1;
                sphVert[idxVert][0]=x2;
                sphVert[idxVert][1]=y2;
                sphVert[idxVert++][2]=z2;
                sphVert[idxVert][0]=x3;
                sphVert[idxVert][1]=y3;
                sphVert[idxVert++][2]=z3;
                sphVert[idxVert][0]=x4;
                sphVert[idxVert][1]=y4;
                sphVert[idxVert++][2]=z4;
                
                //LEFT
                x1=cos(rb1)*cos(ra1);
                y1=sin(rb1);
                z1=-cos(rb1)*sin(ra1);
                x2=cos(rb2)*cos(ra1);
                y2=sin(rb2);
                z2=-cos(rb2)*sin(ra1);
                x3=cos(rb1)*cos(ra1);
                y3=sin(rb1);
                z3=-cos(rb1)*sin(ra1);
                x4=cos(rb2)*cos(ra1);
                y4=sin(rb2);
                z4=-cos(rb2)*sin(ra1);
                
                v1x=x1-x2;
                v1y=y1-y2;
                v1z=z1-z2;
                v2x=x2-0;
                v2y=y2-0;
                v2z=z2-0;
                xn=v1y*v2z-v1z*v2y;
                yn=v1z*v2x-v1x*v2z;
                zn=v1x*v2y-v1y*v2x;
                nn=sqrt(xn*xn+yn*yn+zn*zn);
                xn/=nn;
                yn/=nn;
                zn/=nn;
                
                sphNorm[idxNorm][0]=xn;
                sphNorm[idxNorm][1]=yn;
                sphNorm[idxNorm++][2]=zn;
                sphVert[idxVert][0]=x1;
                sphVert[idxVert][1]=y1;
                sphVert[idxVert++][2]=z1;
                sphVert[idxVert][0]=x2;
                sphVert[idxVert][1]=y2;
                sphVert[idxVert++][2]=z2;
                sphVert[idxVert][0]=x3;
                sphVert[idxVert][1]=y3;
                sphVert[idxVert++][2]=z3;
                sphVert[idxVert][0]=x4;
                sphVert[idxVert][1]=y4;
                sphVert[idxVert++][2]=z4;
                
                //RIGHT
                x1=cos(rb1)*cos(ra2);
                y1=sin(rb1);
                z1=-cos(rb1)*sin(ra2);
                x2=cos(rb2)*cos(ra2);
                y2=sin(rb2);
                z2=-cos(rb2)*sin(ra2);
                x3=cos(rb1)*cos(ra2);
                y3=sin(rb1);
                z3=-cos(rb1)*sin(ra2);
                x4=cos(rb2)*cos(ra2);
                y4=sin(rb2);
                z4=-cos(rb2)*sin(ra2);
                
                v1x=x1-x2;
                v1y=y1-y2;
                v1z=z1-z2;
                v2x=x2-0;
                v2y=y2-0;
                v2z=z2-0;
                xn=v1y*v2z-v1z*v2y;
                yn=v1z*v2x-v1x*v2z;
                zn=v1x*v2y-v1y*v2x;
                nn=sqrt(xn*xn+yn*yn+zn*zn);
                xn/=-nn;
                yn/=-nn;
                zn/=-nn;
                
                
                sphNorm[idxNorm][0]=xn;
                sphNorm[idxNorm][1]=yn;
                sphNorm[idxNorm++][2]=zn;
                sphVert[idxVert][0]=x1;
                sphVert[idxVert][1]=y1;
                sphVert[idxVert++][2]=z1;
                sphVert[idxVert][0]=x2;
                sphVert[idxVert][1]=y2;
                sphVert[idxVert++][2]=z2;
                sphVert[idxVert][0]=x3;
                sphVert[idxVert][1]=y3;
                sphVert[idxVert++][2]=z3;
                sphVert[idxVert][0]=x4;
                sphVert[idxVert][1]=y4;
                sphVert[idxVert++][2]=z4;
                
                //FRONT
                x1=cos(rb1)*cos(ra1);
                y1=sin(rb1);
                z1=-cos(rb1)*sin(ra1);
                x2=cos(rb1)*cos(ra2);
                y2=sin(rb1);
                z2=-cos(rb1)*sin(ra2);
                x3=cos(rb1)*cos(ra1);
                y3=sin(rb1);
                z3=-cos(rb1)*sin(ra1);
                x4=cos(rb1)*cos(ra2);
                y4=sin(rb1);
                z4=-cos(rb1)*sin(ra2);
                
                v1x=x1-x2;
                v1y=y1-y2;
                v1z=z1-z2;
                v2x=x2-0;
                v2y=y2-0;
                v2z=z2-0;
                xn=v1y*v2z-v1z*v2y;
                yn=v1z*v2x-v1x*v2z;
                zn=v1x*v2y-v1y*v2x;
                nn=sqrt(xn*xn+yn*yn+zn*zn);
                xn/=-nn;
                yn/=-nn;
                zn/=-nn;
                
                sphNorm[idxNorm][0]=xn;
                sphNorm[idxNorm][1]=yn;
                sphNorm[idxNorm++][2]=zn;
                sphVert[idxVert][0]=x1;
                sphVert[idxVert][1]=y1;
                sphVert[idxVert++][2]=z1;
                sphVert[idxVert][0]=x2;
                sphVert[idxVert][1]=y2;
                sphVert[idxVert++][2]=z2;
                sphVert[idxVert][0]=x3;
                sphVert[idxVert][1]=y3;
                sphVert[idxVert++][2]=z3;
                sphVert[idxVert][0]=x4;
                sphVert[idxVert][1]=y4;
                sphVert[idxVert++][2]=z4;
                
                //BACK
                x1=cos(rb2)*cos(ra1);
                y1=sin(rb2);
                z1=-cos(rb2)*sin(ra1);
                x2=cos(rb2)*cos(ra2);
                y2=sin(rb2);
                z2=-cos(rb2)*sin(ra2);
                x3=cos(rb2)*cos(ra1);
                y3=sin(rb2);
                z3=-cos(rb2)*sin(ra1);
                x4=cos(rb2)*cos(ra2);
                y4=sin(rb2);
                z4=-cos(rb2)*sin(ra2);
                
                v1x=x1-x2;
                v1y=y1-y2;
                v1z=z1-z2;
                v2x=x2-0;
                v2y=y2-0;
                v2z=z2-0;
                xn=v1y*v2z-v1z*v2y;
                yn=v1z*v2x-v1x*v2z;
                zn=v1x*v2y-v1y*v2x;
                nn=sqrt(xn*xn+yn*yn+zn*zn);
                xn/=nn;
                yn/=nn;
                zn/=nn;
                
                sphNorm[idxNorm][0]=xn;
                sphNorm[idxNorm][1]=yn;
                sphNorm[idxNorm++][2]=zn;
                sphVert[idxVert][0]=x1;
                sphVert[idxVert][1]=y1;
                sphVert[idxVert++][2]=z1;
                sphVert[idxVert][0]=x2;
                sphVert[idxVert][1]=y2;
                sphVert[idxVert++][2]=z2;
                sphVert[idxVert][0]=x3;
                sphVert[idxVert][1]=y3;
                sphVert[idxVert++][2]=z3;
                sphVert[idxVert][0]=x4;
                sphVert[idxVert][1]=y4;
                sphVert[idxVert++][2]=z4;
            }
        
    }
    
    
    glColor4f(1,1,1,1);
    for (int k=0;k<2;k++) {
        glPushMatrix();                     /* Push The Modelview Matrix */
        if (k==0) {
            r0=(float)spectrumDataL[0]/1800.0f;
            if (r0<1) r0=1;
            if (r0>1.1) r0=1.1;
            spectrumDataL[0]=spectrumDataL[1];
            data=spectrumDataL;
            glTranslatef(-6.0, 0.0, 0.0);
            glRotatef(angle/17.0f, 1, 0, 0);
            glRotatef(angle/7.0f, 0, 1, 0);
            glRotatef(angle/3.0f, 0, 0, 1);
            
            glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight[0]);
            glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight[0]);
            glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight[0] );
            
            
            
        } else {
            r0=(float)spectrumDataR[0]/1800.0f;
            if (r0<1) r0=1;
            if (r0>1.1) r0=1.1;
            spectrumDataR[0]=spectrumDataR[1];
            data=spectrumDataR;
            glTranslatef(6.0, 0.0, 0.0);
            glRotatef(angle/13.0f, -1, 0, 0);
            glRotatef(angle/7.0f, 0, -1, 0);
            glRotatef(angle/17.0f, 0, 0, 1);
            
            glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight[1]);
            glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight[1]);
            glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight[1] );
            
            
            
        }
        idxNorm=idxVert=0;
        for (int j=0;j<nb_spectrum_bands;j++)
            for (int i=0; i<nb_spectrum_bands; i++) {
                spL=(float)spectrumDataL[j]/500.0f;
                if (spL>1.5) spL=1.5;
                
                if ((i+j)&1) spL=0;
                
                //UP
                normals[0][0]=sphNorm[idxNorm][0];
                normals[0][1]=sphNorm[idxNorm][1];
                normals[0][2]=sphNorm[idxNorm][2];
                vertices[0][0]=sphVert[idxVert][0]*(4+spL)*r0;
                vertices[0][1]=sphVert[idxVert][1]*(4+spL)*r0;
                vertices[0][2]=sphVert[idxVert++][2]*(4+spL)*r0;
                
                normals[1][0]=sphNorm[idxNorm][0];
                normals[1][1]=sphNorm[idxNorm][1];
                normals[1][2]=sphNorm[idxNorm][2];
                vertices[1][0]=sphVert[idxVert][0]*(4+spL)*r0;
                vertices[1][1]=sphVert[idxVert][1]*(4+spL)*r0;
                vertices[1][2]=sphVert[idxVert++][2]*(4+spL)*r0;
                
                normals[2][0]=sphNorm[idxNorm][0];
                normals[2][1]=sphNorm[idxNorm][1];
                normals[2][2]=sphNorm[idxNorm][2];
                vertices[2][0]=sphVert[idxVert][0]*(4+spL)*r0;
                vertices[2][1]=sphVert[idxVert][1]*(4+spL)*r0;
                vertices[2][2]=sphVert[idxVert++][2]*(4+spL)*r0;
                
                normals[3][0]=sphNorm[idxNorm][0];
                normals[3][1]=sphNorm[idxNorm][1];
                normals[3][2]=sphNorm[idxNorm++][2];
                vertices[3][0]=sphVert[idxVert][0]*(4+spL)*r0;
                vertices[3][1]=sphVert[idxVert][1]*(4+spL)*r0;
                vertices[3][2]=sphVert[idxVert++][2]*(4+spL)*r0;
                
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                
                //LEFT
                normals[0][0]=sphNorm[idxNorm][0];
                normals[0][1]=sphNorm[idxNorm][1];
                normals[0][2]=sphNorm[idxNorm][2];
                vertices[0][0]=sphVert[idxVert][0]*(4+spL)*r0;
                vertices[0][1]=sphVert[idxVert][1]*(4+spL)*r0;
                vertices[0][2]=sphVert[idxVert++][2]*(4+spL)*r0;
                
                normals[1][0]=sphNorm[idxNorm][0];
                normals[1][1]=sphNorm[idxNorm][1];
                normals[1][2]=sphNorm[idxNorm][2];
                vertices[1][0]=sphVert[idxVert][0]*(4+spL)*r0;
                vertices[1][1]=sphVert[idxVert][1]*(4+spL)*r0;
                vertices[1][2]=sphVert[idxVert++][2]*(4+spL)*r0;
                
                normals[2][0]=sphNorm[idxNorm][0];
                normals[2][1]=sphNorm[idxNorm][1];
                normals[2][2]=sphNorm[idxNorm][2];
                vertices[2][0]=sphVert[idxVert][0]*(4)*r0;
                vertices[2][1]=sphVert[idxVert][1]*(4)*r0;
                vertices[2][2]=sphVert[idxVert++][2]*(4)*r0;
                
                normals[3][0]=sphNorm[idxNorm][0];
                normals[3][1]=sphNorm[idxNorm][1];
                normals[3][2]=sphNorm[idxNorm++][2];
                vertices[3][0]=sphVert[idxVert][0]*(4)*r0;
                vertices[3][1]=sphVert[idxVert][1]*(4)*r0;
                vertices[3][2]=sphVert[idxVert++][2]*(4)*r0;
                
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                
                
                //RIGHT
                normals[0][0]=sphNorm[idxNorm][0];
                normals[0][1]=sphNorm[idxNorm][1];
                normals[0][2]=sphNorm[idxNorm][2];
                vertices[0][0]=sphVert[idxVert][0]*(4+spL)*r0;
                vertices[0][1]=sphVert[idxVert][1]*(4+spL)*r0;
                vertices[0][2]=sphVert[idxVert++][2]*(4+spL)*r0;
                
                normals[1][0]=sphNorm[idxNorm][0];
                normals[1][1]=sphNorm[idxNorm][1];
                normals[1][2]=sphNorm[idxNorm][2];
                vertices[1][0]=sphVert[idxVert][0]*(4+spL)*r0;
                vertices[1][1]=sphVert[idxVert][1]*(4+spL)*r0;
                vertices[1][2]=sphVert[idxVert++][2]*(4+spL)*r0;
                
                normals[2][0]=sphNorm[idxNorm][0];
                normals[2][1]=sphNorm[idxNorm][1];
                normals[2][2]=sphNorm[idxNorm][2];
                vertices[2][0]=sphVert[idxVert][0]*(4)*r0;
                vertices[2][1]=sphVert[idxVert][1]*(4)*r0;
                vertices[2][2]=sphVert[idxVert++][2]*(4)*r0;
                
                normals[3][0]=sphNorm[idxNorm][0];
                normals[3][1]=sphNorm[idxNorm][1];
                normals[3][2]=sphNorm[idxNorm++][2];
                vertices[3][0]=sphVert[idxVert][0]*(4)*r0;
                vertices[3][1]=sphVert[idxVert][1]*(4)*r0;
                vertices[3][2]=sphVert[idxVert++][2]*(4)*r0;
                
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                
                //FRONT
                normals[0][0]=sphNorm[idxNorm][0];
                normals[0][1]=sphNorm[idxNorm][1];
                normals[0][2]=sphNorm[idxNorm][2];
                vertices[0][0]=sphVert[idxVert][0]*(4+spL)*r0;
                vertices[0][1]=sphVert[idxVert][1]*(4+spL)*r0;
                vertices[0][2]=sphVert[idxVert++][2]*(4+spL)*r0;
                
                normals[1][0]=sphNorm[idxNorm][0];
                normals[1][1]=sphNorm[idxNorm][1];
                normals[1][2]=sphNorm[idxNorm][2];
                vertices[1][0]=sphVert[idxVert][0]*(4+spL)*r0;
                vertices[1][1]=sphVert[idxVert][1]*(4+spL)*r0;
                vertices[1][2]=sphVert[idxVert++][2]*(4+spL)*r0;
                
                normals[2][0]=sphNorm[idxNorm][0];
                normals[2][1]=sphNorm[idxNorm][1];
                normals[2][2]=sphNorm[idxNorm][2];
                vertices[2][0]=sphVert[idxVert][0]*(4)*r0;
                vertices[2][1]=sphVert[idxVert][1]*(4)*r0;
                vertices[2][2]=sphVert[idxVert++][2]*(4)*r0;
                
                normals[3][0]=sphNorm[idxNorm][0];
                normals[3][1]=sphNorm[idxNorm][1];
                normals[3][2]=sphNorm[idxNorm++][2];
                vertices[3][0]=sphVert[idxVert][0]*(4)*r0;
                vertices[3][1]=sphVert[idxVert][1]*(4)*r0;
                vertices[3][2]=sphVert[idxVert++][2]*(4)*r0;
                
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                
                //BACK
                normals[0][0]=sphNorm[idxNorm][0];
                normals[0][1]=sphNorm[idxNorm][1];
                normals[0][2]=sphNorm[idxNorm][2];
                vertices[0][0]=sphVert[idxVert][0]*(4+spL)*r0;
                vertices[0][1]=sphVert[idxVert][1]*(4+spL)*r0;
                vertices[0][2]=sphVert[idxVert++][2]*(4+spL)*r0;
                
                normals[1][0]=sphNorm[idxNorm][0];
                normals[1][1]=sphNorm[idxNorm][1];
                normals[1][2]=sphNorm[idxNorm][2];
                vertices[1][0]=sphVert[idxVert][0]*(4+spL)*r0;
                vertices[1][1]=sphVert[idxVert][1]*(4+spL)*r0;
                vertices[1][2]=sphVert[idxVert++][2]*(4+spL)*r0;
                
                normals[2][0]=sphNorm[idxNorm][0];
                normals[2][1]=sphNorm[idxNorm][1];
                normals[2][2]=sphNorm[idxNorm][2];
                vertices[2][0]=sphVert[idxVert][0]*(4)*r0;
                vertices[2][1]=sphVert[idxVert][1]*(4)*r0;
                vertices[2][2]=sphVert[idxVert++][2]*(4)*r0;
                
                normals[3][0]=sphNorm[idxNorm][0];
                normals[3][1]=sphNorm[idxNorm][1];
                normals[3][2]=sphNorm[idxNorm++][2];
                vertices[3][0]=sphVert[idxVert][0]*(4)*r0;
                vertices[3][1]=sphVert[idxVert][1]*(4)*r0;
                vertices[3][2]=sphVert[idxVert++][2]*(4)*r0;
                
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                
                
            }
        glPopMatrix();
    }
    /* Disable Vertex Pointer */
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    
    //    glDisable(GL_BLEND);
    
    /* Pop The Matrix */
    glPopMatrix();
    
    glDisable(GL_LIGHT0);
    glDisable( GL_LIGHTING );
    glDisable(GL_COLOR_MATERIAL);
    
}


void RenderUtils::DrawSpectrum3DMorph(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int nb_spectrum_bands) {
    GLfloat x1,x2,x3,x4,y1,y2,y3,y4,z1,z2,spL,spR;
    GLfloat cr,cg,cb,tr,tg,tb;
    //////////////////////////////
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    //	glEnable(GL_BLEND);
    //	glBlendFunc(GL_ONE, GL_ONE);
    
    
    const float aspectRatio = (float)ww/(float)hh;
    const float _hw = 0.1f;
    const float _hh = _hw/aspectRatio;
    glFrustumf(-_hw, _hw, -_hh, _hh, 1.0f, (SPECTRUM_DEPTH-1)*SPECTRUM_ZSIZE+220.0f);
    glPushMatrix();                     /* Push The Modelview Matrix */
    glTranslatef(0.0, 0.0, -180.0);      /* Translate 50 Units Into The Screen */
    if ((mode==3)||(mode==6)) glRotatef(angle/30.0f, 0, 0, 1);
    if ((mode==2)||(mode==5)) glRotatef(90.0f, 0, 0, 1);
    /* Begin Drawing Quads, setup vertex array pointer */
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glColorPointer(4, GL_FLOAT, 0, vertColor);
    /* Enable Vertex Pointer */
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    for (int i=0;i<nb_spectrum_bands;i++) {
        oldSpectrumDataL[SPECTRUM_DEPTH-1][i]=((float)spectrumDataL[i]/128.0f<24?(float)spectrumDataL[i]/128.0f:24);
        oldSpectrumDataR[SPECTRUM_DEPTH-1][i]=((float)spectrumDataR[i]/128.0f<24?(float)spectrumDataR[i]/128.0f:24);
    }
    
    vertColor[0][3]=vertColor[1][3]=vertColor[2][3]=vertColor[3][3]=1;
    
    for (int j=1;j<SPECTRUM_DEPTH;j++) {
        for (int i=0; i<nb_spectrum_bands; i++) {
            oldSpectrumDataL[j-1][i]=oldSpectrumDataL[j][i]*SPECTRUM_DECREASE_FACTOR;
            oldSpectrumDataR[j-1][i]=oldSpectrumDataR[j][i]*SPECTRUM_DECREASE_FACTOR;
            z1=-(j-1)*(SPECTRUM_ZSIZE);
            if (mode<=3) z2=z1-(SPECTRUM_ZSIZE)*0.9f;
            else z2=z1*0.9f;
            if (z1>0) z1=0;
            if (z2>0) z2=0;
            spL=oldSpectrumDataL[j][i];
            spR=oldSpectrumDataR[j][i];
            tg=spR*2/8; if (tg<0) tg=0; if (tg>255) tg=255;
            tb=spR*1/8; if (tb<0) tb=0; if (tb>255) tb=255;
            tr=spR*3/8; if (tr<0) tr=0; if (tr>255) tr=255;
            tr=tr-(tg+tb)/2;if (tr<0) tr=0;
            cr=tg/3;
            cg=tr/3;
            cb=tb;
            
            x1=(25)*cos( (((float)i+0.0f)/(nb_spectrum_bands))*3.146);
            x3=(25)*cos( (((float)i+1.0f)/(nb_spectrum_bands))*3.146);
            
            x2=(25-spL)*cos( (((float)i+0.5f)/(nb_spectrum_bands))*3.146)+(x1-x3)/2;//(25-spL)*cos( (((float)i+0.0f)/(nb_spectrum_bands))*3.146);
            x4=(25-spL)*cos( (((float)i+0.5f)/(nb_spectrum_bands))*3.146)-(x1-x3)/2;//(25-spL)*cos( (((float)i+1.0f)/(nb_spectrum_bands))*3.146);
            
            y1=(25)*sin( (((float)i+0.0f)/(nb_spectrum_bands))*3.146 );
            y3=(25)*sin( (((float)i+1.0f)/(nb_spectrum_bands))*3.146 );
            
            y2=(25-spL)*sin( (((float)i+0.5f)/(nb_spectrum_bands))*3.146 )+(y1-y3)/2;//(25-spL)*sin( (((float)i+0.0f)/(nb_spectrum_bands))*3.146 );
            y4=(25-spL)*sin( (((float)i+0.5f)/(nb_spectrum_bands))*3.146 )-(y1-y3)/2;//(25-spL)*sin( (((float)i+1.0f)/(nb_spectrum_bands))*3.146 );
            
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=x1;
            vertices[0][1]=y1;
            vertices[0][2]=z1;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=x3;
            vertices[1][1]=y3;
            vertices[1][2]=z1;
            cr=tb;
            cg=tr/3;
            cb=tg;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=x2;
            vertices[2][1]=y2;
            vertices[2][2]=z1;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=x4;
            vertices[3][1]=y4;
            vertices[3][2]=z1;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            cr*=0.25f;
            cg*=0.25f;
            cb*=0.25f;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=x2;
            vertices[0][1]=y2;
            vertices[0][2]=z1;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=x2;
            vertices[1][1]=y2;
            vertices[1][2]=z2;
            
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=x4;
            vertices[2][1]=y4;
            vertices[2][2]=z1;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=x4;
            vertices[3][1]=y4;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            
            tg=spR*2/8; if (tg<0) tg=0; if (tg>255) tg=255;
            tb=spR*1/8; if (tb<0) tb=0; if (tb>255) tb=255;
            tr=spR*3/8; if (tr<0) tr=0; if (tr>255) tr=255;
            tr=tr-(tg+tb)/2;if (tr<0) tr=0;
            cr=tg/3;
            cg=tr/3;
            cb=tb;
            
            
            /*			x1=(25)*cos( (((float)i+0.0f)/(nb_spectrum_bands))*3.146);
             x2=(25-spR)*cos( (((float)i+0.0f)/(nb_spectrum_bands))*3.146);
             x3=(25)*cos( (((float)i+1.0f)/(nb_spectrum_bands))*3.146);
             x4=(25-spR)*cos( (((float)i+1.0f)/(nb_spectrum_bands))*3.146);
             y1=-(25)*sin( (((float)i+0.0f)/(nb_spectrum_bands))*3.146 );
             y2=-(25-spR)*sin( (((float)i+0.0f)/(nb_spectrum_bands))*3.146 );
             y3=-(25)*sin( (((float)i+1.0f)/(nb_spectrum_bands))*3.146 );
             y4=-(25-spR)*sin( (((float)i+1.0f)/(nb_spectrum_bands))*3.146 );*/
            
            x1=(25)*cos( (((float)i+0.0f)/(nb_spectrum_bands))*3.146);
            x3=(25)*cos( (((float)i+1.0f)/(nb_spectrum_bands))*3.146);
            
            x2=(25-spR)*cos( (((float)i+0.5f)/(nb_spectrum_bands))*3.146)+(x1-x3)/2;//(25-spL)*cos( (((float)i+0.0f)/(nb_spectrum_bands))*3.146);
            x4=(25-spR)*cos( (((float)i+0.5f)/(nb_spectrum_bands))*3.146)-(x1-x3)/2;//(25-spL)*cos( (((float)i+1.0f)/(nb_spectrum_bands))*3.146);
            
            y1=-(25)*sin( (((float)i+0.0f)/(nb_spectrum_bands))*3.146 );
            y3=-(25)*sin( (((float)i+1.0f)/(nb_spectrum_bands))*3.146 );
            
            y2=-(25-spR)*sin( (((float)i+0.5f)/(nb_spectrum_bands))*3.146 )+(y1-y3)/2;//(25-spL)*sin( (((float)i+0.0f)/(nb_spectrum_bands))*3.146 );
            y4=-(25-spR)*sin( (((float)i+0.5f)/(nb_spectrum_bands))*3.146 )-(y1-y3)/2;//(25-spL)*sin( (((float)i+1.0f)/(nb_spectrum_bands))*3.146 );
            
            
            
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=x1;
            vertices[0][1]=y1;
            vertices[0][2]=z1;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=x3;
            vertices[1][1]=y3;
            vertices[1][2]=z1;
            cr=tg;
            cg=tb;
            cb=tb/3;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=x2;
            vertices[2][1]=y2;
            vertices[2][2]=z1;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=x4;
            vertices[3][1]=y4;
            vertices[3][2]=z1;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            cr*=0.25f;
            cg*=0.25f;
            cb*=0.25f;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=x2;
            vertices[0][1]=y2;
            vertices[0][2]=z1;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=x2;
            vertices[1][1]=y2;
            vertices[1][2]=z2;
            
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=x4;
            vertices[2][1]=y4;
            vertices[2][2]=z1;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=x4;
            vertices[3][1]=y4;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
        }
    }
    /* Disable Vertex Pointer */
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    
    //	glDisable(GL_BLEND);
    
    
    /* Pop The Matrix */
    glPopMatrix();
}

#define MIDIFX_LEN 128
int data_midifx_len=MIDIFX_LEN;
unsigned char data_midifx_note[MIDIFX_LEN][256];
unsigned char data_midifx_instr[MIDIFX_LEN][256];
unsigned char data_midifx_vol[MIDIFX_LEN][256];
unsigned char data_midifx_st[MIDIFX_LEN][256];
int data_midifx_first=1;

int data_pianofx_len=MIDIFX_LEN;
unsigned char data_pianofx_note[MIDIFX_LEN][256];
unsigned char data_pianofx_instr[MIDIFX_LEN][256];
unsigned char data_pianofx_vol[MIDIFX_LEN][256];
unsigned char data_pianofx_st[MIDIFX_LEN][256];
int data_pianofx_first=1;



#define VOICE_FREE	(1<<0)
#define VOICE_ON	(1<<1)
#define VOICE_SUSTAINED	(1<<2)
#define VOICE_OFF	(1<<3)
#define VOICE_DIE	(1<<4)

unsigned int data_midifx_col[16]={
    /*    0x8010E7,0x5D3E79,0x29004D,0xBF7BFD,0xE7CFFD,
     0xFF4500,0x865340,0x551700,0xFF9872,0xFFDBCE,
     0x00E87F,0x3A7A5D,0x004E2A,0x71FDBD,0xCCFDE6,
     0xFFF200*/
    //0x868240,0x555100,0xFFF872,0xFFFDCE
    
    0xFF5512,0x761AFF,0x21ff94,0xffb129,
    0xcb30ff,0x38ffe4,0xfffc40,0xff47ed,
    0x4fd9ff,0xc7ff57,0xff5eb7,0x66a8ff,
    0x9cff6e,0xff7591,0x7d88ff,0x85ff89
};

unsigned char piano_key[12]={0,1,0,1,0,0,1,0,1,0,1,0};
unsigned char piano_key_state[128];
unsigned char piano_key_instr[128];

//extern int texturePiano;

void RenderUtils::DrawPiano3D(int *data,uint ww,uint hh,int fx_len,int automove,float posx,float posy,float posz,float rotx,float roty,int color_mode) {
    int index;
    float key_length,key_lengthBL,key_height,key_heightBL;
    float key_leftpos;
    static int piano_fxcpt;
    static int first_call=1;
    
    if (first_call) {
        memset(piano_key_state,0,128);
        memset(piano_key_instr,0,128);
        first_call=0;
        piano_fxcpt=arc4random()&0xFFF;
    }
    piano_fxcpt++;
    
    GLfloat yf,yn,ynBL,z,yadj;
    GLfloat cr,cg,cb,crt,cgt,cbt;
    
    //////////////////////////////
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const float aspectRatio = (float)ww/(float)hh;
    const float _hw = 52.0/2/16;//0.2f;
    const float _hh = _hw/aspectRatio;
    glFrustumf(-_hw, _hw, -_hh, _hh, 100.0f, 10000.0f);
    
    glPushMatrix();                     /* Push The Modelview Matrix */
    
    if (automove) {
        glTranslatef(0.0, 0.0, -100.0*11);      /* Translate 50 Units Into The Screen */
        
        glRotatef(5.0f*(0.8f*sin((float)piano_fxcpt*3.14159f/769)+
                        0.5f*sin((float)piano_fxcpt*3.14159f/229)+
                        0.3f*sin((float)piano_fxcpt*3.14159f/311)), 0, 1, 0);
        
        glRotatef(30+15.0f*(0.4f*sin((float)piano_fxcpt*3.14159f/191)+
                            0.7f*sin((float)piano_fxcpt*3.14159f/911)+
                            0.3f*sin((float)piano_fxcpt*3.14159f/409)), 1, 0, 0);
    } else {
        glTranslatef(posx,posy,posz-100*12);
        glRotatef(30+rotx, 1, 0, 0);
        glRotatef(roty, 0, 1, 0);
    }
    
    
    
    if (fx_len!=data_pianofx_len) {
        data_pianofx_len=fx_len;
        data_pianofx_first=1;
    }
    
    
    //if first launch, clear buffers
    if (data_pianofx_first) {
        data_pianofx_first=0;
        for (int i=0;i<data_pianofx_len;i++) {
            memset(data_pianofx_note[i],0,256);
        }
    }
    //update old ones
    for (int j=0;j<data_pianofx_len-1;j++) {
        memcpy(data_pianofx_note[j],data_pianofx_note[j+1],256);
        memcpy(data_pianofx_instr[j],data_pianofx_instr[j+1],256);
        memcpy(data_pianofx_vol[j],data_pianofx_vol[j+1],256);
        memcpy(data_pianofx_st[j],data_pianofx_st[j+1],256);
    }
    //add new one
    for (int i=0;i<256;i++) {
        int note=data[i];
        data_pianofx_note[data_pianofx_len-1][i]=note&0xFF;
        data_pianofx_instr[data_pianofx_len-1][i]=(note>>8)&0xFF;
        data_pianofx_st[data_pianofx_len-1][i]=(note>>24)&0xFF;
        data_pianofx_vol[data_pianofx_len-1][i]=(note>>16)&0xFF;
        
    }
    
    
    int j=data_pianofx_len-1-(data_pianofx_len/2);//MIDIFX_OFS;
    //glLineWidth(line_width+2);
    index=0;
    for (int i=0; i<256; i++) {
        if (data_pianofx_note[j][i]) {
            int instr=data_pianofx_instr[j][i];
            int vol=data_pianofx_vol[j][i];
            int st=data_pianofx_st[j][i];
            
            if (vol&&(st&VOICE_ON)) {
                //note pressed
                piano_key_state[(data_pianofx_note[j][i])&127]=4;
                piano_key_instr[(data_pianofx_note[j][i])&127]=instr;
            }
        }
    }
    
    yadj=0.01;
    
#define PIANO3D_DRAWKEY \
if (piano_key_state[i+k]) { \
yn=yf-key_height*4/5*piano_key_state[i+k]/4; \
ynBL=yf-key_heightBL*3/5*piano_key_state[i+k]/4; \
piano_key_state[i+k]--; \
} else { \
yn=ynBL=yf; \
} \
if (piano_ofs==12) piano_ofs=0; \
if (piano_key[piano_ofs]==0) { /*white key*/ \
if (piano_key_state[i+k]) { \
crt=(0.6f*piano_key_state[i+k]+1.0f*(4-piano_key_state[i+k]))/4; \
cgt=(0.6f*piano_key_state[i+k]+1.0f*(4-piano_key_state[i+k]))/4; \
cbt=(1.0f*piano_key_state[i+k]+1.0f*(4-piano_key_state[i+k]))/4; \
} else crt=cgt=cbt=1.0f; \
/*Key / Up Face*/ \
cr=crt;cg=cgt;cb=cbt;\
vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb; \
vertices[0][0]=(float)(white_idx-key_leftpos+0.05f); \
vertices[0][1]=yn+yadj; \
vertices[0][2]=z+0.1f;  \
vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb; \
vertices[1][0]=(float)(white_idx-key_leftpos+0.05f); \
vertices[1][1]=yf+yadj; \
vertices[1][2]=z-key_length;  \
vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb; \
vertices[2][0]=(float)(white_idx-key_leftpos+0.95f); \
vertices[2][1]=yn+yadj; \
vertices[2][2]=z+0.1f;  \
vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb; \
vertices[3][0]=(float)(white_idx-key_leftpos+0.95f); \
vertices[3][1]=yf+yadj; \
vertices[3][2]=z-key_length;  \
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); \
/*Key / Down Face*/ \
cr=crt*0.4;cg=cgt*0.4;cb=cbt*0.4; \
vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb; \
vertices[0][0]=(float)(white_idx-key_leftpos+0.05f); \
vertices[0][1]=yn-key_height; \
vertices[0][2]=z; \
vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb; \
vertices[1][0]=(float)(white_idx-key_leftpos+0.05f); \
vertices[1][1]=yf-key_height; \
vertices[1][2]=z-key_length; \
vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb; \
vertices[2][0]=(float)(white_idx-key_leftpos+0.95f); \
vertices[2][1]=yn-key_height; \
vertices[2][2]=z; \
vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb; \
vertices[3][0]=(float)(white_idx-key_leftpos+0.95f); \
vertices[3][1]=yf-key_height; \
vertices[3][2]=z-key_length; \
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); \
/*Key / Front Face*/ \
cr=crt*0.6f;cg=cgt*0.6f;cb=cbt*0.6f; \
vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb; \
vertices[0][0]=(float)(white_idx-key_leftpos+0.10f); \
vertices[0][1]=yn-key_height; \
vertices[0][2]=z;  \
vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb; \
vertices[1][0]=(float)(white_idx-key_leftpos+0.10f); \
vertices[1][1]=yn+0; \
vertices[1][2]=z;   \
vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb; \
vertices[2][0]=(float)(white_idx-key_leftpos+0.90f); \
vertices[2][1]=yn-key_height; \
vertices[2][2]=z;  \
vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb; \
vertices[3][0]=(float)(white_idx-key_leftpos+0.90f); \
vertices[3][1]=yn; \
vertices[3][2]=z;   \
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); \
/*Key / Back Face*/ \
vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb; \
vertices[0][0]=(float)(white_idx-key_leftpos+0.10f); \
vertices[0][1]=yf-key_height; \
vertices[0][2]=z-key_length; \
vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb; \
vertices[1][0]=(float)(white_idx-key_leftpos+0.10f); \
vertices[1][1]=yf+0; \
vertices[1][2]=z-key_length; \
vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb; \
vertices[2][0]=(float)(white_idx-key_leftpos+0.90f); \
vertices[2][1]=yf-key_height; \
vertices[2][2]=z-key_length; \
vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb; \
vertices[3][0]=(float)(white_idx-key_leftpos+0.90f); \
vertices[3][1]=yf; \
vertices[3][2]=z-key_length; \
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); \
/*Key / Right Face*/ \
vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb; \
vertices[0][0]=(float)(white_idx-key_leftpos+0.90f); \
vertices[0][1]=yn-key_height; \
vertices[0][2]=z;  \
vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb; \
vertices[1][0]=(float)(white_idx-key_leftpos+0.90f); \
vertices[1][1]=yn+0; \
vertices[1][2]=z;  \
vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb; \
vertices[2][0]=(float)(white_idx-key_leftpos+0.90f); \
vertices[2][1]=yf-key_height; \
vertices[2][2]=z-key_length;   \
vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb; \
vertices[3][0]=(float)(white_idx-key_leftpos+0.90f); \
vertices[3][1]=yf; \
vertices[3][2]=z-key_length;  \
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); \
/*Key / Left Face*/ \
vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb; \
vertices[0][0]=(float)(white_idx-key_leftpos+0.10f); \
vertices[0][1]=yf-key_height; \
vertices[0][2]=z-key_length;  \
vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb; \
vertices[1][0]=(float)(white_idx-key_leftpos+0.10f); \
vertices[1][1]=yf+0; \
vertices[1][2]=z-key_length;  \
vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb; \
vertices[2][0]=(float)(white_idx-key_leftpos+0.10f); \
vertices[2][1]=yn-key_height; \
vertices[2][2]=z;  \
vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb; \
vertices[3][0]=(float)(white_idx-key_leftpos+0.10f); \
vertices[3][1]=yn; \
vertices[3][2]=z;  \
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); \
white_idx++; \
} else { /*black key*/ \
if (piano_key_state[i+k]) { \
crt=(1.0f*piano_key_state[i+k]+0.4f*(4-piano_key_state[i+k]))/4; \
cgt=(0.8f*piano_key_state[i+k]+0.4f*(4-piano_key_state[i+k]))/4; \
cbt=(0.8f*piano_key_state[i+k]+0.4f*(4-piano_key_state[i+k]))/4; \
} else crt=cgt=cbt=0.2f; \
/*TOP*/ \
cr=crt;cg=cgt;cb=cbt;\
vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb; \
vertices[0][0]=(float)(white_idx-key_leftpos-0.15f); \
vertices[0][1]=ynBL+key_heightBL; \
vertices[0][2]=z-key_lengthBL*6/5;  \
vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb; \
vertices[1][0]=(float)(white_idx-key_leftpos-0.15f); \
vertices[1][1]=yf+key_heightBL; \
vertices[1][2]=z-key_length;  \
vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb; \
vertices[2][0]=(float)(white_idx-key_leftpos+0.15f); \
vertices[2][1]=ynBL+key_heightBL; \
vertices[2][2]=z-key_lengthBL*6/5;  \
vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb; \
vertices[3][0]=(float)(white_idx-key_leftpos+0.15f); \
vertices[3][1]=yf+key_heightBL; \
vertices[3][2]=z-key_length; \
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); \
cr=crt*0.6f;cg=cgt*0.6f;cb=cbt*0.6f; \
/*FRONT*/ \
vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb; \
vertices[0][0]=(float)(white_idx-key_leftpos-0.3f); \
vertices[0][1]=ynBL; \
vertices[0][2]=z-key_lengthBL;   \
vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb; \
vertices[1][0]=(float)(white_idx-key_leftpos-0.15f); \
vertices[1][1]=ynBL+key_heightBL; \
vertices[1][2]=z-key_lengthBL*6/5; \
vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb; \
vertices[2][0]=(float)(white_idx-key_leftpos+0.3f); \
vertices[2][1]=ynBL; \
vertices[2][2]=z-key_lengthBL; \
vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb; \
vertices[3][0]=(float)(white_idx-key_leftpos+0.15f); \
vertices[3][1]=ynBL+key_heightBL; \
vertices[3][2]=z-key_lengthBL*6/5; \
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); \
/*RIGHT*/ \
vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb; \
vertices[0][0]=(float)(white_idx-key_leftpos+0.3f); \
vertices[0][1]=ynBL; \
vertices[0][2]=z-key_lengthBL; \
vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb; \
vertices[1][0]=(float)(white_idx-key_leftpos+0.15f); \
vertices[1][1]=ynBL+key_heightBL; \
vertices[1][2]=z-key_lengthBL*6/5; \
vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb; \
vertices[2][0]=(float)(white_idx-key_leftpos+0.3f); \
vertices[2][1]=yf; \
vertices[2][2]=z-key_length;  \
vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb; \
vertices[3][0]=(float)(white_idx-key_leftpos+0.15f); \
vertices[3][1]=yf+key_heightBL; \
vertices[3][2]=z-key_length; \
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); \
/*BACK*/ \
vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb; \
vertices[0][0]=(float)(white_idx-key_leftpos-0.3f); \
vertices[0][1]=yf; \
vertices[0][2]=z-key_length; \
vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb; \
vertices[1][0]=(float)(white_idx-key_leftpos-0.15f); \
vertices[1][1]=yf+key_heightBL; \
vertices[1][2]=z-key_length; \
vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb; \
vertices[2][0]=(float)(white_idx-key_leftpos+0.3f); \
vertices[2][1]=yf; \
vertices[2][2]=z-key_length; \
vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb; \
vertices[3][0]=(float)(white_idx-key_leftpos+0.15f); \
vertices[3][1]=yf+key_heightBL; \
vertices[3][2]=z-key_length; \
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); \
/*LEFT*/ \
vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb; \
vertices[0][0]=(float)(white_idx-key_leftpos-0.3f); \
vertices[0][1]=yf; \
vertices[0][2]=z-key_length; \
vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb; \
vertices[1][0]=(float)(white_idx-key_leftpos-0.15f); \
vertices[1][1]=yf+key_heightBL; \
vertices[1][2]=z-key_length; \
vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb; \
vertices[2][0]=(float)(white_idx-key_leftpos-0.3f); \
vertices[2][1]=ynBL; \
vertices[2][2]=z-key_lengthBL; \
vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb; \
vertices[3][0]=(float)(white_idx-key_leftpos-0.15f); \
vertices[3][1]=ynBL+key_heightBL; \
vertices[3][2]=z-key_lengthBL*6/5; \
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);  \
}
    
    //    glEnable(GL_BLEND);
    //    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    //glDisable(GL_DEPTH_TEST);
    
    /* Begin Drawing Quads, setup vertex array pointer */
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glColorPointer(4, GL_FLOAT, 0, vertColor);
    
    /* Enable Vertex Pointer */
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    
    
    //draw piano
    vertColor[0][3]=vertColor[1][3]=vertColor[2][3]=vertColor[3][3]=1.0f;
    int white_idx=0;
    key_length=6;
    key_lengthBL=6*4/9;
    key_height=0.8f;
    key_heightBL=0.6f;
    
    yf=-5;
    yn=-5;
    z=-0-key_length*2;
    
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    key_leftpos=28.0f/2;
    
    int piano_ofs=0;
    int k=0;
    z=0;
    vertColor[0][3]=vertColor[1][3]=vertColor[2][3]=vertColor[3][3]=1.0f;
    
    for (int i=0;i<48;i++,piano_ofs++) {
        PIANO3D_DRAWKEY
    }
    
    z=z-key_length;
    yf=yf+key_height*3;
    key_leftpos+=28.0f;
    
    k=48;
    for (int i=0;i<48;i++,piano_ofs++) {
        PIANO3D_DRAWKEY
    }
    
    z=z-key_length;
    yf=yf+key_height*3;
    key_leftpos+=28.0f-(28-19)/2;
    k=96;
    for (int i=0;i<32;i++,piano_ofs++) {
        PIANO3D_DRAWKEY
    }
    
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    
    /* Pop The Matrix */
    glPopMatrix();
    
    //    glDisable(GL_BLEND);
    
}


int qsort_CompareBar(const void *entryA, const void *entryB) {
    int    valA=((t_data_bar2draw*)entryA)->instr;
    int    valB=((t_data_bar2draw*)entryB)->instr;
    if (valA==valB) {
        valA=(((t_data_bar2draw*)entryA)->note)&127;
        valB=(((t_data_bar2draw*)entryB)->note)&127;
        
        if (valA==valB) {
            valA=((t_data_bar2draw*)entryA)->startidx;
            valB=((t_data_bar2draw*)entryB)->startidx;
            
        }
    }
    return valA-valB;
}



void RenderUtils::DrawPiano3DWithNotesWall(int *data,uint ww,uint hh,int fx_len,int automove,float posx,float posy,float posz,float rotx,float roty,int color_mode,int fxquality) {
    int index;
    float key_length,key_lengthBL,key_height,key_heightBL;
    float key_leftpos;
    static int piano_fxcpt;
    static int first_call=1;
    static int note_min=0;
    static int note_max=127;
    static float ztrans=-100*16-30;
    static float ztrans_tgt=-100*16-30;
    static int ztrans_wait=0;
    static float xtrans=0;
    static float xtrans_tgt=0;
    static float xtransSpeed_tgt=0;
    static float ztransSpeed_tgt=0;
    static int camera_pos=0;
    static int camera_pos_countdown=0;
    
    
    if (first_call) {
        
        memset(piano_key_state,0,128);
        memset(piano_key_instr,0,128);
        first_call=0;
        piano_fxcpt=arc4random()&0xFFF;
        
        for (int i=0;i<MAX_BARS*6*6;i++) vertColorBAR[i][3]=1;
    }
    
    if (camera_pos_countdown==0) {
        camera_pos=arc4random()%8;
        camera_pos_countdown=30*15+(arc4random()&511);//15s min before switching
    } else camera_pos_countdown--;
    
    //    camera_pos=5;
    
    piano_fxcpt++;
    
    GLfloat x,y,z,yf,yn,ynBL,yadj;
    GLfloat cr,cg,cb,crt,cgt,cbt;
    
    //////////////////////////////
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const float aspectRatio = (float)ww/(float)hh;
    const float _hw = 75.0/2/16;//0.2f;
    const float _hh = _hw/aspectRatio;
    glFrustumf(-_hw, _hw, -_hh, _hh, 100.0f, 10000.0f);
    
    glPushMatrix();                     /* Push The Modelview Matrix */
    
    
    //interval to draw
    if (automove) {
        int nb_white_key=(note_max-note_min+5)*7/12+4;
        z=nb_white_key/75.0f*100*16;
        if (z<100*2) z=100*2;
        ztrans_tgt=-z-50;
        
        switch (camera_pos) {
            case 1:
            case 2:
                ztrans_tgt=-z-30;
                break;
                /*            case 3:
                 ztrans_tgt=-z-60;
                 break;*/
            case 5:
            case 7:
                ztrans_tgt=-z-30;
                break;
        }
        
        if (ztrans>ztrans_tgt) {
            ztrans=ztrans+(ztrans_tgt-ztrans)*ztransSpeed_tgt;
            if (ztransSpeed_tgt<0.1) ztransSpeed_tgt=ztransSpeed_tgt+0.001;
            if (ztrans-ztrans_tgt<0.1) {
                ztransSpeed_tgt=0;
            }
            
            ztrans_wait=30*5+arc4random()&255;
        } else {
            if (ztrans_wait==0) {
                ztrans=ztrans+(ztrans_tgt-ztrans)*ztransSpeed_tgt;
                if (ztransSpeed_tgt<0.1) ztransSpeed_tgt=ztransSpeed_tgt+0.001;
                if (ztrans_tgt-ztrans<0.1) {
                    ztrans_wait=30*5+arc4random()&255;
                    ztransSpeed_tgt=0;
                }
            } else ztrans_wait--;
        }
        
        xtrans_tgt=((note_max+note_min)/2-64)*7.0/12;
        xtrans=xtrans+(xtrans_tgt-xtrans)*xtransSpeed_tgt;
        if (xtransSpeed_tgt<0.1) xtransSpeed_tgt=xtransSpeed_tgt+0.001;
        if (abs(xtrans-xtrans_tgt)<0.1) {
            xtransSpeed_tgt=0;
        }
        
        
        float roty_adj,rotx_adj,xrandfact,rotx_randfact,roty_randfact;
        xrandfact=1.0;
        rotx_randfact=8.0f;
        roty_randfact=5.0f;
        switch (camera_pos) {
            case 0:  //front
                rotx_adj=30;
                roty_adj=0;
                break;
            case 1:  //left
                rotx_adj=30;
                roty_adj=45;
                break;
            case 2:  //front
                rotx_adj=25;
                roty_adj=0;
                break;
            case 3:  //right
                rotx_adj=30;
                roty_adj=-45;
                break;
            case 4: //front
                rotx_adj=35;
                roty_adj=0;
                break;
            case 5: //left
                rotx_adj=20;
                roty_adj=75;
                xtrans=0;
                xrandfact=0.2f;
                rotx_randfact=3.0f;
                roty_randfact=2.0f;
                break;
            case 6: //front
                rotx_adj=32;
                roty_adj=0;
                break;
            case 7: //right
                rotx_adj=20;
                roty_adj=-75;
                xtrans=0;
                xrandfact=0.2f;
                rotx_randfact=3.0f;
                roty_randfact=2.0f;
                break;
        }
        
        glTranslatef(-xtrans+xrandfact*(0.9f*sin((float)piano_fxcpt*3.14159f/319)+
                                        0.5f*sin((float)piano_fxcpt*3.14159f/789)-
                                        0.7f*sin((float)piano_fxcpt*3.14159f/1061)),
                     2.0,
                     ztrans-5*(1.2f*cos((float)piano_fxcpt*3.14159f/719)+
                               0.5f*sin((float)piano_fxcpt*3.14159f/289)-
                               0.7f*sin((float)piano_fxcpt*3.14159f/361)));
        
        
        glRotatef(rotx_adj+rotx_randfact*(0.4f*sin((float)piano_fxcpt*3.14159f/91)+
                                          0.7f*sin((float)piano_fxcpt*3.14159f/911)+
                                          0.3f*sin((float)piano_fxcpt*3.14159f/409)), 1, 0, 0);
        glRotatef(roty_adj+roty_randfact*(0.8f*sin((float)piano_fxcpt*3.14159f/173)+
                                          0.5f*sin((float)piano_fxcpt*3.14159f/1029)+
                                          0.3f*sin((float)piano_fxcpt*3.14159f/511)), 0, 1, 0);
        
    } else {
        glTranslatef(posx,posy,posz-100*15);
        glRotatef(30+rotx, 1, 0, 0);
        glRotatef(roty, 0, 1, 0);
    }
    
    
    
    if (fx_len!=data_pianofx_len) {
        data_pianofx_len=fx_len;
        data_pianofx_first=1;
    }
    
    
    //if first launch, clear buffers
    if (data_pianofx_first) {
        data_pianofx_first=0;
        for (int i=0;i<data_pianofx_len;i++) {
            memset(data_pianofx_note[i],0,256);
        }
    }
    //update old ones
    for (int j=0;j<data_pianofx_len-1;j++) {
        memcpy(data_pianofx_note[j],data_pianofx_note[j+1],256);
        memcpy(data_pianofx_instr[j],data_pianofx_instr[j+1],256);
        memcpy(data_pianofx_vol[j],data_pianofx_vol[j+1],256);
        memcpy(data_pianofx_st[j],data_pianofx_st[j+1],256);
    }
    //add new one
    for (int i=0;i<256;i++) {
        int note=data[i];
        data_pianofx_note[data_pianofx_len-1][i]=note&0xFF;
        data_pianofx_instr[data_pianofx_len-1][i]=(note>>8)&0xFF;
        data_pianofx_st[data_pianofx_len-1][i]=(note>>24)&0xFF;
        data_pianofx_vol[data_pianofx_len-1][i]=(note>>16)&0xFF;
    }
    
    if (fx_len!=data_pianofx_len) {
        data_pianofx_len=fx_len;
        data_midifx_first=1;
    }
    
    
    
    int j=data_pianofx_len-1-MIDIFX_OFS;
    //glLineWidth(line_width+2);
    index=0;
    for (int i=0; i<256; i++) {
        if (data_pianofx_note[j][i]) {
            int instr=data_pianofx_instr[j][i];
            int vol=data_pianofx_vol[j][i];
            int st=data_pianofx_st[j][i];
            
            if (vol&&(st&VOICE_ON)) {
                //note pressed
                piano_key_state[(data_pianofx_note[j][i])&127]=4;
                piano_key_instr[(data_pianofx_note[j][i])&127]=instr;
            }
        }
    }
    
    
    //    glEnable(GL_BLEND);
    //    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    //glDisable(GL_DEPTH_TEST);
    
    /* Begin Drawing Quads, setup vertex array pointer */
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glColorPointer(4, GL_FLOAT, 0, vertColor);
    
    /* Enable Vertex Pointer */
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    
    
    //draw piano
    int white_idx=0;
    key_length=6;
    key_lengthBL=6*4/9;
    key_height=0.8f;
    key_heightBL=0.6f;
    
    yf=-5;
    yn=-5;
    z=-0-key_length*2;
    
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    
    key_leftpos=75.0f/2;
    
    int  vertices_count=0;
    
    
    int piano_ofs=0;
    z=0;
    vertColor[0][3]=vertColor[1][3]=vertColor[2][3]=vertColor[3][3]=1.0f;
    yadj=0.01f;
    for (int i=0;i<128;i++) {
        if (piano_key_state[i]) {
            yn=yf-key_height*4/5*piano_key_state[i]/4;
            ynBL=yf-key_heightBL*3/5*piano_key_state[i]/4;
            piano_key_state[i]--;
            
            int colidx;//=i%12;
            if (color_mode==0) {
                colidx=i%12;
            } else if (color_mode==1) {
                colidx=piano_key_instr[i]&31;
            }
            
            crt=(data_midifx_col[colidx&15]>>16)/255.0f;
            cgt=((data_midifx_col[colidx&15]>>8)&0xFF)/255.0f;
            cbt=(data_midifx_col[colidx&15]&0xFF)/255.0f;
            
            if (colidx&0x10) {
                crt=(crt+1)/2;
                cgt=(cgt+1)/2;
                cbt=(cbt+1)/2;
            }
            
        } else {
            yn=ynBL=yf;
        }
        if (piano_ofs==12) piano_ofs=0;
        if (piano_key[piano_ofs]==0) { /*white key*/
            piano_note_type[i]=0;
            piano_note_posx[i]=(float)(white_idx-key_leftpos+0.5f);
            piano_note_posy[i]=yf+yadj;
            piano_note_posz[i]=z-key_length;
            if (piano_key_state[i]) {
                crt=(crt*piano_key_state[i]+1.0f*(4-piano_key_state[i]))/4;
                cgt=(cgt*piano_key_state[i]+1.0f*(4-piano_key_state[i]))/4;
                cbt=(cbt*piano_key_state[i]+1.0f*(4-piano_key_state[i]))/4;
            } else crt=cgt=cbt=1.0f;
            /*Key / Up Face*/
            cr=crt;cg=cgt;cb=cbt;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(float)(white_idx-key_leftpos+0.05f);
            vertices[0][1]=yn+yadj;
            vertices[0][2]=z+0.1f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=(float)(white_idx-key_leftpos+0.05f);
            vertices[1][1]=yf+yadj;
            vertices[1][2]=z-key_length;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(float)(white_idx-key_leftpos+0.95f);
            vertices[2][1]=yn+yadj;
            vertices[2][2]=z+0.1f;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=(float)(white_idx-key_leftpos+0.95f);
            vertices[3][1]=yf+yadj;
            vertices[3][2]=z-key_length;
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertices_count+=4;
            
            
            /*Key / Down Face*/
            cr=crt*0.4;cg=cgt*0.4;cb=cbt*0.4;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(float)(white_idx-key_leftpos+0.05f);
            vertices[0][1]=yn-key_height;
            vertices[0][2]=z;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=(float)(white_idx-key_leftpos+0.05f);
            vertices[1][1]=yf-key_height;
            vertices[1][2]=z-key_length;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(float)(white_idx-key_leftpos+0.95f);
            vertices[2][1]=yn-key_height;
            vertices[2][2]=z;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=(float)(white_idx-key_leftpos+0.95f);
            vertices[3][1]=yf-key_height;
            vertices[3][2]=z-key_length;
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertices_count+=4;
            
            /*Key / Front Face*/
            cr=crt*0.6f;cg=cgt*0.6f;cb=cbt*0.6f;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(float)(white_idx-key_leftpos+0.10f);
            vertices[0][1]=yn-key_height;
            vertices[0][2]=z;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=(float)(white_idx-key_leftpos+0.10f);
            vertices[1][1]=yn+0;
            vertices[1][2]=z;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(float)(white_idx-key_leftpos+0.90f);
            vertices[2][1]=yn-key_height;
            vertices[2][2]=z;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=(float)(white_idx-key_leftpos+0.90f);
            vertices[3][1]=yn;
            vertices[3][2]=z;
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertices_count+=4;
            
            /*Key / Back Face*/
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(float)(white_idx-key_leftpos+0.10f);
            vertices[0][1]=yf-key_height;
            vertices[0][2]=z-key_length;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=(float)(white_idx-key_leftpos+0.10f);
            vertices[1][1]=yf+0;
            vertices[1][2]=z-key_length;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(float)(white_idx-key_leftpos+0.90f);
            vertices[2][1]=yf-key_height;
            vertices[2][2]=z-key_length;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=(float)(white_idx-key_leftpos+0.90f);
            vertices[3][1]=yf;
            vertices[3][2]=z-key_length;
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertices_count+=4;
            
            /*Key / Right Face*/
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(float)(white_idx-key_leftpos+0.90f);
            vertices[0][1]=yn-key_height;
            vertices[0][2]=z;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=(float)(white_idx-key_leftpos+0.90f);
            vertices[1][1]=yn+0;
            vertices[1][2]=z;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(float)(white_idx-key_leftpos+0.90f);
            vertices[2][1]=yf-key_height;
            vertices[2][2]=z-key_length;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=(float)(white_idx-key_leftpos+0.90f);
            vertices[3][1]=yf;
            vertices[3][2]=z-key_length;
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertices_count+=4;
            
            /*Key / Left Face*/
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(float)(white_idx-key_leftpos+0.10f);
            vertices[0][1]=yf-key_height;
            vertices[0][2]=z-key_length;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=(float)(white_idx-key_leftpos+0.10f);
            vertices[1][1]=yf+0;
            vertices[1][2]=z-key_length;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(float)(white_idx-key_leftpos+0.10f);
            vertices[2][1]=yn-key_height;
            vertices[2][2]=z;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=(float)(white_idx-key_leftpos+0.10f);
            vertices[3][1]=yn;
            vertices[3][2]=z;
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertices_count+=4;
            
            white_idx++;
        } else { /*black key*/
            piano_note_type[i]=1;
            piano_note_posx[i]=(float)(white_idx-key_leftpos);
            piano_note_posy[i]=yf+key_heightBL;
            piano_note_posz[i]=z-key_length;
            if (piano_key_state[i]) {
                crt=(crt*piano_key_state[i]+0.4f*(4-piano_key_state[i]))/4;
                cgt=(cgt*piano_key_state[i]+0.4f*(4-piano_key_state[i]))/4;
                cbt=(cbt*piano_key_state[i]+0.4f*(4-piano_key_state[i]))/4;
            } else crt=cgt=cbt=0.2f;
            /*TOP*/
            cr=crt;cg=cgt;cb=cbt;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(float)(white_idx-key_leftpos-0.15f);
            vertices[0][1]=ynBL+key_heightBL;
            vertices[0][2]=z-key_lengthBL*6/5;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=(float)(white_idx-key_leftpos-0.15f);
            vertices[1][1]=yf+key_heightBL;
            vertices[1][2]=z-key_length;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(float)(white_idx-key_leftpos+0.15f);
            vertices[2][1]=ynBL+key_heightBL;
            vertices[2][2]=z-key_lengthBL*6/5;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=(float)(white_idx-key_leftpos+0.15f);
            vertices[3][1]=yf+key_heightBL;
            vertices[3][2]=z-key_length;
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertices_count+=4;
            
            cr=crt*0.6f;cg=cgt*0.6f;cb=cbt*0.6f;
            /*FRONT*/
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(float)(white_idx-key_leftpos-0.3f);
            vertices[0][1]=ynBL;
            vertices[0][2]=z-key_lengthBL;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=(float)(white_idx-key_leftpos-0.15f);
            vertices[1][1]=ynBL+key_heightBL;
            vertices[1][2]=z-key_lengthBL*6/5;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(float)(white_idx-key_leftpos+0.3f);
            vertices[2][1]=ynBL;
            vertices[2][2]=z-key_lengthBL;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=(float)(white_idx-key_leftpos+0.15f);
            vertices[3][1]=ynBL+key_heightBL;
            vertices[3][2]=z-key_lengthBL*6/5;
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertices_count+=4;
            
            /*BACK*/
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(float)(white_idx-key_leftpos-0.3f);
            vertices[0][1]=yf;
            vertices[0][2]=z-key_length;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=(float)(white_idx-key_leftpos-0.15f);
            vertices[1][1]=yf+key_heightBL;
            vertices[1][2]=z-key_length;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(float)(white_idx-key_leftpos+0.3f);
            vertices[2][1]=yf;
            vertices[2][2]=z-key_length;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=(float)(white_idx-key_leftpos+0.15f);
            vertices[3][1]=yf+key_heightBL;
            vertices[3][2]=z-key_length;
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertices_count+=4;
            
            /*RIGHT*/
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(float)(white_idx-key_leftpos+0.3f);
            vertices[0][1]=ynBL;
            vertices[0][2]=z-key_lengthBL;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=(float)(white_idx-key_leftpos+0.15f);
            vertices[1][1]=ynBL+key_heightBL;
            vertices[1][2]=z-key_lengthBL*6/5;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(float)(white_idx-key_leftpos+0.3f);
            vertices[2][1]=yf;
            vertices[2][2]=z-key_length;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=(float)(white_idx-key_leftpos+0.15f);
            vertices[3][1]=yf+key_heightBL;
            vertices[3][2]=z-key_length;
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertices_count+=4;
            
            /*LEFT*/
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(float)(white_idx-key_leftpos-0.3f);
            vertices[0][1]=yf;
            vertices[0][2]=z-key_length;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=(float)(white_idx-key_leftpos-0.15f);
            vertices[1][1]=yf+key_heightBL;
            vertices[1][2]=z-key_length;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(float)(white_idx-key_leftpos-0.3f);
            vertices[2][1]=ynBL;
            vertices[2][2]=z-key_lengthBL;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=(float)(white_idx-key_leftpos-0.15f);
            vertices[3][1]=ynBL+key_heightBL;
            vertices[3][2]=z-key_lengthBL*6/5;
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertices_count+=4;
        }
        piano_ofs++;
    }
    
    
    //    glDisable(GL_DEPTH_TEST);
    //    glEnable(GL_BLEND);
    //    glBlendFunc(GL_ONE,GL_ONE);
    
    vertColor[0][3]=vertColor[1][3]=vertColor[2][3]=vertColor[3][3]=1.0f;
    
    int tgt_note_min=127;
    int tgt_note_max=0;
    
    int data_bar2draw_count=0;
    
    
    
    for (int i=0; i<256; i++) { //for each channels
        int j=0;
        while (j<data_pianofx_len) {  //while not having reach roof
            if (data_pianofx_note[j][i]) {  //do we have a note ?
                int instr=data_pianofx_instr[j][i];
                int vol=data_pianofx_vol[j][i];
                int st=data_pianofx_st[j][i];
                int note=data_pianofx_note[j][i];
                
                if (vol&&(st&VOICE_ON)) {  //check volume & status => we have something
                    data_bar2draw[data_bar2draw_count].startidx=j;
                    data_bar2draw[data_bar2draw_count].note=note&127;
                    data_bar2draw[data_bar2draw_count].instr=instr;
                    data_bar2draw[data_bar2draw_count].size=0;
                    while ((data_pianofx_instr[j][i]==instr)&&(data_pianofx_note[j][i]==note)&&(vol&&(st&VOICE_ON))) {  //while same bar (instru & notes), increase size
                        data_bar2draw[data_bar2draw_count].size++;
                        if (j==(data_pianofx_len-MIDIFX_OFS-1)) data_bar2draw[data_bar2draw_count].note|=128;
                        j++;
                        if (j==data_pianofx_len) break;
                        vol=data_pianofx_vol[j][i];
                        st=data_pianofx_st[j][i];
                    }
                    data_bar2draw_count++;
                } else j++;
            } else j++;
            if (data_bar2draw_count==MAX_BARS) break;
        }
        if (data_bar2draw_count==MAX_BARS) break;
    }
    qsort(data_bar2draw,data_bar2draw_count,sizeof(t_data_bar2draw),qsort_CompareBar);
    
    if (data_bar2draw_count>=2) { //propagate played flag
        for (int i=1;i<data_bar2draw_count;i++) {
            int note=data_bar2draw[i-1].note&127;
            int played=data_bar2draw[i-1].note&128;
            int instr=data_bar2draw[i-1].instr;
            
            if (played) {
                if ((data_bar2draw[i].instr==instr)&&((data_bar2draw[i].note&127)==note)&&
                    (data_bar2draw[i].startidx<=(data_bar2draw[i-1].startidx+data_bar2draw[i-1].size)))
                    data_bar2draw[i].note|=128;
                /*                if ((data_bar2draw[i].instr==instr)&&((data_bar2draw[i].note&127)==note)&&
                 (data_bar2draw[i].startidx==data_bar2draw[i-1].startidx))
                 data_bar2draw[i].note|=128;*/
            }
        }
        
        for (int i=data_bar2draw_count-2;i>=0;i--) {
            int note=data_bar2draw[i+1].note&127;
            int played=data_bar2draw[i+1].note&128;
            int instr=data_bar2draw[i+1].instr;
            
            if (played) {
                if ((data_bar2draw[i].instr==instr)&&((data_bar2draw[i].note&127)==note)&&
                    (data_bar2draw[i+1].startidx<=(data_bar2draw[i].startidx+data_bar2draw[i].size))) data_bar2draw[i].note|=128;
                
                /*if ((data_bar2draw[i].instr==instr)&&((data_bar2draw[i].note&127)==note)&&
                 (data_bar2draw[i+1].startidx==data_bar2draw[i].startidx)) data_bar2draw[i].note|=128;
                 */
            }
        }
    }
    
    for (int i=1;i<data_bar2draw_count;i++) {
        if ((data_bar2draw[i].instr==data_bar2draw[i-1].instr)&&
            (data_bar2draw[i].note==data_bar2draw[i-1].note)&&
            (data_bar2draw[i].startidx>=data_bar2draw[i-1].startidx)&&
            (data_bar2draw[i].startidx+data_bar2draw[i].size<=data_bar2draw[i-1].startidx+data_bar2draw[i-1].size)) {
            data_bar2draw[i].size=0;
        }
    }
    
    int vertices_index=0;
    int indices_index=0;
    glVertexPointer(3, GL_FLOAT, 0, verticesBAR);
    glColorPointer(4, GL_FLOAT, 0, vertColorBAR);
    
    
    //TO OPTIMIZE
    int data_bar_2dmap[128*MIDIFX_LEN];
    memset(data_bar_2dmap,0,128*MIDIFX_LEN*4);
    
    for (int i=0;i<data_bar2draw_count;i++) {
        int note=data_bar2draw[i].note&127;
        int played=data_bar2draw[i].note&128;
        int instr=data_bar2draw[i].instr;
        int colidx;
        if (color_mode==0) { //note
            colidx=note%12;
        } else if (color_mode==1) { //instru
            colidx=instr&31;
        }
        
        if (data_bar2draw[i].size==0) continue;
        
        //printf("i:%d start:%d end:%d instr:%d note:%d played:%d\n",i,data_bar2draw[i].startidx,data_bar2draw[i].startidx+data_bar2draw[i].size,instr,note,played);
        
        
        float adj_size=0;
        for (int j=data_bar2draw[i].startidx;j<data_bar2draw[i].startidx+data_bar2draw[i].size;j++) {
            int _instr=(data_bar_2dmap[note*MIDIFX_LEN+j]>>16);
            int draw_count=data_bar_2dmap[note*MIDIFX_LEN+j]&255;
            if (draw_count) {
                if (_instr!=(instr+1)) {
                    draw_count++;
                    data_bar_2dmap[note*MIDIFX_LEN+j]=(((int)(data_bar2draw[i].instr)+1)<<16)|draw_count;
                }
                if (adj_size<0.3f*(float)(draw_count-1)) adj_size=0.3f*(float)(draw_count-1);
            } else {
                data_bar_2dmap[note*MIDIFX_LEN+j]=(((int)(data_bar2draw[i].instr)+1)<<16)|1;
            }
        }
        //        printf("adj: %f\n",adj_size);
        
        
        crt=(data_midifx_col[colidx&15]>>16)/255.0f/1.0f;
        cgt=((data_midifx_col[colidx&15]>>8)&0xFF)/255.0f/1.0f;
        cbt=(data_midifx_col[colidx&15]&0xFF)/255.0f/1.0f;
        
        if (colidx&0x10) {
            crt=(crt+1)/2;
            cgt=(cgt+1)/2;
            cbt=(cbt+1)/2;
        }
        
        if (played) {
            crt=(crt+2)/3;
            cgt=(cgt+2)/3;
            cbt=(cbt+2)/3;
        }
        
        if (note>tgt_note_max) tgt_note_max=note;
        if (note<tgt_note_min) tgt_note_min=note;
        
        x=piano_note_posx[note&127];
        y=piano_note_posy[note&127]+((float)(data_bar2draw[i].startidx)-MIDIFX_OFS*3)*0.5f;
        z=piano_note_posz[note&127];
        
        
        
        float x1;
        float y1=y;
        float z1=z;
        float sx;
        float sy=0.5f*(float)data_bar2draw[i].size;
        float sz=0.3f;
        
        if (piano_note_type[note&127]) {
            //black key
            x1=x-0.15;
            sx=0.3;
            z1+=key_length*0.55;
        } else {
            //white key
            x1=x-0.3;
            sx=0.6;
            z1+=key_length*0.9;
        }
        
        /*sx=sx-adj_size;x1=x1+adj_size/2;
         sy=sy-adj_size;y1=y1+adj_size/2;
         sz=sz-adj_size;z1=z1-adj_size/2;*/
        z1=z1-adj_size;
        
        
        
        //front
        cr=crt;cg=cgt;cb=cbt;
        vertColorBAR[vertices_index+0][0]=cr;vertColorBAR[vertices_index+0][1]=cg;vertColorBAR[vertices_index+0][2]=cb;
        verticesBAR[vertices_index+0][0]=x1;
        verticesBAR[vertices_index+0][1]=y1;
        verticesBAR[vertices_index+0][2]=z1;
        vertColorBAR[vertices_index+1][0]=cr;vertColorBAR[vertices_index+1][1]=cg;vertColorBAR[vertices_index+1][2]=cb;
        verticesBAR[vertices_index+1][0]=x1+sx;
        verticesBAR[vertices_index+1][1]=y1;
        verticesBAR[vertices_index+1][2]=z1;
        vertColorBAR[vertices_index+2][0]=cr;vertColorBAR[vertices_index+2][1]=cg;vertColorBAR[vertices_index+2][2]=cb;
        verticesBAR[vertices_index+2][0]=x1;
        verticesBAR[vertices_index+2][1]=y1+sy;
        verticesBAR[vertices_index+2][2]=z1;
        vertColorBAR[vertices_index+3][0]=cr;vertColorBAR[vertices_index+3][1]=cg;vertColorBAR[vertices_index+3][2]=cb;
        verticesBAR[vertices_index+3][0]=x1+sx;
        verticesBAR[vertices_index+3][1]=y1+sy;
        verticesBAR[vertices_index+3][2]=z1;
        //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        if (indices_index==0) {
            vertBARindices[indices_index++]=0;
            vertBARindices[indices_index++]=1;
            vertBARindices[indices_index++]=2;
            vertBARindices[indices_index++]=3;
            vertBARindices[indices_index++]=3;
        } else {
            vertBARindices[indices_index++]=vertices_index+0;
            vertBARindices[indices_index++]=vertices_index+0;
            vertBARindices[indices_index++]=vertices_index+1;
            vertBARindices[indices_index++]=vertices_index+2;
            vertBARindices[indices_index++]=vertices_index+3;
            vertBARindices[indices_index++]=vertices_index+3;
        }
        vertices_index+=4;
        
        if (fxquality==2) {
            //back
            cr=crt;cg=cgt;cb=cbt;
            vertColorBAR[vertices_index+0][0]=cr;vertColorBAR[vertices_index+0][1]=cg;vertColorBAR[vertices_index+0][2]=cb;
            verticesBAR[vertices_index+0][0]=x1;
            verticesBAR[vertices_index+0][1]=y1;
            verticesBAR[vertices_index+0][2]=z1-sz;
            vertColorBAR[vertices_index+1][0]=cr;vertColorBAR[vertices_index+1][1]=cg;vertColorBAR[vertices_index+1][2]=cb;
            verticesBAR[vertices_index+1][0]=x1+sx;
            verticesBAR[vertices_index+1][1]=y1;
            verticesBAR[vertices_index+1][2]=z1-sz;
            vertColorBAR[vertices_index+2][0]=cr;vertColorBAR[vertices_index+2][1]=cg;vertColorBAR[vertices_index+2][2]=cb;
            verticesBAR[vertices_index+2][0]=x1;
            verticesBAR[vertices_index+2][1]=y1+sy;
            verticesBAR[vertices_index+2][2]=z1-sz;
            vertColorBAR[vertices_index+3][0]=cr;vertColorBAR[vertices_index+3][1]=cg;vertColorBAR[vertices_index+3][2]=cb;
            verticesBAR[vertices_index+3][0]=x1+sx;
            verticesBAR[vertices_index+3][1]=y1+sy;
            verticesBAR[vertices_index+3][2]=z1-sz;
            //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertBARindices[indices_index++]=vertices_index+0;
            vertBARindices[indices_index++]=vertices_index+0;
            vertBARindices[indices_index++]=vertices_index+1;
            vertBARindices[indices_index++]=vertices_index+2;
            vertBARindices[indices_index++]=vertices_index+3;
            vertBARindices[indices_index++]=vertices_index+3;
            vertices_index+=4;
            
            
            cr=crt/2;cg=cgt/2;cb=cbt/2;
            //left
            vertColorBAR[vertices_index+0][0]=cr;vertColorBAR[vertices_index+0][1]=cg;vertColorBAR[vertices_index+0][2]=cb;
            verticesBAR[vertices_index+0][0]=x1;
            verticesBAR[vertices_index+0][1]=y1;
            verticesBAR[vertices_index+0][2]=z1;
            vertColorBAR[vertices_index+1][0]=cr;vertColorBAR[vertices_index+1][1]=cg;vertColorBAR[vertices_index+1][2]=cb;
            verticesBAR[vertices_index+1][0]=x1;
            verticesBAR[vertices_index+1][1]=y1;
            verticesBAR[vertices_index+1][2]=z1-sz;
            vertColorBAR[vertices_index+2][0]=cr;vertColorBAR[vertices_index+2][1]=cg;vertColorBAR[vertices_index+2][2]=cb;
            verticesBAR[vertices_index+2][0]=x1;
            verticesBAR[vertices_index+2][1]=y1+sy;
            verticesBAR[vertices_index+2][2]=z1;
            vertColorBAR[vertices_index+3][0]=cr;vertColorBAR[vertices_index+3][1]=cg;vertColorBAR[vertices_index+3][2]=cb;
            verticesBAR[vertices_index+3][0]=x1;
            verticesBAR[vertices_index+3][1]=y1+sy;
            verticesBAR[vertices_index+3][2]=z1-sz;
            //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertBARindices[indices_index++]=vertices_index+0;
            vertBARindices[indices_index++]=vertices_index+0;
            vertBARindices[indices_index++]=vertices_index+1;
            vertBARindices[indices_index++]=vertices_index+2;
            vertBARindices[indices_index++]=vertices_index+3;
            vertBARindices[indices_index++]=vertices_index+3;
            vertices_index+=4;
            
            //right
            vertColorBAR[vertices_index+0][0]=cr;vertColorBAR[vertices_index+0][1]=cg;vertColorBAR[vertices_index+0][2]=cb;
            verticesBAR[vertices_index+0][0]=x1+sx;
            verticesBAR[vertices_index+0][1]=y1;
            verticesBAR[vertices_index+0][2]=z1;
            vertColorBAR[vertices_index+1][0]=cr;vertColorBAR[vertices_index+1][1]=cg;vertColorBAR[vertices_index+1][2]=cb;
            verticesBAR[vertices_index+1][0]=x1+sx;
            verticesBAR[vertices_index+1][1]=y1;
            verticesBAR[vertices_index+1][2]=z1-sz;
            vertColorBAR[vertices_index+2][0]=cr;vertColorBAR[vertices_index+2][1]=cg;vertColorBAR[vertices_index+2][2]=cb;
            verticesBAR[vertices_index+2][0]=x1+sx;
            verticesBAR[vertices_index+2][1]=y1+sy;
            verticesBAR[vertices_index+2][2]=z1;
            vertColorBAR[vertices_index+3][0]=cr;vertColorBAR[vertices_index+3][1]=cg;vertColorBAR[vertices_index+3][2]=cb;
            verticesBAR[vertices_index+3][0]=x1+sx;
            verticesBAR[vertices_index+3][1]=y1+sy;
            verticesBAR[vertices_index+3][2]=z1-sz;
            //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertBARindices[indices_index++]=vertices_index+0;
            vertBARindices[indices_index++]=vertices_index+0;
            vertBARindices[indices_index++]=vertices_index+1;
            vertBARindices[indices_index++]=vertices_index+2;
            vertBARindices[indices_index++]=vertices_index+3;
            vertBARindices[indices_index++]=vertices_index+3;
            vertices_index+=4;
            
            cr=crt*1.5f;cg=cgt*1.5f;cb=cbt*1.5f;
            if (cr>1) cr=1;if (cg>1) cg=1;if (cb>1) cb=1;
            //top
            vertColorBAR[vertices_index+0][0]=cr;vertColorBAR[vertices_index+0][1]=cg;vertColorBAR[vertices_index+0][2]=cb;
            verticesBAR[vertices_index+0][0]=x1;
            verticesBAR[vertices_index+0][1]=y1+sy;
            verticesBAR[vertices_index+0][2]=z1;
            vertColorBAR[vertices_index+1][0]=cr;vertColorBAR[vertices_index+1][1]=cg;vertColorBAR[vertices_index+1][2]=cb;
            verticesBAR[vertices_index+1][0]=x1+sx;
            verticesBAR[vertices_index+1][1]=y1+sy;
            verticesBAR[vertices_index+1][2]=z1;
            vertColorBAR[vertices_index+2][0]=cr;vertColorBAR[vertices_index+2][1]=cg;vertColorBAR[vertices_index+2][2]=cb;
            verticesBAR[vertices_index+2][0]=x1;
            verticesBAR[vertices_index+2][1]=y1+sy;
            verticesBAR[vertices_index+2][2]=z1-sz;
            vertColorBAR[vertices_index+3][0]=cr;vertColorBAR[vertices_index+3][1]=cg;vertColorBAR[vertices_index+3][2]=cb;
            verticesBAR[vertices_index+3][0]=x1+sx;
            verticesBAR[vertices_index+3][1]=y1+sy;
            verticesBAR[vertices_index+3][2]=z1-sz;
            //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertBARindices[indices_index++]=vertices_index+0;
            vertBARindices[indices_index++]=vertices_index+0;
            vertBARindices[indices_index++]=vertices_index+1;
            vertBARindices[indices_index++]=vertices_index+2;
            vertBARindices[indices_index++]=vertices_index+3;
            vertBARindices[indices_index++]=vertices_index+3;
            vertices_index+=4;
            
            cr=crt/3;cg=cgt/3;cb=cbt/3;
            //bottom
            vertColorBAR[vertices_index+0][0]=cr;vertColorBAR[vertices_index+0][1]=cg;vertColorBAR[vertices_index+0][2]=cb;
            verticesBAR[vertices_index+0][0]=x1;
            verticesBAR[vertices_index+0][1]=y1;
            verticesBAR[vertices_index+0][2]=z1;
            vertColorBAR[vertices_index+1][0]=cr;vertColorBAR[vertices_index+1][1]=cg;vertColorBAR[vertices_index+1][2]=cb;
            verticesBAR[vertices_index+1][0]=x1+sx;
            verticesBAR[vertices_index+1][1]=y1;
            verticesBAR[vertices_index+1][2]=z1;
            vertColorBAR[vertices_index+2][0]=cr;vertColorBAR[vertices_index+2][1]=cg;vertColorBAR[vertices_index+2][2]=cb;
            verticesBAR[vertices_index+2][0]=x1;
            verticesBAR[vertices_index+2][1]=y1;
            verticesBAR[vertices_index+2][2]=z1-sz;
            vertColorBAR[vertices_index+3][0]=cr;vertColorBAR[vertices_index+3][1]=cg;vertColorBAR[vertices_index+3][2]=cb;
            verticesBAR[vertices_index+3][0]=x1+sx;
            verticesBAR[vertices_index+3][1]=y1;
            verticesBAR[vertices_index+3][2]=z1-sz;
            //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertBARindices[indices_index++]=vertices_index+0;
            vertBARindices[indices_index++]=vertices_index+0;
            vertBARindices[indices_index++]=vertices_index+1;
            vertBARindices[indices_index++]=vertices_index+2;
            vertBARindices[indices_index++]=vertices_index+3;
            vertBARindices[indices_index++]=vertices_index+3;
            vertices_index+=4;
        }
    }
    
    
    glDrawElements(GL_TRIANGLE_STRIP,indices_index,GL_UNSIGNED_SHORT,vertBARindices);
    
    //printf("vertices_count: %d\n",vertices_count+24*data_bar2draw_count);
    
    
    if (tgt_note_max>0) note_max=tgt_note_max;
    if (tgt_note_min<127) note_min=tgt_note_min;
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    
    //glDisable(GL_BLEND);
    //    glEnable(GL_DEPTH_TEST);
    
    /* Pop The Matrix */
    glPopMatrix();
    
    
}

void RenderUtils::DrawMidiFX(int *data,uint ww,uint hh,int horiz_vert,int note_display_range, int note_display_offset,int fx_len,int color_mode) {
    LineVertex *ptsB;
    int crt,cgt,cbt,ca;
    int index;
    //int band_width,ofs_band;
    float band_width;
    int line_width;
    
    if (fx_len!=data_midifx_len) {
        data_midifx_len=fx_len;
        data_midifx_first=1;
    }
    
    //if first launch, clear buffers
    if (data_midifx_first) {
        data_midifx_first=0;
        for (int i=0;i<data_midifx_len;i++) {
            memset(data_midifx_note[i],0,256);
        }
    }
    //update old ones
    for (int j=0;j<data_midifx_len-1;j++) {
        memcpy(data_midifx_note[j],data_midifx_note[j+1],256);
        memcpy(data_midifx_instr[j],data_midifx_instr[j+1],256);
        memcpy(data_midifx_vol[j],data_midifx_vol[j+1],256);
        memcpy(data_midifx_st[j],data_midifx_st[j+1],256);
    }
    //add new one
    for (int i=0;i<256;i++) {
        int note=data[i];
        data_midifx_note[data_midifx_len-1][i]=note&0xFF;
        data_midifx_instr[data_midifx_len-1][i]=(note>>8)&0xFF;
        data_midifx_st[data_midifx_len-1][i]=(note>>24)&0xFF;
        data_midifx_vol[data_midifx_len-1][i]=(note>>16)&0xFF;
        
    }
    
    
    ptsB=(LineVertex*)malloc(sizeof(LineVertex)*2*MAX_BARS);
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    if (horiz_vert==0) {//Horiz
        band_width=(float)(ww+0*ww/4)/data_midifx_len;
        //        ofs_band=(ww-band_width*data_midifx_len)>>1;
        line_width=2*hh/note_display_range;
    } else { //vert
        band_width=(float)(hh+0*hh/4)/data_midifx_len;
        //        ofs_band=(hh-band_width*data_midifx_len)>>1;
        line_width=2*ww/note_display_range;
    }
    
    
    glDisable(GL_BLEND);
    
    glVertexPointer(2, GL_SHORT, sizeof(LineVertex), &ptsB[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertex), &ptsB[0].r);
    
    /*for (int j=data_midifx_len-1;j>=0;j--) {
     if (j!=data_midifx_len-1-MIDIFX_OFS) glLineWidth(line_width);
     else glLineWidth(line_width+2);
     index=0;
     for (int i=0; i<256; i++) {
     if (data_midifx_note[j][i]) {
     int instr=data_midifx_instr[j][i];
     int vol=data_midifx_vol[j][i];
     int st=data_midifx_st[j][i];
     int pos=(data_midifx_note[j][i])*line_width/4-note_display_offset;
     cr=data_midifx_col[instr&0xF]>>16;
     cg=(data_midifx_col[instr&0xF]>>8)&0xFF;
     cb=data_midifx_col[instr&0xF]&0xFF;
     if (instr&0x10) { //if instru is >= 16, reversed palette is used
     cr^=0xFF;
     cg^=0xFF;
     cb^=0xFF;
     }
     cr=(cr*vol>>6);
     cg=(cg*vol>>6);
     cb=(cb*vol>>6);
     if ((j==data_midifx_len-1-MIDIFX_OFS)&&(st&VOICE_ON)&&vol) {
     cr=255;//(cr+255*3)>>2;
     cg=255;//(cg+255*3)>>2;
     cb=255;//(cb+255*3)>>2;
     }
     
     if (cr>255) cr=255;
     if (cg>255) cg=255;
     if (cb>255) cb=255;
     
     if (vol) {
     //ca=vol*vol; if(ca>255) ca=255;
     ca=255;
     if (horiz_vert==0) { //horiz
     if ((pos>=0)&&(pos<hh)) {
     ptsB[index++] = LineVertex(j*band_width, pos,cr,cg,cb,ca);
     ptsB[index++] = LineVertex(j*band_width+band_width, pos,cr,cg,cb,ca);
     }
     } else {
     if ((pos>=0)&&(pos<ww)) {
     ptsB[index++] = LineVertex(pos,j*band_width,cr,cg,cb,ca);
     ptsB[index++] = LineVertex(pos,j*band_width+band_width,cr,cg,cb,ca);
     }
     }
     }
     }
     }
     glDrawArrays(GL_LINES, 0, index);
     
     }
     */
    
    //////////////////////////////////////////////
    
    int data_bar2draw_count=0;
    
    
    
    for (int i=0; i<256; i++) { //for each channels
        int j=0;
        while (j<data_midifx_len) {  //while not having reach roof
            if (data_midifx_note[j][i]) {  //do we have a note ?
                int instr=data_midifx_instr[j][i];
                int vol=data_midifx_vol[j][i];
                int st=data_midifx_st[j][i];
                int note=data_midifx_note[j][i];
                
                if (vol&&(st&VOICE_ON)) {  //check volume & status => we have something
                    data_bar2draw[data_bar2draw_count].startidx=j;
                    data_bar2draw[data_bar2draw_count].note=note&127;
                    data_bar2draw[data_bar2draw_count].instr=instr;
                    data_bar2draw[data_bar2draw_count].size=0;
                    while ((data_midifx_instr[j][i]==instr)&&(data_midifx_note[j][i]==note)&&(vol&&(st&VOICE_ON))) {  //while same bar (instru & notes), increase size
                        data_bar2draw[data_bar2draw_count].size++;
                        if (j==(data_midifx_len-MIDIFX_OFS-1)) data_bar2draw[data_bar2draw_count].note|=128;
                        j++;
                        if (j==data_midifx_len) break;
                        vol=data_midifx_vol[j][i];
                        st=data_midifx_st[j][i];
                    }
                    data_bar2draw_count++;
                } else j++;
            } else j++;
            if (data_bar2draw_count==MAX_BARS) break;
        }
        if (data_bar2draw_count==MAX_BARS) break;
    }
    qsort(data_bar2draw,data_bar2draw_count,sizeof(t_data_bar2draw),qsort_CompareBar);
    
    if (data_bar2draw_count>=2) { //propagate played flag
        for (int i=1;i<data_bar2draw_count;i++) {
            int note=data_bar2draw[i-1].note&127;
            int played=data_bar2draw[i-1].note&128;
            int instr=data_bar2draw[i-1].instr;
            
            if (played) {
                if ((data_bar2draw[i].instr==instr)&&((data_bar2draw[i].note&127)==note)&&
                    (data_bar2draw[i].startidx<=(data_bar2draw[i-1].startidx+data_bar2draw[i-1].size)))
                    data_bar2draw[i].note|=128;
                /*                if ((data_bar2draw[i].instr==instr)&&((data_bar2draw[i].note&127)==note)&&
                 (data_bar2draw[i].startidx==data_bar2draw[i-1].startidx))
                 data_bar2draw[i].note|=128;*/
            }
        }
        
        for (int i=data_bar2draw_count-2;i>=0;i--) {
            int note=data_bar2draw[i+1].note&127;
            int played=data_bar2draw[i+1].note&128;
            int instr=data_bar2draw[i+1].instr;
            
            if (played) {
                if ((data_bar2draw[i].instr==instr)&&((data_bar2draw[i].note&127)==note)&&
                    (data_bar2draw[i+1].startidx<=(data_bar2draw[i].startidx+data_bar2draw[i].size))) data_bar2draw[i].note|=128;
                
                /*if ((data_bar2draw[i].instr==instr)&&((data_bar2draw[i].note&127)==note)&&
                 (data_bar2draw[i+1].startidx==data_bar2draw[i].startidx)) data_bar2draw[i].note|=128;
                 */
            }
        }
    }
    
    for (int i=1;i<data_bar2draw_count;i++) {
        if ((data_bar2draw[i].instr==data_bar2draw[i-1].instr)&&
            (data_bar2draw[i].note==data_bar2draw[i-1].note)&&
            (data_bar2draw[i].startidx>=data_bar2draw[i-1].startidx)&&
            (data_bar2draw[i].startidx+data_bar2draw[i].size<=data_bar2draw[i-1].startidx+data_bar2draw[i-1].size)) {
            data_bar2draw[i].size=0;
        }
    }
    
    glLineWidth(line_width*(is_retina?2:1));
    index=0;
    
    //TO OPTIMIZE
    int data_bar_2dmap[128*MIDIFX_LEN];
    memset(data_bar_2dmap,0,128*MIDIFX_LEN*4);
    
    for (int i=0;i<data_bar2draw_count;i++) {
        int note=data_bar2draw[i].note&127;
        int played=data_bar2draw[i].note&128;
        int instr=data_bar2draw[i].instr;
        int colidx;
        if (color_mode==0) { //note
            colidx=note%12;
        } else if (color_mode==1) { //instru
            colidx=instr&31;
        }
        
        if (data_bar2draw[i].size==0) continue;
        
        //printf("i:%d start:%d end:%d instr:%d note:%d played:%d\n",i,data_bar2draw[i].startidx,data_bar2draw[i].startidx+data_bar2draw[i].size,instr,note,played);
        
        
        int adj_size=0;
        for (int j=data_bar2draw[i].startidx;j<data_bar2draw[i].startidx+data_bar2draw[i].size;j++) {
            int _instr=(data_bar_2dmap[note*MIDIFX_LEN+j]>>16);
            int draw_count=data_bar_2dmap[note*MIDIFX_LEN+j]&255;
            if (draw_count) {
                if (_instr!=(instr+1)) {
                    draw_count++;
                    data_bar_2dmap[note*MIDIFX_LEN+j]=(((int)(data_bar2draw[i].instr)+1)<<16)|draw_count;
                }
                if (adj_size<(draw_count-1)) adj_size=(draw_count-1);
            } else {
                data_bar_2dmap[note*MIDIFX_LEN+j]=(((int)(data_bar2draw[i].instr)+1)<<16)|1;
            }
        }
        //        printf("adj: %f\n",adj_size);
        
        
        crt=(data_midifx_col[colidx&15]>>16);
        cgt=((data_midifx_col[colidx&15]>>8)&0xFF);
        cbt=(data_midifx_col[colidx&15]&0xFF);
        
        if (colidx&0x10) {
            crt=(crt+255)/2;
            cgt=(cgt+255)/2;
            cbt=(cbt+255)/2;
        }
        
        if (played) {
            crt=(crt+255*2)/3;
            cgt=(cgt+255*2)/3;
            cbt=(cbt+255*2)/3;
        }
        ca=255;
        
        if (horiz_vert==0) { //horiz
            int posNote=note*line_width/2-note_display_offset;
            int posStart=(int)(data_bar2draw[i].startidx)*ww/data_midifx_len;
            int posEnd=((int)(data_bar2draw[i].startidx)+(int)(data_bar2draw[i].size))*ww/data_midifx_len;
            if ((posNote>=0)&&(posNote<hh)) {
                ptsB[index++] = LineVertex(posStart, posNote,crt,cgt,cbt,ca);
                ptsB[index++] = LineVertex(posEnd, posNote,crt,cgt,cbt,ca);
            }
        } else {
            int posNote=note*line_width/2-note_display_offset;
            int posStart=(int)(data_bar2draw[i].startidx)*hh/data_midifx_len;
            int posEnd=((int)(data_bar2draw[i].startidx)+(int)(data_bar2draw[i].size))*hh/data_midifx_len;
            if ((posNote>=0)&&(posNote<hh)) {
                ptsB[index++] = LineVertex(posNote, posStart, crt,cgt,cbt,ca);
                ptsB[index++] = LineVertex(posNote, posEnd, crt,cgt,cbt,ca);
            }        }
        
    }
    glDrawArrays(GL_LINES, 0, index);
    
    
    //////////////////////////////////////////////
    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_BLEND);
    
    
    //current playing line
    //    230,76,153
    if (horiz_vert==0) {
        ptsB[0] = LineVertex((data_midifx_len-MIDIFX_OFS-1)*band_width, 0,120,100,200,120);
        ptsB[1] = LineVertex((data_midifx_len-MIDIFX_OFS-1)*band_width, hh, 120,100,200,120);
    } else {
        ptsB[0] = LineVertex( 0,(data_midifx_len-MIDIFX_OFS-1)*band_width,120,100,200,120);
        ptsB[1] = LineVertex(ww,(data_midifx_len-MIDIFX_OFS-1)*band_width,  120,100,200,120);
        
    }
    glLineWidth(band_width*(is_retina?2:1));
    glDrawArrays(GL_LINES, 0, 2);
    
    /*    if (horiz_vert==0) {
     ptsB[0] = LineVertex((data_midifx_len-MIDIFX_OFS-2)*band_width, 0,80,70,100,120);
     ptsB[1] = LineVertex((data_midifx_len-MIDIFX_OFS-2)*band_width, hh, 80,70,100,120);
     ptsB[2] = LineVertex((data_midifx_len-MIDIFX_OFS)*band_width, 0,200,160,250,120);
     ptsB[3] = LineVertex((data_midifx_len-MIDIFX_OFS)*band_width, hh, 200,160,250,120);
     } else {
     ptsB[0] = LineVertex(0,(data_midifx_len-MIDIFX_OFS-1)*band_width,80,70,100,120);
     ptsB[1] = LineVertex(ww,(data_midifx_len-MIDIFX_OFS-1)*band_width,  80,70,100,120);
     ptsB[2] = LineVertex(0,(data_midifx_len-MIDIFX_OFS)*band_width,200,160,250,120);
     ptsB[3] = LineVertex(ww,(data_midifx_len-MIDIFX_OFS)*band_width, 200,160,250,120);
     
     }
     //    glLineWidth(2.0f*(is_retina?2:1));
     glDrawArrays(GL_LINES, 0, 4);*/
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisable(GL_BLEND);
    
    free(ptsB);
    
}
