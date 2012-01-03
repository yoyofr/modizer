/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2004 Masanao Izumo <iz@onicos.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA

    xtrace_i.c - Trace window drawing for X11 based systems
        based on code by Yoshishige Arai <ryo2@on.rim.or.jp>
        modified by Yair Kalvariski <cesium2@gmail.com>
*/

#include "x_trace.h"
#include <stdlib.h>
#include "timer.h"

enum {
  CL_C,		/* column 0 = channel */
  CL_VE,	/* column 1 = velocity */
  CL_VO,	/* column 2 = volume */
  CL_EX,	/* column 3 = expression */
  CL_PR,	/* column 4 = program */
  CL_PA,	/* column 5 = panning */
  CL_PI,	/* column 6 = pitch bend */
  CL_IN,	/* column 7 = instrument name */
  KEYBOARD,
  TCOLUMN,
  CL_BA = 6,	/* column 6 = bank */
  CL_RE,	/* column 7 = reverb */
  CL_CH,	/* column 8 = chorus */
  KEYBOARD2,
  T2COLUMN
};

typedef struct {
  int y;
  int l;
} KeyL;

typedef struct {
  KeyL k[3];
  int xofs;
  Pixel col;
} ThreeL;

typedef struct {
  const int		col;	/* column number */
  const char		**cap;	/* caption strings array */
  const int		*w;  	/* column width array */
  const int		*ofs;	/* column offset array */
} Tplane;

typedef struct {
  int is_drum[MAX_TRACE_CHANNELS];
  int8 c_flags[MAX_TRACE_CHANNELS];
  int8 v_flags[MAX_TRACE_CHANNELS];
  int16 cnote[MAX_TRACE_CHANNELS];
  int16 ctotal[MAX_TRACE_CHANNELS];
  int16 cvel[MAX_TRACE_CHANNELS];
  int16 reverb[MAX_TRACE_CHANNELS];  
  Channel channel[MAX_TRACE_CHANNELS];
  char *inst_name[MAX_TRACE_CHANNELS];
  int pitch, poffset;
  unsigned int tempo, timeratio;
  int xaw_i_voices, last_voice;
  int tempo_width, pitch_width, voices_num_width;
  int plane, multi_part, visible_channels;
  Pixel barcol[MAX_TRACE_CHANNELS];
  Pixmap layer[2], gradient_pixmap[T2COLUMN];
  GC gcs, gct, gc_xcopy, gradient_gc[T2COLUMN];
  int init;
  Window trace;
  Boolean g_cursor_is_in;
  Display *disp;
  tconfig *cfg;
  char *title;
} PanelInfo;

#define gcs Panel->gcs
#define gct Panel->gct
#define gc_xcopy Panel->gc_xcopy
#define gradient_gc Panel->gradient_gc
#define gradient_pixmap Panel->gradient_pixmap
#define plane Panel->plane
#define layer Panel->layer

static PanelInfo *Panel;
static short titlefont_ascent, tracefont_ascent;
static ThreeL *keyG;

static const char *caption[TCOLUMN] =
{"ch", "  vel", " vol", "expr", "prog", "pan", "pit", " instrument",
 "          keyboard"};
static const char *caption2[T2COLUMN] =
{"ch", "  vel", " vol", "expr", "prog", "pan", "bnk", "reverb", "chorus",
 "          keyboard"};

static const int BARH_SPACE[TCOLUMN] = {22, 60, 40, 36, 36, 36, 30, 106, 304};
#define BARH_OFS0	(TRACEH_OFS)
#define BARH_OFS1	(BARH_OFS0+22)
#define BARH_OFS2	(BARH_OFS1+60)
#define BARH_OFS3	(BARH_OFS2+40)
#define BARH_OFS4	(BARH_OFS3+36)
#define BARH_OFS5	(BARH_OFS4+36)
#define BARH_OFS6	(BARH_OFS5+36)
#define BARH_OFS7	(BARH_OFS6+30)
#define BARH_OFS8	(BARH_OFS7+106)
static const int bar0ofs[] = {BARH_OFS0, BARH_OFS1, BARH_OFS2, BARH_OFS3,
  BARH_OFS4, BARH_OFS5, BARH_OFS6, BARH_OFS7, BARH_OFS8};

static const int BARH2_SPACE[T2COLUMN] = {22, 60, 40, 36, 36, 36,
                                          30, 53, 53, 304};
#define BARH2_OFS0	(TRACEH_OFS)
#define BARH2_OFS1	(BARH2_OFS0+22)
#define BARH2_OFS2	(BARH2_OFS1+60)
#define BARH2_OFS3	(BARH2_OFS2+40)
#define BARH2_OFS4	(BARH2_OFS3+36)
#define BARH2_OFS5	(BARH2_OFS4+36)
#define BARH2_OFS6	(BARH2_OFS5+36)
#define BARH2_OFS7	(BARH2_OFS6+30)
#define BARH2_OFS8	(BARH2_OFS7+53)
#define BARH2_OFS9	(BARH2_OFS8+53)
static const int bar1ofs[] = {BARH2_OFS0, BARH2_OFS1, BARH2_OFS2, BARH2_OFS3,
  BARH2_OFS4, BARH2_OFS5, BARH2_OFS6, BARH2_OFS7, BARH2_OFS8, BARH2_OFS9};

static const Tplane pl[] = {
  {TCOLUMN, caption, BARH_SPACE, bar0ofs},
  {T2COLUMN, caption2, BARH2_SPACE, bar1ofs},
};

#define KEY_NUM 111
#define BARSCALE2 0.31111	/* velocity scale   (60-4)/180 */
#define BARSCALE3 0.28125	/* volume scale     (40-4)/128 */
#define BARSCALE4 0.25		/* expression scale (36-4)/128 */
#define BARSCALE5 0.385827	/* expression scale (53-4)/128 */

