/*
 *  RenderUtils.mm
 *  modizer
 *
 *  Created by Yohann Magnien on 23/08/10.
 *  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
 *
 */

extern int NOTES_DISPLAY_TOPMARGIN;

#include "RenderUtils.h"
#include "TextureUtils.h"

#import "SettingsGenViewController.h"
extern volatile t_settings settings[MAX_SETTINGS];

#import "Font.h"
#import "GLString.h"

unsigned int data_midifx_pal1[32]={
    0x0000fe, 0xfd00fe, 0xfe0000, 0x0aff05, 0xff78ff, 0x7900ff, 0x0077fe, 0x9701ff, 0xfeff05, 0x0700ba, 0x77fe77, 0x4187ba, 0xb98744, 0xf034ab, 0xaa31ec, 0xaa0001, 0x00ab05, 0x0003ac, 0xedb1ff, 0x154e56, 0x8d476f, 0x6c8c60, 0xf87574, 0xf6e38b, 0x5b430b, 0xa2f0eb, 0xe3e0f5, 0x115205, 0x39eec0, 0x1f3e9e, 0x89aa0d, 0xfb7810
};

unsigned int *data_midifx_col=data_midifx_pal1;


//class CFont;
//class CGLString;

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

extern int MIDIFX_OFS;

static int max_indices;

int txt_pianoRoll[3];

#define INDICES_SIZE_KEYW 60
#define INDICES_SIZE_KEYB 36
#define INDICES_SIZE_BOX 30

#define MAX_BARS 4096
typedef struct {
    unsigned int frameidx;
    signed short int startidx;
    unsigned char note;
    unsigned char subnote;
    unsigned char instr;
    unsigned char played;
    signed short int size;
    
} t_data_bar2draw;
static t_data_bar2draw data_bar2draw[MAX_BARS];

static int pianoroll_cpt;

#define PR_KEY_PRESSED (1<<0)
#define PR_WHITE_KEY (1<<1)
static uint8_t pianoroll_key_status[SOUND_MAXMOD_CHANNELS][256];

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


void RenderUtils::SetUpOrtho(float rotation,uint width,uint height)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glRotatef(rotation, 0, 0, 1);
    glOrthof(0, width, 0, height, 0, 200);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
}

#define OSCILLO_BUFFER_NB 4
#define OSCILLO_BUFFER_SIZE SOUND_BUFFER_SIZE_SAMPLE*OSCILLO_BUFFER_NB
static signed char *prev_snd_data;
static signed char *prev_snd_dataStereo;
static int snd_data_ofs[SOUND_MAXVOICES_BUFFER_FX];
static signed char cur_snd_data[OSCILLO_BUFFER_SIZE*SOUND_MAXVOICES_BUFFER_FX];

static CFont *mOscilloFont[3]={NULL,NULL,NULL};
static CGLString *mVoicesName[SOUND_MAXVOICES_BUFFER_FX];
static CGLString *mVoicesNamePiano[SOUND_MAXVOICES_BUFFER_FX];
static CGLString *mOctavesIndex[256/12];
static int mVoicesName_FontSize;

#define FX_OSCILLO_MAXROWS 16
#include "ModizerVoicesData.h"

#define absint(a) (a>=0?a:-a)

#define FIXED_POINT_PRECISION 16
void RenderUtils::DrawOscilloMultiple(signed char **snd_data,int snd_data_idx,int num_voices,uint ww,uint hh,uint color_mode,float mScaleFactor,bool isfullscreen,char *voices_label,bool draw_frame) {
    LineVertex *pts;
    int mulfactor;
    int val[SOUND_MAXVOICES_BUFFER_FX];
    int oval[SOUND_MAXVOICES_BUFFER_FX];
    int sp[SOUND_MAXVOICES_BUFFER_FX];
    int osp[SOUND_MAXVOICES_BUFFER_FX];
    
    int colR,colG,colB,tmpR,tmpG,tmpB,colA;
    int count;
    int64_t max_gap,tmp_gap,ofs1,ofs2,old_ofs;
    
    static char first_call=1;
    
    //snd_data_idx--;
    //snd_data_idx-=OSCILLO_BUFFER_NB;
    while (snd_data_idx<0) snd_data_idx+=SOUND_BUFFER_NB;
    while (snd_data_idx>=SOUND_BUFFER_NB) snd_data_idx-=SOUND_BUFFER_NB;
    
    int max_len_oscillo_buffer=735;// 1frame at 60fps & 44100Hz, assume OSCILLO_BUFFER_SIZE>735  OSCILLO_BUFFER_SIZE*2/6;
    int max_ofs=OSCILLO_BUFFER_SIZE-max_len_oscillo_buffer;
    int min_ofs=0;
    
    
    colA=128;
    
    for (int i=0;i<num_voices;i++)
        for (int k=0;k<OSCILLO_BUFFER_NB;k++) {
            for (int j=0;j<SOUND_BUFFER_SIZE_SAMPLE;j++) {
                cur_snd_data[(j+k*SOUND_BUFFER_SIZE_SAMPLE)*SOUND_MAXVOICES_BUFFER_FX+i]=snd_data[(snd_data_idx+k)%SOUND_BUFFER_NB][j*SOUND_MAXVOICES_BUFFER_FX+i];
            }
        }
    
    if (first_call) {
        prev_snd_data=(signed char*)malloc(OSCILLO_BUFFER_SIZE*SOUND_MAXVOICES_BUFFER_FX);
        if (!prev_snd_data) {
            printf("%s: cannot allocate prev_snd_data\n",__func__);
            return;
        }
        memcpy(prev_snd_data,cur_snd_data,OSCILLO_BUFFER_SIZE*SOUND_MAXVOICES_BUFFER_FX);
        
        for (int i=0;i<SOUND_MAXVOICES_BUFFER_FX;i++) {
            snd_data_ofs[i]=max_ofs/2;
            mVoicesName[i]=NULL;
        }
        mVoicesName_FontSize=-1;
        
        first_call=0;
    }
    
    if (!mOscilloFont[0]) {
        NSString *fontPath;
        //if (mScaleFactor<2) fontPath = [[NSBundle mainBundle] pathForResource:@"tracking10" ofType: @"fnt"];
        //else fontPath = [[NSBundle mainBundle] pathForResource:@"tracking14" ofType: @"fnt"];
        fontPath = [[NSBundle mainBundle] pathForResource:@"tracking10" ofType: @"fnt"];
        if (fontPath) mOscilloFont[0] = new CFont([fontPath UTF8String]);
        fontPath = [[NSBundle mainBundle] pathForResource:@"tracking16" ofType: @"fnt"];
        if (fontPath) mOscilloFont[1] = new CFont([fontPath UTF8String]);
        fontPath = [[NSBundle mainBundle] pathForResource:@"tracking24" ofType: @"fnt"];
        if (fontPath) mOscilloFont[2] = new CFont([fontPath UTF8String]);
    }
    
    if (mOscilloFont[1] && voices_label)
        for (int i=0;i<num_voices;i++) {
            if (mVoicesName[i]) {
                if ((settings[OSCILLO_LabelFontSize].detail.mdz_switch.switch_value!=mVoicesName_FontSize) || (strcmp(mVoicesName[i]->mText,voices_label+i*32))) {
                    //not the same, reset string
                    delete mVoicesName[i];
                    mVoicesName[i]=NULL;
                }
            }
            if (!mVoicesName[i]) {
                if (mOscilloFont[settings[OSCILLO_LabelFontSize].detail.mdz_switch.switch_value]) mVoicesName[i]=new CGLString(voices_label+i*32, mOscilloFont[settings[OSCILLO_LabelFontSize].detail.mdz_switch.switch_value],mScaleFactor);
            }
        }
    mVoicesName_FontSize=settings[OSCILLO_LabelFontSize].detail.mdz_switch.switch_value;
    
    int columns_nb=((num_voices-1)/FX_OSCILLO_MAXROWS)+1;
    int columns_width=ww/columns_nb;
    
    int max_voices_by_row=(num_voices+columns_nb-1)/columns_nb;
    float ratio;
    
    //check best config, maximize 16/9 ratio
    if (num_voices>=1)
        for (;;) {
            columns_width=ww/columns_nb;
            max_voices_by_row=(num_voices+columns_nb-1)/columns_nb;
            mulfactor=(hh-8)/(max_voices_by_row)/2;
            ratio=columns_width/(2*mulfactor);
            
            if (ratio<=2) break;
            if (columns_nb>=num_voices) break;
            
            columns_nb++;
            
        }
    // NSLog(@"%d %d / %f",columns_width,mulfactor,ratio);
    
    int xofs=(ww-columns_width*columns_nb)/2;
    int smpl_ofs_incr=(max_len_oscillo_buffer)*(1<<FIXED_POINT_PRECISION)/columns_width;
    int cur_voices=0;
    
    int max_count=2*columns_width*num_voices;
    pts=(LineVertex*)malloc(sizeof(LineVertex)*2*columns_width*num_voices);
    if (!pts) {
        printf("%s: cannot allocate LineVertex buffer\n",__func__);
        return;
    }
    count=0;
    
    //determine min smplincr / width of oscillo on screen, help reduce processing time
    int smplincr=OSCILLO_BUFFER_SIZE/columns_width;
    if (smplincr<1) smplincr=1;
    int bufflen=max_len_oscillo_buffer/smplincr;
    
    // min gap to match/allow
    int min_gap_threshold=0;//bufflen;
    for (int j=0;j<num_voices;j++) {
        // for each voices
        max_gap=0;
        //reset start offset / previous frame
        old_ofs=0;
        
        ofs1=snd_data_ofs[j];
        ofs2=snd_data_ofs[j]-smplincr;
        int right_done=0;
        int left_done=0;
        for (;;) {
            // start analyzing
            
            //check on right side, ofs1
            if ((ofs1<max_ofs)&& !right_done) {
                tmp_gap=0;
                signed char *snd_data_ptr=cur_snd_data+ofs1*SOUND_MAXVOICES_BUFFER_FX+j;
                signed char *prev_snd_data_ptr=prev_snd_data+j;
                int64_t val;
                int incr=smplincr*SOUND_MAXVOICES_BUFFER_FX;
                for (int i=0;i<bufflen;i++) {
                    //compute diff between 2 samples with respective offset
                    val=(int)(*snd_data_ptr)*(int)(*prev_snd_data_ptr);
                    tmp_gap+=val;
                    snd_data_ptr+=incr;
                    prev_snd_data_ptr+=incr;
                }
                
                //if (tmp_gap<min_gap) { //if more aligned, use ofs as new ref
                if (max_gap<tmp_gap) {
                    max_gap=tmp_gap;
                    snd_data_ofs[j]=ofs1;
                }
                
                ofs1+=smplincr;
            } else right_done=1;
            //check on left side, ofs2
            if ((ofs2>0)&& !left_done) {
                tmp_gap=0;
                signed char *snd_data_ptr=cur_snd_data+ofs2*SOUND_MAXVOICES_BUFFER_FX+j;
                signed char *prev_snd_data_ptr=prev_snd_data+j;
                int64_t val;
                int incr=smplincr*SOUND_MAXVOICES_BUFFER_FX;
                for (int i=0;i<bufflen;i++) {
                    //compute diff between 2 samples with respective offset
                    val=(int)(*snd_data_ptr)*(int)(*prev_snd_data_ptr);
                    tmp_gap+=val;
                    snd_data_ptr+=incr;
                    prev_snd_data_ptr+=incr;
                }
                
                if (tmp_gap>max_gap) { //if more aligned, use ofs as new ref
                    max_gap=tmp_gap;
                    snd_data_ofs[j]=ofs2;
                }
                ofs2-=smplincr;
            } else left_done=1;
            
            if ( left_done && right_done ) break;
        }
        //snd_data_ofs[j]=0;
    }
    
    for (int i=0;i<max_len_oscillo_buffer;i++){
        for (int j=0;j<num_voices;j++) {
            prev_snd_data[i*SOUND_MAXVOICES_BUFFER_FX+j]=cur_snd_data[(i+(snd_data_ofs[j]))*SOUND_MAXVOICES_BUFFER_FX+j];
        }
    }
    
    for (int i=0;i<num_voices;i++) {
        val[i]=(signed int)(cur_snd_data[((snd_data_ofs[i]))*SOUND_MAXVOICES_BUFFER_FX+i])*(mulfactor-1)>>7;
        sp[i]=(val[i]); if(sp[i]>=mulfactor) sp[i]=mulfactor-1; if (sp[i]<=-mulfactor) sp[i]=-mulfactor+1;
    }
    
    for (int r=0;r<columns_nb;r++) {
        int xpos=xofs+r*columns_width;
        int max_voices=num_voices*(r+1)/columns_nb;
        int ypos=hh-mulfactor;
        
        for (;cur_voices<max_voices;cur_voices++,ypos-=2*mulfactor) {
            int smpl_ofs=snd_data_ofs[cur_voices]<<FIXED_POINT_PRECISION;
            
            if (color_mode==1) {
                colR=(settings[OSCILLO_MONO_COLOR].detail.mdz_color.rgb>>16)&0xFF;
                colG=(settings[OSCILLO_MONO_COLOR].detail.mdz_color.rgb>>8)&0xFF;
                colB=(settings[OSCILLO_MONO_COLOR].detail.mdz_color.rgb>>0)&0xFF;
                
            } else {
                colR=((m_voice_voiceColor[cur_voices]>>16)&0xFF);
                colG=((m_voice_voiceColor[cur_voices]>>8)&0xFF);
                colB=((m_voice_voiceColor[cur_voices]>>0)&0xFF);
                /*colR*=1.2f;
                 colG*=1.2f;
                 colB*=1.2f;*/
            }
            
            //draw label if specified
            if (voices_label&&mVoicesName[cur_voices]) {
                glPushMatrix();
                glTranslatef(xpos+4,ypos+mulfactor-4-(mOscilloFont[settings[OSCILLO_LabelFontSize].detail.mdz_switch.switch_value]->maxCharHeight/mScaleFactor), 0.0f);
                mVoicesName[cur_voices]->Render(255);
                glPopMatrix();
            }
            
            for (int i=0; i<columns_width-2; i++) {
                oval[cur_voices]=val[cur_voices];
                val[cur_voices]=cur_snd_data[((smpl_ofs>>FIXED_POINT_PRECISION))*SOUND_MAXVOICES_BUFFER_FX+cur_voices];
                osp[cur_voices]=sp[cur_voices];
                sp[cur_voices]=(val[cur_voices])*(mulfactor-1)>>7; if(sp[cur_voices]>=mulfactor) sp[cur_voices]=mulfactor-1; if (sp[cur_voices]<=-mulfactor) sp[cur_voices]=-mulfactor+1;
                
                tmpR=colR;//+((val[cur_voices]-oval[cur_voices])<<1);
                tmpG=colG;//+((val[cur_voices]-oval[cur_voices])<<1);
                tmpB=colB;//+((val[cur_voices]-oval[cur_voices])<<1);
                if (tmpR>255) tmpR=255;if (tmpG>255) tmpG=255;if (tmpB>255) tmpB=255;
                if (tmpR<0) tmpR=0;if (tmpG<0) tmpG=0;if (tmpB<0) tmpB=0;
                
                if (count>=max_count-1) break;
                pts[count++] = LineVertex(xpos+i, osp[cur_voices]+ypos,tmpR,tmpG,tmpB,colA);
                pts[count++] = LineVertex(xpos+i+1, sp[cur_voices]+ypos,tmpR,tmpG,tmpB,colA);
                
                smpl_ofs+=smpl_ofs_incr;//*3/4;
            }
        }
    }
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    //    glLineWidth(2.0f*mScaleFactor);
    switch (settings[OSCILLO_LINE_Width].detail.mdz_switch.switch_value) {
        case 0:glLineWidth(1.0f*mScaleFactor);
            break;
        case 1:glLineWidth(2.0f*mScaleFactor);
            break;
        case 2:glLineWidth(3.0f*mScaleFactor);
            break;
    }
    
    glVertexPointer(2, GL_SHORT, sizeof(LineVertex), &pts[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertex), &pts[0].r);
    
    if (count>=max_count) {
        printf("%s: count too high: %d / %d\n",__func__,count,max_count);
    } else glDrawArrays(GL_LINES, 0, count);
    
    if (draw_frame) {
        //draw frame
        count=0;
        glLineWidth(1.0f*mScaleFactor);
        colR=(settings[OSCILLO_GRID_COLOR].detail.mdz_color.rgb>>16)&0xFF;
        colG=(settings[OSCILLO_GRID_COLOR].detail.mdz_color.rgb>>8)&0xFF;
        colB=(settings[OSCILLO_GRID_COLOR].detail.mdz_color.rgb>>0)&0xFF;
        //top
        pts[count++] = LineVertex(0, hh-1,colR,colG,colB,colA);
        pts[count++] = LineVertex(ww-1,hh-1,colR,colG,colB,colA);
        //right
        pts[count++] = LineVertex(ww-1,hh-1,colR,colG,colB,colA);
        pts[count++] = LineVertex(ww-1,hh-mulfactor*max_voices_by_row*2,colR,colG,colB,colA);
        //bottom
        pts[count++] = LineVertex(ww-1,hh-mulfactor*max_voices_by_row*2,colR,colG,colB,colA);
        pts[count++] = LineVertex(0,hh-mulfactor*max_voices_by_row*2,colR,colG,colB,colA);
        //left
        pts[count++] = LineVertex(0,hh-mulfactor*max_voices_by_row*2,colR,colG,colB,colA);
        pts[count++] = LineVertex(0,hh-1,colR,colG,colB,colA);
        
        for (int r=0;r<columns_nb;r++) {
            int xpos=xofs+r*columns_width;
            int max_voices=num_voices*(r+1)/columns_nb;
            int ypos=hh-mulfactor;
            
            pts[count++] = LineVertex(xpos,hh-1,colR,colG,colB,colA);
            pts[count++] = LineVertex(xpos,hh-mulfactor*max_voices_by_row*2,colR,colG,colB,colA);
        }
        for (int r=0;r<max_voices_by_row;r++) {
            pts[count++] = LineVertex(0,hh-mulfactor*r*2,colR,colG,colB,colA);
            pts[count++] = LineVertex(ww-1,hh-mulfactor*r*2,colR,colG,colB,colA);
        }
        
        if (count>=max_count) {
            printf("%s: count too high: %d / %d\n",__func__,count,max_count);
        } else glDrawArrays(GL_LINES, 0, count);
    }
    
    glLineWidth(1.0f*mScaleFactor);
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisable(GL_BLEND);
    free(pts);
}

