/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
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
 
    x_trace.h: 
        based on code by Yoshishige Arai (ryo2@on.rim.or.jp)
        modified by Yair Kalvariski <cesium2@gmail.com>
*/

#ifndef _X_TRACE_H
#define _X_TRACE_H

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"

#define MAX_TRACE_CHANNELS MAX_CHANNELS
#if MAX_TRACE_CHANNELS > 16
#define D_VISIBLE_CHANNELS 16
#else
#define D_VISIBLE_CHANNELS MAX_TRACE_CHANNELS
#endif /* MAX_TRACE_CHANNELS > 16 */

#define TRACE_UPDATE_TIME 0.1
#define DELTA_VEL	32

#define TRACE_WIDTH	627	/* default height of trace_vport */
#define TRACE_HEADER	22
#define TRACE_FOOT	20
#define TRACEH_OFS	0
#define BAR_SPACE	20
#define BAR_HEIGHT	16
#define DISP_INST_NAME_LEN 13
#define INST_NAME_SIZE	16

#define CHANNEL_HEIGHT(ch) BAR_SPACE*(ch)+TRACE_HEADER
#define TRACE_HEIGHT CHANNEL_HEIGHT(D_VISIBLE_CHANNELS)
#define TRACE_HEIGHT_WITH_FOOTER (TRACE_HEIGHT+TRACE_FOOT)

typedef struct {
  int Red_depth, Green_depth, Blue_depth;
  int Red_sft, Green_sft, Blue_sft;
} RGBInfo;

typedef struct _tconfig {
  Boolean gradient_bar;
  Dimension trace_width, trace_height;
  XFontSet trace_font, ttitle_font;
  Pixel common_fgcolor, text_bgcolor,
        velocity_color, drumvelocity_color, volume_color, expr_color, pan_color,
        trace_bgcolor, rim_color, box_color, caption_color, sus_color,
        white_key_color, black_key_color, play_color, rev_color, cho_color;
  String untitled;
} tconfig;

extern int getLowestVisibleChan(void);
extern int getVisibleChanNum(void);
extern int handleTraceinput(char *);
extern void initStatus(void);
extern void initTrace(Display *, Window, char *, tconfig *);
extern void redrawCaption(Boolean);
extern void redrawTrace(Boolean);
extern void scrollTrace(int);
extern void toggleTracePlane(Boolean);
extern void uninitTrace(Boolean);

#endif /* _X_TRACE_I_H */