#define FLAG_NOTE_OFF	1
#define FLAG_NOTE_ON	2
#define FLAG_BANK	1
#define FLAG_PROG	2
#define FLAG_PROG_ON	4
#define FLAG_PAN	8
#define FLAG_SUST	16
#define FLAG_BENDT	32

#define VOICES_NUM_OFS	6
#define VOICENUM_WIDTH	56
#define TEMPO_WIDTH	56
#define TEMPO_SPACE	6
#define PITCH_WIDTH	106
#define PITCH_SPACE	6
#define TTITLE_OFS	120

#define VISIBLE_CHANNELS Panel->visible_channels
#define VISLOW Panel->multi_part
#define XAWLIMIT(ch) ((VISLOW <= (ch)) && ((ch) < (VISLOW+VISIBLE_CHANNELS)))

#define disp		Panel->disp

#define boxcolor	Panel->cfg->box_color
#define capcolor	Panel->cfg->caption_color
#define chocolor	Panel->cfg->cho_color
#define expcolor	Panel->cfg->expr_color
#define pancolor	Panel->cfg->pan_color
#define playcolor	Panel->cfg->play_color
#define revcolor	Panel->cfg->rev_color
#define rimcolor	Panel->cfg->rim_color
#define suscolor	Panel->cfg->sus_color
#define textcolor	Panel->cfg->common_fgcolor
#define textbgcolor	Panel->cfg->text_bgcolor
#define tracecolor	Panel->cfg->trace_bgcolor
#define volcolor	Panel->cfg->volume_color

#define gradient_bar	Panel->cfg->gradient_bar
#define black		Panel->cfg->black_key_color
#define white		Panel->cfg->white_key_color

#define trace_height_raw	Panel->cfg->trace_height
#define trace_height_nf		(Panel->cfg->trace_height - TRACE_FOOT)
#define trace_width		Panel->cfg->trace_width

#define trace_font	Panel->cfg->trace_font
#define ttitle_font	Panel->cfg->ttitle_font

#define UNTITLED_STR	Panel->cfg->untitled
/* Privates */

static int bitcount(int);
static void ctl_channel_note(int, int, int);
static void drawBar(int, int, int, int, Pixel);
static void drawKeyboardAll(Drawable, GC);
static void draw1Note(int, int, int);
static void drawProg(int, int, Boolean);
static void drawPan(int, int, Boolean);
static void draw1Chan(int, int, char);
static void drawVol(int, int);
static void drawExp(int, int);
static void drawPitch(int, int);
static void drawInstname(int, char *);
static void drawDrumPart(int, int);
static void drawBank(int, int);
static void drawReverb(int, int);
static void drawChorus(int, int);
static void drawVoices(void);
static void drawTitle(char *);
static void drawTempo(void);
static void drawOverallPitch(int, int);
static void drawMute(int, int);
static int getdisplayinfo(RGBInfo *);
static int sftcount(int *);

static int bitcount(int d) {
  int rt = 0;

  while ((d & 0x01) == 0x01) {
    d >>= 1;
    rt++;
  }
  return rt;
}

static int sftcount(int *mask) {
  int rt = 0;

  while ((*mask & 0x01) == 0) {
    (*mask) >>= 1;
    rt++;
  }
  return rt;
}

static int getdisplayinfo(RGBInfo *rgb) {
  XWindowAttributes xvi;
  XGetWindowAttributes(disp, Panel->trace, &xvi);
  if (xvi.depth >= 16) {
    rgb->Red_depth = (xvi.visual)->red_mask;
    rgb->Green_depth = (xvi.visual)->green_mask;
    rgb->Blue_depth = (xvi.visual)->blue_mask;
    rgb->Red_sft = sftcount(&(rgb->Red_depth));
    rgb->Green_sft = sftcount(&(rgb->Green_depth));
    rgb->Blue_sft = sftcount(&(rgb->Blue_depth));
    rgb->Red_depth = bitcount(rgb->Red_depth);
    rgb->Green_depth = bitcount(rgb->Green_depth);
    rgb->Blue_depth = bitcount(rgb->Blue_depth);
  }
  return xvi.depth;
}