void RenderUtils::DrawOscilloStereo(short int **snd_data,int snd_data_idx,uint ww,uint hh,uint color_mode,float mScaleFactor,bool isfullscreen,bool draw_frame) {
    LineVertex *pts;
    int mulfactor;
    int val[SOUND_MAXVOICES_BUFFER_FX];
    int oval[SOUND_MAXVOICES_BUFFER_FX];
    int sp[SOUND_MAXVOICES_BUFFER_FX];
    int osp[SOUND_MAXVOICES_BUFFER_FX];
    
    int colR,colG,colB,tmpR,tmpG,tmpB,colA;
    int count;
    int min_gap,tmp_gap,ofs1,ofs2,old_ofs;
    
    static char first_call=1;
    
    //uint color_mode,float mScaleFactor,bool isfullscreen,char *voices_label,bool draw_frame) {
    int num_voices=2;
    char *voices_label=NULL;
    
    //snd_data_idx--;
    //snd_data_idx-=OSCILLO_BUFFER_NB;
    while (snd_data_idx<0) snd_data_idx+=SOUND_BUFFER_NB;
    while (snd_data_idx>=SOUND_BUFFER_NB) snd_data_idx-=SOUND_BUFFER_NB;
    
    int max_len_oscillo_buffer=735;// 1frame at 60fps & 44100Hz, assume OSCILLO_BUFFER_SIZE>735  OSCILLO_BUFFER_SIZE*2/6;
    int max_ofs=OSCILLO_BUFFER_SIZE-max_len_oscillo_buffer;
    int min_ofs=0;
    
    
    colA=128;
    
    for (int i=0;i<num_voices;i++)
        for (int k=0;k<OSCILLO_BUFFER_NB;k++) {
            for (int j=0;j<SOUND_BUFFER_SIZE_SAMPLE;j++) {
                cur_snd_data[(j+k*SOUND_BUFFER_SIZE_SAMPLE)*SOUND_MAXVOICES_BUFFER_FX+i]=snd_data[(snd_data_idx+k)%SOUND_BUFFER_NB][j*2+i]>>8;
            }
        }
    
    if (first_call) {
        prev_snd_data=(signed char*)malloc(OSCILLO_BUFFER_SIZE*SOUND_MAXVOICES_BUFFER_FX);
        if (!prev_snd_data) {
            printf("%s: cannot allocate prev_snd_data\n",__func__);
            return;
        }
        memcpy(prev_snd_data,cur_snd_data,OSCILLO_BUFFER_SIZE*SOUND_MAXVOICES_BUFFER_FX);
        
        for (int i=0;i<SOUND_MAXVOICES_BUFFER_FX;i++) {
            snd_data_ofs[i]=max_ofs/2;
        }
        
        first_call=0;
    }
    
    int columns_nb=((num_voices-1)/FX_OSCILLO_MAXROWS)+1;
    int columns_width=ww/columns_nb;
    
    int max_voices_by_row=(num_voices+columns_nb-1)/columns_nb;
    float ratio;
    
    //check best config, maximize 16/9 ratio
    //    if (num_voices>=1)
    //        for (;;) {
    //            columns_width=ww/columns_nb;
    //            max_voices_by_row=(num_voices+columns_nb-1)/columns_nb;
    //            mulfactor=(hh-8)/(max_voices_by_row)/2;
    //            ratio=columns_width/(2*mulfactor);
    //
    //            if (ratio<=2) break;
    //            if (columns_nb>=num_voices) break;
    //
    //            columns_nb++;
    //
    //        }
    // NSLog(@"%d %d / %f",columns_width,mulfactor,ratio);
    //stereo, force 1 column
    columns_nb=1;
    columns_width=ww/columns_nb;
    max_voices_by_row=(num_voices+columns_nb-1)/columns_nb;
    mulfactor=(hh-8)/(max_voices_by_row)/2;
    ratio=columns_width/(2*mulfactor);
    
    int xofs=(ww-columns_width*columns_nb)/2;
    int smpl_ofs_incr=(max_len_oscillo_buffer)*(1<<FIXED_POINT_PRECISION)/columns_width;
    int cur_voices=0;
    
    int max_count=2*columns_width*num_voices;
    pts=(LineVertex*)malloc(sizeof(LineVertex)*2*columns_width*num_voices);
    if (!pts) {
        printf("%s: cannot allocate LineVertex buffer\n",__func__);
        return;
    }
    count=0;
    
    //determine min smplincr / width of oscillo on screen, help reduce processing time
    int smplincr=OSCILLO_BUFFER_SIZE/columns_width;
    if (smplincr<1) smplincr=1;
    int bufflen=max_len_oscillo_buffer/smplincr;
    
    // min gap to match/allow
    int min_gap_threshold=0;//bufflen;
    
    for (int j=0;j<num_voices;j++) {
        // for each voices
        min_gap=bufflen*256;
        //reset start offset / previous frame
        old_ofs=0;
        
        ofs1=snd_data_ofs[j];
        ofs2=snd_data_ofs[j];
        int right_done=0;
        int left_done=0;
        for (;;) {
            // start analyzing
            
            //check on right side, ofs1
            if ((ofs1<max_ofs)&& !right_done) {
                tmp_gap=0;
                signed char *snd_data_ptr=cur_snd_data+ofs1*SOUND_MAXVOICES_BUFFER_FX+j;
                signed char *prev_snd_data_ptr=prev_snd_data+j;
                int val;
                int incr=smplincr*SOUND_MAXVOICES_BUFFER_FX;
                for (int i=0;i<bufflen;i++) {
                    //compute diff between 2 samples with respective offset
                    val=(int)(*snd_data_ptr)-(int)(*prev_snd_data_ptr);
                    if (val>0) tmp_gap+=val;
                    else if (val<0) tmp_gap-=val;
                    if (tmp_gap>=min_gap) break; //do not need to pursue, already more gap/previous one
                    snd_data_ptr+=incr;
                    prev_snd_data_ptr+=incr;
                }
                
                if (tmp_gap<min_gap) { //if more aligned, use ofs as new ref
                    min_gap=tmp_gap;
                    snd_data_ofs[j]=ofs1;
                    if (min_gap<=min_gap_threshold) {
                        left_done=1;
                        right_done=1;
                        break;
                    }
                }
                
                ofs1+=smplincr;
            } else right_done=1;
            //check on left side, ofs2
            if ((ofs2>0)&& !left_done) {
                tmp_gap=0;
                signed char *snd_data_ptr=cur_snd_data+ofs2*SOUND_MAXVOICES_BUFFER_FX+j;
                signed char *prev_snd_data_ptr=prev_snd_data+j;
                int val;
                int incr=smplincr*SOUND_MAXVOICES_BUFFER_FX;
                for (int i=0;i<bufflen;i++) {
                    //compute diff between 2 samples with respective offset
                    val=(int)(*snd_data_ptr)-(int)(*prev_snd_data_ptr);
                    if (val>0) tmp_gap+=val;
                    else if (val<0) tmp_gap-=val;
                    if (tmp_gap>=min_gap) break; //do not need to pursue, already more gap/previous one
                    snd_data_ptr+=incr;
                    prev_snd_data_ptr+=incr;
                }
                
                if (tmp_gap<min_gap) { //if more aligned, use ofs as new ref
                    min_gap=tmp_gap;
                    snd_data_ofs[j]=ofs2;
                    if (min_gap<=min_gap_threshold)  {
                        left_done=1;
                        right_done=1;
                        break;
                    }
                }
                ofs2-=smplincr;
            } else left_done=1;
            
            if ( left_done && right_done ) break;
        }
        //snd_data_ofs[j]=0;
    }
    
    for (int i=0;i<max_len_oscillo_buffer;i++){
        for (int j=0;j<num_voices;j++) {
            prev_snd_data[i*SOUND_MAXVOICES_BUFFER_FX+j]=cur_snd_data[(i+(snd_data_ofs[j]))*SOUND_MAXVOICES_BUFFER_FX+j];
        }
    }
    
    for (int i=0;i<num_voices;i++) {
        val[i]=(signed int)(cur_snd_data[((snd_data_ofs[i]))*SOUND_MAXVOICES_BUFFER_FX+i])*(mulfactor-1)>>7;
        sp[i]=(val[i]); if(sp[i]>=mulfactor) sp[i]=mulfactor-1; if (sp[i]<=-mulfactor) sp[i]=-mulfactor+1;
    }
    
    for (int r=0;r<columns_nb;r++) {
        int xpos=xofs+r*columns_width;
        int max_voices=num_voices*(r+1)/columns_nb;
        int ypos=hh-mulfactor;
        
        for (;cur_voices<max_voices;cur_voices++,ypos-=2*mulfactor) {
            int smpl_ofs=snd_data_ofs[cur_voices]<<FIXED_POINT_PRECISION;
            
            if (color_mode==1) {
                colR=(settings[OSCILLO_MONO_COLOR].detail.mdz_color.rgb>>16)&0xFF;
                colG=(settings[OSCILLO_MONO_COLOR].detail.mdz_color.rgb>>8)&0xFF;
                colB=(settings[OSCILLO_MONO_COLOR].detail.mdz_color.rgb>>0)&0xFF;
                
            } else {
                colR=((m_voice_voiceColor[cur_voices]>>16)&0xFF);
                colG=((m_voice_voiceColor[cur_voices]>>8)&0xFF);
                colB=((m_voice_voiceColor[cur_voices]>>0)&0xFF);
                /*colR*=1.2f;
                 colG*=1.2f;
                 colB*=1.2f;*/
            }
            
            for (int i=0; i<columns_width-2; i++) {
                oval[cur_voices]=val[cur_voices];
                val[cur_voices]=cur_snd_data[((smpl_ofs>>FIXED_POINT_PRECISION))*SOUND_MAXVOICES_BUFFER_FX+cur_voices];
                osp[cur_voices]=sp[cur_voices];
                sp[cur_voices]=(val[cur_voices])*(mulfactor-1)>>7; if(sp[cur_voices]>=mulfactor) sp[cur_voices]=mulfactor-1; if (sp[cur_voices]<=-mulfactor) sp[cur_voices]=-mulfactor+1;
                
                tmpR=colR;//+((val[cur_voices]-oval[cur_voices])<<1);
                tmpG=colG;//+((val[cur_voices]-oval[cur_voices])<<1);
                tmpB=colB;//+((val[cur_voices]-oval[cur_voices])<<1);
                if (tmpR>255) tmpR=255;if (tmpG>255) tmpG=255;if (tmpB>255) tmpB=255;
                if (tmpR<0) tmpR=0;if (tmpG<0) tmpG=0;if (tmpB<0) tmpB=0;
                
                if (count>=max_count-1) break;
                pts[count++] = LineVertex(xpos+i, osp[cur_voices]+ypos,tmpR,tmpG,tmpB,colA);
                pts[count++] = LineVertex(xpos+i+1, sp[cur_voices]+ypos,tmpR,tmpG,tmpB,colA);
                
                smpl_ofs+=smpl_ofs_incr;//*3/4;
            }
        }
    }
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    //    glLineWidth(2.0f*mScaleFactor);
    switch (settings[OSCILLO_LINE_Width].detail.mdz_switch.switch_value) {
        case 0:glLineWidth(1.0f*mScaleFactor);
            break;
        case 1:glLineWidth(2.0f*mScaleFactor);
            break;
        case 2:glLineWidth(3.0f*mScaleFactor);
            break;
    }
    
    glVertexPointer(2, GL_SHORT, sizeof(LineVertex), &pts[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertex), &pts[0].r);
    
    if (count>=max_count) {
        printf("%s: count too high: %d / %d\n",__func__,count,max_count);
    } else glDrawArrays(GL_LINES, 0, count);
    
    if (draw_frame) {
        //draw frame
        count=0;
        glLineWidth(1.0f*mScaleFactor);
        colR=(settings[OSCILLO_GRID_COLOR].detail.mdz_color.rgb>>16)&0xFF;
        colG=(settings[OSCILLO_GRID_COLOR].detail.mdz_color.rgb>>8)&0xFF;
        colB=(settings[OSCILLO_GRID_COLOR].detail.mdz_color.rgb>>0)&0xFF;
        //top
        pts[count++] = LineVertex(0, hh-1,colR,colG,colB,colA);
        pts[count++] = LineVertex(ww-1,hh-1,colR,colG,colB,colA);
        //right
        pts[count++] = LineVertex(ww-1,hh-1,colR,colG,colB,colA);
        pts[count++] = LineVertex(ww-1,hh-mulfactor*max_voices_by_row*2,colR,colG,colB,colA);
        //bottom
        pts[count++] = LineVertex(ww-1,hh-mulfactor*max_voices_by_row*2,colR,colG,colB,colA);
        pts[count++] = LineVertex(0,hh-mulfactor*max_voices_by_row*2,colR,colG,colB,colA);
        //left
        pts[count++] = LineVertex(0,hh-mulfactor*max_voices_by_row*2,colR,colG,colB,colA);
        pts[count++] = LineVertex(0,hh-1,colR,colG,colB,colA);
        
        for (int r=0;r<columns_nb;r++) {
            int xpos=xofs+r*columns_width;
            int max_voices=num_voices*(r+1)/columns_nb;
            int ypos=hh-mulfactor;
            
            pts[count++] = LineVertex(xpos,hh-1,colR,colG,colB,colA);
            pts[count++] = LineVertex(xpos,hh-mulfactor*max_voices_by_row*2,colR,colG,colB,colA);
        }
        for (int r=0;r<max_voices_by_row;r++) {
            pts[count++] = LineVertex(0,hh-mulfactor*r*2,colR,colG,colB,colA);
            pts[count++] = LineVertex(ww-1,hh-mulfactor*r*2,colR,colG,colB,colA);
        }
        
        if (count>=max_count) {
            printf("%s: count too high: %d / %d\n",__func__,count,max_count);
        } else glDrawArrays(GL_LINES, 0, count);
    }
    
    glLineWidth(1.0f*mScaleFactor);
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisable(GL_BLEND);
    free(pts);
}


