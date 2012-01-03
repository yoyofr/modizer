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

    xaw.h: written by Yoshishige Arai (ryo2@on.rim.or.jp) 12/8/98
           modified by Yair Kalvariski (cesium2@gmail.com)
    */
#ifndef _XAW_H_
#define _XAW_H_
/*
 * XAW configurations
 */

#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include "timidity.h"

/* Define to use Xaw3d */
/* #define XAW3D */

/* Define to use libXaw3d's tip widget (requires version 1.5E) */
/* #define HAVE_XAW3D_TIP */

/* Define to use neXtaw */
/* #define NEXTAW */

/* Define to use XawPlus */
/* #define XAWPLUS */

/* Define to use code for old xaw versions (Xaw6 and such) */
/* #define OLDXAW */

/* Define to use Timidity's implmentation of submenus */
/* #define TimNmenu */

/* Define to use scrollable Text widget instead of Label widget */
/* #define WIDGET_IS_LABEL_WIDGET */

/*** Initial dot file name at home directory ***/
#define INITIAL_CONFIG ".xtimidity"

/*
 * SET CORRECT PATHS AND CAPABILITIES.
 */

#ifdef XAW3D
#define XAWINCDIR(x) <X11/Xaw3d/x>
#elif defined(NEXTAW)
#define XAWINCDIR(x) <X11/neXtaw/x>
#include XAWINCDIR(XawVersion.h)
#elif defined(XAWPLUS)
#define XAWINCDIR(x) <X11/XawPlus/x>
#else
#define XAWINCDIR(x) <X11/Xaw/x>
#include <X11/Xmu/Converters.h>
#include XAWINCDIR(XawInit.h)
#ifndef XawVersion
#define OLDXAW
#else
#if XawVersion < 7000L
#define OLDXAW
#endif /* XawVersion < 7000L */
#endif /* !XawVersion */
#ifndef OLDXAW
#define XAW
#endif /* !OLDXAW */
#endif

#if (defined (XAW3D) && !defined(HAVE_XAW3D_TIP)) || defined(OLDXAW) || \
     defined(XAWPLUS) || defined(NEXTAW)
/*
 * XtNmenuItem in SmeBSBObject Widgets is not supported in
 * Xaw3d v1.5, Xaw6, XawPlus and neXtaw.
 */
#define TimNmenu
#endif /* (XAW3D && !HAVE_XAW3D_TIP) || OLDXAW || XAWPLUS || NEXTAW */

#if (defined(XAW3D) && defined(HAVE_XAW3D_TIP)) \
      || (defined(XAW) && !(defined(OLDXAW))) || defined(XAWPLUS)
/* Tooltips support exists only in Xaw7 (and above), Xaw3d 1.5E and XawPlus */
#define HAVE_TIP
#endif /* (XAW3D && HAVE_XAW3D_TIP) || (XAW && !OLDXAW) || XAWPLUS */

#if defined(XAW3D) || defined(XAWPLUS)
  /*
   * Xaw3D 1.5/1.5E crashes on XtVaSetValues to XtNvalue in Dialogwidget,
   * where the string is used in place, internationalization is on,
   * and a new string is set - it tries to free the wrong pointer.
   * The Fedora package includes a one-line patch for this problem, but
   * we can't rely on the user's Xaw3D package being patched.
   * XawPlus has the same bug.
   */
#define CLEARVALUE
#endif /* XAW3D || XAWPLUS */

#if defined(NEXTAW) || defined(XAW3D) || defined(XAWPLUS)
  /*
   * These toolkits crash on XtDestroyWidget(w), when
   * w == popup_sformat if this hack isn't used.
   */
#define RECFMTGROUPDESTHACK
#endif /* NEXTAW || XAW3D || XAWPLUS */

#if defined(NEXTAW) || defined(XAWPLUS)
#define DONTUSEOVALTOGGLES
#endif /* NEXTAW || XAWPLUS */