static void drawBar(int ch, int len, int xofs, int column, Pixel color) {
  static Pixel column1color0;
  static XColor x_boxcolor;
  static XGCValues gv;
  static RGBInfo rgb;
  static int gradient_set[T2COLUMN], depth;
  int col, i, screen;
  XColor x_color;

  ch -= VISLOW;
  screen = DefaultScreen(disp);
  if (!Panel->init) {
    for(i=0;i<T2COLUMN;i++) gradient_set[i] = 0;
    depth = getdisplayinfo(&rgb);
    if ((16 <= depth) && (gradient_bar)) {
      x_boxcolor.pixel = boxcolor;
      XQueryColor(disp, DefaultColormap(disp, 0), &x_boxcolor);
      gv.fill_style = FillTiled;
      gv.fill_rule = WindingRule;
    }
    Panel->init = 0;
  }
  if ((16 <= depth) && (gradient_bar)) {
    if (column < T2COLUMN) {
      col = column;
      if (column == 1) {
        if (gradient_set[0] == 0) {
          column1color0 = color;
          col = 0;
        }
        else if ((gradient_set[1] == 0) && (column1color0 != color)) {
          col = 1;
        }
        else {
          if (column1color0 == color) col = 0;
          else col = 1;
        }
      }
      if (gradient_set[col] == 0) {
        unsigned long pxl;
        int r, g, b;

        gradient_pixmap[col] = XCreatePixmap(disp, Panel->trace,
                      BARH2_SPACE[column], 1, DefaultDepth(disp, screen));
        x_color.pixel = color;
        XQueryColor(disp, DefaultColormap(disp, 0), &x_color);
        for (i=0;i<BARH2_SPACE[column];i++) {
          r = x_boxcolor.red +
            (x_color.red - x_boxcolor.red) * i / BARH2_SPACE[column];
          g = x_boxcolor.green +
            (x_color.green - x_boxcolor.green) * i / BARH2_SPACE[column];
          b = x_boxcolor.blue + 
            (x_color.blue - x_boxcolor.blue) * i / BARH2_SPACE[column];
          if (r<0) r = 0;
          if (g<0) g = 0;
          if (b<0) b = 0;
          r >>= 8;
          g >>= 8;
          b >>= 8;
          if (r>255) r = 255;
          if (g>255) g = 255;
          if (b>255) b = 255;
          pxl  = (r >> (8-rgb.Red_depth)) << rgb.Red_sft;
          pxl |= (g >> (8-rgb.Green_depth)) << rgb.Green_sft;
          pxl |= (b >> (8-rgb.Blue_depth)) << rgb.Blue_sft;
          XSetForeground(disp, gct, pxl);
          XDrawPoint(disp, gradient_pixmap[col], gct, i, 0);
        }
        gv.tile = gradient_pixmap[col];
        gradient_gc[col] = XCreateGC(disp, Panel->trace,
                                     GCFillStyle | GCFillRule | GCTile, &gv);
        gradient_set[col] = 1;
      }
      XSetForeground(disp, gct, boxcolor);
      XFillRectangle(disp, Panel->trace, gct,
                     xofs+len+2, CHANNEL_HEIGHT(ch)+2,
                     pl[plane].w[column] - len -4, BAR_HEIGHT);
      gv.ts_x_origin = xofs + 2 - BARH2_SPACE[column] + len;
      XChangeGC(disp, gradient_gc[col], GCTileStipXOrigin, &gv);
      XFillRectangle(disp, Panel->trace, gradient_gc[col],
                     xofs+2, CHANNEL_HEIGHT(ch)+2,
                     len, BAR_HEIGHT);
    }
  }
  else {
    XSetForeground(disp, gct, boxcolor);
    XFillRectangle(disp, Panel->trace, gct,
                   xofs+len+2, CHANNEL_HEIGHT(ch)+2,
                   pl[plane].w[column] -len - 4, BAR_HEIGHT);
    XSetForeground(disp, gct, color);
    XFillRectangle(disp, Panel->trace, gct,
                   xofs+2, CHANNEL_HEIGHT(ch)+2,
                   len, BAR_HEIGHT);
  }
}

static void drawProg(int ch, int val, Boolean do_clean) {
  char s[4];

  ch -= VISLOW;
  if (do_clean == True) {
    XSetForeground(disp, gct, boxcolor);
    XFillRectangle(disp,Panel->trace,gct,
                   pl[plane].ofs[CL_PR]+2,CHANNEL_HEIGHT(ch)+2,
                   pl[plane].w[CL_PR]-4,BAR_HEIGHT);
  }
  XSetForeground(disp, gct, textcolor);
  sprintf(s, "%3d", val);
  XmbDrawString(disp, Panel->trace, trace_font, gct,
                pl[plane].ofs[CL_PR]+5, CHANNEL_HEIGHT(ch)+16, s, 3);
}

static void drawPan(int ch, int val, Boolean setcolor) {
  int ap, bp;
  int x;
  XPoint pp[3];

  if (val < 0) return;

  ch -= VISLOW;
  if (setcolor == True) {
    XSetForeground(disp, gct, boxcolor);
    XFillRectangle(disp, Panel->trace, gct,
                   pl[plane].ofs[CL_PA]+2, CHANNEL_HEIGHT(ch)+2,
                   pl[plane].w[CL_PA]-4, BAR_HEIGHT);
    XSetForeground(disp, gct, pancolor);
  }
  x = pl[plane].ofs[CL_PA] + 3;
  ap = 31 * val/127;
  bp = 31 - ap - 1;
  pp[0].x = ap + x; pp[0].y = 12 + BAR_SPACE*(ch+1);
  pp[1].x = bp + x; pp[1].y = 8 + BAR_SPACE*(ch+1);
  pp[2].x = bp + x; pp[2].y = 16 + BAR_SPACE*(ch+1);
  XFillPolygon(disp, Panel->trace, gct, pp, 3,
               (int)Nonconvex, (int)CoordModeOrigin);
}

static void draw1Chan(int ch, int val, char cmd) {
  if ((cmd == '*') || (cmd == '&'))
    drawBar(ch, (int)(val*BARSCALE2), pl[plane].ofs[CL_VE],
             CL_VE, Panel->barcol[ch]);
}

static void drawVol(int ch, int val) {
  drawBar(ch, (int)(val*BARSCALE3), pl[plane].ofs[CL_VO], CL_VO, volcolor);
}

static void drawExp(int ch, int val) {
  drawBar(ch, (int)(val*BARSCALE4), pl[plane].ofs[CL_EX], CL_EX, expcolor);
}

static void drawReverb(int ch, int val) {
  drawBar(ch, (int)(val*BARSCALE5), pl[plane].ofs[CL_RE], CL_RE, revcolor);
}

static void drawChorus(int ch, int val) {
  drawBar(ch, (int)(val*BARSCALE5), pl[plane].ofs[CL_CH], CL_CH, chocolor);
}

static void drawPitch(int ch, int val) {
  char s[3];

  ch -= VISLOW;
  XSetForeground(disp, gct, boxcolor);
  XFillRectangle(disp,Panel->trace,gct,
                 pl[plane].ofs[CL_PI]+2,CHANNEL_HEIGHT(ch)+2,
                 pl[plane].w[CL_PI] -4,BAR_HEIGHT);
  XSetForeground(disp, gct, Panel->barcol[9]);
  if (val != 0) {
    if (val < 0) {
      sprintf(s, "=");
    } else {
      if (val == 0x2000) sprintf(s, "*");
      else if (val > 0x3000) sprintf(s, ">>");
      else if (val > 0x2000) sprintf(s, ">");
      else if (val > 0x1000) sprintf(s, "<");
      else sprintf(s, "<<");
    }
    XmbDrawString(disp, Panel->trace, trace_font, gct,
                pl[plane].ofs[CL_PI]+4, CHANNEL_HEIGHT(ch)+16, s, strlen(s));
  }
}