void RenderUtils::DrawOscillo(short int *snd_data,int numval,uint ww,uint hh,uint bg,uint type_oscillo,uint pos,float mScaleFactor) {
    LineVertex *pts,*ptsB;
    int mulfactor;
    int dval,valL,valR,ovalL,ovalR,ospl,ospr,spl,spr,colR1,colL1,colR2,colL2,ypos;
    int count;
    
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
        glLineWidth(2.0f*mScaleFactor);
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
        glLineWidth(1.0f*mScaleFactor);
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
        glLineWidth(2.0f*mScaleFactor);
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

static int DrawSpectrum_first_call=1;
static int spectrumPeakValueL[SPECTRUM_BANDS];
static int spectrumPeakValueR[SPECTRUM_BANDS];
static int spectrumPeakValueL_index[SPECTRUM_BANDS];
static int spectrumPeakValueR_index[SPECTRUM_BANDS];


void RenderUtils::DrawSpectrum(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,uint bg,uint peaks,uint _pos,int nb_spectrum_bands,float mScaleFactor) {
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
    
    glLineWidth((band_width-1)*mScaleFactor);
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

void RenderUtils::DrawFXTouchGrid(uint _ww,uint _hh,int fade_level,int min_level,int active_idx,int cpt,float mScaleFactor) {
    LineVertex pts[24];
    //set the opengl state
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glVertexPointer(2, GL_SHORT, sizeof(LineVertex), &pts[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertex), &pts[0].r);
    
    int menu_cell_size;
    if (_ww<_hh) menu_cell_size=_ww;
    else menu_cell_size=_hh;
    
    int fade_lev=fade_level*0.75;
    if (fade_lev<+min_level) fade_lev=min_level;
    if (fade_lev>255*0.8) fade_lev=255*0.8;
    pts[0] = LineVertex(0, 0,		0,0,0,fade_lev);
    pts[1] = LineVertex(_ww, 0,		0,0,0,fade_lev);
    pts[2] = LineVertex(0, _hh,		0,0,0,fade_lev);
    pts[3] = LineVertex(_ww, _hh,	0,0,0,fade_lev);
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    pts[0] = LineVertex(menu_cell_size*1/4-1, 0,		255,255,255,fade_level);
    pts[1] = LineVertex(menu_cell_size*1/4-1, menu_cell_size,		55,55,155,fade_level);
    pts[2] = LineVertex(menu_cell_size*2/4-1, 0,		55,55,155,fade_level);
    pts[3] = LineVertex(menu_cell_size*2/4-1, menu_cell_size,		255,255,255,fade_level);
    pts[4] = LineVertex(menu_cell_size*3/4-1, 0,		55,55,155,fade_level);
    pts[5] = LineVertex(menu_cell_size*3/4-1, menu_cell_size,		255,255,255,fade_level);
    pts[6] = LineVertex(menu_cell_size*1/4+1, 0,		255,255,255,fade_level/4);
    pts[7] = LineVertex(menu_cell_size*1/4+1, menu_cell_size,		55,55,155,fade_level/4);
    pts[8] = LineVertex(menu_cell_size*2/4+1, 0,		55,55,155,fade_level/4);
    pts[9] = LineVertex(menu_cell_size*2/4+1, menu_cell_size,		255,255,255,fade_level/4);
    pts[10] = LineVertex(menu_cell_size*3/4+1, 0,		55,55,155,fade_level/4);
    pts[11] = LineVertex(menu_cell_size*3/4+1, menu_cell_size,		255,255,255,fade_level/4);
    
    pts[12] = LineVertex(0,	menu_cell_size*1/4-1, 	55,55,155,fade_level);
    pts[13] = LineVertex(menu_cell_size,	menu_cell_size*1/4-1, 	255,255,255,fade_level);
    pts[14] = LineVertex(0,	menu_cell_size*2/4-1, 	255,255,255,fade_level);
    pts[15] = LineVertex(menu_cell_size,	menu_cell_size*2/4-1, 	55,55,155,fade_level);
    pts[16] = LineVertex(0,	menu_cell_size*3/4-1, 	255,255,255,fade_level);
    pts[17] = LineVertex(menu_cell_size,	menu_cell_size*3/4-1, 	55,55,155,fade_level);
    pts[18] = LineVertex(0,	menu_cell_size*1/4+1, 	55,55,155,fade_level/4);
    pts[19] = LineVertex(menu_cell_size,	menu_cell_size*1/4+1, 	255,255,255,fade_level/4);
    pts[20] = LineVertex(0,	menu_cell_size*2/4+1, 	255,255,255,fade_level/4);
    pts[21] = LineVertex(menu_cell_size,	menu_cell_size*2/4+1, 	55,55,155,fade_level/4);
    pts[22] = LineVertex(0,	menu_cell_size*3/4+1, 	255,255,255,fade_level/4);
    pts[23] = LineVertex(menu_cell_size,	menu_cell_size*3/4+1, 	55,55,155,fade_level/4);
    
    for (int i=0;i<24;i++) pts[i].y+=+(_hh-menu_cell_size)/2;
    
    glLineWidth(1.0f*mScaleFactor);
    glDrawArrays(GL_LINES, 0, 24);
    
    
    pts[0] = LineVertex(menu_cell_size*1/4, 0,		255,255,255,fade_level/2);
    pts[1] = LineVertex(menu_cell_size*1/4, menu_cell_size,		55,55,155,fade_level/2);
    pts[2] = LineVertex(menu_cell_size*2/4, 0,		55,55,155,fade_level/2);
    pts[3] = LineVertex(menu_cell_size*2/4, menu_cell_size,		255,255,255,fade_level/2);
    pts[4] = LineVertex(menu_cell_size*3/4, 0,		55,55,155,fade_level/2);
    pts[5] = LineVertex(menu_cell_size*3/4, menu_cell_size,		255,255,255,fade_level/2);
    
    pts[6] = LineVertex(0,	menu_cell_size*1/4, 	55,55,155,fade_level/2);
    pts[7] = LineVertex(menu_cell_size,	menu_cell_size*1/4, 	255,255,255,fade_level/2);
    pts[8] = LineVertex(0,	menu_cell_size*2/4, 	255,255,255,fade_level/2);
    pts[9] = LineVertex(menu_cell_size,	menu_cell_size*2/4, 	55,55,155,fade_level/2);
    pts[10] = LineVertex(0,	menu_cell_size*3/4, 	255,255,255,fade_level/2);
    pts[11] = LineVertex(menu_cell_size,	menu_cell_size*3/4, 	55,55,155,fade_level/2);
    
    for (int i=0;i<12;i++) pts[i].y+=+(_hh-menu_cell_size)/2;
    
    glLineWidth(2.0f*mScaleFactor);
    glDrawArrays(GL_LINES, 0, 12);
    
    int factA,factB;
    factA=180;
    factB=32;
    int colbgAR=factA+factB*(0.3*sin(cpt*7*3.1459/8192)+1.2*sin(cpt*17*8*3.1459/8192)+0.7*sin(cpt*31*8*3.1459/8192));
    int colbgAG=factA+factB*(0.3*sin(cpt*5*3.1459/8192)+1.2*sin(cpt*11*8*3.1459/8192)-0.7*sin(cpt*27*8*3.1459/8192));
    int colbgAB=factA+factB*(1.2*sin(cpt*7*3.1459/8192)-0.5*sin(cpt*13*8*3.1459/8192)+1.5*sin(cpt*57*8*3.1459/8192));
    cpt+=1;
    int colbgBR=factA+factB*(0.3*sin(cpt*3*3.1459/8192)+1.2*sin(cpt*15*8*3.1459/8192)+0.7*sin(cpt*19*8*3.1459/8192));
    int colbgBG=factA+factB*(0.3*sin(cpt*2*3.1459/8192)+1.2*sin(cpt*9*8*3.1459/8192)-0.7*sin(cpt*27*8*3.1459/8192));
    int colbgBB=factA+factB*(1.2*sin(cpt*5*3.1459/8192)-0.5*sin(cpt*17*8*3.1459/8192)+1.5*sin(cpt*37*8*3.1459/8192));
    
    if (colbgAR<0) colbgAR=0; if (colbgAR>255) colbgAR=255;
    if (colbgAG<0) colbgAG=0; if (colbgAG>255) colbgAG=255;
    if (colbgAB<0) colbgAB=0; if (colbgAB>255) colbgAB=255;
    if (colbgBR<0) colbgBR=0; if (colbgBR>255) colbgBR=255;
    if (colbgBG<0) colbgBG=0; if (colbgBG>255) colbgBG=255;
    if (colbgBB<0) colbgBB=0; if (colbgBB>255) colbgBB=255;
    glLineWidth(4.0f*mScaleFactor);
    fade_lev=255;
    glDisable(GL_BLEND);
    for (int y=0;y<4;y++)
        for (int x=0;x<4;x++) {
            if (active_idx&(1<<((3-y)*4+x))) {
                pts[0] = LineVertex(x*menu_cell_size/4+3, y*menu_cell_size/4+3,		colbgAR,colbgAG,colbgAB,fade_lev);
                pts[1] = LineVertex((x+1)*menu_cell_size/4-3, y*menu_cell_size/4+3,		colbgBR,colbgBG,colbgBB,fade_lev);
                pts[2] = LineVertex((x+1)*menu_cell_size/4-3, (y+1)*menu_cell_size/4-3,	colbgAR,colbgAG,colbgAB,fade_lev);
                pts[3] = LineVertex(x*menu_cell_size/4+3, (y+1)*menu_cell_size/4-3,		colbgBR,colbgBG,colbgBB,fade_lev);
                
                for (int i=0;i<4;i++) pts[i].y+=+(_hh-menu_cell_size)/2;
                
                glDrawArrays(GL_LINE_LOOP, 0, 4);
            }
        }
    
    
    
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
}

void RenderUtils::DrawChanLayout(uint _ww,uint _hh,int display_note_mode,int chanNb,float pixOfs,float char_width,float char_height,float mScaleFactor) {
    int count=0;
    float col_size,col_ofs;
    LineVertex pts[10*MAX_VISIBLE_CHAN+10],ptsD[4*2];
    //set the opengl state
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.0f*mScaleFactor);
    
    switch (display_note_mode){
        case 0:col_size=11*char_width;col_ofs=(char_width)*2.5f+8+6-2;break;
        case 1:col_size=6*char_width;col_ofs=(char_width)*2.5f+8+6-2;break;
        case 2:col_size=4*char_width;col_ofs=(char_width)*2.5f+8+6-2;break;
    }
    
    //then draw channels frame
    
    for (int i=0; i<chanNb; i++) {
        if (pixOfs+col_size*i+col_ofs-2.0f>_ww) break;
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
    
    //Header line
    pts[count++] = LineVertex(1,     _hh-NOTES_DISPLAY_TOPMARGIN+(char_height+2-0)+2,			140,160,255,255);
    pts[count++] = LineVertex(_ww-1, _hh-NOTES_DISPLAY_TOPMARGIN+(char_height+2-0)+2,		60,100,255,255);
    
    pts[count++] = LineVertex(1,     _hh-NOTES_DISPLAY_TOPMARGIN+(char_height+2-0)+1,		140/3,160/3,255/3,255);
    pts[count++] = LineVertex(_ww-1, _hh-NOTES_DISPLAY_TOPMARGIN+(char_height+2-0)+1,	60/3,100/3,255/3,255);
    pts[count++] = LineVertex(1,     _hh-NOTES_DISPLAY_TOPMARGIN+(char_height+2-0),		140/3,160/3,255/3,255);
    pts[count++] = LineVertex(_ww-1, _hh-NOTES_DISPLAY_TOPMARGIN+(char_height+2-0),		60/3,100/3,255/3,255);
    pts[count++] = LineVertex(1,     _hh-NOTES_DISPLAY_TOPMARGIN+(char_height+2-0)-1,		140/3,160/3,255/3,255);
    pts[count++] = LineVertex(_ww-1, _hh-NOTES_DISPLAY_TOPMARGIN+(char_height+2-0)-1,	60/3,100/3,255/3,255);
    
    pts[count++] = LineVertex(1,     _hh-NOTES_DISPLAY_TOPMARGIN+(char_height+2-0)-2,		140/6,160/6,255/6,255);
    pts[count++] = LineVertex(_ww-1, _hh-NOTES_DISPLAY_TOPMARGIN+(char_height+2-0)-2,	60/6,100/6,255/6,255);
    
    
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
    glLineWidth(2.0f*mScaleFactor);
    glVertexPointer(2, GL_SHORT, sizeof(LineVertex), &ptsD[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertex), &ptsD[0].r);
    glDrawArrays(GL_LINES, 0, 8);
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisable(GL_BLEND);
    
}

void RenderUtils::DrawChanLayoutAfter(uint _ww,uint _hh,int display_note_mode,int *volumeData,int chanNb,float pixOfs,float char_width,float char_height,float char_yOfs,int rowToHighlight,float mScaleFactor) {
    LineVertex pts[64*2];
    int ii;
    int count=0;
    float col_size,col_ofs;
    int colr,colg,colb,cola;
    
    //set the opengl state
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnable(GL_BLEND);
    glLineWidth(2.0f*mScaleFactor);
    glVertexPointer(2, GL_SHORT, sizeof(LineVertex), &pts[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertex), &pts[0].r);
    
    colr=230;colg=76;colb=153;cola=150;
    //Draw current playing line
    ii=_hh-NOTES_DISPLAY_TOPMARGIN-rowToHighlight*(char_height+4)-char_yOfs-char_height+(char_yOfs+char_height)/2;
    
    pts[0] = LineVertex(0,     ii-3,		colr,colg,colb,cola);
    pts[1] = LineVertex(_ww-1, ii-3,		colr,colg,colb,cola);
    pts[2] = LineVertex(0,     ii+char_height+3,		colr,colg,colb,cola);
    pts[3] = LineVertex(_ww-1, ii+char_height+3,		colr,colg,colb,cola);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    pts[0] = LineVertex(0,     ii-4,     colr/2,colg/2,colb/2,cola);
    pts[1] = LineVertex(_ww-1, ii-4,     colr/2,colg/2,colb/2,cola);
    colr*=1.4f;if (colr>255) colr=255;
    colg*=1.4f;if (colg>255) colg=255;
    colb*=1.4f;if (colb>255) colb=255;
    cola*=1.4f;if (cola>255) cola=255;
    pts[2] = LineVertex(0,     ii+char_height+4,     colr,colg,colb,cola);
    pts[3] = LineVertex(_ww-1, ii+char_height+4,     colr,colg,colb,cola);
    glDrawArrays(GL_LINES, 0, 4);
    
    switch (display_note_mode){
        case 0:col_size=11*char_width;col_ofs=(char_width)*2+8+6-2;break;
        case 1:col_size=6*char_width;col_ofs=(char_width)*2+8+6-2;break;
        case 2:col_size=4*char_width;col_ofs=(char_width)*2+8+6-2;break;
    }
    
    //Volumes bar
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
    LineVertex *pts;
    int index=0;
    GLfloat spL,spR;
    GLfloat crt,cgt,cbt;
    GLfloat x,y,sx,sy;
    
    for (int i=0;i<nb_spectrum_bands;i++) {
        barSpectrumDataL[i]=(float)1.f*spectrumDataL[i]/512.0f;
        barSpectrumDataR[i]=(float)1.f*spectrumDataR[i]/512.0f;
    }
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    pts=(LineVertex*)malloc(sizeof(LineVertex)*6*nb_spectrum_bands*2);
    
    glDisable(GL_BLEND);
    
    glVertexPointer(2, GL_SHORT, sizeof(LineVertex), &pts[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertex), &pts[0].r);
    
    crt=0;
    cgt=0;
    cbt=0;
    
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
        
        if (mode==1) {
            x=ww*(i+4)/(nb_spectrum_bands+8);
            sx=ww/(nb_spectrum_bands+8)-1;
            y=hh/2+hh/8;
            sy=spL*hh/32;
            
            pts[index++] = LineVertex(x, y,crt*255,cgt*255,cbt*255,255);
            pts[index++] = LineVertex(x, y+sy,crt*255,cgt*255,cbt*255,255);
            pts[index++] = LineVertex(x+sx, y+sy,crt*255,cgt*255,cbt*255,255);
            
            pts[index++] = LineVertex(x+sx, y+sy,crt*255,cgt*255,cbt*255,255);
            pts[index++] = LineVertex(x+sx, y,crt*255,cgt*255,cbt*255,255);
            pts[index++] = LineVertex(x, y,crt*255,cgt*255,cbt*255,255);
        } else if (mode==2) {
            x=ww*(i+4)/(nb_spectrum_bands+8);
            sx=ww/(nb_spectrum_bands+8)-1;
            y=hh/2;
            sy=spL*hh/32;
            
            pts[index++] = LineVertex(x, y,crt*255,cgt*255,cbt*255,255);
            pts[index++] = LineVertex(x, y+sy,crt*255,cgt*255,cbt*255,255);
            pts[index++] = LineVertex(x+sx, y+sy,crt*255,cgt*255,cbt*255,255);
            
            pts[index++] = LineVertex(x+sx, y+sy,crt*255,cgt*255,cbt*255,255);
            pts[index++] = LineVertex(x+sx, y,crt*255,cgt*255,cbt*255,255);
            pts[index++] = LineVertex(x, y,crt*255,cgt*255,cbt*255,255);
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
        
        
        if (mode==1) {
            x=ww*(i+4)/(nb_spectrum_bands+8);
            sx=ww/(nb_spectrum_bands+8)-1;
            y=0*hh/2+hh/4;
            sy=spR*hh/32;
            
            pts[index++] = LineVertex(x, y,crt*255,cgt*255,cbt*255,255);
            pts[index++] = LineVertex(x, y+sy,crt*255,cgt*255,cbt*255,255);
            pts[index++] = LineVertex(x+sx, y+sy,crt*255,cgt*255,cbt*255,255);
            
            pts[index++] = LineVertex(x+sx, y+sy,crt*255,cgt*255,cbt*255,255);
            pts[index++] = LineVertex(x+sx, y,crt*255,cgt*255,cbt*255,255);
            pts[index++] = LineVertex(x, y,crt*255,cgt*255,cbt*255,255);
        } else if (mode==2) {
            x=ww*(i+4)/(nb_spectrum_bands+8);
            sx=ww/(nb_spectrum_bands+8)-1;
            y=hh/2;
            sy=-spR*hh/32;
            
            pts[index++] = LineVertex(x, y,crt*255,cgt*255,cbt*255,255);
            pts[index++] = LineVertex(x, y+sy,crt*255,cgt*255,cbt*255,255);
            pts[index++] = LineVertex(x+sx, y+sy,crt*255,cgt*255,cbt*255,255);
            
            pts[index++] = LineVertex(x+sx, y+sy,crt*255,cgt*255,cbt*255,255);
            pts[index++] = LineVertex(x+sx, y,crt*255,cgt*255,cbt*255,255);
            pts[index++] = LineVertex(x, y,crt*255,cgt*255,cbt*255,255);
        }
    }
    glDrawArrays(GL_TRIANGLES, 0, index);
    
    /* Disable Vertex Pointer */
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    
    free(pts);
    
    //    glDisable(GL_BLEND);
    
    /* Pop The Matrix */
    //glPopMatrix();
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
                         15*(0.8f*sin((float)frameCpt*0.1f*3.14159f/991)+
                             1.7f*sin((float)frameCpt*0.1f*3.14159f/3065)-
                             0.3f*sin((float)frameCpt*0.1f*3.14159f/5009)));
            
            glRotatef(-90+5.0f*(0.8f*sin((float)frameCpt*0.1f*3.14159f/2691)+
                                0.7f*sin((float)frameCpt*0.1f*3.14159f/3113)-
                                0.3f*sin((float)frameCpt*0.1f*3.14159f/7409)),0,0,1);
            
            
            glRotatef(3*360.0f*(0.5f*sin((float)frameCpt*0.1f*3.14159f/761)-
                                0.7f*sin((float)frameCpt*0.1f*3.14159f/1211)-
                                0.9f*sin((float)frameCpt*0.1f*3.14159f/2213)), 0, 1, 0);
            
            
            glRotatef(5.0f*(0.8f*sin((float)frameCpt*0.1f*3.14159f/891)-
                            0.2f*sin((float)frameCpt*0.1f*3.14159f/211)-
                            0.4f*sin((float)frameCpt*0.1f*3.14159f/5213)),0,0,1);
            
            break;
        case 2:
            glTranslatef(0.0, 0.0, -190.0+
                         15*(0.8f*sin((float)frameCpt*0.1f*3.14159f/991)+
                             1.7f*sin((float)frameCpt*0.1f*3.14159f/3065)-
                             0.3f*sin((float)frameCpt*0.1f*3.14159f/5009)));
            
            
            
            glRotatef(20+10.0f*(0.8f*sin((float)frameCpt*0.1f*3.14159f/2691)+
                                0.7f*sin((float)frameCpt*0.1f*3.14159f/3113)-
                                0.3f*sin((float)frameCpt*0.1f*3.14159f/7409)),1,0,0);
            
            glRotatef(5.0f*(0.8f*sin((float)frameCpt*0.1f*3.14159f/891)-
                            0.2f*sin((float)frameCpt*0.1f*3.14159f/211)-
                            0.4f*sin((float)frameCpt*0.1f*3.14159f/5213)),0,0,1);
            
            
            glRotatef(360.0f*(0.5f*sin((float)frameCpt*0.1f*3.14159f/761)-
                              0.7f*sin((float)frameCpt*0.1f*3.14159f/1211)-
                              0.9f*sin((float)frameCpt*0.1f*3.14159f/2213)), 0, 1, 0);
            
            break;
        case 3:
            glTranslatef(0.0, 0.0, -150.0+
                         15*(0.8f*sin((float)frameCpt*0.1f*3.14159f/991)+
                             1.7f*sin((float)frameCpt*0.1f*3.14159f/3065)-
                             0.3f*sin((float)frameCpt*0.1f*3.14159f/5009)));
            
            glRotatef(-90+5.0f*(0.8f*sin((float)frameCpt*0.1f*3.14159f/2691)+
                                0.7f*sin((float)frameCpt*0.1f*3.14159f/3113)-
                                0.3f*sin((float)frameCpt*0.1f*3.14159f/7409)),0,0,1);
            
            
            glRotatef(3*360.0f*(0.5f*sin((float)frameCpt*0.1f*3.14159f/761)-
                                0.7f*sin((float)frameCpt*0.1f*3.14159f/1211)-
                                0.9f*sin((float)frameCpt*0.1f*3.14159f/2213)), 0, 1, 0);
            
            
            glRotatef(5.0f*(0.8f*sin((float)frameCpt*0.1f*3.14159f/891)-
                            0.2f*sin((float)frameCpt*0.1f*3.14159f/211)-
                            0.4f*sin((float)frameCpt*0.1f*3.14159f/5213)),0,0,1);
            
            break;
        case 4:
            glTranslatef(0.0, 0.0, -150.0+
                         0*(0.8f*sin((float)frameCpt*0.1f*3.14159f/991)+
                            1.7f*sin((float)frameCpt*0.1f*3.14159f/3065)-
                            0.3f*sin((float)frameCpt*0.1f*3.14159f/5009)));
            
            glRotatef(-90+5.0f*(0.8f*sin((float)frameCpt*0.1f*3.14159f/2691)+
                                0.7f*sin((float)frameCpt*0.1f*3.14159f/3113)-
                                0.3f*sin((float)frameCpt*0.1f*3.14159f/7409)),0,0,1);
            
            
            glRotatef(90+0*360.0f*(0.5f*sin((float)frameCpt*0.1f*3.14159f/761)-
                                   0.7f*sin((float)frameCpt*0.1f*3.14159f/1211)-
                                   0.9f*sin((float)frameCpt*0.1f*3.14159f/2213)), 0, 1, 0);
            
            
            glRotatef(5.0f*(0.8f*sin((float)frameCpt*0.1f*3.14159f/891)-
                            0.2f*sin((float)frameCpt*0.1f*3.14159f/211)-
                            0.4f*sin((float)frameCpt*0.1f*3.14159f/5213)),0,0,1);
            
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
        
        glRotatef(-3*360.0f*(0.5f*sin((float)frameCpt*0.1f*3.14159f/761)-
                             0.7f*sin((float)frameCpt*0.1f*3.14159f/1211)-
                             0.9f*sin((float)frameCpt*0.1f*3.14159f/2213)), 0, 1, 0);
        
        //glRotatef(180,0,0,1);
        glTranslatef(12,0,0);
        
        glRotatef(-3*360.0f*(0.5f*sin((float)frameCpt*0.1f*3.14159f/761)-
                             0.7f*sin((float)frameCpt*0.1f*3.14159f/1211)-
                             0.9f*sin((float)frameCpt*0.1f*3.14159f/2213)), 0, 1, 0);
        
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
        
        /*glRotatef(-3*360.0f*(0.5f*sin((float)frameCpt*0.1f*3.14159f/761)-
         0.7f*sin((float)frameCpt*0.1f*3.14159f/1211)-
         0.9f*sin((float)frameCpt*0.1f*3.14159f/2213)), 0, 1, 0);
         */
        //glRotatef(180,0,0,1);
        glTranslatef(0,0,12);
        
        glRotatef(-45,0,1,0);
        
        /*glRotatef(-3*360.0f*(0.5f*sin((float)frameCpt*0.1f*3.14159f/761)-
         0.7f*sin((float)frameCpt*0.1f*3.14159f/1211)-
         0.9f*sin((float)frameCpt*0.1f*3.14159f/2213)), 0, 1, 0);
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
        
        glRotatef(-3*360.0f*(0.5f*sin((float)frameCpt*0.1f*3.14159f/761)-
                             0.7f*sin((float)frameCpt*0.1f*3.14159f/1211)-
                             0.9f*sin((float)frameCpt*0.1f*3.14159f/2213)), 0, 1, 0);
        
        //glRotatef(180,0,0,1);
        glTranslatef(15,0,0);
        
        glRotatef(-3*360.0f*(0.5f*sin((float)frameCpt*0.1f*3.14159f/761)-
                             0.7f*sin((float)frameCpt*0.1f*3.14159f/1211)-
                             0.9f*sin((float)frameCpt*0.1f*3.14159f/2213)), 0, 1, 0);
        
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
    
    //glShadeModel(GL_SMOOTH);
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

#define MIDIFX_LEN 128*2
unsigned int data_midifx_framecpt=0;
int data_midifx_len=MIDIFX_LEN;
unsigned char data_midifx_note[MIDIFX_LEN][256];
unsigned char data_midifx_subnote[MIDIFX_LEN][256];
unsigned char data_midifx_instr[MIDIFX_LEN][256];
unsigned char data_midifx_vol[MIDIFX_LEN][256];
unsigned int data_midifx_st[MIDIFX_LEN][256];
static int data_midifx_first=1;

int data_pianofx_len=MIDIFX_LEN;
unsigned int data_pianofx_framecpt=0;
unsigned char data_pianofx_note[MIDIFX_LEN][256];
unsigned char data_pianofx_subnote[MIDIFX_LEN][256];
unsigned char data_pianofx_instr[MIDIFX_LEN][256];
unsigned char data_pianofx_vol[MIDIFX_LEN][256];
unsigned int data_pianofx_st[MIDIFX_LEN][256];
int data_pianofx_first=1;



#define VOICE_FREE	(1<<0)
#define VOICE_ON	(1<<1)
#define VOICE_SUSTAINED	(1<<2)
#define VOICE_OFF	(1<<3)
#define VOICE_DIE	(1<<4)


//unsigned int data_midifx_col[16]={
////    0x8010E7,0x5D3E79,0x29004D,0xBF7BFD,0xE7CFFD,
////     0xFF4500,0x865340,0x551700,0xFF9872,0xFFDBCE,
////     0x00E87F,0x3A7A5D,0x004E2A,0x71FDBD,0xCCFDE6,
////     0xFFF200
//    //0x868240,0x555100,0xFFF872,0xFFFDCE
//
//    0xFF5512,0x761AFF,0x21ff94,0xffb129,
//    0xcb30ff,0x38ffe4,0xfffc40,0xff47ed,
//    0x4fd9ff,0xc7ff57,0xff5eb7,0x66a8ff,
//    0x9cff6e,0xff7591,0x7d88ff,0x85ff89
//};

unsigned char piano_key[12]={0,1,0,1,0,0,1,0,1,0,1,0};
unsigned char piano_key_state[128];
unsigned char piano_key_instr[128];

//extern int texturePiano;

void RenderUtils::DrawPiano3D(uint ww,uint hh,int automove,float posx,float posy,float posz,float rotx,float roty,int color_mode) {
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
    
    
    
    int j=MIDIFX_LEN-MIDIFX_OFS-1;//-(data_pianofx_len/2);//MIDIFX_OFS;
    //glLineWidth(line_width+2);
    index=0;
    for (int i=0; i<256; i++) {
        if (data_pianofx_note[j][i]) {
            int instr=data_pianofx_instr[j][i];
            int vol=data_pianofx_vol[j][i];
            unsigned int st=data_pianofx_st[j][i];
            
            if (vol&&(st&VOICE_ON)) {
                //note pressed
                piano_key_state[(data_pianofx_note[j][i])&127]=8;
                piano_key_instr[(data_pianofx_note[j][i])&127]=instr;
            }
        }
    }
    
    yadj=0.01;
    
#define PIANO3D_DRAWKEY \
if (piano_key_state[i+k]) { \
yn=yf-key_height*4/5*piano_key_state[i+k]/8; \
ynBL=yf-key_heightBL*3/5*piano_key_state[i+k]/8; \
piano_key_state[i+k]--; \
} else { \
yn=ynBL=yf; \
} \
if (piano_ofs==12) piano_ofs=0; \
if (piano_key[piano_ofs]==0) { /*white key*/ \
if (piano_key_state[i+k]) { \
crt=(0.3f*piano_key_state[i+k]+1.0f*(8-piano_key_state[i+k]))/8; \
cgt=(0.3f*piano_key_state[i+k]+1.0f*(8-piano_key_state[i+k]))/8; \
cbt=(0.9f*piano_key_state[i+k]+1.0f*(8-piano_key_state[i+k]))/8; \
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
crt=(0.9f*piano_key_state[i+k]+0.4f*(8-piano_key_state[i+k]))/8; \
cgt=(0.3f*piano_key_state[i+k]+0.4f*(8-piano_key_state[i+k]))/8; \
cbt=(0.3f*piano_key_state[i+k]+0.4f*(8-piano_key_state[i+k]))/8; \
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
    //have playing bar drawn first
    int valA;
    int valB;
    
    
    valA=((t_data_bar2draw*)entryA)->frameidx;
    valB=((t_data_bar2draw*)entryB)->frameidx;
    
    if (valA==valB) {
        //try to have min start idx first
        //        valA=((t_data_bar2draw*)entryA)->startidx;
        //        valB=((t_data_bar2draw*)entryB)->startidx;
        valA=-((t_data_bar2draw*)entryA)->played;
        valB=-((t_data_bar2draw*)entryB)->played;
        
        if (valA==valB) {
            //if same startidx, start with longer bar
            valA=-((t_data_bar2draw*)entryA)->size;
            valB=-((t_data_bar2draw*)entryB)->size;
            
            if (valA==valB) {
                //if same size, use note
                valA=(((t_data_bar2draw*)entryA)->note);
                valB=(((t_data_bar2draw*)entryB)->note);
                
                if (valA==valB) {
                    //if same note, use instr
                    valA=((t_data_bar2draw*)entryA)->instr;
                    valB=((t_data_bar2draw*)entryB)->instr;
                }
            }
        }
    }
    return valA-valB;
}

void RenderUtils::UpdateDataPiano(unsigned int *data,bool clearbuffer,bool paused) {
    //if first launch, clear buffers
    if (data_pianofx_first|clearbuffer) {
        data_pianofx_first=0;
        for (int i=0;i<data_pianofx_len;i++) {
            memset(data_pianofx_note[i],0,256);
            memset(data_pianofx_subnote[i],0,256);
            memset(data_pianofx_instr[i],0,256);
            memset(data_pianofx_vol[i],0,256);
            memset(data_pianofx_st[i],0,256*sizeof(unsigned int));
        }
    }
    //update old ones
    for (int j=0;j<data_pianofx_len-1;j++) {
        memcpy(data_pianofx_note[j],data_pianofx_note[j+1],256);
        memcpy(data_pianofx_subnote[j],data_pianofx_note[j+1],256);
        memcpy(data_pianofx_instr[j],data_pianofx_instr[j+1],256);
        memcpy(data_pianofx_vol[j],data_pianofx_vol[j+1],256);
        memcpy(data_pianofx_st[j],data_pianofx_st[j+1],256*sizeof(unsigned int));
    }
    //add new one
    for (int i=0;i<256;i++) {
        unsigned int note=data[i];
        data_pianofx_note[data_pianofx_len-1][i]=note&0xFF;
        data_pianofx_subnote[data_pianofx_len-1][i]=(note>>28)&0xF;
        data_pianofx_instr[data_pianofx_len-1][i]=(note>>8)&0xFF;
        data_pianofx_vol[data_pianofx_len-1][i]=(note>>16)&0xFF;
        data_pianofx_st[data_pianofx_len-1][i]=((note>>24)&0xF)|(data_pianofx_framecpt<<8);
    }
    
    if (settings[GLOB_FXPianoCutLine].detail.mdz_switch.switch_value==0) { //cut note bars after piano
        for (int j=0;j<data_pianofx_len-1-MIDIFX_OFS;j++) {
            memset(data_pianofx_note[j],0,256);
            memset(data_pianofx_subnote[j],0,256);
            memset(data_pianofx_instr[j],0,256);
            memset(data_pianofx_vol[j],0,256);
            memset(data_pianofx_st[j],0,256*sizeof(unsigned int));
        }
    }
    
    if (!paused) {
        data_pianofx_framecpt++;
        pianoroll_cpt++;
    }
}


void RenderUtils::DrawPiano3DWithNotesWall(uint ww,uint hh,int automove,float posx,float posy,float posz,float rotx,float roty,int color_mode,int fxquality) {
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
            ztrans=ztrans+(ztrans_tgt-ztrans)*ztransSpeed_tgt*0.5f;
            if (ztransSpeed_tgt<0.1) ztransSpeed_tgt=ztransSpeed_tgt+0.001;
            if (ztrans-ztrans_tgt<0.1) {
                ztransSpeed_tgt=0;
            }
            
            ztrans_wait=60*5+arc4random()&511;
        } else {
            if (ztrans_wait==0) {
                ztrans=ztrans+(ztrans_tgt-ztrans)*ztransSpeed_tgt*0.5f;
                if (ztransSpeed_tgt<0.1) ztransSpeed_tgt=ztransSpeed_tgt+0.001;
                if (ztrans_tgt-ztrans<0.1) {
                    ztrans_wait=60*5+arc4random()&511;
                    ztransSpeed_tgt=0;
                }
            } else ztrans_wait--;
        }
        
        xtrans_tgt=((note_max+note_min)/2-64)*7.0/12;
        xtrans=xtrans+(xtrans_tgt-xtrans)*xtransSpeed_tgt*0.5f;
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
        
        glTranslatef(-xtrans+xrandfact*(0.9f*sin((float)piano_fxcpt*0.5*3.14159f/319)+
                                        0.5f*sin((float)piano_fxcpt*0.5*3.14159f/789)-
                                        0.7f*sin((float)piano_fxcpt*0.5*3.14159f/1061)),
                     2.0,
                     ztrans-5*(1.2f*cos((float)piano_fxcpt*0.5*3.14159f/719)+
                               0.5f*sin((float)piano_fxcpt*0.5*3.14159f/289)-
                               0.7f*sin((float)piano_fxcpt*0.5*3.14159f/361)));
        
        
        glRotatef(rotx_adj+rotx_randfact*(0.4f*sin((float)piano_fxcpt*0.5*3.14159f/91)+
                                          0.7f*sin((float)piano_fxcpt*0.5*3.14159f/911)+
                                          0.3f*sin((float)piano_fxcpt*0.5*3.14159f/409)), 1, 0, 0);
        glRotatef(roty_adj+roty_randfact*(0.8f*sin((float)piano_fxcpt*0.5*3.14159f/173)+
                                          0.5f*sin((float)piano_fxcpt*0.5*3.14159f/1029)+
                                          0.3f*sin((float)piano_fxcpt*0.5*3.14159f/511)), 0, 1, 0);
        
    } else {
        glTranslatef(posx,posy,posz-100*15);
        glRotatef(30+rotx, 1, 0, 0);
        glRotatef(roty, 0, 1, 0);
    }
    
    
    int j=data_pianofx_len-1-MIDIFX_OFS;
    //glLineWidth(line_width+2);
    index=0;
    for (int i=0; i<256; i++) {
        if (data_pianofx_note[j][i]) {
            int instr=data_pianofx_instr[j][i];
            int vol=data_pianofx_vol[j][i];
            unsigned int st=data_pianofx_st[j][i];
            
            if (vol&&(st&VOICE_ON)) {
                //note pressed
                piano_key_state[(data_pianofx_note[j][i])&127]=8;
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
            yn=yf-key_height*4/5*piano_key_state[i]/8;
            ynBL=yf-key_heightBL*3/5*piano_key_state[i]/8;
            piano_key_state[i]--;
            
            int colidx;//=i%12;
            if (color_mode==0) {
                colidx=(i%12);
            } else if (color_mode==1) {
                colidx=(piano_key_instr[i])&63;
            }
            
            crt=((data_midifx_col[colidx&31]>>16)&0xFF)/255.0f;
            cgt=((data_midifx_col[colidx&31]>>8)&0xFF)/255.0f;
            cbt=(data_midifx_col[colidx&31]&0xFF)/255.0f;
            
            if (colidx&0x20) {
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
                crt=(crt*piano_key_state[i]+1.0f*(8-piano_key_state[i]))/8;
                cgt=(cgt*piano_key_state[i]+1.0f*(8-piano_key_state[i]))/8;
                cbt=(cbt*piano_key_state[i]+1.0f*(8-piano_key_state[i]))/8;
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
                crt=(crt*piano_key_state[i]+0.4f*(8-piano_key_state[i]))/8;
                cgt=(cgt*piano_key_state[i]+0.4f*(8-piano_key_state[i]))/8;
                cbt=(cbt*piano_key_state[i]+0.4f*(8-piano_key_state[i]))/8;
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
                unsigned int st=data_pianofx_st[j][i];
                int note=data_pianofx_note[j][i];
                
                if (vol&&(st&VOICE_ON)) {  //check volume & status => we have something
                    data_bar2draw[data_bar2draw_count].startidx=j;
                    data_bar2draw[data_bar2draw_count].note=note;
                    data_bar2draw[data_bar2draw_count].instr=instr;
                    data_bar2draw[data_bar2draw_count].size=0;
                    data_bar2draw[data_bar2draw_count].played=0;
                    data_bar2draw[data_bar2draw_count].frameidx=st>>8;
                    while ( (data_pianofx_instr[j][i]==instr)&&
                           (data_pianofx_note[j][i]==note)&&
                           (data_pianofx_vol[j][i]&&
                            (data_pianofx_st[j][i]&VOICE_ON))) {  //while same bar (instru & notes), increase size
                        data_bar2draw[data_bar2draw_count].size++;
                        //propagate lowest frame nb
                        data_pianofx_st[j][i]=st;
                        if (j==(data_pianofx_len-MIDIFX_OFS-1)) data_bar2draw[data_bar2draw_count].played=1;
                        j++;
                        if (j==data_pianofx_len) break;
                        if (settings[GLOB_FXPianoCutLine].detail.mdz_switch.switch_value==1) {
                            if (j==(data_pianofx_len-MIDIFX_OFS-1)) break;
                        }
                    }
                    data_bar2draw_count++;
                } else j++;
            } else j++;
            if (data_bar2draw_count==MAX_BARS) break;
        }
        if (data_bar2draw_count==MAX_BARS) break;
    }
    qsort(data_bar2draw,data_bar2draw_count,sizeof(t_data_bar2draw),qsort_CompareBar);
    
    /*
     if (data_bar2draw_count>=2) { //propagate played flag
     for (int i=1;i<data_bar2draw_count;i++) {
     int note=data_bar2draw[i-1].note;
     int played=data_bar2draw[i-1].played;
     int instr=data_bar2draw[i-1].instr;
     
     if (played) {
     if ((data_bar2draw[i].instr==instr)&&((data_bar2draw[i].note)==note)&&
     (data_bar2draw[i].startidx<=(data_bar2draw[i-1].startidx+data_bar2draw[i-1].size)))
     data_bar2draw[i].played=1;
     }
     }
     
     for (int i=data_bar2draw_count-2;i>=0;i--) {
     int note=data_bar2draw[i+1].note;
     int played=data_bar2draw[i+1].played;
     int instr=data_bar2draw[i+1].instr;
     
     if (played) {
     if ((data_bar2draw[i].instr==instr)&&((data_bar2draw[i].note)==note)&&
     (data_bar2draw[i+1].startidx<=(data_bar2draw[i].startidx+data_bar2draw[i].size))) data_bar2draw[i].played=1;
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
     */
    int vertices_index=0;
    int indices_index=0;
    glVertexPointer(3, GL_FLOAT, 0, verticesBAR);
    glColorPointer(4, GL_FLOAT, 0, vertColorBAR);
    
    
    //TO OPTIMIZE
    unsigned int data_bar_2dmap[128*MIDIFX_LEN];
    memset(data_bar_2dmap,0,128*MIDIFX_LEN*sizeof(unsigned int));
    
    for (int i=0;i<data_bar2draw_count;i++) {
        int note=data_bar2draw[i].note&127;
        int played=data_bar2draw[i].played;
        int instr=data_bar2draw[i].instr;
        int colidx;
        if (color_mode==0) { //note
            colidx=(note%12);
        } else if (color_mode==1) { //instru
            colidx=(instr)&63;
        }
        
        if (data_bar2draw[i].size==0) continue;
        
        //printf("i:%d start:%d end:%d instr:%d note:%d played:%d\n",i,data_bar2draw[i].startidx,data_bar2draw[i].startidx+data_bar2draw[i].size,instr,note,played);
        
        
        float adj_size=0;
        int max_draw_count=0;
        for (int j=data_bar2draw[i].startidx;j<data_bar2draw[i].startidx+data_bar2draw[i].size;j++) {
            //            int _instr=(data_bar_2dmap[(note&127)*MIDIFX_LEN+j]>>16);
            int draw_count=data_bar_2dmap[(note&127)*MIDIFX_LEN+j]&255;
            if (draw_count) {
                if (adj_size<0.1f*(float)(draw_count)) adj_size=0.1f*(float)(draw_count);
                //                if (_instr!=(instr+1)) {
                draw_count++;
                if (max_draw_count<draw_count) max_draw_count=draw_count;
                //                    data_bar_2dmap[(note&127)*MIDIFX_LEN+j]=(((int)(data_bar2draw[i].instr)+1)<<16)|draw_count;
                data_bar_2dmap[(note&127)*MIDIFX_LEN+j]=draw_count;
                //                }
            } else {
                //                data_bar_2dmap[(note&127)*MIDIFX_LEN+j]=(((int)(data_bar2draw[i].instr)+1)<<16)|1;
                data_bar_2dmap[(note&127)*MIDIFX_LEN+j]=1;
                if (max_draw_count<1) max_draw_count=1;
            }
        }
        //        printf("adj: %f\n",adj_size);
        for (int j=data_bar2draw[i].startidx;j<data_bar2draw[i].startidx+data_bar2draw[i].size;j++) {
            data_bar_2dmap[(note&127)*MIDIFX_LEN+j]=max_draw_count;
        }
        
        
        crt=((data_midifx_col[colidx&31]>>16)&0xFF)/255.0f/1.0f;
        cgt=((data_midifx_col[colidx&31]>>8)&0xFF)/255.0f/1.0f;
        cbt=(data_midifx_col[colidx&31]&0xFF)/255.0f/1.0f;
        
        if (colidx&0x20) {
            crt=(crt+1)/2;
            cgt=(cgt+1)/2;
            cbt=(cbt+1)/2;
        }
        
        if (played) {
            crt=(crt*2+1*3)/5;
            cgt=(cgt*2+1*3)/5;
            cbt=(cbt*2+1*3)/5;
            if (crt>1) crt=1;
            if (cgt>1) cgt=1;
            if (cbt>1) cbt=1;
        }
        
        if ( ((data_bar2draw[i].startidx+data_bar2draw[i].size)<data_pianofx_len-MIDIFX_OFS) && !played && (settings[GLOB_FXPianoCutLine].detail.mdz_switch.switch_value==1)) { //shade note bars after piano
            crt=(crt)/2;
            cgt=(cgt)/2;
            cbt=(cbt)/2;
        }
        
        if (note>tgt_note_max) tgt_note_max=note;
        if (note<tgt_note_min) tgt_note_min=note;
        
        x=piano_note_posx[note&127];
        y=piano_note_posy[note&127]+((float)(data_bar2draw[i].startidx)-(data_pianofx_len-MIDIFX_OFS)+MIDIFX_OFS*3*0)*0.5f;
        z=piano_note_posz[note&127];
        
        float x1;
        float y1=y;
        float z1=z;
        float sx;
        float sy=0.5f*(float)data_bar2draw[i].size;
        float sz=0.1f;
        
        if (piano_note_type[note&127]) {
            //black key
            sx=0.2;
            z1+=key_length*0.55;
        } else {
            //white key
            sx=0.4;
            z1+=key_length*0.9;
        }
        
        if (played) {
            double adj=sx*0.5f;
            sx=sx+adj;
            
            //sz+=0.1f;
            //z1-=0.05f;
        }
        x1=x-sx/2;
        
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

void RenderUtils::UpdateDataMidiFX(unsigned int *data,bool clearBuffer,bool paused) {
    //if first launch, clear buffers
    if (data_midifx_first||clearBuffer) {
        data_midifx_first=0;
        for (int i=0;i<MIDIFX_LEN;i++) {
            memset(data_midifx_note[i],0,256);
            memset(data_midifx_subnote[i],0,256);
            memset(data_midifx_instr[i],0,256);
            memset(data_midifx_vol[i],0,256);
            memset(data_midifx_st[i],0,256*sizeof(unsigned int));
        }
    }
    if (!paused) {
        //update old ones
        for (int j=0;j<MIDIFX_LEN-1;j++) {
            memcpy(data_midifx_note[j],data_midifx_note[j+1],256);
            memcpy(data_midifx_subnote[j],data_midifx_subnote[j+1],256);
            memcpy(data_midifx_instr[j],data_midifx_instr[j+1],256);
            memcpy(data_midifx_vol[j],data_midifx_vol[j+1],256);
            memcpy(data_midifx_st[j],data_midifx_st[j+1],256*sizeof(unsigned int));
        }
        //add new one
        for (int i=0;i<256;i++) {
            unsigned int note=data[i];
            data_midifx_note[MIDIFX_LEN-1][i]=note&0xFF;
            data_midifx_subnote[MIDIFX_LEN-1][i]=(note>>28)&0xF;
            data_midifx_instr[MIDIFX_LEN-1][i]=(note>>8)&0xFF;
            data_midifx_vol[MIDIFX_LEN-1][i]=(note>>16)&0xFF;
            data_midifx_st[MIDIFX_LEN-1][i]=((note>>24)&0xF)|(data_midifx_framecpt<<8);
        }
    }
    
    if (settings[GLOB_FXMIDICutLine].detail.mdz_switch.switch_value==0) { //cut note bars after piano
        for (int j=0;j<MIDIFX_LEN-1-MIDIFX_OFS;j++) {
            memset(data_midifx_note[j],0,256);
            memset(data_midifx_subnote[j],0,256);
            memset(data_midifx_instr[j],0,256);
            memset(data_midifx_vol[j],0,256);
            memset(data_midifx_st[j],0,256*sizeof(unsigned int));
        }
    }
    
    if (!paused) data_midifx_framecpt++;
}

void RenderUtils::DrawMidiFX(uint ww,uint hh,int horiz_vert,float note_display_range, float note_display_offset,int fx_len,int color_mode,float mScaleFactor) {
    LineVertexF *ptsB;
    coordData *texcoords; /* Holds Float Info For 4 Sets Of Texture coordinates. */
    int crt,cgt,cbt,ca;
    int crtp[4],cgtp[4],cbtp[4],cap[4];
    int index;
    //int band_width,ofs_band;
    float band_width;
    float line_width;
    float line_width_extra;
    uint8_t sparkPresent[256];
    static uint8_t sparkIntensity[256];
    static bool first_call=true;
    
    if (first_call) {
        first_call=false;
        memset(sparkIntensity,0,sizeof(sparkIntensity));
    }
    
    
    if (fx_len>MIDIFX_LEN) fx_len=MIDIFX_LEN;
    if (fx_len<=MIDIFX_OFS) fx_len=MIDIFX_OFS+1;
    
    if (fx_len!=data_midifx_len) {
        data_midifx_len=fx_len;
        //data_midifx_first=1;
    }
    
    ptsB=(LineVertexF*)malloc(sizeof(LineVertexF)*30*MAX_BARS);
    max_indices=30*MAX_BARS;
    texcoords=(coordData*)malloc(sizeof(coordData)*256*6*8); //max 256 notes, 6pts/spark and max 8 sparks/notes
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    if (horiz_vert==0) {//Horiz
        band_width=(float)(ww+0*ww/4)/data_midifx_len;
        //        ofs_band=(ww-band_width*data_midifx_len)>>1;
        line_width=1.0f*hh/note_display_range;
    } else { //vert
        band_width=(float)(hh+0*hh/4)/data_midifx_len;
        //        ofs_band=(hh-band_width*data_midifx_len)>>1;
        line_width=1.0f*ww/note_display_range;
    }
    //line_width_extra=line_width*0.2f;
    //    if (line_width_extra<2) line_width_extra=2;
    line_width_extra=3;
    
    
    
    //glDisable(GL_BLEND);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    
    glVertexPointer(2, GL_FLOAT, sizeof(LineVertexF), &ptsB[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertexF), &ptsB[0].r);
    
    
    //////////////////////////////////////////////
    
    int data_bar2draw_count=0;
    
    for (int i=0; i<256; i++) { //for each channels
        int j=MIDIFX_LEN-data_midifx_len;
        while (j<MIDIFX_LEN) {  //while not having reach roof
            if (data_midifx_note[j][i]) {  //do we have a note ?
                unsigned int instr=data_midifx_instr[j][i];
                unsigned int vol=data_midifx_vol[j][i];
                unsigned int st=data_midifx_st[j][i];
                unsigned int note=data_midifx_note[j][i];
                int subnote=data_midifx_subnote[j][i];
                
                if (vol&&(st&VOICE_ON)) {  //check volume & status => we have something
                    data_bar2draw[data_bar2draw_count].startidx=j-(MIDIFX_LEN-data_midifx_len);
                    data_bar2draw[data_bar2draw_count].note=note;
                    data_bar2draw[data_bar2draw_count].subnote=subnote;
                    data_bar2draw[data_bar2draw_count].instr=instr;
                    data_bar2draw[data_bar2draw_count].size=0;
                    data_bar2draw[data_bar2draw_count].played=0;
                    data_bar2draw[data_bar2draw_count].frameidx=st>>8;
                    while ( (data_midifx_instr[j][i]==instr)&&
                           (data_midifx_note[j][i]==note)&&
                           (data_midifx_vol[j][i]<=vol)&&
                           (data_midifx_st[j][i]&VOICE_ON) ) {  //while same bar (instru & notes), increase size
                        data_bar2draw[data_bar2draw_count].size++;
                        //propagate lowest frame nb
                        data_midifx_st[j][i]=st;
                        if ((settings[GLOB_FXMIDIBarVibrato].detail.mdz_switch.switch_value==1)&&(data_midifx_subnote[j][i]!=subnote)) break;
                        //take most recent subnote if before playing bar
                        if (j<MIDIFX_LEN-MIDIFX_OFS) data_bar2draw[data_bar2draw_count].subnote=data_midifx_subnote[j][i];
                        if (j==(MIDIFX_LEN-MIDIFX_OFS-1)) data_bar2draw[data_bar2draw_count].played=1;
                        //update vol to latest encountered one, allow to retrigger if volume increases from last bar to new one and manage special case with vol = 2111121111
                        vol=data_midifx_vol[j][i];
                        j++;
                        if (settings[GLOB_FXMIDICutLine].detail.mdz_switch.switch_value==1) {
                            if (j==(MIDIFX_LEN-MIDIFX_OFS-1)) break;
                        }
                        /*if (data_midifx_vol[j][i]>vol) {
                         break;
                         }*/
                        if (j==MIDIFX_LEN) break;
                    }
                    data_bar2draw_count++;
                    //j++;
                } else j++;
            } else j++;
            if (data_bar2draw_count==MAX_BARS) break;
        }
        if (data_bar2draw_count==MAX_BARS) break;
    }
    qsort(data_bar2draw,data_bar2draw_count,sizeof(t_data_bar2draw),qsort_CompareBar);
    
    index=0;
    
    for (int i=0;i<data_bar2draw_count;i++) {
        int played=data_bar2draw[i].played;
        int note=data_bar2draw[i].note;
        int subnote=(data_bar2draw[i].subnote<8?data_bar2draw[i].subnote:data_bar2draw[i].subnote-8-7);
        
        int instr=data_bar2draw[i].instr;
        int colidx;
        if (color_mode==0) { //note
            colidx=(note%12);
        } else if (color_mode==1) { //instru
            colidx=(instr)&63;
        }
        
        if (data_bar2draw[i].size==0) continue;
        
        //printf("i:%d start:%d end:%d instr:%d note:%d played:%d\n",i,data_bar2draw[i].startidx,data_bar2draw[i].startidx+data_bar2draw[i].size,instr,note,played);
        
        
        crt=((data_midifx_col[colidx&31]>>16)&0xFF);
        cgt=((data_midifx_col[colidx&31]>>8)&0xFF);
        cbt=(data_midifx_col[colidx&31]&0xFF);
        
        if (colidx&0x20) {
            crt=(crt+255)/2;
            cgt=(cgt+255)/2;
            cbt=(cbt+255)/2;
        }
        
        if (played) {
            crt=(crt*3+255*3)/6;
            cgt=(cgt*3+255*3)/6;
            cbt=(cbt*3+255*3)/6;
            if (crt>255) crt=255;
            if (cgt>255) cgt=255;
            if (cbt>255) cbt=255;
            line_width_extra=3;
            ca=192;
        } else {
            line_width_extra=0;
            ca=192;
        }
        
        if ( ((data_bar2draw[i].startidx+data_bar2draw[i].size)<data_midifx_len-MIDIFX_OFS) && !played && (settings[GLOB_FXMIDICutLine].detail.mdz_switch.switch_value==1)) { //shade note bars after piano
            crt=(crt)/2;
            cgt=(cgt)/2;
            cbt=(cbt)/2;
        }
        
        switch (settings[GLOB_FXMIDIBarStyle].detail.mdz_switch.switch_value) {
            case 0:
                for (int ii=0;ii<4;ii++) {
                    double fact=1;
                    double ofs=0;
                    
                    crtp[ii]=crt*fact+ofs;cgtp[ii]=cgt*fact+ofs;cbtp[ii]=cbt*fact+ofs;cap[ii]=ca;
                    if (crtp[ii]>255) crtp[ii]=255;
                    if (cgtp[ii]>255) cgtp[ii]=255;
                    if (cbtp[ii]>255) cbtp[ii]=255;
                    if (cap[ii]>255) cap[ii]=255;
                }
                break;
            case 1:
                for (int ii=0;ii<4;ii++) {
                    double fact=1;
                    double ofs=0;
                    if ((ii==3)||(ii==2)) {
                        fact=1.5f;
                        ofs=64;
                    }
                    if ((ii==0)||(ii==1)) {
                        fact=0.5f;
                        ofs=0;
                    }
                    crtp[ii]=crt*fact+ofs;
                    cgtp[ii]=cgt*fact+ofs;
                    cbtp[ii]=cbt*fact+ofs;
                    cap[ii]=ca;
                    if (crtp[ii]>255) crtp[ii]=255;
                    if (cgtp[ii]>255) cgtp[ii]=255;
                    if (cbtp[ii]>255) cbtp[ii]=255;
                    if (cap[ii]>255) cap[ii]=255;
                }
                break;
            case 2:
                for (int ii=0;ii<4;ii++) {
                    double fact=1;
                    double ofs=0;
                    if ((ii==3)||(ii==2)) {
                        fact=1.5f;
                        ofs=96;
                    }
                    if ((ii==0)||(ii==1)) {
                        fact=0.5f;
                        ofs=0;
                    }
                    crtp[ii]=crt*fact+ofs;cgtp[ii]=cgt*fact+ofs;cbtp[ii]=cbt*fact+ofs;cap[ii]=ca;
                    if (crtp[ii]>255) crtp[ii]=255;
                    if (cgtp[ii]>255) cgtp[ii]=255;
                    if (cbtp[ii]>255) cbtp[ii]=255;
                    if (cap[ii]>255) cap[ii]=255;
                }
                crt=crtp[3];cgt=cgtp[3];cbt=cbtp[3];
                for (int ii=2;ii<4;ii++) {
                    crtp[ii]=crtp[0];
                    cgtp[ii]=cgtp[0];
                    cbtp[ii]=cbtp[0];
                    cap[ii]=cap[0];
                }
                break;
        }
        
        if (horiz_vert==0) { //horiz
            float posNote=note*line_width-note_display_offset+subnote;
            float posStart=(int)(data_bar2draw[i].startidx)*ww/data_midifx_len;
            float posEnd=((int)(data_bar2draw[i].startidx)+(int)(data_bar2draw[i].size))*ww/data_midifx_len;
            
            if ( ((posNote+(line_width)+line_width_extra)>=0) && ((posNote-line_width_extra)<(int)hh)) {
                
                if ((settings[GLOB_FXMIDIBarStyle].detail.mdz_switch.switch_value==0)||(settings[GLOB_FXMIDIBarStyle].detail.mdz_switch.switch_value==2)) {
                    
                    if (index+12>=max_indices) {
                        glDrawArrays(GL_TRIANGLES, 0, index);
                        index=0;
                    }
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote-line_width_extra,crtp[0],cgtp[0],cbtp[0],cap[0]);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote-line_width_extra,crtp[1],cgtp[1],cbtp[1],cap[1]);
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote+(line_width)/2,crt,cgt,cbt,ca);
                    
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote-line_width_extra,crtp[1],cgtp[1],cbtp[1],cap[1]);
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote+(line_width)/2,crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote+(line_width)/2,crt,cgt,cbt,ca);
                    
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote+(line_width)/2,crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote+(line_width)/2,crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote+line_width+line_width_extra,crtp[2],cgtp[2],cbtp[2],cap[2]);
                    
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote+(line_width)/2,crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote+line_width+line_width_extra,crtp[2],cgtp[2],cbtp[2],cap[2]);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote+line_width+line_width_extra,crtp[3],cgtp[3],cbtp[3],cap[3]);
                } else {
                    int border_size=(line_width>=8?2:1);
                    
                    if (index+30>=max_indices) {
                        glDrawArrays(GL_TRIANGLES, 0, index);
                        index=0;
                    }
                    //top
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote-line_width_extra,crtp[0],cgtp[0],cbtp[0],cap[0]);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote-line_width_extra,crtp[1],cgtp[1],cbtp[1],cap[1]);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote-line_width_extra+border_size,crtp[1],cgtp[1],cbtp[1],cap[1]);
                    
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote-line_width_extra,crtp[0],cgtp[0],cbtp[0],cap[0]);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote-line_width_extra+border_size,crtp[1],cgtp[1],cbtp[1],cap[1]);
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote-line_width_extra+border_size,crtp[0],cgtp[0],cbtp[0],cap[0]);
                    
                    //left
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote-line_width_extra,crtp[0],cgtp[0],cbtp[0],cap[0]);
                    ptsB[index++] = LineVertexF(posStart-line_width_extra+border_size, posNote-line_width_extra,crtp[1],cgtp[1],cbtp[1],cap[1]);
                    ptsB[index++] = LineVertexF(posStart-line_width_extra+border_size, posNote+(line_width)+line_width_extra,crtp[1],cgtp[1],cbtp[1],cap[1]);
                    
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote-line_width_extra,crtp[0],cgtp[0],cbtp[0],cap[0]);
                    ptsB[index++] = LineVertexF(posStart-line_width_extra+border_size, posNote+(line_width)+line_width_extra,crtp[1],cgtp[1],cbtp[1],cap[1]);
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote+(line_width)+line_width_extra,crtp[1],cgtp[1],cbtp[1],cap[1]);
                    
                    //bottom
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote+(line_width)+line_width_extra,crtp[2],cgtp[2],cbtp[2],cap[2]);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote+(line_width)+line_width_extra,crtp[3],cgtp[3],cbtp[3],cap[3]);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote+(line_width)+line_width_extra-border_size,crtp[3],cgtp[3],cbtp[3],cap[3]);
                    
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote+(line_width)+line_width_extra,crtp[2],cgtp[2],cbtp[2],cap[2]);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote+(line_width)+line_width_extra-border_size,crtp[3],cgtp[3],cbtp[3],cap[3]);
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote+(line_width)+line_width_extra-border_size,crtp[2],cgtp[2],cbtp[2],cap[2]);
                    
                    //right
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote-line_width_extra,crtp[2],cgtp[2],cbtp[2],cap[2]);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra-border_size, posNote-line_width_extra,crtp[3],cgtp[3],cbtp[3],cap[3]);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra-border_size, posNote+(line_width)+line_width_extra,crtp[3],cgtp[3],cbtp[3],cap[3]);
                    
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote-line_width_extra,crtp[2],cgtp[2],cbtp[2],cap[2]);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra-border_size, posNote+(line_width)+line_width_extra,crtp[3],cgtp[3],cbtp[3],cap[3]);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote+(line_width)+line_width_extra,crtp[3],cgtp[3],cbtp[3],cap[3]);
                    
                    //inner part
                    ptsB[index++] = LineVertexF(posStart-line_width_extra+border_size, posNote-line_width_extra+border_size,crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra-border_size, posNote-line_width_extra+border_size,crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posStart-line_width_extra+border_size, posNote+(line_width)+line_width_extra-border_size,crt,cgt,cbt,ca);
                    
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra-border_size, posNote-line_width_extra+border_size,crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posStart-line_width_extra+border_size, posNote+(line_width)+line_width_extra-border_size,crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra-border_size, posNote+(line_width)+line_width_extra-border_size,crt,cgt,cbt,ca);
                    
                    
                }
            }
        } else {  //vert
            float posNote=note*line_width-note_display_offset+subnote;
            float posStart=(int)(data_bar2draw[i].startidx)*hh/data_midifx_len;
            float posEnd=((int)(data_bar2draw[i].startidx)+(int)(data_bar2draw[i].size))*hh/data_midifx_len;
            if ( ((posNote+(line_width)+line_width_extra)>=0) && ((posNote-line_width_extra)<(int)ww)) {
                
                if ((settings[GLOB_FXMIDIBarStyle].detail.mdz_switch.switch_value==0)||(settings[GLOB_FXMIDIBarStyle].detail.mdz_switch.switch_value==2)) {
                    
                    if (index+12>=max_indices) {
                        glDrawArrays(GL_TRIANGLES, 0, index);
                        index=0;
                    }
                    
                    ptsB[index++] = LineVertexF(posNote-line_width_extra, posStart-line_width_extra,crtp[0],cgtp[0],cbtp[0],cap[0]);
                    ptsB[index++] = LineVertexF(posNote-line_width_extra, posEnd+line_width_extra,crtp[1],cgtp[1],cbtp[1],cap[1]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)/2, posStart-line_width_extra, crt,cgt,cbt,ca);
                    
                    ptsB[index++] = LineVertexF(posNote-line_width_extra, posEnd+line_width_extra,crtp[1],cgtp[1],cbtp[1],cap[1]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)/2, posStart-line_width_extra, crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posNote+(line_width)/2, posEnd+line_width_extra, crt,cgt,cbt,ca);
                    
                    ptsB[index++] = LineVertexF(posNote+(line_width)/2, posStart-line_width_extra, crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posNote+(line_width)/2, posEnd+line_width_extra, crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra, posStart-line_width_extra,crtp[2],cgtp[2],cbtp[2],cap[2]);
                    
                    ptsB[index++] = LineVertexF(posNote+(line_width)/2, posEnd+line_width_extra, crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra, posStart-line_width_extra,crtp[2],cgtp[2],cbtp[2],cap[2]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra, posEnd+line_width_extra,crtp[3],cgtp[3],cbtp[3],cap[3]);
                } else {
                    int border_size=(line_width>=8?2:1);
                    
                    if (index+30>=max_indices) {
                        glDrawArrays(GL_TRIANGLES, 0, index);
                        index=0;
                    }
                    
                    //top
                    ptsB[index++] = LineVertexF(posNote-line_width_extra, posStart-line_width_extra, crtp[0],cgtp[0],cbtp[0],cap[0]);
                    ptsB[index++] = LineVertexF(posNote-line_width_extra, posStart-line_width_extra+border_size, crtp[1],cgtp[1],cbtp[1],cap[1]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra, posStart-line_width_extra+border_size, crtp[1],cgtp[1],cbtp[1],cap[1]);
                    
                    ptsB[index++] = LineVertexF(posNote-line_width_extra, posStart-line_width_extra, crtp[0],cgtp[0],cbtp[0],cap[0]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra, posStart-line_width_extra+border_size, crtp[1],cgtp[1],cbtp[1],cap[1]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra, posStart-line_width_extra, crtp[1],cgtp[1],cbtp[1],cap[1]);
                    
                    //left
                    ptsB[index++] = LineVertexF(posNote-line_width_extra, posStart-line_width_extra, crtp[0],cgtp[0],cbtp[0],cap[0]);
                    ptsB[index++] = LineVertexF(posNote-line_width_extra, posEnd+line_width_extra, crtp[1],cgtp[1],cbtp[1],cap[1]);
                    ptsB[index++] = LineVertexF(posNote-line_width_extra+border_size, posEnd+line_width_extra, crtp[1],cgtp[1],cbtp[1],cap[1]);
                    
                    ptsB[index++] = LineVertexF(posNote-line_width_extra, posStart-line_width_extra, crtp[0],cgtp[0],cbtp[0],cap[0]);
                    ptsB[index++] = LineVertexF(posNote-line_width_extra+border_size, posEnd+line_width_extra, crtp[1],cgtp[1],cbtp[1],cap[1]);
                    ptsB[index++] = LineVertexF(posNote-line_width_extra+border_size, posStart-line_width_extra, crtp[0],cgtp[0],cbtp[0],cap[0]);
                    
                    //right
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra,posStart-line_width_extra, crtp[2],cgtp[2],cbtp[2],cap[2]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra,posEnd+line_width_extra, crtp[3],cgtp[3],cbtp[3],cap[3]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra-border_size,posEnd+line_width_extra, crtp[3],cgtp[3],cbtp[3],cap[3]);
                    
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra,posStart-line_width_extra, crtp[2],cgtp[2],cbtp[2],cap[2]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra-border_size,posEnd+line_width_extra, crtp[3],cgtp[3],cbtp[3],cap[3]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra-border_size,posStart-line_width_extra, crtp[2],cgtp[2],cbtp[2],cap[2]);
                    
                    //bottom
                    ptsB[index++] = LineVertexF(posNote-line_width_extra, posEnd+line_width_extra, crtp[2],cgtp[2],cbtp[2],cap[2]);
                    ptsB[index++] = LineVertexF(posNote-line_width_extra, posEnd+line_width_extra-border_size, crtp[3],cgtp[3],cbtp[3],cap[3]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra,posEnd+line_width_extra-border_size, crtp[3],cgtp[3],cbtp[3],cap[3]);
                    
                    ptsB[index++] = LineVertexF(posNote-line_width_extra, posEnd+line_width_extra, crtp[2],cgtp[2],cbtp[2],cap[2]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra, posEnd+line_width_extra-border_size, crtp[3],cgtp[3],cbtp[3],cap[3]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra, posEnd+line_width_extra, crtp[3],cgtp[3],cbtp[3],cap[3]);
                    
                    //inner part
                    ptsB[index++] = LineVertexF(posNote-line_width_extra+border_size, posStart-line_width_extra+border_size, crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posNote-line_width_extra+border_size, posEnd+line_width_extra-border_size, crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra-border_size, posStart-line_width_extra+border_size, crt,cgt,cbt,ca);
                    
                    ptsB[index++] = LineVertexF(posNote-line_width_extra+border_size, posEnd+line_width_extra-border_size, crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra-border_size, posStart-line_width_extra+border_size, crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra-border_size, posEnd+line_width_extra-border_size, crt,cgt,cbt,ca);
                }
            }
        }
    }
    
    //    printf("total: %d\n",index);
    
    glDrawArrays(GL_TRIANGLES, 0, index);
    
    //////////////////////////////////////////////
    
    //    glEnable(GL_BLEND);
    //    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    
    //current playing line
    //    230,76,153
    index=0;
    if (horiz_vert==0) {
        index=DrawBox(ptsB, index,
                      (data_midifx_len-MIDIFX_OFS-1)*band_width-band_width*2,
                      0,
                      band_width*2,hh,1,
                      235,210,255,200,0);
        
    } else {
        index=DrawBox(ptsB, index,
                      0,
                      (data_midifx_len-MIDIFX_OFS-1)*band_width-band_width*2,
                      ww,
                      band_width*2,1,
                      235,210,255,200,0);
    }
    glDrawArrays(GL_TRIANGLES, 0, index);
    //    glLineWidth(band_width*mScaleFactor);
    //    glDrawArrays(GL_LINES, 0, 2);
    
    