#if defined(XAW) || defined(OLDXAW)
  /*
   * libXaw has a bug: it doesn't set length=width-minimumThumb, but
   * length=width so the thumb may become invisible when it reaches the
   * end of the scrollbar. (Other toolkits always substracts minimumThumb
   * from length, so we can't use the same codepath).
   */
#define SCROLLBARLENGTHBUG
#endif /* XAW || OLDXAW */

#if defined(NEXTAW) || defined(XAWPLUS) || defined(XAW3D)
 /*
  * These toolkits won't scroll textarea when sentence is wrapped.
  * We'll redraw to get it to scroll.
  */
#define BYPASSTEXTSCROLLBUG
#endif /* NEXTAW || XAW3D || XAWPLUS */

/*
 * CONSTANTS FOR XAW MENUS
 */
#define MAXVOLUME MAX_AMPLIFICATION
#define MAX_XAW_MIDI_CHANNELS MAX_CHANNELS

#define APP_CLASS "TiMidity"
#define APP_NAME "timidity"

#define PIPE_LENGTH 300
#ifndef PATH_MAX
#define PATH_MAX 512
#endif
#define MAX_DIRECTORY_ENTRY BUFSIZ

#define MODUL_N 0
#define PORTA_N 1
#define NRPNV_N 2
#define REVERB_N 3
#define CHPRESSURE_N 4
#define OVERLAPV_N 5
#define TXTMETA_N 6
#define MAX_OPTION_N 7

#define MODUL_BIT (1<<MODUL_N)
#define PORTA_BIT (1<<PORTA_N)
#define NRPNV_BIT (1<<NRPNV_N)
#define REVERB_BIT (1<<REVERB_N)
#define CHPRESSURE_BIT (1<<CHPRESSURE_N)
#define OVERLAPV_BIT (1<<OVERLAPV_N)
#define TXTMETA_BIT (1<<TXTMETA_N)

#ifdef MODULATION_WHEEL_ALLOW
#define INIT_OPTIONS0 MODUL_BIT
#else
#define INIT_OPTIONS0 0
#endif

#ifdef PORTAMENTO_ALLOW
#define INIT_OPTIONS1 PORTA_BIT
#else
#define INIT_OPTIONS1 0
#endif

#ifdef NRPN_VIBRATO_ALLOW
#define INIT_OPTIONS2 NRPNV_BIT
#else
#define INIT_OPTIONS2 0
#endif

#ifdef REVERB_CONTROL_ALLOW
#define INIT_OPTIONS3 REVERB_BIT
#define DEFAULT_REVERB 1
#else
#ifdef FREEVERB_CONTROL_ALLOW
#define INIT_OPTIONS3 REVERB_BIT
#define DEFAULT_REVERB 3
#else
#define INIT_OPTIONS3 0
#define DEFAULT_REVERB 1
#endif /* FREEVERB_CONTROL_ALLOW */
#endif /* REVERB_CONTROL_ALLOW */

#ifdef GM_CHANNEL_PRESSURE_ALLOW
#define INIT_OPTIONS4 CHPRESSURE_BIT
#else
#define INIT_OPTIONS4 0
#endif

#ifdef OVERLAP_VOICE_ALLOW
#define INIT_OPTIONS5 OVERLAPV_BIT
#else
#define INIT_OPTIONS5 0
#endif

#ifdef ALWAYS_TRACE_TEXT_META_EVENT
#define INIT_OPTIONS6 TXTMETA_BIT
#else
#define INIT_OPTIONS6 0
#endif

#define DEFAULT_OPTIONS (INIT_OPTIONS0+INIT_OPTIONS1+INIT_OPTIONS2+INIT_OPTIONS3+INIT_OPTIONS4+INIT_OPTIONS5+INIT_OPTIONS6)

#ifdef CHORUS_CONTROL_ALLOW
#define DEFAULT_CHORUS 1
#else
#define DEFAULT_CHORUS 0
#endif

#define XAW_UPDATE_TIME 0.1

#ifndef XawFmt8Bit
#define XawFmt8Bit FMT8BIT
#endif
/* Only defined since X11R6 */

#endif /* _XAW_H_ */