static void drawInstname(int ch, char *name) {
  int len;

  if (plane != 0) return;
  ch -= VISLOW;
  XSetForeground(disp, gct, boxcolor);
  XFillRectangle(disp, Panel->trace, gct,
                 pl[plane].ofs[CL_IN]+2, CHANNEL_HEIGHT(ch)+2,
                 pl[plane].w[CL_IN] -4, BAR_HEIGHT);
  XSetForeground(disp, gct,
                   ((Panel->is_drum[ch+VISLOW])?capcolor:textcolor));
  len = strlen(name);
  XmbDrawString(disp, Panel->trace, trace_font, gct,
              pl[plane].ofs[CL_IN]+4, CHANNEL_HEIGHT(ch)+15,
              name, (len > DISP_INST_NAME_LEN)?DISP_INST_NAME_LEN:len);
}

static void drawDrumPart(int ch, int is_drum) {

  if (plane != 0) return;
  if (is_drum) Panel->barcol[ch] = Panel->cfg->drumvelocity_color;
  else         Panel->barcol[ch] = Panel->cfg->velocity_color;
}

static void draw1Note(int ch, int note, int flag) {
  int i, j;
  XSegment dot[3];

  j = note - 9;
  if (j < 0) return;
  ch -= VISLOW;
  if (flag == '*') {
    XSetForeground(disp, gct, playcolor);
  } else if (flag == '&') {
    XSetForeground(disp, gct,
                   ((keyG[j].col == black)?suscolor:Panel->barcol[0]));
  } else {
    XSetForeground(disp, gct, keyG[j].col);
  }
  for(i=0; i<3; i++) {
    dot[i].x1 = keyG[j].xofs + i;
    dot[i].y1 = CHANNEL_HEIGHT(ch) + keyG[j].k[i].y;
    dot[i].x2 = dot[i].x1;
    dot[i].y2 = dot[i].y1 + keyG[j].k[i].l;
  }
  XDrawSegments(disp, Panel->trace, gct, dot, 3);
}

static void ctl_channel_note(int ch, int note, int velocity) {
  if (velocity == 0) {
    if (note == Panel->cnote[ch])
      Panel->v_flags[ch] = FLAG_NOTE_OFF;
    Panel->cvel[ch] = 0;
  } else if (velocity > Panel->cvel[ch]) {
    Panel->cvel[ch] = velocity;
    Panel->cnote[ch] = note;
    Panel->ctotal[ch] = velocity * Panel->channel[ch].volume *
      Panel->channel[ch].expression / (127*127);
    Panel->v_flags[ch] = FLAG_NOTE_ON;
  }
}

static void drawKeyboardAll(Drawable pix, GC gc) {
  int i, j;
  XSegment dot[3];

  XSetForeground(disp, gc, tracecolor);
  XFillRectangle(disp, pix, gc, 0, 0, BARH_OFS8, BAR_SPACE);
  XSetForeground(disp, gc, boxcolor);
  XFillRectangle(disp, pix, gc, BARH_OFS8, 0,
                 trace_width-BARH_OFS8+1, BAR_SPACE);
  for(i=0; i<KEY_NUM; i++) {
    XSetForeground(disp, gc, keyG[i].col);
    for(j=0; j<3; j++) {
      dot[j].x1 = keyG[i].xofs + j;
      dot[j].y1 = keyG[i].k[j].y;
      dot[j].x2 = dot[j].x1;
      dot[j].y2 = dot[j].y1 + keyG[i].k[j].l;
    }
    XDrawSegments(disp, pix, gc, dot, 3);
  }
}

static void drawBank(int ch, int val) {
  char s[4];

  ch -= VISLOW;
  XSetForeground(disp, gct, textcolor);
  sprintf(s, "%3d", val);
  XmbDrawString(disp, Panel->trace, trace_font, gct,
              pl[plane].ofs[CL_BA], CHANNEL_HEIGHT(ch)+15, s, strlen(s));
}

static void drawVoices(void) {
  char s[20];
  int l;

  XSetForeground(disp, gct, tracecolor);
  XFillRectangle(disp, Panel->trace, gct, Panel->voices_num_width+4,
                 trace_height_nf+1, VOICENUM_WIDTH, TRACE_FOOT);  
  l = snprintf(s, sizeof(s), "%3d/%d", Panel->last_voice, Panel->xaw_i_voices);
  if (l >= sizeof(s) || (l < 0)) l = sizeof(s) - 1;
  XSetForeground(disp, gct, capcolor);  
  XmbDrawString(disp, Panel->trace, trace_font, gct,
                Panel->voices_num_width+6, trace_height_nf+tracefont_ascent,
                s, l);
}

static void drawTempo(void) {
  char s[20];
  int l;

  XSetForeground(disp, gct, tracecolor);
  XFillRectangle(disp, Panel->trace, gct, Panel->voices_num_width+
                 4+VOICENUM_WIDTH+VOICES_NUM_OFS+Panel->tempo_width,
                 trace_height_nf+1, TEMPO_WIDTH+TEMPO_SPACE, TRACE_FOOT);
  l = snprintf(s, sizeof(s), "%d/%3d%%", Panel->tempo*Panel->timeratio/100,
               Panel->timeratio);
  if (l >= sizeof(s) || (l < 0)) l = sizeof(s) - 1;
  XSetForeground(disp, gct, capcolor);
  XmbDrawString(disp, Panel->trace, trace_font, gct,
                VOICES_NUM_OFS+Panel->voices_num_width+4+VOICENUM_WIDTH+
                TEMPO_SPACE+Panel->tempo_width,
                trace_height_nf+tracefont_ascent, s, l);
}