#if 0
    //Draw spark fx
    glDisable(GL_DEPTH_TEST);           /* Disable Depth Testing     */
    glEnable(GL_TEXTURE_2D);            /* Enable 2D Texture Mapping */
    glBindTexture(GL_TEXTURE_2D, txt_pianoRoll[TXT_PIANOROLL_SPARK]);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    
    
    glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
    /* Enable Texture Coordinations Pointer */
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    memset(sparkPresent,0,sizeof(sparkPresent));
    index=0;
    int midi_data_ofs=data_midifx_len-MIDIFX_OFS-1;
    if (settings[GLOB_FXPianoRollSpark].detail.mdz_switch.switch_value)
        for (int i=0; i<256; i++) { //for each channels
            if ((data_midifx_note[MIDIFX_LEN-MIDIFX_OFS-1][i])&&
                data_midifx_vol[MIDIFX_LEN-MIDIFX_OFS-1][i] &&
                ( data_midifx_vol[MIDIFX_LEN-MIDIFX_OFS-1][i]>=data_midifx_vol[MIDIFX_LEN-MIDIFX_OFS-1+1][i]) ) {  //do we have a note ?
                unsigned int note=data_midifx_note[MIDIFX_LEN-MIDIFX_OFS-1][i];
                if (!note) note=data_midifx_note[MIDIFX_LEN-MIDIFX_OFS-1+1][i];
                
                //avoid rendering twice for same note
                if (sparkPresent[note]) continue;
                sparkPresent[note]=1;
                if (sparkIntensity[note]<128) sparkIntensity[note]+=8;
                
                unsigned int instr=data_midifx_instr[MIDIFX_LEN-MIDIFX_OFS-1][i];
                int colidx=instr&63;
                int crt=((data_midifx_col[colidx&31]>>16)&0xFF);
                int cgt=((data_midifx_col[colidx&31]>>8)&0xFF);
                int cbt=(data_midifx_col[colidx&31]&0xFF);
                
                if (colidx&0x20) {
                    crt=(crt+255)/2;
                    cgt=(cgt+255)/2;
                    cbt=(cbt+255)/2;
                }
                
                crt=(crt*3+255*3)/6;
                cgt=(cgt*3+255*3)/6;
                cbt=(cbt*3+255*3)/6;
                
                if (crt>255) crt=255;
                if (cgt>255) cgt=255;
                if (cbt>255) cbt=255;
                
                
                line_width_extra=2;
                
                float posNote,posStart;
                float wd;
                float width=line_width;
                
                if (horiz_vert==0) { //horiz
                    posNote=note*line_width-note_display_offset;
                    posStart=(data_midifx_len-MIDIFX_OFS-1)*band_width;//+band_width;
                    
                    if ( ((posNote+(line_width)+line_width_extra)>=0) && ((posNote-line_width_extra)<(int)hh)) {
                        
                        wd=width;
                        posNote-=wd/2;
                        wd=wd*2;
                        
                        for (int sp=0;sp<4;sp++) {
                            
                            texcoords[index+0].u=0.0f; texcoords[index+0].v=80.0/128;
                            texcoords[index+1].u=0.0f; texcoords[index+1].v=18.0/128;
                            texcoords[index+2].u=1.0f; texcoords[index+2].v=80.0/128;
                            
                            texcoords[index+3].u=0.0f; texcoords[index+3].v=18.0/128;
                            texcoords[index+4].u=1.0f; texcoords[index+4].v=80.0/128;
                            texcoords[index+5].u=1.0f; texcoords[index+5].v=18.0/128;
                            
                            
                            ptsB[index+0].x=posStart;ptsB[index+0].y=posNote+wd;
                            ptsB[index+1].x=posStart+wd/2;ptsB[index+1].y=posNote+wd;
                            ptsB[index+2].x=posStart;ptsB[index+2].y=posNote;
                            
                            ptsB[index+3].x=posStart+wd/2;ptsB[index+3].y=posNote+wd;
                            ptsB[index+4].x=posStart;ptsB[index+4].y=posNote;
                            ptsB[index+5].x=posStart+wd/2;ptsB[index+5].y=posNote;
                            
                            //apply some distortion
                            float wd_distX=wd/9.0;
                            float wd_distY=wd/3.0;
                            
                            float distorFactors[4][4][6]={
                                {   {+0.7,  3 ,+0.2,  5, -0.3, 11},
                                    {+0.2,  3 ,+0.5,  5, -0.4, 5},
                                    
                                    {+0.5,  5 ,-0.1,  7, +0.4, 13},
                                    {+0.3,  2 ,-0.5,  3, +0.2, 3}},
                                
                                {   {+0.5,  1 ,+0.2,  7, -0.3, 7},
                                    {+0.3,  3 ,+0.5,  2, -0.4, 7},
                                    
                                    {-0.3,  7 ,-0.1,  9, +0.4, 5},
                                    {-0.2,  5 ,-0.5,  5, +0.2, 11}},
                                
                                {   {-0.7,  2 ,+0.2,  11, -0.3, 9},
                                    {+0.2,  5 ,+0.5,  13, +0.7, 5},
                                    
                                    {+0.6,  9 ,-0.1,  3, +0.4, 3},
                                    {+0.4,  1 ,-0.5,  3, +0.2, 8}},
                                
                                {   {+0.8,  9 ,+0.2,  7, -0.3, 9},
                                    {+0.4,  11 ,+0.5, 7, +0.4, 5},
                                    
                                    {-0.5,  3 ,+0.1,  4, -0.4, 11},
                                    {-0.3,  5 ,+0.5,  5, +0.2, 3}}};
                            
                            ptsB[index+1].x+=wd_distX*(distorFactors[sp][0][0]*sin(pianoroll_cpt*distorFactors[sp][0][1]*3.14159/32)
                                                       +distorFactors[sp][0][2]*sin(pianoroll_cpt*distorFactors[sp][0][3]*3.14159/32)
                                                       +distorFactors[sp][0][4]*sin(pianoroll_cpt*distorFactors[sp][0][5]*3.14159/32));
                            
                            ptsB[index+3].x+=wd_distX*(distorFactors[sp][0][0]*sin(pianoroll_cpt*distorFactors[sp][0][1]*3.14159/32)
                                                       +distorFactors[sp][0][2]*sin(pianoroll_cpt*distorFactors[sp][0][3]*3.14159/32)
                                                       +distorFactors[sp][0][4]*sin(pianoroll_cpt*distorFactors[sp][0][5]*3.14159/32));
                            
                            ptsB[index+1].y+=wd_distY*(distorFactors[sp][1][0]*sin(pianoroll_cpt*distorFactors[sp][1][1]*3.14159/32)
                                                       +distorFactors[sp][1][2]*sin(pianoroll_cpt*distorFactors[sp][1][3]*3.14159/32)
                                                       +distorFactors[sp][1][4]*sin(pianoroll_cpt*distorFactors[sp][1][5]*3.14159/32));
                            
                            ptsB[index+3].y+=wd_distY*(distorFactors[sp][1][0]*sin(pianoroll_cpt*distorFactors[sp][1][1]*3.14159/32)
                                                       +distorFactors[sp][1][2]*sin(pianoroll_cpt*distorFactors[sp][1][3]*3.14159/32)
                                                       +distorFactors[sp][1][4]*sin(pianoroll_cpt*distorFactors[sp][1][5]*3.14159/32));
                            
                            ptsB[index+5].x+=wd_distX*(distorFactors[sp][2][0]*sin(pianoroll_cpt*distorFactors[sp][2][1]*3.14159/32)
                                                       +distorFactors[sp][2][2]*sin(pianoroll_cpt*distorFactors[sp][2][3]*3.14159/32)
                                                       +distorFactors[sp][2][4]*sin(pianoroll_cpt*distorFactors[sp][2][5]*3.14159/32));
                            
                            ptsB[index+5].y+=wd_distY*(distorFactors[sp][3][0]*sin(pianoroll_cpt*distorFactors[sp][3][1]*3.14159/32)
                                                       +distorFactors[sp][3][2]*sin(pianoroll_cpt*distorFactors[sp][3][3]*3.14159/32)
                                                       +distorFactors[sp][3][4]*sin(pianoroll_cpt*distorFactors[sp][3][5]*3.14159/32));
                            
                            if (sp&1) {
                                for (int ii=0;ii<6;ii++) {
                                    texcoords[index+ii].u=1-texcoords[index+ii].u;
                                }
                                
                            }
                            for (int ii=0;ii<6;ii++) {
                                if (settings[GLOB_FXPianoRollSpark].detail.mdz_switch.switch_value==2) {
                                    ptsB[index+ii].r=255;
                                    ptsB[index+ii].g=255;
                                    ptsB[index+ii].b=255;
                                } else {
                                    ptsB[index+ii].r=crt;
                                    ptsB[index+ii].g=cgt;
                                    ptsB[index+ii].b=cbt;
                                }
                                ptsB[index+ii].a=sparkIntensity[note]/4;///4;
                            }
                            index+=6;
                            
                        }
                    }
                }else { //vert
                    posNote=note*line_width-note_display_offset;
                    posStart=(int)midi_data_ofs*hh/data_midifx_len;
                }
            }
        }
    glDrawArrays(GL_TRIANGLES, 0, index);
    
    glBindTexture(GL_TEXTURE_2D, txt_pianoRoll[TXT_PIANOROLL_LIGHT]);
    memset(sparkPresent,0,sizeof(sparkPresent));
    index=0;
    if (settings[GLOB_FXPianoRollSpark].detail.mdz_switch.switch_value)
        for (int i=0; i<256; i++) { //for each channels
            if ((data_midifx_note[MIDIFX_LEN-MIDIFX_OFS-1][i])&&
                data_midifx_vol[MIDIFX_LEN-MIDIFX_OFS-1][i] &&
                ( data_midifx_vol[MIDIFX_LEN-MIDIFX_OFS-1][i]>=data_midifx_vol[MIDIFX_LEN-MIDIFX_OFS-1+1][i]) ) {  //do we have a note ?
                unsigned int note=data_midifx_note[MIDIFX_LEN-MIDIFX_OFS-1][i];
                if (!note) note=data_midifx_note[MIDIFX_LEN-MIDIFX_OFS-1+1][i];
                
                //avoid rendering twice for same note
                if (sparkPresent[note]) continue;
                sparkPresent[note]=1;
                if (sparkIntensity[note]<128) sparkIntensity[note]+=8;
                
                unsigned int instr=data_midifx_instr[MIDIFX_LEN-MIDIFX_OFS-1][i];
                int colidx=instr&63;
                int crt=((data_midifx_col[colidx&31]>>16)&0xFF);
                int cgt=((data_midifx_col[colidx&31]>>8)&0xFF);
                int cbt=(data_midifx_col[colidx&31]&0xFF);
                
                if (colidx&0x20) {
                    crt=(crt+255)/2;
                    cgt=(cgt+255)/2;
                    cbt=(cbt+255)/2;
                }
                
                crt=(crt*3+255*3)/6;
                cgt=(cgt*3+255*3)/6;
                cbt=(cbt*3+255*3)/6;
                
                if (crt>255) crt=255;
                if (cgt>255) cgt=255;
                if (cbt>255) cbt=255;
                
                
                line_width_extra=2;
                
                float posNote,posStart;
                float wd;
                float width=line_width;
                
                if (horiz_vert==0) { //horiz
                    posNote=note*line_width-note_display_offset;
                    posStart=(data_midifx_len-MIDIFX_OFS-1)*band_width;//+band_width;
                    
                    if ( ((posNote+(line_width)+line_width_extra)>=0) && ((posNote-line_width_extra)<(int)hh)) {
                        
                        wd=width*2;
                        posStart-=wd;
                        posNote-=wd/2+width/2;
                        wd=wd*2;
                        
                        for (int sp=0;sp<4;sp++) {
                            
                            texcoords[index+0].u=0.0f; texcoords[index+0].v=128.0/128;
                            texcoords[index+1].u=0.0f; texcoords[index+1].v=0.0/128;
                            texcoords[index+2].u=1.0f; texcoords[index+2].v=128.0/128;
                            
                            texcoords[index+3].u=0.0f; texcoords[index+3].v=0.0/128;
                            texcoords[index+4].u=1.0f; texcoords[index+4].v=128.0/128;
                            texcoords[index+5].u=1.0f; texcoords[index+5].v=0.0/128;
                            
                            
                            ptsB[index+0].x=posStart;ptsB[index+0].y=posNote+wd;
                            ptsB[index+1].x=posStart+wd*1.2;ptsB[index+1].y=posNote+wd;
                            ptsB[index+2].x=posStart;ptsB[index+2].y=posNote;
                            
                            ptsB[index+3].x=posStart+wd*1.2;ptsB[index+3].y=posNote+wd;
                            ptsB[index+4].x=posStart;ptsB[index+4].y=posNote;
                            ptsB[index+5].x=posStart+wd*1.2;ptsB[index+5].y=posNote;
                            
                            //apply some distortion
                            float wd_distX=wd/18.0;
                            float wd_distY=wd/6.0;
                            
                            float distorFactors[4][4][6]={
                                {   {+0.7,  3 ,+0.2,  5, -0.3, 11},
                                    {+0.2,  3 ,+0.5,  5, -0.4, 5},
                                    
                                    {+0.5,  5 ,-0.1,  7, +0.4, 13},
                                    {+0.3,  2 ,-0.5,  3, +0.2, 3}},
                                
                                {   {+0.5,  1 ,+0.2,  7, -0.3, 7},
                                    {+0.3,  3 ,+0.5,  2, -0.4, 7},
                                    
                                    {-0.3,  7 ,-0.1,  9, +0.4, 5},
                                    {-0.2,  5 ,-0.5,  5, +0.2, 11}},
                                
                                {   {-0.7,  2 ,+0.2,  11, -0.3, 9},
                                    {+0.2,  5 ,+0.5,  13, +0.7, 5},
                                    
                                    {+0.6,  9 ,-0.1,  3, +0.4, 3},
                                    {+0.4,  1 ,-0.5,  3, +0.2, 8}},
                                
                                {   {+0.8,  9 ,+0.2,  7, -0.3, 9},
                                    {+0.4,  11 ,+0.5, 7, +0.4, 5},
                                    
                                    {-0.5,  3 ,+0.1,  4, -0.4, 11},
                                    {-0.3,  5 ,+0.5,  5, +0.2, 3}}};
                            
                            ptsB[index+1].x+=wd_distX*(distorFactors[sp][0][0]*sin(pianoroll_cpt*distorFactors[sp][0][1]*3.14159/32)
                                                       +distorFactors[sp][0][2]*sin(pianoroll_cpt*distorFactors[sp][0][3]*3.14159/32)
                                                       +distorFactors[sp][0][4]*sin(pianoroll_cpt*distorFactors[sp][0][5]*3.14159/32));
                            
                            ptsB[index+3].x+=wd_distX*(distorFactors[sp][0][0]*sin(pianoroll_cpt*distorFactors[sp][0][1]*3.14159/32)
                                                       +distorFactors[sp][0][2]*sin(pianoroll_cpt*distorFactors[sp][0][3]*3.14159/32)
                                                       +distorFactors[sp][0][4]*sin(pianoroll_cpt*distorFactors[sp][0][5]*3.14159/32));
                            
                            ptsB[index+1].y+=wd_distY*(distorFactors[sp][1][0]*sin(pianoroll_cpt*distorFactors[sp][1][1]*3.14159/32)
                                                       +distorFactors[sp][1][2]*sin(pianoroll_cpt*distorFactors[sp][1][3]*3.14159/32)
                                                       +distorFactors[sp][1][4]*sin(pianoroll_cpt*distorFactors[sp][1][5]*3.14159/32));
                            
                            ptsB[index+3].y+=wd_distY*(distorFactors[sp][1][0]*sin(pianoroll_cpt*distorFactors[sp][1][1]*3.14159/32)
                                                       +distorFactors[sp][1][2]*sin(pianoroll_cpt*distorFactors[sp][1][3]*3.14159/32)
                                                       +distorFactors[sp][1][4]*sin(pianoroll_cpt*distorFactors[sp][1][5]*3.14159/32));
                            
                            ptsB[index+5].x+=wd_distX*(distorFactors[sp][2][0]*sin(pianoroll_cpt*distorFactors[sp][2][1]*3.14159/32)
                                                       +distorFactors[sp][2][2]*sin(pianoroll_cpt*distorFactors[sp][2][3]*3.14159/32)
                                                       +distorFactors[sp][2][4]*sin(pianoroll_cpt*distorFactors[sp][2][5]*3.14159/32));
                            
                            ptsB[index+5].y+=wd_distY*(distorFactors[sp][3][0]*sin(pianoroll_cpt*distorFactors[sp][3][1]*3.14159/32)
                                                       +distorFactors[sp][3][2]*sin(pianoroll_cpt*distorFactors[sp][3][3]*3.14159/32)
                                                       +distorFactors[sp][3][4]*sin(pianoroll_cpt*distorFactors[sp][3][5]*3.14159/32));
                            
                            if (sp&1) {
                                for (int ii=0;ii<6;ii++) {
                                    texcoords[index+ii].u=1-texcoords[index+ii].u;
                                }
                                
                            }
                            for (int ii=0;ii<6;ii++) {
                                if (settings[GLOB_FXPianoRollSpark].detail.mdz_switch.switch_value==2) {
                                    ptsB[index+ii].r=255;
                                    ptsB[index+ii].g=255;
                                    ptsB[index+ii].b=255;
                                } else {
                                    ptsB[index+ii].r=crt;
                                    ptsB[index+ii].g=cgt;
                                    ptsB[index+ii].b=cbt;
                                }
                                ptsB[index+ii].a=sparkIntensity[note]/4;
                            }
                            index+=6;
                            
                        }
                    }
                }else { //vert
                    posNote=note*line_width-note_display_offset;
                    posStart=(int)midi_data_ofs*hh/data_midifx_len;
                }
            }
        }
    glDrawArrays(GL_TRIANGLES, 0, index);
    
    //reset spark intensity if no note played
    for (int i=0;i<256;i++) {
        if (sparkPresent[i]==0) {
            if (sparkIntensity[i]>8) sparkIntensity[i]-=8;
            else sparkIntensity[i]=0;
        }
    }
    
    
    glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_TEXTURE_2D);            /* Enable 2D Texture Mapping */
    glEnableClientState(GL_COLOR_ARRAY);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    index=0;