static void drawOverallPitch(int p, int o) {
  int i, pitch;
  static const char *keysig_name[] = {
    "Cb", "Gb", "Db", "Ab", "Eb", "Bb", "F ", "C ",
    "G ", "D ", "A ", "E ", "B ", "F#", "C#", "G#",
    "D#", "A#"
  };
  char s[13];

  pitch = p + ((p < 8) ? 7 : -6);
  if (o > 0)
    for (i = 0; i < o; i++)
      pitch += (pitch > 10) ? -5 : 7;
  else
    for (i = 0; i < -o; i++)
      pitch += (pitch < 7) ? 5 : -7;
  if (p < 8)
    i = snprintf(s, sizeof(s), "%s Maj (%+03d)", keysig_name[pitch], o);
  else
    i = snprintf(s, sizeof(s), "%s Min (%+03d)", keysig_name[pitch], o);
  if ((i >= sizeof(s)) || (i < 0)) i = sizeof(s) - 1;
  XSetForeground(disp, gct, tracecolor);
  XFillRectangle(disp, Panel->trace, gct,
                Panel->voices_num_width+4+VOICENUM_WIDTH+VOICES_NUM_OFS+
                TEMPO_WIDTH+2*TEMPO_SPACE+Panel->tempo_width+Panel->pitch_width,
                trace_height_nf+1, PITCH_WIDTH+PITCH_SPACE, TRACE_FOOT);
  XSetForeground(disp, gct, capcolor);
  XmbDrawString(disp, Panel->trace, ttitle_font, gct,
                VOICES_NUM_OFS+Panel->voices_num_width+4+VOICENUM_WIDTH+
                TEMPO_WIDTH+2*TEMPO_SPACE+Panel->tempo_width+PITCH_SPACE+
                Panel->pitch_width, trace_height_nf+tracefont_ascent, s, i);
}

static void drawTitle(char *str) {
  char *p = str;

  if (!strcmp(p, "(null)")) p = (char *)UNTITLED_STR;
  XSetForeground(disp, gcs, capcolor);
  XmbDrawString(disp, Panel->trace, ttitle_font,
               gcs, VOICES_NUM_OFS+Panel->voices_num_width+4+VOICENUM_WIDTH+
               TEMPO_WIDTH+2*TEMPO_SPACE+Panel->tempo_width+
               PITCH_WIDTH+2*PITCH_SPACE+Panel->pitch_width,
               trace_height_nf+titlefont_ascent, p, strlen(p));
}

static void drawMute(int ch, int mute) {
  char s[16];

  if (mute != 0) {
    SET_CHANNELMASK(channel_mute, ch);
    XSetForeground(disp, gct, textbgcolor);
  }
  else {
    UNSET_CHANNELMASK(channel_mute, ch);
    XSetForeground(disp, gct, textcolor);
  }
  if (!XAWLIMIT(ch)) return;
  ch++;
  /* timidity internals counts from 0. timidity ui counts from 1 */
  sprintf(s, "%2d", ch);
  XmbDrawString(disp, Panel->trace, trace_font, gct,
              pl[plane].ofs[CL_C]+2, CHANNEL_HEIGHT(ch-VISLOW)-5, s, 2);
}

/* End of privates */

int handleTraceinput(char *local_buf) {
  char c;
  int ch, i, n;

  switch (local_buf[0]) {
  case 'R':
    redrawTrace(True);
    break;
  case 'v':
    c = *(local_buf+1);
    n = atoi(local_buf+2);
    if (c == 'L')
      Panel->xaw_i_voices = n;
    else
      Panel->last_voice = n;
    drawVoices();
    break;
  case 'M':
    ch = *(local_buf+1) - 'A';
    n = atoi(local_buf+2);
    drawMute(ch, n);
    break;
  case 'Y':
    {
      int note;

      ch = *(local_buf+1) - 'A';
      c = *(local_buf+2);
      note = (*(local_buf+3) - '0') * 100 + (*(local_buf+4) - '0') * 10 +
              *(local_buf+5) - '0';
      n = atoi(local_buf+6);
      if ((c == '*') || (c == '&')) {
        Panel->c_flags[ch] |= FLAG_PROG_ON;
      } else {
        Panel->c_flags[ch] &= ~FLAG_PROG_ON; n = 0;
      }
      ctl_channel_note(ch, note, n);
      if (!XAWLIMIT(ch)) break;
      draw1Note(ch, note, c);
      draw1Chan(ch, Panel->ctotal[ch], c);
    }
    break;
  case 'I':
    ch = *(local_buf+1) - 'A';
    strlcpy(Panel->inst_name[ch], (char *)&local_buf[2], INST_NAME_SIZE);
    if (!XAWLIMIT(ch)) break;
    drawInstname(ch, Panel->inst_name[ch]);
    break;
  case 'i':
    ch = *(local_buf+1) - 'A';
    Panel->is_drum[ch] = *(local_buf+2) - 'A';
    if (!XAWLIMIT(ch)) break;
    drawDrumPart(ch, Panel->is_drum[ch]);
    break;
  case 'P':
    c = *(local_buf+1);
    ch = *(local_buf+2)-'A';
    n = atoi(local_buf+3);
    switch(c) {
    case 'A':        /* panning */
      Panel->channel[ch].panning = n;
      Panel->c_flags[ch] |= FLAG_PAN;
      if (!XAWLIMIT(ch)) break;
      drawPan(ch, n, True);
      break;
    case 'B':        /* pitch_bend */
      Panel->channel[ch].pitchbend = n;
      Panel->c_flags[ch] |= FLAG_BENDT;
      if (!XAWLIMIT(ch)) break;
      if (!plane) drawPitch(ch, n);
      break;
    case 'b':        /* tonebank */
      Panel->channel[ch].bank = n;
      if (!XAWLIMIT(ch)) break;
      if (plane) drawBank(ch, n);
      break;
    case 'r':        /* reverb */
      Panel->reverb[ch] = n;
      if (!XAWLIMIT(ch)) break;
      if (plane) drawReverb(ch, n);
      break;
    case 'c':        /* chorus */
      Panel->channel[ch].chorus_level = n;
      if (!XAWLIMIT(ch)) break;
      if (plane) drawChorus(ch, n);
      break;
    case 'S':        /* sustain */
      Panel->channel[ch].sustain = n;
      Panel->c_flags[ch] |= FLAG_SUST;
      break;
    case 'P':        /* program */
      Panel->channel[ch].program = n;
      Panel->c_flags[ch] |= FLAG_PROG;
      if (!XAWLIMIT(ch)) break;
      drawProg(ch, n, True);
      break;
    case 'E':        /* expression */
      Panel->channel[ch].expression = n;
      ctl_channel_note(ch, Panel->cnote[ch], Panel->cvel[ch]);
      if (!XAWLIMIT(ch)) break;
      drawExp(ch, n);
      break;
    case 'V':        /* volume */
      Panel->channel[ch].volume = n;
      ctl_channel_note(ch, Panel->cnote[ch], Panel->cvel[ch]);
      if (!XAWLIMIT(ch)) break;
      drawVol(ch, n);
      break;
    }
    break;
  case 'U':       /* update timer */
    {
      static double last_time = 0;
      double d, t;
      Bool need_flush;
      double delta_time;

      t = get_current_calender_time();
      d = t - last_time;
      if (d > 1) d = 1;
      delta_time = d / TRACE_UPDATE_TIME;
      last_time = t;
      need_flush = False;
      for(i=0; i<MAX_TRACE_CHANNELS; i++)
        if (Panel->v_flags[i] != 0) {
          if (Panel->v_flags[i] == FLAG_NOTE_OFF) {
            Panel->ctotal[i] -= DELTA_VEL * delta_time;
            if (Panel->ctotal[i] <= 0) {
              Panel->ctotal[i] = 0;
              Panel->v_flags[i] = 0;
            }
            if (XAWLIMIT(i)) draw1Chan(i, Panel->ctotal[i], '*');
            need_flush = True;
          } else {
            Panel->v_flags[i] = 0;
          }
        }
      if (need_flush) XFlush(disp);
    }
    break;
  case 'r':         /* rhythem, tempo */
    n = atoi(local_buf+1);
    Panel->tempo = (int) (500000/ (double)n * 120 + 0.5);
    drawTempo();
    break;
  case 'q':         /* quotient, ratio */
    Panel->timeratio = atoi(local_buf+1);
    drawTempo();
    break;
  case 'o':         /* pitch offset */
    Panel->poffset = n = atoi(local_buf+2);
    drawOverallPitch(Panel->pitch, n);
    break;
  case 'p':         /* pitch */
    Panel->pitch = n = atoi(local_buf+2);
    drawOverallPitch(n, Panel->poffset);
    break;
  default:
    return -1;
  }
  return 0;
}

void redrawTrace(Boolean draw) {
  int i;
  char s[3];

  for(i=0; i<VISIBLE_CHANNELS; i++) {
    XGCValues gv;
    gv.tile = layer[plane];
    gv.ts_x_origin = 0;
    gv.ts_y_origin = CHANNEL_HEIGHT(i);
    XChangeGC(disp, gc_xcopy, GCTile|GCTileStipXOrigin|GCTileStipYOrigin, &gv);
    XFillRectangle(disp, Panel->trace, gc_xcopy,
                   0, CHANNEL_HEIGHT(i), trace_width, BAR_SPACE);
  }
  XSetForeground(disp, gct, capcolor);
  XDrawLine(disp, Panel->trace, gct, BARH_OFS0, trace_height_nf,
            trace_width-1, trace_height_nf);

  for(i=VISLOW+1; i<VISLOW+VISIBLE_CHANNELS+1; i++) {
    if (IS_SET_CHANNELMASK(channel_mute, i-1))
      XSetForeground(disp, gct, textbgcolor);
    else XSetForeground(disp, gct, textcolor);
    sprintf(s, "%2d", i);
    XmbDrawString(disp, Panel->trace, trace_font, gct,
                  pl[plane].ofs[CL_C]+2, CHANNEL_HEIGHT(i-VISLOW)-5,
                  s, 2);
  }

  if (Panel->g_cursor_is_in) {
    XSetForeground(disp, gct, capcolor);
    XFillRectangle(disp, Panel->trace, gct, 0, 0, trace_width, TRACE_HEADER);
  }
  redrawCaption(Panel->g_cursor_is_in);

  XSetForeground(disp, gct, tracecolor);
  XFillRectangle(disp, Panel->trace, gct, 0, trace_height_nf+1,
                 trace_width, TRACE_FOOT);
  XSetForeground(disp, gct, capcolor);  
  XmbDrawString(disp, Panel->trace, trace_font, gct,
                VOICES_NUM_OFS, trace_height_nf+tracefont_ascent, "Voices", 6);
  drawVoices();
  XmbDrawString(disp, Panel->trace, trace_font, gct,
                VOICES_NUM_OFS+Panel->voices_num_width+4+VOICENUM_WIDTH,
                trace_height_nf+tracefont_ascent, "Tempo", 5);
  drawTempo();
  XmbDrawString(disp, Panel->trace, trace_font, gct,
                VOICES_NUM_OFS+Panel->voices_num_width+4+VOICENUM_WIDTH+
                TEMPO_WIDTH+2*TEMPO_SPACE+Panel->tempo_width,
                trace_height_nf+tracefont_ascent, "Key", 3);
  drawOverallPitch(Panel->pitch, Panel->poffset);
  drawTitle(Panel->title);
  if (draw) {
    for(i=VISLOW; i<VISLOW+VISIBLE_CHANNELS; i++)
      if ((Panel->ctotal[i] != 0) && (Panel->c_flags[i] & FLAG_PROG_ON))
        draw1Chan(i, Panel->ctotal[i], '*');
    XSetForeground(disp, gct, pancolor);
    for(i=VISLOW; i<VISLOW+VISIBLE_CHANNELS; i++) {
      if (Panel->c_flags[i] & FLAG_PAN)
        drawPan(i, Panel->channel[i].panning, False);
    }
    XSetForeground(disp, gct, textcolor);
    for(i=VISLOW; i<VISLOW+VISIBLE_CHANNELS; i++) {
      drawProg(i, Panel->channel[i].program, False);
      drawVol(i, Panel->channel[i].volume);
      drawExp(i, Panel->channel[i].expression);
      if (plane) {
        drawBank(i, Panel->channel[i].bank);
        drawReverb(i, Panel->reverb[i]);
        drawChorus(i, Panel->channel[i].chorus_level);
      } else {
        drawPitch(i, Panel->channel[i].pitchbend);
        drawInstname(i, Panel->inst_name[i]);
      }
    }
  }
}