#endif
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisable(GL_BLEND);
    
    free(ptsB);
    free(texcoords);
}


int RenderUtils::DrawBox(LineVertexF *ptsB,int index,float x,float y,float width,float height,float border_size,int crt,int cgt,int cbt,int ca,int subnote) {
    int crtp[6],cgtp[6],cbtp[6],cap[6];
    
    for (int ii=0;ii<4;ii++) {
        double fact=1;
        double ofs=0;
        if ((ii==3)||(ii==2)) {
            fact=1.5f;
            ofs=100;
        }
        if ((ii==0)||(ii==1)) {
            fact=0.5f;
            ofs=0;
        }
        crtp[ii]=crt*fact+ofs;
        cgtp[ii]=cgt*fact+ofs;
        cbtp[ii]=cbt*fact+ofs;
        cap[ii]=ca;
        if (crtp[ii]>255) crtp[ii]=255;
        if (cgtp[ii]>255) cgtp[ii]=255;
        if (cbtp[ii]>255) cbtp[ii]=255;
        if (cap[ii]>255) cap[ii]=255;
    }
    //top
    ptsB[index++] = LineVertexF(x, y,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width, y,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x+width, y+border_size,crtp[1],cgtp[1],cbtp[1],cap[1]);
    
    ptsB[index++] = LineVertexF(x, y,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width, y+border_size,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x, y+border_size,crtp[0],cgtp[0],cbtp[0],cap[0]);
    
    //left
    ptsB[index++] = LineVertexF(x, y,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+border_size, y,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x+border_size, y+height,crtp[1],cgtp[1],cbtp[1],cap[1]);
    
    ptsB[index++] = LineVertexF(x, y,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+border_size, y+height,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x, y+height,crtp[1],cgtp[1],cbtp[1],cap[1]);
    
    //bottom
    ptsB[index++] = LineVertexF(x, y+height,crtp[2],cgtp[2],cbtp[2],cap[2]);
    ptsB[index++] = LineVertexF(x+width, y+height,crtp[3],cgtp[3],cbtp[3],cap[3]);
    ptsB[index++] = LineVertexF(x+width, y+height-border_size,crtp[3],cgtp[3],cbtp[3],cap[3]);
    
    ptsB[index++] = LineVertexF(x, y+height,crtp[2],cgtp[2],cbtp[2],cap[2]);
    ptsB[index++] = LineVertexF(x+width, y+height-border_size,crtp[3],cgtp[3],cbtp[3],cap[3]);
    ptsB[index++] = LineVertexF(x, y+height-border_size,crtp[2],cgtp[2],cbtp[2],cap[2]);
    
    //right
    ptsB[index++] = LineVertexF(x+width, y,crtp[2],cgtp[2],cbtp[2],cap[2]);
    ptsB[index++] = LineVertexF(x+width-border_size, y,crtp[3],cgtp[3],cbtp[3],cap[3]);
    ptsB[index++] = LineVertexF(x+width-border_size, y+height,crtp[3],cgtp[3],cbtp[3],cap[3]);
    
    ptsB[index++] = LineVertexF(x+width, y,crtp[2],cgtp[2],cbtp[2],cap[2]);
    ptsB[index++] = LineVertexF(x+width-border_size, y+height,crtp[3],cgtp[3],cbtp[3],cap[3]);
    ptsB[index++] = LineVertexF(x+width, y+height,crtp[3],cgtp[3],cbtp[3],cap[3]);
    
    //inner part
    if (subnote) {
        if (subnote>0) {
            float fact=1.1+(float)subnote*0.25/8.0;
            float ofs=32+subnote*4;
            crtp[4]=crt*fact+ofs;cgtp[4]=cgt*fact+ofs;cbtp[4]=cbt*fact+ofs;cap[4]=ca;
            crtp[5]=crt*fact+ofs;cgtp[5]=cgt*fact+ofs;cbtp[5]=cbt*fact+ofs;cap[5]=ca;
        } else {
            float fact=1;
            float ofs=0;
            fact=1.1-(float)subnote*0.25/8.0; ofs=32-subnote*4;
            crtp[4]=crt*fact+ofs;cgtp[4]=cgt*fact+ofs;cbtp[4]=cbt*fact+ofs;cap[4]=ca;
            crtp[5]=crt*fact+ofs;cgtp[5]=cgt*fact+ofs;cbtp[5]=cbt*fact+ofs;cap[5]=ca;
        }
        for (int ii=4;ii<6;ii++) {
            if (crtp[ii]>255) crtp[ii]=255;
            if (cgtp[ii]>255) cgtp[ii]=255;
            if (cbtp[ii]>255) cbtp[ii]=255;
            if (cap[ii]>255) cap[ii]=255;
        }
    } else {
        crtp[4]=crt;cgtp[4]=cgt;cbtp[4]=cbt;cap[4]=ca;
        crtp[5]=crt;cgtp[5]=cgt;cbtp[5]=cbt;cap[5]=ca;
    }
    ptsB[index++] = LineVertexF(x+border_size, y+border_size,crtp[4],cgtp[4],cbtp[4],cap[4]);
    ptsB[index++] = LineVertexF(x+width-border_size, y+border_size,crtp[5],cgtp[5],cbtp[5],cap[5]);
    ptsB[index++] = LineVertexF(x+border_size, y+height-border_size,crtp[4],cgtp[4],cbtp[4],cap[4]);
    
    ptsB[index++] = LineVertexF(x+width-border_size, y+border_size,crtp[5],cgtp[5],cbtp[5],cap[5]);
    ptsB[index++] = LineVertexF(x+border_size, y+height-border_size,crtp[4],cgtp[4],cbtp[4],cap[4]);
    ptsB[index++] = LineVertexF(x+width-border_size, y+height-border_size,crtp[5],cgtp[5],cbtp[5],cap[5]);
    
    return index;
}

int lastkey_type;

#define PR_SHADOW_WHITE (1<<0)
#define PR_SHADOW_SMALL_BLACK (1<<1)
#define PR_SHADOW_LARGE_BLACK (1<<2)

int RenderUtils::DrawKeyW(LineVertexF *ptsB,int index,float x,float y,float width,float height,float border_size,int crt,int cgt,int cbt,int ca,int subnote,int note_idx,int channel) {
    int crtp[6],cgtp[6],cbtp[6],cap[6];
    float height2;
    uint8_t shadow_type=0;
    float shadow_left_black_ofs=0;
    
    bool pressed=pianoroll_key_status[channel][note_idx]&PR_KEY_PRESSED;
    
    if (note_idx) { //not 1st key
        if (pianoroll_key_status[channel][note_idx-1]&PR_WHITE_KEY) {
            //no black key on the left
            if (!(pianoroll_key_status[channel][note_idx-1]&PR_KEY_PRESSED)) {
                //left white key not pressed
                //shadow if current key is pressed
                if (pressed) shadow_type=PR_SHADOW_WHITE;
            }
        } else {
            //black key on the left
            if (!(pianoroll_key_status[channel][note_idx-1]&PR_KEY_PRESSED)) {
                //left black key not pressed, shadow on
                //small shadow if key not pressed or else large shadow
                if (pressed) shadow_type=PR_SHADOW_LARGE_BLACK;
                else shadow_type=PR_SHADOW_SMALL_BLACK;
                
                //check black key type
                uint8_t note_type=(pianoroll_key_status[channel][note_idx-1]>>4)%12;
                switch (note_type) {
                    case 1://C#
                    case 6://F#
                        shadow_left_black_ofs=(width*2.0f/12);
                        break;
                    case 3://D#
                    case 10://A#
                        shadow_left_black_ofs=(width*4.0f/12);
                        break;
                    case 8://G#
                        shadow_left_black_ofs=(width*3.0f/12);
                        break;
                    default:
                        break;
                }
            }
            //check white key on the left
            if (note_idx>=2) {
                if (!(pianoroll_key_status[channel][note_idx-2]&PR_KEY_PRESSED)) {
                    //left white key not pressed
                    //shadow if current key is pressed
                    if (pressed) shadow_type|=PR_SHADOW_WHITE;
                }
            }
        }
    }
    
    
    if (!pressed) {
        //y+=height/16;
        //height=height*15/16;
        height2=height/8;
    } else {
        height2=height/24;
        crt=crt*0.8f;
        cgt=cgt*0.8f;
        cbt=cbt*0.8f;
    }
    
    for (int ii=0;ii<4;ii++) {
        double fact=1;
        double ofs=0;
        switch (ii) {
            case 3:fact=1.5f; ofs=100; break;
            case 2:fact=0.75f; ofs=0; break;
            case 1:fact=0.5f; ofs=0; break;
            case 0:fact=0.25f; ofs=0; break;
        }
        crtp[ii]=crt*fact+ofs;cgtp[ii]=cgt*fact+ofs;cbtp[ii]=cbt*fact+ofs;cap[ii]=ca;
        if (crtp[ii]>255) crtp[ii]=255;if (cgtp[ii]>255) cgtp[ii]=255;if (cbtp[ii]>255) cbtp[ii]=255;
        if (cap[ii]>255) cap[ii]=255;
    }
    if (subnote) {
        if (subnote>0) {
            float fact=1.1+(float)subnote*0.1/8.0;
            float ofs=16+subnote*2;
            crtp[4]=crt*fact+ofs;cgtp[4]=cgt*fact+ofs;cbtp[4]=cbt*fact+ofs;cap[4]=ca;
            crtp[5]=crt*fact+ofs;cgtp[5]=cgt*fact+ofs;cbtp[5]=cbt*fact+ofs;cap[5]=ca;
        } else {
            float fact=1;
            float ofs=0;
            fact=1.1-(float)subnote*0.1/8.0; ofs=16-subnote*2;
            crtp[4]=crt*fact+ofs;cgtp[4]=cgt*fact+ofs;cbtp[4]=cbt*fact+ofs;cap[4]=ca;
            crtp[5]=crt*fact+ofs;cgtp[5]=cgt*fact+ofs;cbtp[5]=cbt*fact+ofs;cap[5]=ca;
        }
        for (int ii=4;ii<6;ii++) {
            if (crtp[ii]>255) crtp[ii]=255;
            if (cgtp[ii]>255) cgtp[ii]=255;
            if (cbtp[ii]>255) cbtp[ii]=255;
            if (cap[ii]>255) cap[ii]=255;
        }
    } else {
        crtp[4]=crt;cgtp[4]=cgt;cbtp[4]=cbt;cap[4]=ca;
        crtp[5]=crt;cgtp[5]=cgt;cbtp[5]=cbt;cap[5]=ca;
    }
    
    //left high
    ptsB[index++] = LineVertexF(x               , y+height2         ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+1             , y+height2         ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+1             , y+height  ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    
    ptsB[index++] = LineVertexF(x               , y+height2         ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+1             , y+height  ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x               , y+height  ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    
    //left low
    ptsB[index++] = LineVertexF(x               , y         ,0,0,0,cap[0]);
    ptsB[index++] = LineVertexF(x+width/12      , y         ,0,0,0,cap[0]);
    ptsB[index++] = LineVertexF(x+width/12      , y+height2 ,0,0,0,cap[0]);
    
    ptsB[index++] = LineVertexF(x               , y          ,0,0,0,cap[0]);
    ptsB[index++] = LineVertexF(x+width/12      , y+height2  ,0,0,0,cap[0]);
    ptsB[index++] = LineVertexF(x               , y+height2  ,0,0,0,cap[0]);
    
    //right high
    ptsB[index++] = LineVertexF(x+width             , y+height2     ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width-1           , y+height2     ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width-1           , y+height      ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    
    ptsB[index++] = LineVertexF(x+width             , y+height2 ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width-1           , y+height  ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width             , y+height  ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    
    //right low
    ptsB[index++] = LineVertexF(x+width             , y         ,0,0,0,cap[0]);
    ptsB[index++] = LineVertexF(x+width-width/12    , y         ,0,0,0,cap[0]);
    ptsB[index++] = LineVertexF(x+width-width/12    , y+height2 ,0,0,0,cap[0]);
    
    ptsB[index++] = LineVertexF(x+width             , y          ,0,0,0,cap[0]);
    ptsB[index++] = LineVertexF(x+width-width/12    , y+height2  ,0,0,0,cap[0]);
    ptsB[index++] = LineVertexF(x+width             , y+height2  ,0,0,0,cap[0]);
    
    //inner part high
    
    ptsB[index++] = LineVertexF(x+1           , y+height2+2   ,crtp[4],cgtp[4],cbtp[4],cap[4]);
    ptsB[index++] = LineVertexF(x+width-1     , y+height2+2   ,crtp[5],cgtp[5],cbtp[5],cap[5]);
    ptsB[index++] = LineVertexF(x+1           , y+height      ,crtp[4],cgtp[4],cbtp[4],cap[4]);
    
    ptsB[index++] = LineVertexF(x+width-1     , y+height2+2   ,crtp[5],cgtp[5],cbtp[5],cap[5]);
    ptsB[index++] = LineVertexF(x+1           , y+height      ,crtp[4],cgtp[4],cbtp[4],cap[4]);
    ptsB[index++] = LineVertexF(x+width-1     , y+height      ,crtp[5],cgtp[5],cbtp[5],cap[5]);
    
    //top
    //    ptsB[index++] = LineVertexF(x+1               , y+height     ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    //    ptsB[index++] = LineVertexF(x+width-1         , y+height     ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    //    ptsB[index++] = LineVertexF(x+width-1         , y+height-2   ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    //
    //    ptsB[index++] = LineVertexF(x+1               , y+height     ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    //    ptsB[index++] = LineVertexF(x+width-1         , y+height-2   ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    //    ptsB[index++] = LineVertexF(x+1               , y+height-2   ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    
    
    //inner part low
    ptsB[index++] = LineVertexF(x+width/16           , y+2   ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x+width-width/16     , y+2   ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x+width/16           , y+height2      ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    
    ptsB[index++] = LineVertexF(x+width-width/16     , y+2   ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x+width/16           , y+height2      ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x+width-width/16     , y+height2      ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    
    //bottom high
    ptsB[index++] = LineVertexF(x+1       , y+height2             ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    ptsB[index++] = LineVertexF(x+width-1 , y+height2             ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    ptsB[index++] = LineVertexF(x+width-1 , y+height2+2           ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    
    ptsB[index++] = LineVertexF(x+1       , y+height2             ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    ptsB[index++] = LineVertexF(x+width-1 , y+height2+2           ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    ptsB[index++] = LineVertexF(x+1       , y+height2+2           ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    
    //bottom low
    ptsB[index++] = LineVertexF(x+width/16       , y             ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width-width/16 , y             ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width-width/16 , y+2 ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    
    ptsB[index++] = LineVertexF(x+width/16       , y             ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width-width/16 , y+2 ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width/16       , y+2 ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    
    //shadow
    if (shadow_type) {
        if (shadow_type&PR_SHADOW_WHITE) {
            ptsB[index++] = LineVertexF(x+1       , y+height*7/8    ,0,0,0,128);
            ptsB[index++] = LineVertexF(x+width/3 , y+height2+height2       ,0,0,0,0);
            ptsB[index++] = LineVertexF(x+1       , y+height2           ,0,0,0,128);
        }
        
        if (shadow_type&PR_SHADOW_SMALL_BLACK) {
            ptsB[index++] = LineVertexF(x+shadow_left_black_ofs         , y+height    ,0,0,0,128);
            ptsB[index++] = LineVertexF(x+shadow_left_black_ofs+width/4 , y+height*2/5+height2       ,0,0,0,0);
            ptsB[index++] = LineVertexF(x+shadow_left_black_ofs         , y+height*2/5           ,0,0,0,128);
        }
        
        if (shadow_type&PR_SHADOW_LARGE_BLACK) {
            ptsB[index++] = LineVertexF(x+shadow_left_black_ofs         , y+height    ,0,0,0,128);
            ptsB[index++] = LineVertexF(x+shadow_left_black_ofs+width/5 , y+height      ,0,0,0,128);
            ptsB[index++] = LineVertexF(x+shadow_left_black_ofs         , y+height*2/5           ,0,0,0,128);
            
            ptsB[index++] = LineVertexF(x+shadow_left_black_ofs+width/5 , y+height    ,0,0,0,128);
            ptsB[index++] = LineVertexF(x+shadow_left_black_ofs+width/2 , y+height*2/5+height2*3       ,0,0,0,0);
            ptsB[index++] = LineVertexF(x+shadow_left_black_ofs         , y+height*2/5           ,0,0,0,128);
        }
    }
    
    return index;
}


int RenderUtils::DrawKeyB(LineVertexF *ptsB,int index,float x,float y,float width,float height,float border_size,int crt,int cgt,int cbt,int ca,int subnote,int note_idx,int channel) {
    int crtp[6],cgtp[6],cbtp[6],cap[6];
    float height2;
    
    bool pressed=pianoroll_key_status[channel][note_idx]&PR_KEY_PRESSED;
    
    if (!pressed) {
        //y+=height/16;
        //height=height*15/16;
        height2=height/4;
    } else {
        height2=height/12;
        crt=crt*0.8f;
        cgt=cgt*0.8f;
        cbt=cbt*0.8f;
    }
    
    for (int ii=0;ii<4;ii++) {
        double fact=1;
        double ofs=0;
        switch (ii) {
            case 3:fact=1.5f; ofs=128; break;
            case 2:fact=1.2f; ofs=128; break;
            case 1:fact=1.25f; ofs=100; break;
            case 0:fact=0.5f; ofs=0; break;
        }
        crtp[ii]=crt*fact+ofs;cgtp[ii]=cgt*fact+ofs;cbtp[ii]=cbt*fact+ofs;cap[ii]=ca;
        if (crtp[ii]>255) crtp[ii]=255;if (cgtp[ii]>255) cgtp[ii]=255;if (cbtp[ii]>255) cbtp[ii]=255;
        if (cap[ii]>255) cap[ii]=255;
    }
    if (subnote) {
        if (subnote>0) {
            float fact=1.1+(float)subnote*0.25/8.0;
            float ofs=32+subnote*4;
            crtp[4]=crt*fact+ofs;cgtp[4]=cgt*fact+ofs;cbtp[4]=cbt*fact+ofs;cap[4]=ca;
            crtp[5]=crt*fact+ofs;cgtp[5]=cgt*fact+ofs;cbtp[5]=cbt*fact+ofs;cap[5]=ca;
        } else {
            float fact=1;
            float ofs=0;
            fact=1.1-(float)subnote*0.25/8.0; ofs=32-subnote*4;
            crtp[4]=crt*fact+ofs;cgtp[4]=cgt*fact+ofs;cbtp[4]=cbt*fact+ofs;cap[4]=ca;
            crtp[5]=crt*fact+ofs;cgtp[5]=cgt*fact+ofs;cbtp[5]=cbt*fact+ofs;cap[5]=ca;
        }
        for (int ii=4;ii<6;ii++) {
            if (crtp[ii]>255) crtp[ii]=255;
            if (cgtp[ii]>255) cgtp[ii]=255;
            if (cbtp[ii]>255) cbtp[ii]=255;
            if (cap[ii]>255) cap[ii]=255;
        }
    } else {
        crtp[4]=crt;cgtp[4]=cgt;cbtp[4]=cbt;cap[4]=ca;
        crtp[5]=crt;cgtp[5]=cgt;cbtp[5]=cbt;cap[5]=ca;
    }
    
    //to have the black key slightly overlapping the felt
    height*=1.02;
    
    //left
    ptsB[index++] = LineVertexF(x               , y         ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width/8             , y+height2         ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x+width/8             , y+height  ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    
    ptsB[index++] = LineVertexF(x               , y         ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width/8             , y+height  ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x               , y+height  ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    
    //right
    ptsB[index++] = LineVertexF(x+width             , y         ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width-width/8           , y+height2         ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x+width-width/8           , y+height  ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    
    ptsB[index++] = LineVertexF(x+width             , y         ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width-width/8           , y+height  ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x+width             , y+height  ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    
    //inner part high
    
    ptsB[index++] = LineVertexF(x+width/8           , y+height2+2   ,crtp[4],cgtp[4],cbtp[4],cap[4]);
    ptsB[index++] = LineVertexF(x+width-width/8     , y+height2+2   ,crtp[5],cgtp[5],cbtp[5],cap[5]);
    ptsB[index++] = LineVertexF(x+width/8           , y+height      ,crtp[4],cgtp[4],cbtp[4],cap[4]);
    
    ptsB[index++] = LineVertexF(x+width-width/8     , y+height2+2   ,crtp[5],cgtp[5],cbtp[5],cap[5]);
    ptsB[index++] = LineVertexF(x+width/8           , y+height     ,crtp[4],cgtp[4],cbtp[4],cap[4]);
    ptsB[index++] = LineVertexF(x+width-width/8     , y+height      ,crtp[5],cgtp[5],cbtp[5],cap[5]);
    
    //top
    //    ptsB[index++] = LineVertexF(x+1               , y+height     ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    //    ptsB[index++] = LineVertexF(x+width-1         , y+height     ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    //    ptsB[index++] = LineVertexF(x+width-1         , y+height-2   ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    //
    //    ptsB[index++] = LineVertexF(x+1               , y+height     ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    //    ptsB[index++] = LineVertexF(x+width-1         , y+height-2   ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    //    ptsB[index++] = LineVertexF(x+1               , y+height-2   ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    
    
    //inner part low
    ptsB[index++] = LineVertexF(x           , y+2         ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width     , y+2         ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width/8           , y+height2   ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    
    ptsB[index++] = LineVertexF(x+width     , y+2         ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width/8           , y+height2   ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x+width-width/8     , y+height2   ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    
    //bottom high
    ptsB[index++] = LineVertexF(x+width/8       , y+height2             ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    ptsB[index++] = LineVertexF(x+width-width/8 , y+height2             ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    ptsB[index++] = LineVertexF(x+width-width/8 , y+height2+2           ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    
    ptsB[index++] = LineVertexF(x+width/8       , y+height2             ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    ptsB[index++] = LineVertexF(x+width-width/8 , y+height2+2           ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    ptsB[index++] = LineVertexF(x+width/8       , y+height2+2           ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    
    //bottom low
    ptsB[index++] = LineVertexF(x      , y             ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width , y             ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width , y+2 ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    
    ptsB[index++] = LineVertexF(x       , y             ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width , y+2 ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x       , y+2 ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    
    return index;
}


void RenderUtils::DrawPianoRollFX(uint ww,uint hh,int horiz_vert,float note_display_range, float note_display_offset,int fx_len,int color_mode,float mScaleFactor,char *voices_label) {
    LineVertexF *ptsB;
    int crt,cgt,cbt,ca;
    int index;
    int voices_posX[SOUND_MAXVOICES_BUFFER_FX];
    //int band_width,ofs_band;
    static bool first_call=true;
    
    
    if (first_call) {
        for (int i=0;i<SOUND_MAXVOICES_BUFFER_FX;i++) {
            mVoicesNamePiano[i]=NULL;
        }
        first_call=false;
        memset(mOctavesIndex,0,sizeof(mOctavesIndex));
    }
    
    if (fx_len>MIDIFX_LEN) fx_len=MIDIFX_LEN;
    if (fx_len<=MIDIFX_OFS) fx_len=MIDIFX_OFS+1;
    
    if (fx_len!=data_midifx_len) {
        data_midifx_len=fx_len;
        //data_midifx_first=1;
    }
    
    if (!mOscilloFont[0]) {
        NSString *fontPath;
        //if (mScaleFactor<2) fontPath = [[NSBundle mainBundle] pathForResource:@"tracking10" ofType: @"fnt"];
        //else fontPath = [[NSBundle mainBundle] pathForResource:@"tracking14" ofType: @"fnt"];
        fontPath = [[NSBundle mainBundle] pathForResource:@"tracking10" ofType: @"fnt"];
        mOscilloFont[0] = new CFont([fontPath UTF8String]);
        fontPath = [[NSBundle mainBundle] pathForResource:@"tracking16" ofType: @"fnt"];
        mOscilloFont[1] = new CFont([fontPath UTF8String]);
        fontPath = [[NSBundle mainBundle] pathForResource:@"tracking24" ofType: @"fnt"];
        mOscilloFont[2] = new CFont([fontPath UTF8String]);
    }
    
    if (mOscilloFont[1] && voices_label)
        for (int i=0;i<m_genNumMidiVoicesChannels;i++) {
            if (mVoicesNamePiano[i]) {
                if (strcmp(mVoicesNamePiano[i]->mText,voices_label+i*32)) {
                    //not the same, reset string
                    delete mVoicesNamePiano[i];
                    mVoicesNamePiano[i]=NULL;
                }
            }
            if (!mVoicesNamePiano[i]) {
                mVoicesNamePiano[i]=new CGLString(voices_label+i*32, mOscilloFont[1],mScaleFactor);
            }
        }
    
    if (mOscilloFont[1] && (mOctavesIndex[0]==NULL)) {
        char str_tmp[3];
        for (int i=0;i<256/12;i++) {
            snprintf(str_tmp,3,"%d",i);
            mOctavesIndex[i]=new CGLString(str_tmp, mOscilloFont[1],mScaleFactor);
        }
    }
    
    ptsB=(LineVertexF*)malloc(sizeof(LineVertexF)*30*MAX_BARS);
    max_indices=30*MAX_BARS;
    
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    
    glVertexPointer(2, GL_FLOAT, sizeof(LineVertexF), &ptsB[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertexF), &ptsB[0].r);
    
    
    //////////////////////////////////////////////
    
    float note_posX[256];
    uint8_t note_posType[256];
    int midi_data_ofs=MIDIFX_LEN-MIDIFX_OFS-1;
    
    //white keys
    float visible_wkeys_range=(note_display_range*7.0/12.0);
    float width=(float)(ww)/visible_wkeys_range;
    float height=width*4;
    float x;
    float y;
    //black keys
    float widthB=width/2.0;
    float heightB=height*3/5;
    float xB;
    float yB;
    
    int border_size=2;
    int num_rows=hh/(height+16);
    if (m_genNumMidiVoicesChannels<num_rows) num_rows=m_genNumMidiVoicesChannels;
    
    //draw piano felt
    index=0;
    int crtp[2],cgtp[2],cbtp[2],cap[2];
    crtp[0]=60;crtp[1]=120;
    cgtp[0]=60;cgtp[1]=0;
    cbtp[0]=60;cbtp[1]=0;
    cap[0]=255;cap[1]=255;
    float wd=height/32.0;
    for (int j=0;j<num_rows;j++) {
        y=hh-(height+16)*(j+1)+height;
        ptsB[index++] = LineVertexF(0,y     ,crtp[0],cgtp[0],cbtp[0],cap[0]);
        ptsB[index++] = LineVertexF(ww,y    ,crtp[0],cgtp[0],cbtp[0],cap[0]);
        ptsB[index++] = LineVertexF(ww,y+wd ,crtp[1],cgtp[1],cbtp[1],cap[1]);
        
        ptsB[index++] = LineVertexF(0,y      ,crtp[0],cgtp[0],cbtp[0],cap[0]);
        ptsB[index++] = LineVertexF(ww,y+wd  ,crtp[1],cgtp[1],cbtp[1],cap[1]);
        ptsB[index++] = LineVertexF(0,y+wd   ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    }
    glDrawArrays(GL_TRIANGLES, 0, index);
    
    index=0;
    
    //prepapre key data & check all key status
    int wk_idx=0;//white key counter
    int bk_idx=0;//black key counter
    for (int note=0;note<256;note++) {
        bool whitekey=false;
        int note_idx=(note%12);
        if ( (note_idx==0)||(note_idx==2)||(note_idx==4)||(note_idx==5)||(note_idx==7)||(note_idx==9)||(note_idx==11)) whitekey=true;
        
        if (whitekey) {
            x=(float)(ww)*wk_idx/visible_wkeys_range-note_display_offset;
            note_posX[note]=x;
            note_posType[note]=0;
            wk_idx++;
        } else {
            x=(float)(ww)*(wk_idx-1)/visible_wkeys_range-note_display_offset;
            switch (bk_idx%5) {
                case 0: //C#
                    xB=(x+width*8.0f/12);
                    break;
                case 1://D#
                    xB=(x+width*10.0f/12);
                    break;
                case 2://F#
                    xB=(x+width*8.0f/12);
                    break;
                case 3://G#
                    xB=(x+width*9.0f/12);
                    break;
                case 4://A#
                    xB=(x+width*10.0f/12);
                    break;
            }
            note_posX[note]=xB;
            note_posType[note]=1;
            bk_idx++;
        }
        for (int i=0;i<SOUND_MAXMOD_CHANNELS;i++) {
            pianoroll_key_status[i][note]=(whitekey?PR_WHITE_KEY:0)|(note_idx<<4);
        }
    }
    for (int i=0; i<256; i++) { //for each channels
        if ((data_midifx_note[midi_data_ofs][i])&&
            (data_midifx_vol[midi_data_ofs][i]>=data_midifx_vol[midi_data_ofs+1][i]) ) {  //do we have a note ?
            unsigned int note=data_midifx_note[midi_data_ofs][i];
            unsigned int instr=data_midifx_instr[midi_data_ofs][i]&63;
            
            pianoroll_key_status[instr%num_rows][note]|=PR_KEY_PRESSED;
        }
    }
    
    //draw white keys
    for (int j=0;j<num_rows;j++) {
        y=hh-(height+16)*(j+1);
        
        for (int note=0;note<256;note++) {
            if (note_posType[note]==0) { //white key
                x=note_posX[note];
                if ((x+width>0)||(x<ww) ) {
                    if (index+INDICES_SIZE_KEYW>=max_indices) {
                        glDrawArrays(GL_TRIANGLES, 0, index);
                        index=0;
                    }
                    index=DrawKeyW(ptsB,index,x,y,width,height,2,220,220,220,255,0,note,j);
                }
            }
        }
    }
    
    //1st pass draw notes - white keys
    for (int i=0; i<256; i++) { //for each channels
        if ((data_midifx_note[midi_data_ofs][i])&&
            (data_midifx_vol[midi_data_ofs][i]>=data_midifx_vol[midi_data_ofs+1][i]) ) {  //do we have a note ?
            unsigned int note=data_midifx_note[midi_data_ofs][i];
            unsigned int instr=data_midifx_instr[midi_data_ofs][i]&63;
            unsigned int vol=data_midifx_vol[midi_data_ofs][i];
            unsigned int st=data_midifx_st[midi_data_ofs][i];
            
            if ( note_posType[note]==0 ) { //white key
                int subnote=data_midifx_subnote[midi_data_ofs][i];
                subnote=(subnote<8?subnote:subnote-8-7);
                
                int colidx=instr&63;
                int crt=((data_midifx_col[colidx&31]>>16)&0xFF);
                int cgt=((data_midifx_col[colidx&31]>>8)&0xFF);
                int cbt=(data_midifx_col[colidx&31]&0xFF);
                
                if (colidx&0x20) {
                    crt=(crt+255)/2;
                    cgt=(cgt+255)/2;
                    cbt=(cbt+255)/2;
                }
                
                if (vol&&(st&VOICE_ON)) {  //check volume & status => we have something
                    x=note_posX[note];
                    
                    if (note_posType[note]==0) { //white key
                        y=hh-(height+16)*((instr%num_rows)+1);
                        if (index+INDICES_SIZE_KEYW>=max_indices) {
                            glDrawArrays(GL_TRIANGLES, 0, index);
                            index=0;
                        }
                        if ( (x+width>0)||(x<ww)) {
                            index=DrawKeyW(ptsB,index,x,y,width,height,border_size,crt,cgt,cbt,255,subnote,note,instr%num_rows);
                        }
                    }
                }
            }
        }
    }
    
    //draw black keys
    for (int j=0;j<num_rows;j++) {
        y=hh-(height+16)*(j+1);
        for (int note=0;note<256;note++) {
            if (note_posType[note]) {
                yB=y+height-heightB;
                xB=note_posX[note];
                if (index+INDICES_SIZE_KEYB>=max_indices) {
                    glDrawArrays(GL_TRIANGLES, 0, index);
                    index=0;
                }
                if ( (xB+widthB>0)||(xB<ww)) index=DrawKeyB(ptsB,index,xB,yB,widthB,heightB,border_size,40,40,40,255,0,note,j);
            }
        }
    }
    
    //2nd pass draw notes - black keys
    for (int i=0; i<256; i++) { //for each channels
        if ((data_midifx_note[midi_data_ofs][i])&&
            (data_midifx_vol[midi_data_ofs][i]>=data_midifx_vol[midi_data_ofs+1][i]) ) {  //do we have a note ?
            unsigned int note=data_midifx_note[midi_data_ofs][i];
            
            uint8_t note_idx=note%12;
            if ((note_idx==1)||(note_idx==3)||(note_idx==6)||(note_idx==8)||(note_idx==10)) { //black key
                
                unsigned int instr=data_midifx_instr[midi_data_ofs][i]&63;
                unsigned int vol=data_midifx_vol[midi_data_ofs][i];
                unsigned int st=data_midifx_st[midi_data_ofs][i];
                
                int subnote=data_midifx_subnote[midi_data_ofs][i];
                subnote=(subnote<8?subnote:subnote-8-7);
                
                int colidx=instr&63;
                int crt=((data_midifx_col[colidx&31]>>16)&0xFF);
                int cgt=((data_midifx_col[colidx&31]>>8)&0xFF);
                int cbt=(data_midifx_col[colidx&31]&0xFF);
                
                if (colidx&0x20) {
                    crt=(crt+255)/2;
                    cgt=(cgt+255)/2;
                    cbt=(cbt+255)/2;
                }
                
                if (vol&&(st&VOICE_ON)) {  //check volume & status => we have something
                    //                    printf("B instr %d note %d note%%12 %d type %d x %f\n",instr,note,note%12,note_posType[note],x);
                    
                    x=note_posX[note];
                    
                    if (note_posType[note]==1) { //back key
                        y=hh-(height+16)*((instr%num_rows)+1)+height-heightB;
                        if (index+INDICES_SIZE_KEYB>=max_indices) {
                            glDrawArrays(GL_TRIANGLES, 0, index);
                            index=0;
                        }
                        if ( (x+widthB>0)||(x<ww))  index=DrawKeyB(ptsB,index,x,y,widthB,heightB,border_size,crt,cgt,cbt,255,subnote,note,instr%num_rows);
                    }
                }
            }
        }
    }
    
    
    
    memset(voices_posX,0,sizeof(voices_posX));
    //draw label small colored boxes
    if (voices_label&&settings[GLOB_FXPianoRollVoicesLabels].detail.mdz_switch.switch_value)
        for (int i=0;i<m_genNumMidiVoicesChannels;i++) {
            int j=i%num_rows;
            y=hh-(height+16)*(j+1)+height+1;
            
            if (mVoicesNamePiano[i]) {
                x=voices_posX[j]+16;
                
                voices_posX[j]+=16+mOscilloFont[1]->maxCharWidth*strlen(mVoicesNamePiano[i]->mText)/mScaleFactor;
                
                int colidx=i&63;
                int crt=((data_midifx_col[colidx&31]>>16)&0xFF);
                int cgt=((data_midifx_col[colidx&31]>>8)&0xFF);
                int cbt=(data_midifx_col[colidx&31]&0xFF);
                
                if (colidx&0x20) {
                    crt=(crt+255)/2;
                    cgt=(cgt+255)/2;
                    cbt=(cbt+255)/2;
                }
                
                y=hh-(height+16)*(j+1)+height+4;
                if (index+INDICES_SIZE_BOX>=max_indices) {
                    glDrawArrays(GL_TRIANGLES, 0, index);
                    index=0;
                }
                index=DrawBox(ptsB,index,x-10,y,8,8,1/*border_size*/,crt,cgt,cbt,255,0);
            }
        }
    
    glDrawArrays(GL_TRIANGLES, 0, index);
    
    
    
    //////////////////////////////////////////////
    
    //    glEnable(GL_BLEND);
    //    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisable(GL_BLEND);
    
    memset(voices_posX,0,sizeof(voices_posX));
    
    //draw label
    if (voices_label&&settings[GLOB_FXPianoRollVoicesLabels].detail.mdz_switch.switch_value) {
        for (int i=0;i<m_genNumMidiVoicesChannels;i++) {
            int j=i%num_rows;
            y=hh-(height+16)*(j+1)+height+4+1;
            
            if (mVoicesNamePiano[i]) {
                x=voices_posX[j]+16;
                glPushMatrix();
                glTranslatef(x,y, 0.0f);
                mVoicesNamePiano[i]->Render(255);
                voices_posX[j]+=16+mOscilloFont[1]->maxCharWidth*strlen(mVoicesNamePiano[i]->mText)/mScaleFactor;
                glPopMatrix();
            }
        }
    }
    
    if (mOctavesIndex[0]&&settings[GLOB_FXPianoRollOctavesLabels].detail.mdz_switch.switch_value) {
        for (int i=0;i<m_genNumMidiVoicesChannels;i++) {
            int j=i%num_rows;
            for (int o=0;o<256/12;o++) {
                x=o*width*7.0-note_display_offset;
                
                if (mOctavesIndex[o]) {
                    glPushMatrix();
                    
                    if (pianoroll_key_status[j][o*12]&PR_KEY_PRESSED) y=hh-(height+16)*(j+1)+16-12+height/24;
                    else y=hh-(height+16)*(j+1)+16-12+height/8;
                    
                    float lblwidth=strlen(mOctavesIndex[o]->mText)*mOscilloFont[1]->maxCharWidth/mScaleFactor;
                    x+=(width-lblwidth)/2;
                    
                    glTranslatef(x,y, 0.0f);
                    mOctavesIndex[o]->Render(32);
                    
                    glPopMatrix();
                }
            }
        }
    }
    free(ptsB);
}

void RenderUtils::DrawPianoRollSynthesiaFX(uint ww,uint hh,int horiz_vert,float note_display_range, float note_display_offset,int fx_len,int color_mode,float mScaleFactor,char *voices_label) {
    LineVertexF *ptsB;
    coordData *texcoords; /* Holds Float Info For 4 Sets Of Texture coordinates. */
    int crt,cgt,cbt,ca;
    int index;
    static bool first_call=true;
    float band_width;
    float line_width;
    float line_width_extra;
    uint8_t sparkPresent[256];
    static uint8_t sparkIntensity[256];
    float ofsy;
    
    
    if (first_call) {
        for (int i=0;i<SOUND_MAXVOICES_BUFFER_FX;i++) {
            mVoicesNamePiano[i]=NULL;
        }
        first_call=false;
        memset(mOctavesIndex,0,sizeof(mOctavesIndex));
        memset(sparkIntensity,0,sizeof(sparkIntensity));
        pianoroll_cpt=0;
    }
    
    if (!mOscilloFont[0]) {
        NSString *fontPath;
        //if (mScaleFactor<2) fontPath = [[NSBundle mainBundle] pathForResource:@"tracking10" ofType: @"fnt"];
        //else fontPath = [[NSBundle mainBundle] pathForResource:@"tracking14" ofType: @"fnt"];
        fontPath = [[NSBundle mainBundle] pathForResource:@"tracking10" ofType: @"fnt"];
        mOscilloFont[0] = new CFont([fontPath UTF8String]);
        fontPath = [[NSBundle mainBundle] pathForResource:@"tracking16" ofType: @"fnt"];
        mOscilloFont[1] = new CFont([fontPath UTF8String]);
        fontPath = [[NSBundle mainBundle] pathForResource:@"tracking24" ofType: @"fnt"];
        mOscilloFont[2] = new CFont([fontPath UTF8String]);
    }
    
    float note_posX[256];
    uint8_t note_posType[256];
    int midi_data_ofs=MIDIFX_LEN-MIDIFX_OFS-1;
    
    //white keys
    float visible_wkeys_range=(note_display_range*7.0/12.0);
    float width=(float)(ww)/visible_wkeys_range;
    float height=width*4;
    float x;
    float y;
    //black keys
    float widthB=round(width/2.0);
    float heightB=height*3/5;
    float xB;
    float yB;
    int voices_posX[SOUND_MAXVOICES_BUFFER_FX];
    
    ofsy=0;
    
    if (mOscilloFont[1] && voices_label)
        for (int i=0;i<m_genNumMidiVoicesChannels;i++) {
            if (mVoicesNamePiano[i]) {
                if (strcmp(mVoicesNamePiano[i]->mText,voices_label+i*32)) {
                    //not the same, reset string
                    delete mVoicesNamePiano[i];
                    mVoicesNamePiano[i]=NULL;
                }
            }
            if (!mVoicesNamePiano[i]) {
                mVoicesNamePiano[i]=new CGLString(voices_label+i*32, mOscilloFont[1],mScaleFactor);
            }
        }
    
    if (mOscilloFont[1] && (mOctavesIndex[0]==NULL)) {
        char str_tmp[3];
        for (int i=0;i<256/12;i++) {
            snprintf(str_tmp,3,"%d",i-1);
            mOctavesIndex[i]=new CGLString(str_tmp, mOscilloFont[1],mScaleFactor);
        }
    }
    
    if (voices_label&&settings[GLOB_FXPianoRollVoicesLabels].detail.mdz_switch.switch_value) {
        int labels_lines_needed=1;
        x=16;
        for (int i=0;i<m_genNumMidiVoicesChannels;i++) {
            int j=i;
            
            if (mVoicesNamePiano[i]) {
                
                x+=16+mOscilloFont[1]->maxCharWidth*strlen(mVoicesNamePiano[i]->mText)/mScaleFactor;
                if (x>ww) {
                    x=16+16+mOscilloFont[1]->maxCharWidth*strlen(mVoicesNamePiano[i]->mText)/mScaleFactor;
                    labels_lines_needed+=1;
                }
            }
        }
        if (labels_lines_needed>3) labels_lines_needed=3;
        ofsy=16*labels_lines_needed;
    }
    
    ptsB=(LineVertexF*)malloc(sizeof(LineVertexF)*30*MAX_BARS);
    max_indices=30*MAX_BARS;
    texcoords=(coordData*)malloc(sizeof(coordData)*256*6*8); //max 256 notes, 6pts/spark and max 8 sparks/notes
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    glVertexPointer(2, GL_FLOAT, sizeof(LineVertexF), &ptsB[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertexF), &ptsB[0].r);
    
    data_midifx_len=MIDIFX_OFS+1; //yoyofr: to review
    band_width=(float)(hh+0*hh/4)/data_midifx_len;
    line_width=1.0f*ww/note_display_range;
    line_width_extra=3;
    
    //////////////////////////////////////////////
    
    int data_bar2draw_count=0;
    
    for (int i=0; i<256; i++) { //for each channels
        int j=MIDIFX_LEN-data_midifx_len;
        while (j<MIDIFX_LEN) {  //while not having reach roof
            if (data_midifx_note[j][i]) {  //do we have a note ?
                unsigned int instr=data_midifx_instr[j][i];
                unsigned int vol=data_midifx_vol[j][i];
                unsigned int st=data_midifx_st[j][i];
                unsigned int note=data_midifx_note[j][i];
                int subnote=data_midifx_subnote[j][i];
                
                if (vol&&(st&VOICE_ON)) {  //check volume & status => we have something
                    data_bar2draw[data_bar2draw_count].startidx=j-(MIDIFX_LEN-data_midifx_len);
                    data_bar2draw[data_bar2draw_count].note=note;
                    data_bar2draw[data_bar2draw_count].subnote=subnote;
                    data_bar2draw[data_bar2draw_count].instr=instr;
                    data_bar2draw[data_bar2draw_count].size=0;
                    data_bar2draw[data_bar2draw_count].played=0;
                    data_bar2draw[data_bar2draw_count].frameidx=st>>8;
                    while ( (data_midifx_instr[j][i]==instr)&&
                           (data_midifx_note[j][i]==note)&&
                           (data_midifx_vol[j][i]<=vol)&&
                           (data_midifx_st[j][i]&VOICE_ON) ) {  //while same bar (instru & notes), increase size
                        data_bar2draw[data_bar2draw_count].size++;
                        //propagate lowest frame nb
                        data_midifx_st[j][i]=st;
                        if ((settings[GLOB_FXMIDIBarVibrato].detail.mdz_switch.switch_value==1)&&(data_midifx_subnote[j][i]!=subnote)) break;
                        //take most recent subnote if before playing bar
                        if (j<MIDIFX_LEN-MIDIFX_OFS) data_bar2draw[data_bar2draw_count].subnote=data_midifx_subnote[j][i];
                        if (j==(MIDIFX_LEN-MIDIFX_OFS-1)) data_bar2draw[data_bar2draw_count].played=1;
                        //update vol to latest encountered one, allow to retrigger if volume increases from last bar to new one and manage special case with vol = 2111121111
                        vol=data_midifx_vol[j][i];
                        j++;
                        
                        if (j==(MIDIFX_LEN-MIDIFX_OFS-1)) break;
                        
                        if (j==MIDIFX_LEN) break;
                    }
                    data_bar2draw_count++;
                    //j++;
                } else j++;
            } else j++;
            if (data_bar2draw_count==MAX_BARS) break;
        }
        if (data_bar2draw_count==MAX_BARS) break;
    }
    qsort(data_bar2draw,data_bar2draw_count,sizeof(t_data_bar2draw),qsort_CompareBar);
    
    int border_size=2;
    
    //prepapre key data & check all key status
    int wk_idx=0;//white key counter
    int bk_idx=0;//black key counter
    for (int note=0;note<256;note++) {
        bool whitekey=false;
        int note_idx=(note%12);
        if ( (note_idx==0)||(note_idx==2)||(note_idx==4)||(note_idx==5)||(note_idx==7)||(note_idx==9)||(note_idx==11)) whitekey=true;
        
        if (whitekey) {
            x=(float)(ww)*wk_idx/visible_wkeys_range-note_display_offset;
            note_posX[note]=x;
            note_posType[note]=0;
            wk_idx++;
        } else {
            x=(float)(ww)*(wk_idx-1)/visible_wkeys_range-note_display_offset;
            switch (bk_idx%5) {
                case 0: //C#
                    xB=(x+width*8.0f/12);
                    break;
                case 1://D#
                    xB=(x+width*10.0f/12);
                    break;
                case 2://F#
                    xB=(x+width*8.0f/12);
                    break;
                case 3://G#
                    xB=(x+width*9.0f/12);
                    break;
                case 4://A#
                    xB=(x+width*10.0f/12);
                    break;
            }
            note_posX[note]=xB;
            note_posType[note]=1;
            bk_idx++;
        }
        pianoroll_key_status[0][note]=(whitekey?PR_WHITE_KEY:0)|(note_idx<<4);
    }
    for (int i=0; i<256; i++) { //for each channels
        if ((data_midifx_note[midi_data_ofs][i])&&
            (data_midifx_vol[midi_data_ofs][i]>=data_midifx_vol[midi_data_ofs+1][i]) ) {  //do we have a note ?
            unsigned int note=data_midifx_note[midi_data_ofs][i];
            unsigned int instr=data_midifx_instr[midi_data_ofs][i]&63;
            
            pianoroll_key_status[0][note]|=PR_KEY_PRESSED;
        }
    }
    
    
    index=0;
    for (int i=0;i<data_bar2draw_count;i++) {
        int played=data_bar2draw[i].played;
        int note=data_bar2draw[i].note;
        
        int subnote=(data_bar2draw[i].subnote<8?data_bar2draw[i].subnote:data_bar2draw[i].subnote-8-7);
        
        int instr=data_bar2draw[i].instr;
        int colidx;
        if (color_mode==0) { //note
            colidx=(note%12);
        } else if (color_mode==1) { //instru
            colidx=(instr)&63;
        }
        
        if (data_bar2draw[i].size==0) continue;
        
        //printf("i:%d start:%d end:%d instr:%d note:%d played:%d\n",i,data_bar2draw[i].startidx,data_bar2draw[i].startidx+data_bar2draw[i].size,instr,note,played);
        
        
        crt=((data_midifx_col[colidx&31]>>16)&0xFF);
        cgt=((data_midifx_col[colidx&31]>>8)&0xFF);
        cbt=(data_midifx_col[colidx&31]&0xFF);
        
        if (colidx&0x20) {
            crt=(crt+255)/2;
            cgt=(cgt+255)/2;
            cbt=(cbt+255)/2;
        }
        
        if (played) {
            crt=(crt*3+255*3)/6;
            cgt=(cgt*3+255*3)/6;
            cbt=(cbt*3+255*3)/6;
            if (crt>255) crt=255;
            if (cgt>255) cgt=255;
            if (cbt>255) cbt=255;
            line_width_extra=2;
            ca=192;
        } else {
            line_width_extra=0;
            ca=192;
        }
        
        float posNote;
        float wd;
        posNote=note_posX[note];
        if (note_posType[note]==0) wd=width+line_width_extra*2;
        else wd=widthB+line_width_extra*2;
        
        //if (wd>=3) wd-=2;
        
        
        
        //posNote+=2;
        
        float posStart=(int)(data_bar2draw[i].startidx)*(hh-height-16)/data_midifx_len+height+0+ofsy+height/32;
        float posEnd=((int)(data_bar2draw[i].startidx)+(int)(data_bar2draw[i].size))*(hh-height-8)/data_midifx_len+height+0+ofsy+height/32;
        if ( (posNote-line_width_extra+wd>=0) && (posNote-line_width_extra<(int)ww)) {
            int border_size=(line_width>=8?2:1);
            
            index=DrawBox(ptsB,index,
                          posNote-line_width_extra, //x
                          posStart-line_width_extra, //y
                          wd, //w
                          posEnd-posStart+line_width_extra*2, //h
                          border_size,crt,cgt,cbt,255,0);
            
        }
    }
    //    printf("total: %d\n",index);
    glDrawArrays(GL_TRIANGLES, 0, index);
    
    
    //Draw spark fx
    glDisable(GL_DEPTH_TEST);           /* Disable Depth Testing     */
    glEnable(GL_TEXTURE_2D);            /* Enable 2D Texture Mapping */
    glBindTexture(GL_TEXTURE_2D, txt_pianoRoll[TXT_PIANOROLL_SPARK]);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    
    
    glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
    /* Enable Texture Coordinations Pointer */
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    memset(sparkPresent,0,sizeof(sparkPresent));
    index=0;
    if (settings[GLOB_FXPianoRollSpark].detail.mdz_switch.switch_value)
        for (int i=0; i<256; i++) { //for each channels
            if ((data_midifx_note[midi_data_ofs][i]/*||data_midifx_note[midi_data_ofs+1][i]*/)&&
                data_midifx_vol[midi_data_ofs][i] &&
                ( data_midifx_vol[midi_data_ofs][i]>=data_midifx_vol[midi_data_ofs+1][i]) ) {  //do we have a note ?
                unsigned int note=data_midifx_note[midi_data_ofs][i];
                if (!note) note=data_midifx_note[midi_data_ofs+1][i];
                
                //avoid rendering twice for same note
                if (sparkPresent[note]) continue;
                sparkPresent[note]=1;
                if (sparkIntensity[note]<128) sparkIntensity[note]+=8;
                
                unsigned int instr=data_midifx_instr[midi_data_ofs][i];
                int colidx=instr&63;
                int crt=((data_midifx_col[colidx&31]>>16)&0xFF);
                int cgt=((data_midifx_col[colidx&31]>>8)&0xFF);
                int cbt=(data_midifx_col[colidx&31]&0xFF);
                
                if (colidx&0x20) {
                    crt=(crt+255)/2;
                    cgt=(cgt+255)/2;
                    cbt=(cbt+255)/2;
                }
                
                crt=(crt*3+255*3)/6;
                cgt=(cgt*3+255*3)/6;
                cbt=(cbt*3+255*3)/6;
                
                //                crt=crt*1.5f+64;
                //                cgt=cgt*1.5f+64;
                //                cbt=cbt*1.5f+64;
                //
                if (crt>255) crt=255;
                if (cgt>255) cgt=255;
                if (cbt>255) cbt=255;
                
                line_width_extra=2;
                
                float posNote;
                float wd;
                
                posNote=note_posX[note];
                if (note_posType[note]==0) wd=width+line_width_extra*2;
                else wd=widthB+line_width_extra*2;
                
                posNote-=wd/2;
                wd=wd*2;
                
                for (int sp=0;sp<4;sp++) {
                    
                    texcoords[index+0].u=0.0f; texcoords[index+0].v=80.0/128;
                    texcoords[index+1].u=0.0f; texcoords[index+1].v=18.0/128;
                    texcoords[index+2].u=1.0f; texcoords[index+2].v=80.0/128;
                    
                    texcoords[index+3].u=0.0f; texcoords[index+3].v=18.0/128;
                    texcoords[index+4].u=1.0f; texcoords[index+4].v=80.0/128;
                    texcoords[index+5].u=1.0f; texcoords[index+5].v=18.0/128;
                    
                    
                    ptsB[index+0].x=posNote;ptsB[index+0].y=ofsy+0+height+height/32;
                    ptsB[index+1].x=posNote;ptsB[index+1].y=ofsy+0+height+height/32+wd/3;
                    ptsB[index+2].x=posNote+wd;ptsB[index+2].y=ofsy+0+height+height/32;
                    
                    ptsB[index+3].x=posNote;ptsB[index+3].y=ofsy+0+height+height/32+wd/3;
                    ptsB[index+4].x=posNote+wd;ptsB[index+4].y=ofsy+0+height+height/32;
                    ptsB[index+5].x=posNote+wd;ptsB[index+5].y=ofsy+0+height+height/32+wd/3;
                    
                    //apply some distortion
                    float wd_distX=wd/3.0;
                    float wd_distY=wd/9.0;
                    
                    float distorFactors[4][4][6]={
                        {   {+0.7,  3 ,+0.2,  5, -0.3, 11},
                            {+0.2,  3 ,+0.5,  5, -0.4, 5},
                            
                            {+0.5,  5 ,-0.1,  7, +0.4, 13},
                            {+0.3,  2 ,-0.5,  3, +0.2, 3}},
                        
                        {   {+0.5,  1 ,+0.2,  7, -0.3, 7},
                            {+0.3,  3 ,+0.5,  2, -0.4, 7},
                            
                            {-0.3,  7 ,-0.1,  9, +0.4, 5},
                            {-0.2,  5 ,-0.5,  5, +0.2, 11}},
                        
                        {   {-0.7,  2 ,+0.2,  11, -0.3, 9},
                            {+0.2,  5 ,+0.5,  13, +0.7, 5},
                            
                            {+0.6,  9 ,-0.1,  3, +0.4, 3},
                            {+0.4,  1 ,-0.5,  3, +0.2, 8}},
                        
                        {   {+0.8,  9 ,+0.2,  7, -0.3, 9},
                            {+0.4,  11 ,+0.5, 7, +0.4, 5},
                            
                            {-0.5,  3 ,+0.1,  4, -0.4, 11},
                            {-0.3,  5 ,+0.5,  5, +0.2, 3}}};
                    
                    ptsB[index+1].x+=wd_distX*(distorFactors[sp][0][0]*sin(pianoroll_cpt*distorFactors[sp][0][1]*3.14159/32)
                                               +distorFactors[sp][0][2]*sin(pianoroll_cpt*distorFactors[sp][0][3]*3.14159/32)
                                               +distorFactors[sp][0][4]*sin(pianoroll_cpt*distorFactors[sp][0][5]*3.14159/32));
                    
                    ptsB[index+3].x+=wd_distX*(distorFactors[sp][0][0]*sin(pianoroll_cpt*distorFactors[sp][0][1]*3.14159/32)
                                               +distorFactors[sp][0][2]*sin(pianoroll_cpt*distorFactors[sp][0][3]*3.14159/32)
                                               +distorFactors[sp][0][4]*sin(pianoroll_cpt*distorFactors[sp][0][5]*3.14159/32));
                    
                    ptsB[index+1].y+=wd_distY*(distorFactors[sp][1][0]*sin(pianoroll_cpt*distorFactors[sp][1][1]*3.14159/32)
                                               +distorFactors[sp][1][2]*sin(pianoroll_cpt*distorFactors[sp][1][3]*3.14159/32)
                                               +distorFactors[sp][1][4]*sin(pianoroll_cpt*distorFactors[sp][1][5]*3.14159/32));
                    
                    ptsB[index+3].y+=wd_distY*(distorFactors[sp][1][0]*sin(pianoroll_cpt*distorFactors[sp][1][1]*3.14159/32)
                                               +distorFactors[sp][1][2]*sin(pianoroll_cpt*distorFactors[sp][1][3]*3.14159/32)
                                               +distorFactors[sp][1][4]*sin(pianoroll_cpt*distorFactors[sp][1][5]*3.14159/32));
                    
                    ptsB[index+5].x+=wd_distX*(distorFactors[sp][2][0]*sin(pianoroll_cpt*distorFactors[sp][2][1]*3.14159/32)
                                               +distorFactors[sp][2][2]*sin(pianoroll_cpt*distorFactors[sp][2][3]*3.14159/32)
                                               +distorFactors[sp][2][4]*sin(pianoroll_cpt*distorFactors[sp][2][5]*3.14159/32));
                    
                    ptsB[index+5].y+=wd_distY*(distorFactors[sp][3][0]*sin(pianoroll_cpt*distorFactors[sp][3][1]*3.14159/32)
                                               +distorFactors[sp][3][2]*sin(pianoroll_cpt*distorFactors[sp][3][3]*3.14159/32)
                                               +distorFactors[sp][3][4]*sin(pianoroll_cpt*distorFactors[sp][3][5]*3.14159/32));
                    
                    if (sp&1) {
                        for (int ii=0;ii<6;ii++) {
                            texcoords[index+ii].u=1-texcoords[index+ii].u;
                        }
                        
                    }
                    for (int ii=0;ii<6;ii++) {
                        if (settings[GLOB_FXPianoRollSpark].detail.mdz_switch.switch_value==2) {
                            ptsB[index+ii].r=255;
                            ptsB[index+ii].g=255;
                            ptsB[index+ii].b=255;
                        } else {
                            ptsB[index+ii].r=crt;
                            ptsB[index+ii].g=cgt;
                            ptsB[index+ii].b=cbt;
                        }
                        ptsB[index+ii].a=sparkIntensity[note]/4;
                    }
                    index+=6;
                }
                
            }
        }
    glDrawArrays(GL_TRIANGLES, 0, index);
    
    //reset spark intensity if no note played
    for (int i=0;i<256;i++) {
        if (sparkPresent[i]==0) {
            if (sparkIntensity[i]>8) sparkIntensity[i]-=8;
            else sparkIntensity[i]=0;
        }
    }
    
    
    glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_TEXTURE_2D);            /* Enable 2D Texture Mapping */
    glEnableClientState(GL_COLOR_ARRAY);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    index=0;
    //draw piano felt
    int crtp[2],cgtp[2],cbtp[2],cap[2];
    crtp[0]=60;crtp[1]=120;
    cgtp[0]=60;cgtp[1]=0;
    cbtp[0]=60;cbtp[1]=0;
    cap[0]=255;cap[1]=255;
    float wd=height/32.0;
        y=ofsy+height;
        ptsB[index++] = LineVertexF(0,y     ,crtp[0],cgtp[0],cbtp[0],cap[0]);
        ptsB[index++] = LineVertexF(ww,y    ,crtp[0],cgtp[0],cbtp[0],cap[0]);
        ptsB[index++] = LineVertexF(ww,y+wd ,crtp[1],cgtp[1],cbtp[1],cap[1]);
        
        ptsB[index++] = LineVertexF(0,y      ,crtp[0],cgtp[0],cbtp[0],cap[0]);
        ptsB[index++] = LineVertexF(ww,y+wd  ,crtp[1],cgtp[1],cbtp[1],cap[1]);
        ptsB[index++] = LineVertexF(0,y+wd   ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    glDrawArrays(GL_TRIANGLES, 0, index);
    
    index=0;
    
    
    //draw white keys
    y=ofsy+0;
    for (int note=0;note<256;note++) {
        if ( note_posType[note]==0) { //white key
            x=note_posX[note];
            if (index+INDICES_SIZE_KEYW>=max_indices) {
                glDrawArrays(GL_TRIANGLES, 0, index);
                index=0;
            }
            if ((x+width>0)||(x<ww)) index=DrawKeyW(ptsB,index,x,y,width,height,border_size,220,220,220,255,0,note,0);
        }
    }
    
    //1st pass draw notes - white keys
    for (int i=0; i<256; i++) { //for each channels
        if ((data_midifx_note[midi_data_ofs][i]/*||data_midifx_note[midi_data_ofs+1][i]*/)&&
            data_midifx_vol[midi_data_ofs][i] &&
            (data_midifx_vol[midi_data_ofs][i]>=data_midifx_vol[midi_data_ofs+1][i]) ) {  //do we have a note ?
            unsigned int note=data_midifx_note[midi_data_ofs][i];
            if (!note) note=data_midifx_note[midi_data_ofs+1][i];
            
            if (note_posType[note]==0) { //white key
                
                unsigned int instr=data_midifx_instr[midi_data_ofs][i];
                unsigned int vol=data_midifx_vol[midi_data_ofs][i];
                unsigned int st=data_midifx_st[midi_data_ofs][i];
                
                int subnote=data_midifx_subnote[midi_data_ofs][i];
                subnote=(subnote<8?subnote:subnote-8-7);
                
                int colidx=instr&63;
                int crt=((data_midifx_col[colidx&31]>>16)&0xFF);
                int cgt=((data_midifx_col[colidx&31]>>8)&0xFF);
                int cbt=(data_midifx_col[colidx&31]&0xFF);
                
                if (colidx&0x20) {
                    crt=(crt+255)/2;
                    cgt=(cgt+255)/2;
                    cbt=(cbt+255)/2;
                }
                
                if (vol&&(st&VOICE_ON)) {  //check volume & status => we have something
                    x=note_posX[note];
                    
                    y=ofsy+0;
                    if (index+INDICES_SIZE_KEYW>=max_indices) {
                        glDrawArrays(GL_TRIANGLES, 0, index);
                        index=0;
                    }
                    if ( (x+width>0)||(x<ww) ) index=DrawKeyW(ptsB,index,x,y,width,height,border_size,crt,cgt,cbt,255,subnote,note,0);
                }
            }
        }
    }
    
    //draw black keys
    y=ofsy+0;
    for (int note=0;note<256;note++) {
        if (note_posType[note] ) { //black key
            yB=y+height-heightB;
            xB=note_posX[note];
            if (index+INDICES_SIZE_KEYB>=max_indices) {
                glDrawArrays(GL_TRIANGLES, 0, index);
                index=0;
            }
            if ( (xB+widthB>0)||(xB<ww) ) index=DrawKeyB(ptsB,index,xB,yB,widthB,heightB,border_size,40,40,40,255,0,note,0);
        }
    }
    
    //2nd pass draw notes - black keys
    for (int i=0; i<256; i++) { //for each channels
        if ((data_midifx_note[midi_data_ofs][i]/*||data_midifx_note[midi_data_ofs+1][i]*/)&&
            (data_midifx_vol[midi_data_ofs][i]>=data_midifx_vol[midi_data_ofs+1][i]) ) {  //do we have a note ?
            unsigned int note=data_midifx_note[midi_data_ofs][i];
            if (!note) note=data_midifx_note[midi_data_ofs+1][i];
            
            if (note_posType[note]) { //black key
                
                unsigned int instr=data_midifx_instr[midi_data_ofs][i];
                unsigned int vol=data_midifx_vol[midi_data_ofs][i];
                unsigned int st=data_midifx_st[midi_data_ofs][i];
                
                int subnote=data_midifx_subnote[midi_data_ofs][i];
                subnote=(subnote<8?subnote:subnote-8-7);
                
                int colidx=instr&63;
                int crt=((data_midifx_col[colidx&31]>>16)&0xFF);
                int cgt=((data_midifx_col[colidx&31]>>8)&0xFF);
                int cbt=(data_midifx_col[colidx&31]&0xFF);
                
                if (colidx&0x20) {
                    crt=(crt+255)/2;
                    cgt=(cgt+255)/2;
                    cbt=(cbt+255)/2;
                }
                
                if (vol&&(st&VOICE_ON)) {  //check volume & status => we have something
                    //                    printf("B instr %d note %d note%%12 %d type %d x %f\n",instr,note,note%12,note_posType[note],x);
                    x=note_posX[note];
                    y=ofsy+height-heightB+0;
                    if (index+INDICES_SIZE_KEYB>=max_indices) {
                        glDrawArrays(GL_TRIANGLES, 0, index);
                        index=0;
                    }
                    if ( (x+widthB>0)||(x<ww) )  index=DrawKeyB(ptsB,index,x,y,widthB,heightB,border_size,crt,cgt,cbt,255,subnote,note,0);
                }
            }
        }
    }
    
    
    
    memset(voices_posX,0,sizeof(voices_posX));
    
    //draw label small colored boxes
    if (voices_label&&settings[GLOB_FXPianoRollVoicesLabels].detail.mdz_switch.switch_value)
        x=16;
    y=ofsy-16+4;
    for (int i=0;i<m_genNumMidiVoicesChannels;i++) {
        int j=i;
        
        if (mVoicesNamePiano[i]) {
            float widthx=16+mOscilloFont[1]->maxCharWidth*strlen(mVoicesNamePiano[i]->mText)/mScaleFactor;
            
            int colidx=i&63;
            int crt=((data_midifx_col[colidx&31]>>16)&0xFF);
            int cgt=((data_midifx_col[colidx&31]>>8)&0xFF);
            int cbt=(data_midifx_col[colidx&31]&0xFF);
            
            if (colidx&0x20) {
                crt=(crt+255)/2;
                cgt=(cgt+255)/2;
                cbt=(cbt+255)/2;
            }
            
            if (x+widthx>ww) {
                x=16;
                y-=16;
            }
            if (index+INDICES_SIZE_BOX>=max_indices) {
                glDrawArrays(GL_TRIANGLES, 0, index);
                index=0;
            }
            index=DrawBox(ptsB,index,x-10,y,8,8,1/*border_size*/,crt,cgt,cbt,255,0);
            
            x+=widthx;
        }
    }
    
    glDrawArrays(GL_TRIANGLES, 0, index);
    
    
    //Draw light fx
    glDisable(GL_DEPTH_TEST);           /* Disable Depth Testing     */
    glEnable(GL_TEXTURE_2D);            /* Enable 2D Texture Mapping */
    glBindTexture(GL_TEXTURE_2D, txt_pianoRoll[TXT_PIANOROLL_LIGHT]);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    
    
    glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
    /* Enable Texture Coordinations Pointer */
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    memset(sparkPresent,0,sizeof(sparkPresent));
    index=0;
    if (settings[GLOB_FXPianoRollSpark].detail.mdz_switch.switch_value)
        for (int i=0; i<256; i++) { //for each channels
            if ((data_midifx_note[midi_data_ofs][i]/*||data_midifx_note[midi_data_ofs+1][i]*/)&&
                (data_midifx_vol[midi_data_ofs][i]>=data_midifx_vol[midi_data_ofs+1][i]) ) {  //do we have a note ?
                unsigned int note=data_midifx_note[midi_data_ofs][i];
                if (!note) note=data_midifx_note[midi_data_ofs+1][i];
                
                //avoid rendering twice for same note
                if (sparkPresent[note]) continue;
                sparkPresent[note]=1;
                
                unsigned int instr=data_midifx_instr[midi_data_ofs][i];
                int colidx=instr&63;
                int crt=((data_midifx_col[colidx&31]>>16)&0xFF);
                int cgt=((data_midifx_col[colidx&31]>>8)&0xFF);
                int cbt=(data_midifx_col[colidx&31]&0xFF);
                
                if (colidx&0x20) {
                    crt=(crt+255)/2;
                    cgt=(cgt+255)/2;
                    cbt=(cbt+255)/2;
                }
                
                crt=(crt*3+255*3)/6;
                cgt=(cgt*3+255*3)/6;
                cbt=(cbt*3+255*3)/6;
                
                if (crt>255) crt=255;
                if (cgt>255) cgt=255;
                if (cbt>255) cbt=255;
                
                line_width_extra=2;
                
                float posNote;
                float wd;
                posNote=note_posX[note];
                if (note_posType[note]==0) wd=width+line_width_extra*2;
                else wd=widthB+line_width_extra*2;
                
                //if (wd>=3) wd-=2;
                //posNote+=2;
                posNote-=wd*1.0;
                wd=wd*3;
                
                for (int sp=0;sp<4;sp++) {
                    
                    texcoords[index+0].u=0.0f; texcoords[index+0].v=128.0/128;
                    texcoords[index+1].u=0.0f; texcoords[index+1].v=0.0/128;
                    texcoords[index+2].u=1.0f; texcoords[index+2].v=128.0/128;
                    
                    texcoords[index+3].u=0.0f; texcoords[index+3].v=0.0/128;
                    texcoords[index+4].u=1.0f; texcoords[index+4].v=128.0/128;
                    texcoords[index+5].u=1.0f; texcoords[index+5].v=0.0/128;
                    
                    
                    ptsB[index+0].x=posNote;ptsB[index+0].y=ofsy+0+height-wd/2+height/32+height/16;
                    ptsB[index+1].x=posNote;ptsB[index+1].y=ofsy+0+height+wd/2+height/32+height/16;
                    ptsB[index+2].x=posNote+wd;ptsB[index+2].y=ofsy+0+height-wd/2+height/32+height/16;
                    
                    ptsB[index+3].x=posNote;ptsB[index+3].y=ofsy+0+height+wd/2+height/32+height/16;
                    ptsB[index+4].x=posNote+wd;ptsB[index+4].y=ofsy+0+height-wd/2+height/32+height/16;
                    ptsB[index+5].x=posNote+wd;ptsB[index+5].y=ofsy+0+height+wd/2+height/32+height/16;
                    
                    //apply some distortion
                    float wd_distX=wd/3.0;
                    float wd_distY=wd/3.0;
                    
                    float distorFactors[4][4][6]={
                        {   {+0.7,  3 ,+0.2,  5, -0.3, 11},
                            {+0.2,  3 ,+0.5,  5, -0.4, 5},
                            
                            {+0.5,  5 ,-0.1,  7, +0.4, 13},
                            {+0.3,  2 ,-0.5,  3, +0.2, 3}},
                        
                        {   {+0.5,  1 ,+0.2,  7, -0.3, 7},
                            {+0.3,  3 ,+0.5,  2, -0.4, 7},
                            
                            {-0.3,  7 ,-0.1,  9, +0.4, 5},
                            {-0.2,  5 ,-0.5,  5, +0.2, 11}},
                        
                        {   {-0.7,  2 ,+0.2,  11, -0.3, 9},
                            {+0.2,  5 ,+0.5,  13, +0.7, 5},
                            
                            {+0.6,  9 ,-0.1,  3, +0.4, 3},
                            {+0.4,  1 ,-0.5,  3, +0.2, 8}},
                        
                        {   {+0.8,  9 ,+0.2,  7, -0.3, 9},
                            {+0.4,  11 ,+0.5, 7, +0.4, 5},
                            
                            {-0.5,  3 ,+0.1,  4, -0.4, 11},
                            {-0.3,  5 ,+0.5,  5, +0.2, 3}}};
                    
                    ptsB[index+1].x+=wd_distX*(distorFactors[sp][0][0]*sin(pianoroll_cpt*distorFactors[sp][0][1]*3.14159/32)
                                               +distorFactors[sp][0][2]*sin(pianoroll_cpt*distorFactors[sp][0][3]*3.14159/32)
                                               +distorFactors[sp][0][4]*sin(pianoroll_cpt*distorFactors[sp][0][5]*3.14159/32));
                    
                    ptsB[index+3].x+=wd_distX*(distorFactors[sp][0][0]*sin(pianoroll_cpt*distorFactors[sp][0][1]*3.14159/32)
                                               +distorFactors[sp][0][2]*sin(pianoroll_cpt*distorFactors[sp][0][3]*3.14159/32)
                                               +distorFactors[sp][0][4]*sin(pianoroll_cpt*distorFactors[sp][0][5]*3.14159/32));
                    
                    ptsB[index+1].y+=wd_distY*(distorFactors[sp][1][0]*sin(pianoroll_cpt*distorFactors[sp][1][1]*3.14159/32)
                                               +distorFactors[sp][1][2]*sin(pianoroll_cpt*distorFactors[sp][1][3]*3.14159/32)
                                               +distorFactors[sp][1][4]*sin(pianoroll_cpt*distorFactors[sp][1][5]*3.14159/32));
                    
                    ptsB[index+3].y+=wd_distY*(distorFactors[sp][1][0]*sin(pianoroll_cpt*distorFactors[sp][1][1]*3.14159/32)
                                               +distorFactors[sp][1][2]*sin(pianoroll_cpt*distorFactors[sp][1][3]*3.14159/32)
                                               +distorFactors[sp][1][4]*sin(pianoroll_cpt*distorFactors[sp][1][5]*3.14159/32));
                    
                    ptsB[index+5].x+=wd_distX*(distorFactors[sp][2][0]*sin(pianoroll_cpt*distorFactors[sp][2][1]*3.14159/32)
                                               +distorFactors[sp][2][2]*sin(pianoroll_cpt*distorFactors[sp][2][3]*3.14159/32)
                                               +distorFactors[sp][2][4]*sin(pianoroll_cpt*distorFactors[sp][2][5]*3.14159/32));
                    
                    ptsB[index+5].y+=wd_distY*(distorFactors[sp][3][0]*sin(pianoroll_cpt*distorFactors[sp][3][1]*3.14159/32)
                                               +distorFactors[sp][3][2]*sin(pianoroll_cpt*distorFactors[sp][3][3]*3.14159/32)
                                               +distorFactors[sp][3][4]*sin(pianoroll_cpt*distorFactors[sp][3][5]*3.14159/32));
                    
                    for (int ii=0;ii<6;ii++) {
                        if (settings[GLOB_FXPianoRollSpark].detail.mdz_switch.switch_value==2) {
                            ptsB[index+ii].r=255;
                            ptsB[index+ii].g=255;
                            ptsB[index+ii].b=255;
                        } else {
                            ptsB[index+ii].r=crt;
                            ptsB[index+ii].g=cgt;
                            ptsB[index+ii].b=cbt;
                        }
                        ptsB[index+ii].a=sparkIntensity[note]/4;
                    }
                    index+=6;
                }
                
            }
        }
    glDrawArrays(GL_TRIANGLES, 0, index);
    
    glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_TEXTURE_2D);            /* Enable 2D Texture Mapping */
    glEnableClientState(GL_COLOR_ARRAY);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    index=0;
    
    
    //////////////////////////////////////////////
    
    //    glEnable(GL_BLEND);
    //    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisable(GL_BLEND);
    
    memset(voices_posX,0,sizeof(voices_posX));
    
    //draw label
    if (voices_label&&settings[GLOB_FXPianoRollVoicesLabels].detail.mdz_switch.switch_value) {
        y=ofsy-16;//height+16;
        x=16;
        for (int i=0;i<m_genNumMidiVoicesChannels;i++) {
            int j=i;
            
            if (mVoicesNamePiano[i]) {
                float widthx=16+mOscilloFont[1]->maxCharWidth*strlen(mVoicesNamePiano[i]->mText)/mScaleFactor;
                if (x+widthx>ww) {
                    x=16;
                    y-=16;
                }
                
                glPushMatrix();
                glTranslatef(x,y+3, 0.0f);
                mVoicesNamePiano[i]->Render(255);
                x+=widthx;
                glPopMatrix();
            }
        }
    }
    
    if (mOctavesIndex[0]&&settings[GLOB_FXPianoRollOctavesLabels].detail.mdz_switch.switch_value) {
            for (int o=0;o<256/12;o++) {
                x=o*width*7.0-note_display_offset;
                
                if (mOctavesIndex[o]) {
                    
                    if (pianoroll_key_status[0][o*12]&PR_KEY_PRESSED) y=ofsy+0+3+height/24;
                    else y=ofsy+0+3+height/8;
                    
                    glPushMatrix();
                    
                    float lblwidth=strlen(mOctavesIndex[o]->mText)*mOscilloFont[1]->maxCharWidth/mScaleFactor;
                    x+=(width-lblwidth)/2;
                    
                    glTranslatef(x,y, 0.0f);
                    mOctavesIndex[o]->Render(32);
                    
                    glPopMatrix();
                }
            }
    }
    free(ptsB);
    free(texcoords);
}