void redrawCaption(Boolean cursor_is_in) {
  const char *p;
  int i;

  if (cursor_is_in) {
    XSetForeground(disp, gct, capcolor);
    XFillRectangle(disp, Panel->trace, gct, 0, 0, trace_width, TRACE_HEADER);
    XSetBackground(disp, gct, expcolor);
    XSetForeground(disp, gct, tracecolor);
  } else {
    XSetForeground(disp, gct, tracecolor);
    XFillRectangle(disp, Panel->trace, gct, 0, 0, trace_width, TRACE_HEADER);
    XSetBackground(disp, gct, tracecolor);
    XSetForeground(disp, gct, capcolor);
  }
  Panel->g_cursor_is_in = cursor_is_in;
  for(i=0; i<pl[plane].col; i++) {
    p = pl[plane].cap[i];
    XmbDrawString(disp, Panel->trace, trace_font, gct,
    pl[plane].ofs[i]+4, 16, p, strlen(p));
  }
}

void initStatus(void) {
  int i;

  for(i=0; i<MAX_TRACE_CHANNELS; i++) {
    Panel->c_flags[i] = 0;
    Panel->channel[i].bank = 0;
    Panel->channel[i].chorus_level = 0;
    Panel->channel[i].expression = 0;
    Panel->channel[i].panning = -1;
    Panel->channel[i].pitchbend = 0;
    Panel->channel[i].program = 0;
    Panel->channel[i].sustain = 0;
    Panel->channel[i].volume = 0;
    Panel->cnote[i] = 0;
    Panel->cvel[i] = 0;
    Panel->ctotal[i] = 0;
    Panel->is_drum[i] = 0;
    *(Panel->inst_name[i]) = '\0';
    Panel->reverb[i] = 0;
    Panel->v_flags[i] = 0;
  }
  Panel->multi_part = 0;
  Panel->pitch = 7;
  Panel->poffset = 0;
  Panel->tempo = 100;
  Panel->timeratio = 100;
  Panel->last_voice = 0;
}

void scrollTrace(int direction) {
  if (direction > 0) {
    if (Panel->multi_part < (MAX_TRACE_CHANNELS - 2*VISIBLE_CHANNELS))
      Panel->multi_part += VISIBLE_CHANNELS;
    else if (Panel->multi_part < (MAX_TRACE_CHANNELS - VISIBLE_CHANNELS))
      Panel->multi_part = MAX_TRACE_CHANNELS - VISIBLE_CHANNELS;
    else
      Panel->multi_part = 0;
  } else {
    if (Panel->multi_part > VISIBLE_CHANNELS)
      Panel->multi_part -= VISIBLE_CHANNELS;
    else if (Panel->multi_part > 0)
      Panel->multi_part = 0;
    else 
      Panel->multi_part = MAX_TRACE_CHANNELS - VISIBLE_CHANNELS;
  }
  redrawTrace(True);
}

void toggleTracePlane(Boolean draw) {
  plane ^= 1;
  redrawTrace(draw);
}

int getLowestVisibleChan(void) {
  return Panel->multi_part;
}

int getVisibleChanNum(void) {
  return Panel->visible_channels;
}

void initTrace(Display *dsp, Window trace, char *title, tconfig *cfg) {
  XGCValues gv, gcval;
  GC gc;
  unsigned long gcmask;
  XFontStruct **fs_list;
  XFontStruct *font0;
  char **ml;
  int i, j, k, tmpi, w, screen;

  Panel = (PanelInfo *)safe_malloc(sizeof(PanelInfo));
  Panel->trace = trace;
  Panel->title = title;
  Panel->cfg = cfg;
  Panel->init = 1;
  plane = 0;
  Panel->g_cursor_is_in = False;
  disp = dsp;
  screen = DefaultScreen(disp);
  for(i=0; i<MAX_TRACE_CHANNELS; i++) {
    if (ISDRUMCHANNEL(i)) {
      Panel->is_drum[i] = 1;
      Panel->barcol[i] = cfg->drumvelocity_color;
    } else {
      Panel->barcol[i] = cfg->velocity_color;
    }
    Panel->inst_name[i] = (char *)safe_malloc(sizeof(char) * INST_NAME_SIZE);
  }
  initStatus();
  Panel->xaw_i_voices = 0;
  Panel->voices_num_width = XmbTextEscapement(trace_font,
                                              "Voices", 6) + VOICES_NUM_OFS;
  Panel->tempo_width = XmbTextEscapement(trace_font, "Tempo", 5);
  Panel->pitch_width = XmbTextEscapement(trace_font, "Key", 3);
  Panel->visible_channels = (cfg->trace_height - TRACE_HEADER - TRACE_FOOT) /
                            BAR_SPACE;
  if (Panel->visible_channels > MAX_CHANNELS)
    Panel->visible_channels = MAX_CHANNELS;
  else if (Panel->visible_channels < 1)
    Panel->visible_channels = 1;
  cfg->trace_height = Panel->visible_channels * BAR_SPACE +
                      TRACE_HEADER + TRACE_FOOT;
  /* Prevent empty space between the trace foot and the channels bars. */

  gcmask = GCForeground | GCBackground | GCFont;
  gcval.foreground = 1;
  gcval.background = 1;
  gcval.plane_mask = 1;
  XFontsOfFontSet(ttitle_font, &fs_list, &ml);
  font0 = fs_list[0];
  if (font0->fid == 0) {
    font0 = XLoadQueryFont(disp, ml[0]);
    if (font0 == NULL) {
      fprintf(stderr, "can't load fonts %s\n", ml[0]);
      exit(1);
    }
  }
  titlefont_ascent = font0->ascent + 3;
  gcval.font = font0->fid;
  gcs = XCreateGC(disp, RootWindow(disp, screen), gcmask, &gcval);

  gv.fill_style = FillTiled;
  gv.fill_rule = WindingRule;
  gc_xcopy = XCreateGC(disp, RootWindow(disp, screen),
                       GCFillStyle | GCFillRule, &gv);
  gct = XCreateGC(disp, RootWindow(disp, screen), 0, NULL);
  gc = XCreateGC(disp, RootWindow(disp, screen), 0, NULL);

  XFontsOfFontSet(trace_font, &fs_list, &ml);
  font0 = fs_list[0];
  if (font0->fid == 0) {
    font0 = XLoadQueryFont(disp, ml[0]);
    if (font0 == NULL) {
      fprintf(stderr, "can't load fonts %s\n", ml[0]);
      exit(1);
    }
  }
  tracefont_ascent = font0->ascent + 3;
  if (tracefont_ascent > titlefont_ascent)
    tracefont_ascent = titlefont_ascent;
  else
    titlefont_ascent = tracefont_ascent;
  XSetFont(disp, gct, font0->fid);

  keyG = (ThreeL *)safe_malloc(sizeof(ThreeL) * KEY_NUM);
  for(i=0, j=BARH_OFS8+1; i<KEY_NUM; i++) {
    tmpi = i%12;
    switch (tmpi) {
    case 0:
    case 5:
    case 10:
      keyG[i].k[0].y = 11; keyG[i].k[0].l = 7;
      keyG[i].k[1].y = 2; keyG[i].k[1].l = 16;
      keyG[i].k[2].y = 11; keyG[i].k[2].l = 7;
      keyG[i].col = white;
      break;
    case 2:
    case 7:
      keyG[i].k[0].y = 11; keyG[i].k[0].l = 7;
      keyG[i].k[1].y = 2; keyG[i].k[1].l = 16;
      keyG[i].k[2].y = 2; keyG[i].k[2].l = 16;
      keyG[i].col = white;
      break;
    case 3:
    case 8:
      j += 2;
      keyG[i].k[0].y = 2; keyG[i].k[0].l = 16;
      keyG[i].k[1].y = 2; keyG[i].k[1].l = 16;
      keyG[i].k[2].y = 11; keyG[i].k[2].l = 7;
      keyG[i].col = white;
      break;
    default:  /* black key */
      keyG[i].k[0].y = 2; keyG[i].k[0].l = 8;
      keyG[i].k[1].y = 2; keyG[i].k[1].l = 8;
      keyG[i].k[2].y = 2; keyG[i].k[2].l = 8;
      keyG[i].col = black;
      break;
    }
    keyG[i].xofs = j; j += 2;
  }

  /* draw on template pixmaps that includes one channel row */
  for(i=0; i<2; i++) {
    layer[i] = XCreatePixmap(disp, Panel->trace, trace_width, BAR_SPACE,
                             DefaultDepth(disp, screen));
    drawKeyboardAll(layer[i], gc);
    XSetForeground(disp, gc, capcolor);
    XDrawLine(disp, layer[i], gc, 0, 0, trace_width, 0);
    XDrawLine(disp, layer[i], gc, 0, 0, 0, BAR_SPACE);
    XDrawLine(disp, layer[i], gc, trace_width-1, 0, trace_width-1, BAR_SPACE);

    for(j=0; j < pl[i].col-1; j++) {
      tmpi = TRACEH_OFS; w = pl[i].w[j];
      for(k=0; k<j; k++) tmpi += pl[i].w[k];
      tmpi = pl[i].ofs[j];
      XSetForeground(disp, gc, capcolor);
      XDrawLine(disp, layer[i], gc, tmpi+w, 0, tmpi+w, BAR_SPACE);
      XSetForeground(disp, gc, rimcolor);
      XDrawLine(disp, layer[i], gc, tmpi+w-2, 2, tmpi+w-2, BAR_HEIGHT+1);
      XDrawLine(disp, layer[i], gc, tmpi+2, BAR_HEIGHT+2, tmpi+w-2,
                BAR_HEIGHT+2);
      XSetForeground(disp, gc, j?boxcolor:textbgcolor);
      XFillRectangle(disp, layer[i], gc, tmpi+2, 2, w-4, BAR_HEIGHT);
    }
  }
  XFreeGC(disp, gc);
}

void uninitTrace(Boolean free_server_resources) {
  int i;

  if (free_server_resources) {
    XFreePixmap(disp, layer[0]); XFreePixmap(disp, layer[1]);
#if 0
    if (!Panel->init) for (i=0; i<T2COLUMN; i++) {
      XFreePixmap(disp, gradient_pixmap[i]);
      XFreeGC(disp, gradient_gc[i]);
    }
#endif
    XFreeGC(disp, gcs); XFreeGC(disp, gct); XFreeGC(disp, gc_xcopy); 
  }

  for (i=0; i<MAX_TRACE_CHANNELS; i++) free(Panel->inst_name[i]);
  free(Panel); free(keyG);
}
