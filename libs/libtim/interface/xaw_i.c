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

    xaw_i.c - XAW Interface
        from Tomokazu Harada <harada@prince.pe.u-tokyo.ac.jp>
        modified by Yoshishige Arai <ryo2@on.rim.or.jp>
        modified by Yair Kalvariski <cesium2@gmail.com>
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif /* !NO_STRING_H */
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#include <pwd.h>
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */

#include "xaw.h"
#include "x_trace.h"
#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "controls.h"
#include "strtab.h"
#include "arc.h"

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#ifdef HAVE_GETPID
#include <X11/Xatom.h>
#endif

#include XAWINCDIR(AsciiText.h)
#include XAWINCDIR(Box.h)
#include XAWINCDIR(Cardinals.h)
#include XAWINCDIR(Dialog.h)
#include XAWINCDIR(Form.h)
#include XAWINCDIR(Label.h)
#include XAWINCDIR(List.h)
#include XAWINCDIR(MenuButton.h)
#include XAWINCDIR(Paned.h)
#include XAWINCDIR(Scrollbar.h)
#include XAWINCDIR(Simple.h)
#include XAWINCDIR(SimpleMenu.h)
#include XAWINCDIR(SmeBSB.h)
#include XAWINCDIR(SmeLine.h)
#if defined(HAVE_TIP) && !defined(XAWPLUS)
#include XAWINCDIR(Tip.h)
#endif /* HAVE_TIP && !XAWPLUS */
#include XAWINCDIR(Toggle.h)
#ifdef XawTraversal
#include XAWINCDIR(Traversal.h)
#endif /* XawTraversal */
#include XAWINCDIR(Viewport.h)

#ifdef OFFIX
#include <OffiX/DragAndDrop.h>
#include <OffiX/DragAndDropTypes.h>
#endif /* OFFIX */
#ifdef XDND
#include "xdnd.h"
#endif /* XDND */

#include "bitmaps/arrow.xbm"
#include "bitmaps/back.xbm"
#include "bitmaps/check.xbm"
#include "bitmaps/fast.xbm"
#include "bitmaps/fwrd.xbm"
#include "bitmaps/keyup.xbm"
#include "bitmaps/keydown.xbm"
#include "bitmaps/next.xbm"
#include "bitmaps/on.xbm"
#include "bitmaps/off.xbm"
#include "bitmaps/pause.xbm"
#include "bitmaps/play.xbm"
#include "bitmaps/prev.xbm"
#include "bitmaps/quit.xbm"
#include "bitmaps/random.xbm"
#include "bitmaps/repeat.xbm"
#include "bitmaps/slow.xbm"
#include "bitmaps/stop.xbm"
#include "bitmaps/timidity.xbm"

#ifndef S_ISDIR
#define S_ISDIR(mode)   (((mode)&0xF000) == 0x4000)
#endif /* !S_ISDIR */

#define DEFAULT_REG_WIDTH	400 /* default width when not in trace mode */

#define INIT_FLISTNUM MAX_DIRECTORY_ENTRY

#define MAX_POPUPNAME 15
#define MAX_FILTER 20
#define POPUP_HEIGHT 400
#define PANE_HEIGHT 200
#define INFO_HEIGHT 20
#define ran() (int) (51.0 * (rand() / (RAND_MAX + 1.0))) - 25;

typedef struct {
  char *dirname;
  char *basename;
} DirPath;

typedef struct {
  String *StringArray;
  unsigned int number;
} StringList;

typedef struct {
 char ld_basepath[PATH_MAX];
 char ld_popupname[MAX_POPUPNAME];
 Widget ld_popup_load;
 Widget ld_load_d;
 Widget ld_load_b;
 Widget ld_load_ok;
 Widget ld_load_pane;
 Widget ld_load_vport;
 Widget ld_load_vportdir;
 Widget ld_load_flist;
 Widget ld_load_dlist;
 Widget ld_cwd_l;
 Widget ld_load_info;
 Dimension ld_ldwidth;
 Dimension ld_ldheight;
 StringList ld_fdirlist;
 StringList ld_fulldirlist;
 StringList ld_ddirlist;
 char ld_filter[MAX_FILTER];
 char ld_cur_filter[MAX_FILTER];
} load_dialog;

typedef load_dialog *ldPointer;

typedef struct {
 ldPointer ld;
 char *name; 
 struct ldStore *next;
} ldStore;

typedef ldStore *ldStorePointer;

static ldStorePointer ldSstart;
static ldPointer ld;

#define basepath ld->ld_basepath
#define popupname ld->ld_popupname
#define popup_load ld->ld_popup_load
#define load_d ld->ld_load_d
#define load_b ld->ld_load_b
#define load_ok ld->ld_load_ok
#define load_pane ld->ld_load_pane
#define load_vport ld->ld_load_vport
#define load_vportdir ld->ld_load_vportdir
#define load_flist ld->ld_load_flist
#define load_dlist ld->ld_load_dlist
#define cwd_l ld->ld_cwd_l
#define load_info ld->ld_load_info
#define ldwidth ld->ld_ldwidth
#define ldheight ld->ld_ldheight
#define fdirlist ld->ld_fdirlist.StringArray
#define filenum ld->ld_fdirlist.number
#define fulldirlist ld->ld_fulldirlist.StringArray
#define fullfilenum ld->ld_fulldirlist.number
#define ddirlist ld->ld_ddirlist.StringArray
#define dirnum ld->ld_ddirlist.number
#define filter ld->ld_filter
#define cur_filter ld->ld_cur_filter

#define LISTDIALOGBASENAME "dialog_list"

static Widget toplevel, m_box, base_f, file_mb, file_sm, bsb, quit_b, play_b,
  pause_b, stop_b, prev_b, next_b, fwd_b, back_b, random_b, repeat_b,
  fast_b, slow_b, keyup_b, keydown_b, b_box, v_box, t_box, vol_l0,
  vol_l, vol_bar, tune_l0, tune_l, tune_bar, trace_vport, trace, file_vport,
  file_list, title_mb, title_sm, time_l, lyric_t, chorus_b,
  popup_file = NULL, popup_opt = NULL;
static Widget *psmenu = NULL;
#ifndef TimNmenu
/* We'll be using XAWs implementation of submenus (XtNmenuName).
 * If its parameter isn't constantly available, it will fail to work.
 */
static String *sb;
#endif /* !TimNmenu */
static char local_buf[PIPE_LENGTH];
static char window_title[300];
static char *home;
int amplitude = DEFAULT_AMPLIFICATION;

static int maxentry_on_a_menu = 0, current_n_displayed = 0;
static long submenu_n = 0;
#define IsRealized(w) (((w) != NULL) && (XtIsRealized(w)))

static Boolean lockevents = False;
/*
 * There's a race condition, where after writing a request to the pipe
 * which changes the current song (like "N" for next song) events from
 * the (then current) song still pass through, but are counted
 * as events from the next song. This variable is used for rudimentary
 * locking to prevent such events from influencing the trace display.
 */

typedef struct {
  Boolean	confirmexit;
  Boolean	repeat;
  Boolean	autostart;
  Boolean	autoexit;
  Boolean	disptext;
  Boolean	shuffle;
  Boolean	disptrace;
  int		amplitude;
  int		extendopt;
  int		chorusopt;
  Boolean	tooltips;
  Boolean	showdotfiles;
  char		*DefaultDir;
  Boolean	save_list;
} Config;

/* Default configuration for Xaw interface */
/* (confirmexit repeat autostart autoexit disptext shuffle disptrace amplitude
    options chorus tooltips showdotfiles defaultdir savelist) */
static Config Cfg = {
  False, False, True, False, True, False, False,
  DEFAULT_AMPLIFICATION, DEFAULT_OPTIONS, DEFAULT_CHORUS,
#ifndef XAW3D
  True,
#else
/* The Xaw3d v1.5E tooltip function is surprisingly slow. */
  False,
#endif /* !XAW3D */
  False, NULL, True
};

enum {
  S_ConfirmExit = 0,
  S_RepeatPlay,
  S_AutoStart,
  S_DispText,
  S_DispTrace,
  S_CurVol,
  S_ShufflePlay,
  S_AutoExit,
  S_ExtOptions,
  S_ChorusOption,
  S_Tooltips,
  S_Showdotfiles,
  S_DefaultDirectory,
  S_SaveList,
  S_MidiFile,
  CFGITEMSNUMBER
};

static const char *cfg_items[CFGITEMSNUMBER] = {
  "ConfirmExit", "RepeatPlay", "AutoStart", "Disp:text", "Disp:trace",
  "CurVol", "ShufflePlay", "AutoExit", "ExtOptions", "ChorusOption",
  "Tooltips", "Showdotfiles", "DefaultDir", "SaveList", "File",
};

#define COMMON_BGCOLOR		"gray67"
#define COMMANDBUTTON_COLOR	"gray78"
#define TEXTBG_COLOR		"gray82"

typedef struct {
  char id_char;
  char *id_name;
} id_list;

typedef struct {
  id_list *output_list;
  unsigned short max;
  unsigned short current;
  unsigned short def;
  char *lbuf;
  Widget formatGroup;
  Widget *toggleGroup;
} outputs;

static Display *disp;
static Atom wm_delete_window;
#ifdef HAVE_GETPID
static Atom net_wm_pid;
static pid_t pid;
#endif /* HAVE_GETPID */
static XtAppContext app_con;
static Pixmap check_mark, arrow_mark, on_mark, off_mark;

#define GET_BITMAP(Pix) XCreateBitmapFromData(disp, \
                        RootWindowOfScreen(XtScreen(toplevel)), \
                        (char *)Pix##_bits, Pix##_width, Pix##_height)

#ifdef XDND
static DndClass _DND;
static DndClass *dnd = &_DND;
#endif /* XDND */

static int root_height, root_width;
static Dimension curr_width, curr_height, base_height, lyric_height,
       trace_v_height;
static int max_files = 0,
           init_options = DEFAULT_OPTIONS, init_chorus = DEFAULT_CHORUS;
static outputs *record, *play;
static Boolean recording = False, use_own_start_scroll = False;
static String *flist = NULL;
static int max_num = INIT_FLISTNUM;
static int total_time = 0, curr_time = 0, halt = 0;

typedef enum {
  NORESPONSE = -1,
  OK,
  CANCEL
} resvalues;
typedef struct {
  resvalues val;
  Widget widget;
} resvar;
static resvar response = {NORESPONSE, NULL};

static struct _app_resources {
  tconfig tracecfg;
  Boolean arrange_title;
  Dimension text_height, menu_width;
  Pixel common_bgcolor, menub_bgcolor, text2_bgcolor, toggle_fgcolor,
        button_fgcolor, button_bgcolor;
  String more_text, file_text, no_playing;
  XFontSet label_font, volume_font, text_font;
  String labelfile, popup_confirm, load_LISTDIALOGBASENAME_title,
         save_LISTDIALOGBASENAME_title;
} app_resources;

#define capcolor	app_resources.tracecfg.caption_color
#define bgcolor		app_resources.common_bgcolor
#define buttonbgcolor	app_resources.button_bgcolor
#define buttoncolor	app_resources.button_fgcolor
#define menubcolor	app_resources.menub_bgcolor
#define textcolor	app_resources.tracecfg.common_fgcolor
#define textbgcolor	app_resources.tracecfg.text_bgcolor
#define text2bgcolor	app_resources.text2_bgcolor
#define togglecolor	app_resources.toggle_fgcolor

#define ID_LOAD			100
#define ID_SAVE			101
#define ID_LOAD_PLAYLIST	102
#define ID_SAVE_PLAYLIST	103
#define ID_SAVECONFIG		104
#define ID_HIDETXT		105
#define ID_HIDETRACE		106
#define ID_SHUFFLE		107
#define ID_REPEAT		108
#define ID_AUTOSTART		109
#define ID_AUTOQUIT		110
#define ID_LINE			111
#define ID_FILELIST		112
#define ID_OPTIONS		113
#define ID_LINE2		114
#define ID_ABOUT		115
#define ID_QUIT			116

#define R(x) #x
#define S(x) R(x)

typedef struct {
  const int	id;
  const String	name;
  WidgetClass	*class;
  Widget	widget;
} ButtonRec;

static ButtonRec file_menu[] = {
  {ID_LOAD, "load", &smeBSBObjectClass},
  {ID_SAVE, "save", &smeBSBObjectClass},
  {ID_LOAD_PLAYLIST, "load_playlist", &smeBSBObjectClass},
  {ID_SAVE_PLAYLIST, "save_playlist", &smeBSBObjectClass},
  {ID_SAVECONFIG, "saveconfig", &smeBSBObjectClass},
  {ID_HIDETXT, "hidetext", &smeBSBObjectClass},
  {ID_HIDETRACE, "hidetrace", &smeBSBObjectClass},
  {ID_SHUFFLE, "shuffle", &smeBSBObjectClass},
  {ID_REPEAT, "repeat", &smeBSBObjectClass},
  {ID_AUTOSTART, "autostart", &smeBSBObjectClass},
  {ID_AUTOQUIT, "autoquit", &smeBSBObjectClass},
  {ID_LINE, "line", &smeLineObjectClass},
  {ID_FILELIST, "filelist", &smeBSBObjectClass},
  {ID_OPTIONS, "modes", &smeBSBObjectClass},
  {ID_LINE2, "line2", &smeLineObjectClass},
  {ID_ABOUT, "about", &smeBSBObjectClass},
  {ID_QUIT, "quit", &smeBSBObjectClass},
};

typedef struct {
  const int	bit;
  Widget	widget;
} OptionRec;

static OptionRec option_num[] = {
  {MODUL_BIT, NULL},
  {PORTA_BIT, NULL},
  {NRPNV_BIT, NULL},
  {REVERB_BIT, NULL},
  {CHPRESSURE_BIT, NULL},
  {OVERLAPV_BIT, NULL},
  {TXTMETA_BIT, NULL},
};

typedef union {
  float f;
  XtArgVal x;
} barfloat;

static char *dotfile = NULL;
static char **dotfile_flist;
static int dot_nfile = 0;
#define SPREFIX "set "
#define SPLEN 4
#define SLINELEN PATH_MAX+SPLEN+20

static void a_init_interface(int);
void a_start_interface(int);
extern void a_pipe_write(const char *, ...);
extern int a_pipe_read(char *, size_t);
extern int a_pipe_nread(char *, size_t);
extern void a_pipe_sync(void);
static void a_print_text(Widget, char *);
static void a_print_msg(Widget);
static void handle_input(XtPointer, int *, XtInputId *);

static void a_readconfig(Config *, char **);
static void a_saveconfig(char *, Boolean);
static void aboutACT(Widget, XEvent *, String *, Cardinal *);
static void addOneFile(int, long, char *);
static void addFlist(char *, long);
static void backspaceACT(Widget, XEvent *, String *, Cardinal *);
static void backCB(Widget, XtPointer, XtPointer);
static void callFilterDirList(Widget, XtPointer, XtPointer);
static void callRedrawTrace(Boolean);
static void callInitTrace(void);
static void cancelACT(Widget, XEvent *, String *, Cardinal *);
static void cancelCB(Widget, XtPointer, XtPointer);
static char *canonicalize_path(char *);
#ifdef TimNmenu
static void checkRightAndPopupSubmenuACT(Widget, XEvent *, String *,Cardinal *);
#endif /* TimNmenu */
#ifdef CLEARVALUE
static void clearValue(Widget);
#endif /* CLEARVALUE */
static void closeParentACT(Widget, XEvent *, String *, Cardinal *);
static void closeWidgetCB(Widget, XtPointer, XtPointer);
static int configcmp(char *, int *);
static int confirmCB(Widget, char *, Boolean);
static void completeDirACT(Widget , XEvent *, String *, Cardinal *);
static void createBars(void);
static void createButtons(void);
static void createDialog(Widget, ldPointer);
static void createFlist(void);
static void createOptions(void);
static void createTraceWidgets(void);
static Widget
  createOutputSelectionWidgets(Widget, Widget, Widget, outputs *, Boolean);
#ifndef WIDGET_IS_LABEL_WIDGET
static void deleteTextACT(Widget, XEvent *, String *, Cardinal *);
#endif /* !WIDGET_IS_LABEL_WIDGET */
static void destroyWidgetCB(Widget, XtPointer, XtPointer);
static void downACT(Widget, XEvent *, String *, Cardinal *);
static void exchgWidthACT(Widget, XEvent *, String *, Cardinal *);
static char *expandDir(char *, DirPath *, char *);
static void fdelallCB(Widget, XtPointer, XtPointer);
static void fdeleteCB(Widget, XtPointer, XtPointer);
#ifdef OFFIX
static void FileDropedHandler(Widget, XtPointer, XEvent *, Boolean *);
#endif /* OFFIX */
static void filemenuACT(Widget, XEvent *, String *, Cardinal *);
static void filemenuCB(Widget, XtPointer, XtPointer);
static void filterDirList(Widget, ldPointer, Boolean);
static void flistMoveACT(Widget, XEvent *, String *, Cardinal *);
static void flistpopupACT(Widget, XEvent *, String *, Cardinal *);
static void forwardCB(Widget, XtPointer, XtPointer);
static void fselectCB(Widget, XtPointer, XtPointer);
#if 0
static void free_ldS(ldStorePointer ldS);
#endif
static void freevarCB(Widget, XtPointer, XtPointer);
static char *get_user_home_dir(void);
static ldStorePointer getldsPointer(ldStorePointer lds, char *Popname);
static ldStorePointer init_ldS(void);
static void init_output_lists(void);
static Boolean IsTracePlaying(void);
static Boolean IsEffectiveFile(char *);
static void leaveSubmenuACT(Widget, XEvent *, String *, Cardinal *);
static void menuCB(Widget, XtPointer, XtPointer);
static void muteChanACT(Widget, XEvent *, String *, Cardinal *);
static void nextCB(Widget, XtPointer, XtPointer);
static void offPauseButton(void);
static void offPlayButton(void);
static Boolean onPlayOffPause(void);
static void okCB(Widget, XtPointer, XtPointer);
static void okACT(Widget, XEvent *, String *, Cardinal *);
static void optionsCB(Widget, XtPointer, XtPointer);
static void optionscloseCB(Widget, XtPointer, XtPointer);
static void optionspopupACT(Widget, XEvent *, String *, Cardinal *);
static void pauseACT(Widget, XEvent *, String *, Cardinal *);
static void pauseCB(Widget, XtPointer, XtPointer);
static void pitchCB(Widget, XtPointer, XtPointer);
static void playCB(Widget, XtPointer, XtPointer);
static void popdownAddALL(Widget, XtPointer, XtPointer);
static void popdownAddALLACT(Widget, XEvent *, String *, Cardinal *);
static void popdownCB(Widget, XtPointer, XtPointer);
static void popdownLoadfile(Widget, XtPointer, XtPointer);
static void popdownLoadPL(Widget, XtPointer, XtPointer);
static void popdownSavefile(Widget, XtPointer, XtPointer);
static void popdownSavePL(Widget, XtPointer, XtPointer);
static void popdownSubmenuACT(Widget, XEvent *, String *, Cardinal *);
static void popdownSubmenuCB(Widget, XtPointer, XtPointer);
static void popdownfilemenuACT(Widget, XEvent *, String *, Cardinal *);
static void popupfilemenuACT(Widget, XEvent *, String *, Cardinal *);
static void popupDialog(Widget, char *, String *, XtCallbackProc,
                        ldStorePointer);
static void prevCB(Widget, XtPointer, XtPointer);
static void quitCB(Widget, XtPointer, XtPointer);
static void randomCB(Widget, XtPointer, XtPointer);
static int readPlaylist(char *);
static void recordCB(Widget, XtPointer, XtPointer);
static void recordACT(Widget, XEvent *, String *, Cardinal *);
static void redrawACT(Widget, XEvent *, String *, Cardinal *);
static void redrawCaptionACT(Widget, XEvent *, String *, Cardinal *);
static void repeatCB(Widget, XtPointer, XtPointer);
static void resizeToplevelACT(Widget, XEvent *, String *, Cardinal *);
static void restoreDefaultOSelectionCB(Widget, XtPointer, XtPointer);
static void restoreLDPointer(Widget, XtPointer, XEvent *, Boolean *);
static void savePlaylist(char *);
static void saveformatDialog(Widget, char *, ldPointer);
static void setupWindow(Widget, String, Boolean, Boolean);
static int setDirList(ldPointer, char *);
static void setSizeHints(Dimension);
static void scrollTextACT(Widget, XEvent *, String *, Cardinal *);
static void scrollTraceACT(Widget, XEvent *, String *, Cardinal *);
static void scrollListACT(Widget, XEvent *, String *, Cardinal *);
static void setDirACT(Widget, XEvent *, String *, Cardinal *);
static void setDirLoadCB(Widget, XtPointer, XawListReturnStruct *);
static void setFileLoadCB(Widget, XtPointer, XawListReturnStruct *);
static void setThumb(Widget, barfloat);
static Widget seekTransientShell(Widget);
static void sndspecACT(Widget, XEvent *, String *, Cardinal *);
static void soundkeyACT(Widget, XEvent *, String *, Cardinal *);
static void speedACT(Widget, XEvent *, String *, Cardinal *);
static void simulateArrowsCB(Widget, XtPointer, XtPointer);
static void StartScrollACT(Widget, XEvent *, String *, Cardinal *);
static void stopCB(Widget, XtPointer, XtPointer);
static char *strmatch(char *, char *);
static void tempoCB(Widget, XtPointer, XtPointer);
#ifdef HAVE_TIP
static void TipEnable(Widget, String);
static void TipDisable(Widget);
static void xawTipSet(Boolean);
#endif /* HAVE_TIP */
static void tnotifyCB(Widget, XtPointer, XtPointer);
static void toggleMark(Widget, Boolean);
static void toggleTraceACT(Widget, XEvent *, String *, Cardinal *);
static void tunesetACT(Widget, XEvent *, String *, Cardinal *);
static void tuneslideCB(Widget, XtPointer, XtPointer);
static void upACT(Widget, XEvent *, String *, Cardinal *);
static void voiceACT(Widget, XEvent *, String *, Cardinal *);
static void volsetCB(Widget, XtPointer, XtPointer);
static void volupdownACT(Widget, XEvent *, String *, Cardinal *);
static Widget warnCB(Widget, char *, Boolean);
static void xaw_vendor_setup(void);
static void xawtipsetACT(Widget, XEvent *, String *, Cardinal *);
#ifdef XDND
static void a_dnd_init(void);
static void enable_dnd_for_widget(DndClass *, Widget, dnd_callback_t);
static void xdnd_file_drop_handler(char *);
static void xdnd_listener(Widget, XtPointer, XEvent *, Boolean *);
#endif /* XDND */

static void
offPauseButton(void) {
  Boolean s;

  XtVaGetValues(pause_b, XtNstate,&s, NULL);
  if (s == True) {
    XtVaSetValues(pause_b, XtNstate,False, NULL);
    a_pipe_write("U");
  }
}

static void
offPlayButton(void) {
  Boolean s;

  XtVaGetValues(play_b, XtNstate,&s, NULL);
  if (s == True) {
    XtVaSetValues(play_b, XtNstate,False, NULL);
    a_pipe_write("T 0");
  }
}

static Boolean
IsTracePlaying(void) {
  Boolean s;

  if ((!ctl->trace_playing) || (lockevents == True)) return False;
  XtVaGetValues(play_b, XtNstate,&s, NULL);
  return s;
}

static Boolean
IsEffectiveFile(char *file) {
  char *p2;
  struct stat st;

  if ((p2 = strrchr(file, '#')) != NULL) *p2 = '\0';
  if (stat(file, &st) != -1)
    if (st.st_mode & S_IFMT & (S_IFDIR|S_IFREG|S_IFLNK)) {
      if (p2 != NULL) *p2 = '#';
      return True;
    }
  return False;
}

static Boolean
onPlayOffPause(void) {
  Boolean s, play_on = False;

  XtVaGetValues(play_b, XtNstate,&s, NULL);
  if (s == False) {
    XtVaSetValues(play_b, XtNstate,True, NULL);
    play_on = True;
  }
  offPauseButton();
  return play_on;
}

static void
optionsCB(Widget w, XtPointer id_data, XtPointer data) {
  Boolean s;

  XtVaGetValues(w, XtNstate,&s, NULL);
  XtVaSetValues(w, XtNbitmap,s?on_mark:off_mark, NULL);
}

static void
optionspopupACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  if (popup_opt == NULL) createOptions();
  setupWindow(popup_opt, "do-optionsclose()", False, False);
}

static void
flistpopupACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  Dimension x, y;

  createFlist();
  XtVaGetValues(toplevel, XtNx,&x, XtNy,&y, NULL);
  XtVaSetValues(popup_file, XtNx,x+DEFAULT_REG_WIDTH, XtNy,y, NULL);
  setupWindow(popup_file, "do-closeparent()", True, False);
}

static void
aboutACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  char s[12], *p;
  char lbuf[30];
  int i;
  Widget popup_about, popup_abox, popup_aok, about_lbl[5];

  char *info[] = {"TiMidity++ %s%s - Xaw interface",
                  "- MIDI to WAVE converter and player -",
                  "by Masanao Izumo and Tomokazu Harada",
                  "modified by Yoshishige Arai", " ", NULL};

  if ((popup_about = XtNameToWidget(toplevel, "popup_about")) != NULL) {
    XtPopup(popup_about, XtGrabNone);
    XSetInputFocus(disp, XtWindow(popup_about), RevertToParent, CurrentTime);
    return;
  }
  popup_about = XtVaCreatePopupShell("popup_about",transientShellWidgetClass,
                                toplevel,NULL);
  popup_abox = XtVaCreateManagedWidget("popup_abox",boxWidgetClass,
                                popup_about, XtNwidth,320, XtNheight,120,
                                XtNorientation,XtorientVertical,
                                XtNbackground,bgcolor, NULL);
  for(i=0, p=info[0]; p; p=info[++i]) {
    snprintf(s, sizeof(s), "about_lbl%d", i);
    snprintf(lbuf, sizeof(lbuf), p,
    		(strcmp(timidity_version, "current")) ? "version " : "",
    		timidity_version);
    about_lbl[i] = XtVaCreateManagedWidget(s,labelWidgetClass,popup_abox,
                                XtNlabel,lbuf, XtNwidth,320, XtNresize,False,
                                XtNfontSet,app_resources.label_font,
                                XtNforeground,textcolor, XtNborderWidth,0,
                                XtNbackground,bgcolor, NULL);
  }
  popup_aok = XtVaCreateManagedWidget("OK",commandWidgetClass,popup_abox,
                                XtNwidth,320, XtNresize,False, NULL);
  XtAddCallback(popup_aok, XtNcallback,closeWidgetCB, (XtPointer)popup_about);
  XtVaSetValues(popup_about, XtNx,root_width/2 - 160,
                  XtNy,root_height/2 - 60, NULL);
  setupWindow(popup_about, "do-closeparent()", False, True);
  XtSetKeyboardFocus(popup_about, popup_abox);
}

static void
optionscloseCB(Widget w, XtPointer client_data, XtPointer call_data) {
  Boolean s1;
  id_list *result = NULL;
  int flags = 0, cflag = 0, i;

  if (play != NULL) result = (id_list *)XawToggleGetCurrent(play->formatGroup);
  for(i=0; i<MAX_OPTION_N; i++) {
    XtVaGetValues(option_num[i].widget, XtNstate,&s1, NULL);
    flags |= s1?option_num[i].bit:0;
  }
  XtVaGetValues(chorus_b, XtNstate,&s1, NULL);
  if (s1 == True) cflag = Cfg.chorusopt?Cfg.chorusopt:DEFAULT_CHORUS;
  if ((init_options != flags) || (init_chorus != cflag) ||
      (recording == True)) {
    stopCB(NULL, NULL, NULL);
  }
  if (init_options != flags) {
    init_options = flags;
    a_pipe_write("E %03d", init_options);
  }
  if (init_chorus != cflag) {
    init_chorus = cflag;
    if (s1 == False) a_pipe_write("C 0");
    else a_pipe_write("C %03d", init_chorus);
  }

  if (result != NULL) {
    a_pipe_write("p%c", result->id_char);
    while (strncmp(local_buf, "Z3", 2)) {
      XtAppProcessEvent(app_con, XtIMAll);
    }
    if (*(local_buf + 2) == 'E') goto popdownopt;
    play->def = play->current;
  }
popdownopt:
  XtPopdown(popup_opt);
}

static Widget
warnCB(Widget w, char *mesname, Boolean destroy) {
  Widget popup_warning, popup_wbox, popup_message, popup_wok;

  if (mesname == NULL) return None;
  popup_warning = XtVaCreatePopupShell("popup_warning",
            transientShellWidgetClass,toplevel, NULL);
  popup_wbox = XtVaCreateManagedWidget("popup_wbox", boxWidgetClass,
				  popup_warning, XtNbackground,bgcolor,
                                  XtNorientation,XtorientVertical, NULL);
  popup_message = XtVaCreateManagedWidget(mesname, labelWidgetClass,
                                          popup_wbox,
                                          XtNfontSet,app_resources.label_font,
                                          XtNforeground,textcolor,
                                          XtNbackground,bgcolor,
                                          XtNresize,False,
                                          XtNborderWidth,0, NULL);
  popup_wok = XtVaCreateManagedWidget("OK",commandWidgetClass,
                    popup_wbox, XtNbackground,buttonbgcolor,
                    XtNresize,False, NULL);
  XtAddCallback(popup_wok, XtNcallback,closeWidgetCB, (XtPointer)popup_warning);
  XtSetKeyboardFocus(popup_warning, popup_wbox);
  setupWindow(popup_warning, "do-closeparent()", False, destroy);
  return popup_warning;
}

static void
closeWidgetCB(Widget w, XtPointer client_data, XtPointer call_data) {
  XtPopdown((Widget)client_data);
}

static int
confirmCB(Widget w, char *mesname, Boolean multiple) {
  Widget popup_confirm, popup_cform, popup_message, popup_ccancel, popup_cok;
  char s[21];
  Dimension mw, ow, cw;

  if (mesname == NULL) return NORESPONSE;
  snprintf(s, sizeof(s), "confirm_%s", mesname);
  if ((multiple == False) && ((popup_confirm = XtNameToWidget(w, s)) != NULL)) {
    XtPopup(popup_confirm, XtGrabNone);
    XSetInputFocus(disp, XtWindow(popup_confirm), RevertToParent, CurrentTime);
    return CANCEL;
  }
  popup_confirm = XtVaCreatePopupShell(s,transientShellWidgetClass,w,
                                       XtNtitle,app_resources.popup_confirm,
                                       NULL);
  popup_cform = XtVaCreateManagedWidget("popup_cform",formWidgetClass,
                                  popup_confirm, XtNbackground,bgcolor,
                                  XtNorientation,XtorientVertical, NULL);
  popup_message = XtVaCreateManagedWidget(mesname,labelWidgetClass,
                                          popup_cform, XtNresize,False,
                                          XtNfontSet,app_resources.label_font,
                                          XtNforeground,textcolor,
                                          XtNbackground,bgcolor,
                                          XtNborderWidth,0, NULL);
  popup_cok = XtVaCreateManagedWidget("OK",commandWidgetClass,
                    popup_cform, XtNbackground,buttonbgcolor,
                    XtNresize,False, XtNfromVert,popup_message, NULL);
  popup_ccancel = XtVaCreateManagedWidget("Cancel",commandWidgetClass,
                    popup_cform, XtNbackground,buttonbgcolor,
                    XtNresize,False, XtNfromVert,popup_message, 
                    XtNfromHoriz,popup_cok, NULL);
  XtVaGetValues(popup_message, XtNwidth,&mw, NULL);
  XtVaGetValues(popup_cok, XtNwidth,&ow, NULL);
  XtVaGetValues(popup_ccancel, XtNwidth,&cw, NULL);
  if (mw > ow+cw) XtVaSetValues(popup_cok, XtNhorizDistance,(mw-ow-cw)/2, NULL);

  XtAddCallback(popup_cok, XtNcallback,okCB, (XtPointer)popup_confirm);
  XtAddCallback(popup_ccancel, XtNcallback,cancelCB, (XtPointer)popup_confirm);
  XtSetKeyboardFocus(popup_confirm, popup_cform);
  setupWindow(popup_confirm, "do-cancel()", False, True);
  response.val = NORESPONSE;
  while ((response.val == NORESPONSE) || (response.widget != popup_confirm)) {
    XtAppProcessEvent(app_con, XtIMAll);
  }
  XtPopdown(popup_confirm);
  return response.val;
}

static void
okCB(Widget w, XtPointer client_data, XtPointer call_data) {
  response.widget = (Widget)client_data;
  response.val = OK;
}

static void
okACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  response.widget = seekTransientShell(w);
  response.val = OK;
}

static void
cancelCB(Widget w, XtPointer client_data, XtPointer call_data) {
  response.widget = (Widget)client_data;
  response.val = CANCEL;
}

static void
cancelACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  response.widget = seekTransientShell(w);
  response.val = CANCEL;
}

static void
quitCB(Widget w, XtPointer client_data, XtPointer call_data) {
  if (Cfg.confirmexit == True) {
#ifdef XAW3D
      XtPopdown(file_sm); 
     /* Otherwise, when selecting "Quit" from the file menu, the menu
      * may obscure the confirm dialog on XAW3D v1.5.
      */
#endif
     if (confirmCB(toplevel, "confirmexit", False) != OK) return;
  }
  a_pipe_write("Q");
}

static void
sndspecACT(Widget w, XEvent *e, String *v, Cardinal *n) {
#ifdef SUPPORT_SOUNDSPEC
  a_pipe_write("g");
#endif
}

static void
playCB(Widget w, XtPointer client_data, XtPointer call_data) {
  float thumb;

  if (max_files == 0) return;
  onPlayOffPause();
  XtVaGetValues(tune_bar, XtNtopOfThumb,&thumb, NULL);
  if (thumb != 0) {
    a_pipe_write("P");
    a_pipe_write("U");
    a_pipe_sync();
    a_pipe_write("T %d", (int)(total_time * thumb));
    a_pipe_write("U");
  }
  else a_pipe_write("P");
}

static void
pauseACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  Boolean s;

  XtVaGetValues(play_b, XtNstate,&s, NULL);
  if ((e->type == KeyPress) && (s == True)) {
    XtVaGetValues(pause_b, XtNstate,&s, NULL);
    s ^= True;
    XtVaSetValues(pause_b, XtNstate,s, NULL);
    halt = s?1:0;
    a_pipe_write("U");
  }
}

static void
soundkeyACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  if (*(int *)n == 0) {
    if (IsTracePlaying())
      XtCallActionProc(keyup_b, (String)"set", NULL, NULL, ZERO);
    a_pipe_write("+");
  } else {
    if (IsTracePlaying())
      XtCallActionProc(keydown_b, (String)"set", NULL, NULL, ZERO);
    a_pipe_write("-");
  }
}

static void
speedACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  if (*(int *)n == 0) {
    if (IsTracePlaying())
      XtCallActionProc(fast_b, (String)"set", NULL, NULL, ZERO);
    a_pipe_write(">");
  } else {
    if (IsTracePlaying())
      XtCallActionProc(slow_b, (String)"set", NULL, NULL, ZERO);
    a_pipe_write("<");
  }
}

static void
voiceACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  a_pipe_write(*(int *)n == 0 ? "O":"o");
}

static void
pauseCB(Widget w, XtPointer client_data, XtPointer call_data) {
  Boolean s;

  XtVaGetValues(play_b, XtNstate,&s, NULL);
  if (s == True) {
    halt = 1;
    a_pipe_write("U");
  }
  XtVaGetValues(pause_b, XtNstate,&s, NULL);
  if (s == False) halt = 0;
}

static void
stopCB(Widget w, XtPointer client_data, XtPointer call_data) {
  offPlayButton();
  offPauseButton();
  a_pipe_write("S");
  lockevents = True;
  if (recording == True) a_pipe_write("wS");
  if (ctl->trace_playing) initStatus();
  XtVaSetValues(tune_l0, XtNlabel,"0:00", NULL);
  XawScrollbarSetThumb(tune_bar, 0.0, -1.0);
  snprintf(window_title, sizeof(window_title), "%s : %s",
           APP_CLASS, app_resources.no_playing);
  XtVaSetValues(toplevel, XtNtitle,window_title, NULL);
  callRedrawTrace(False);
}

static void
nextCB(Widget w, XtPointer client_data, XtPointer call_data) {
  onPlayOffPause();
  a_pipe_write("N");
  lockevents = True;
  if (ctl->trace_playing) initStatus();
}

static void
prevCB(Widget w, XtPointer client_data, XtPointer call_data) {
  onPlayOffPause();
  a_pipe_write("B");
  lockevents = True;
  if (ctl->trace_playing) initStatus();
}

static void
forwardCB(Widget w, XtPointer client_data, XtPointer call_data) {
  if (onPlayOffPause()) a_pipe_write("P");
  a_pipe_write("f");
  if (ctl->trace_playing) initStatus();
}

static void
backCB(Widget w, XtPointer client_data, XtPointer call_data) {
  if (onPlayOffPause()) a_pipe_write("P");
  a_pipe_write("b");
  if (ctl->trace_playing) initStatus();
}

static void
repeatCB(Widget w, XtPointer client_data, XtPointer call_data) {
  Boolean s;
  Boolean *set = (Boolean *)client_data;

  if (set != NULL) {
    s = *set;
    XtVaSetValues(repeat_b, XtNstate,s, NULL);
    toggleMark(file_menu[ID_REPEAT-100].widget, s);
  } else {
    XtVaGetValues(repeat_b, XtNstate,&s, NULL);
    toggleMark(file_menu[ID_REPEAT-100].widget, s);
    Cfg.repeat = s;
  }
  if (s == True) a_pipe_write("R 1");
  else a_pipe_write("R 0");
}

static void
randomCB(Widget w, XtPointer client_data, XtPointer call_data) {
  Boolean s;
  Boolean *set = (Boolean *)client_data;

  onPlayOffPause();
  if (set != NULL) {
    s = *set;
    XtVaSetValues(random_b, XtNstate,s, NULL);
    toggleMark(file_menu[ID_SHUFFLE-100].widget, s);
  } else {
    XtVaGetValues(random_b, XtNstate,&s, NULL);
    toggleMark(file_menu[ID_SHUFFLE-100].widget, s);
    Cfg.shuffle = s;
  }
  if (s == True) {
    onPlayOffPause();
    a_pipe_write("D 1");
  } else {
    offPlayButton();
    offPauseButton();
    a_pipe_write("D 2");
  }
}

static void
tempoCB(Widget w, XtPointer client_data, XtPointer call_data) {
  a_pipe_write( (client_data == (XtPointer)TRUE) ? ">":"<");
}

static void
pitchCB(Widget w, XtPointer client_data, XtPointer call_data) {
  a_pipe_write( (client_data == (XtPointer)TRUE) ? "+":"-");
}

static void
menuCB(Widget w, XtPointer client_data, XtPointer call_data) {
  onPlayOffPause();
  lockevents = True;
  if (ctl->trace_playing) initStatus();
  a_pipe_write("L %ld", ((long)client_data)+1);
}

static void
setVolbar(int val) {
  char s[8];
  barfloat thumb;

  amplitude = (val > MAXVOLUME)? MAXVOLUME : val;
  a_pipe_write("V %03d", amplitude);
  snprintf(s, sizeof(s), "%d", amplitude);
  XtVaSetValues(vol_l, XtNlabel,s, NULL);
  thumb.f = (float)amplitude / (float)MAXVOLUME;
  setThumb(vol_bar, thumb);
}

static void
volsetCB(Widget w, XtPointer client_data, XtPointer percent_ptr) {
  float percent = *(float *)percent_ptr;
  int val = MAXVOLUME * percent;

  if (amplitude == val) return;
  setVolbar(val);
}

static void
volupdownACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  int i = atoi(*v);

  i += amplitude;
  setVolbar(i);
}

static void
tunesetACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  int click_y;
  Dimension h;
  barfloat thumb;

  if (!halt) return;
  halt = 0;
  XtVaGetValues(tune_bar, XtNtopOfThumb,&thumb.f, XtNheight,&h, NULL);
  click_y = e->xbutton.y;
  if ((click_y > h) || (click_y < 0)) {
    char s[10];

    snprintf(s, sizeof(s), "%d:%02d", curr_time / 60, curr_time % 60);
    XtVaSetValues(tune_l0, XtNlabel,s, NULL);
    thumb.f = (float)curr_time / (float)total_time;
    setThumb(tune_bar, thumb);
    return;
  }
  a_pipe_write("T %d", (int)(total_time * thumb.f));
  /*  local_buf[0] = '\0';
  a_print_text(lyric_t, local_buf);*/
}

static void
tuneslideCB(Widget w, XtPointer client_data, XtPointer percent_ptr) {
  char s[16];
  int value;
  float l_thumb = *(float *)percent_ptr;

  halt = 1;
  value = l_thumb * total_time;
  snprintf(s, sizeof(s), "%d:%02d", value / 60, value % 60);
  XtVaSetValues(tune_l0, XtNlabel,s, NULL);
}

static void
setSizeHints(Dimension height) {
  XSizeHints *xsh;

  xsh = XAllocSizeHints();
  if (xsh == NULL) return;

  xsh->flags = PMaxSize/* | PMinSize*/;
  if (Cfg.disptrace == False) {
    xsh->max_width = root_width;
    xsh->min_height = base_height;
  } else {
    xsh->max_width = TRACE_WIDTH+8;
    xsh->min_height = base_height + trace_v_height;
  }
  xsh->min_width = DEFAULT_REG_WIDTH;
  if (XtIsManaged(lyric_t)) xsh->max_height = root_height;
  else xsh->max_height = height;

  XSetWMNormalHints(disp, XtWindow(toplevel), xsh);
  XFree(xsh);
  return;
}

static void
resizeToplevelACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  XConfigureEvent *xce = (XConfigureEvent *)e;

  if (xce != NULL) {
    if ((xce->width == curr_width) && (xce->height == curr_height))
      return;
    curr_width = xce->width;
    curr_height = xce->height;
  }

  XawFormDoLayout(base_f, False);
  setSizeHints(curr_height);

  if (XtIsManaged(lyric_t)) {
    if (Cfg.disptrace == True) {
      if (base_height + trace_v_height + 4 > curr_height) lyric_height = 4;
      else lyric_height = curr_height - base_height - trace_v_height;
    } else {
      if (base_height + 4 > curr_height) lyric_height = 4;
      else lyric_height = curr_height - base_height;
    }
    XtResizeWidget(lyric_t, curr_width-10,lyric_height, 1);
    XtVaGetValues(lyric_t, XtNheight,&lyric_height, NULL);
  }

  if (Cfg.disptrace == True) {
    XtManageChild(trace_vport);
    XtVaSetValues(trace_vport, XtNtop,XawChainBottom, NULL);
  }

  XawFormDoLayout(base_f, True);
  XSync(disp, False);
}

#ifndef WIDGET_IS_LABEL_WIDGET
static void
a_print_text(Widget w, char *st) {
  XawTextPosition pos;
  XawTextBlock tb;

  st = strcat(st, "\n");
  pos = XawTextGetInsertionPoint(w);
  tb.firstPos = 0;
  tb.length = strlen(st);
  tb.ptr = st;
  tb.format = XawFmt8Bit;
  XawTextReplace(w, pos, pos, &tb);
  XawTextSetInsertionPoint(w, pos + tb.length);
}
#else
static void
a_print_text(Widget w, char *st) {
  XtVaSetValues(w, XtNlabel,st, NULL);
}
#endif /* !WIDGET_IS_LABEL_WIDGET */

static ldStorePointer
init_ldS(void) {
  ldStorePointer p;
  p = (ldStorePointer)safe_malloc(sizeof(ldStore));
  p->name = NULL;
  return p;
}

#if 0
static void
free_ldS(ldStorePointer ldS) {
  if (ldS == NULL) return;
  if (ldS->name == NULL) free(ldS);
  else {
    free_ldS( (ldStorePointer)ldS->next);
    free(ldS->fdirlist); free(ldS->ddirlist); free(ldS->fulldirlist);
    free(ldS->ld);
    free(ldS->name);
    free(ldS);
  }
  return;
}
#endif

static ldStorePointer
getldsPointer(ldStorePointer lds, char *Popname) {
  if ((lds == NULL) || (Popname == NULL)) {
    fprintf(stderr, "getldPointer received NULL parameter!\n");
    exit(1);
  }
  if (lds->name == NULL) {
    lds->name = safe_strdup(Popname);
    lds->ld = (ldPointer)safe_malloc(sizeof(load_dialog));
    strlcpy(lds->ld->ld_popupname, Popname, MAX_POPUPNAME);
    strlcpy(lds->ld->ld_basepath, Cfg.DefaultDir, sizeof(lds->ld->ld_basepath));
    lds->ld->ld_cur_filter[0] = '\0';
    if (strcmp(LISTDIALOGBASENAME, Popname))
      strlcpy(lds->ld->ld_filter, "*.mid", 6);
    else strlcpy(lds->ld->ld_filter, "*.tpl", 6);
    lds->ld->ld_popup_load = NULL;
    lds->ld->ld_fdirlist.StringArray = NULL;
    lds->ld->ld_ddirlist.StringArray = NULL;
    lds->ld->ld_fulldirlist.StringArray = NULL;
    lds->next = (struct ldStore *)init_ldS();
    return lds;
  }
  if (!strncmp(lds->name, Popname, MAX_POPUPNAME)) return lds;
  return getldsPointer( (ldStorePointer)lds->next, Popname);
}

static void
restoreLDPointer(Widget w, XtPointer client_data, XEvent *ev, Boolean *bool) {
  if (ev->xany.type == FocusIn) ld = (ldPointer)client_data;
  return;
}

static void
createDialog(Widget w, ldPointer ld) {
  Position popup_x, popup_y, top_x, top_y;
  Widget load_l,load_t;

  XtVaGetValues(toplevel, XtNx,&top_x, XtNy,&top_y, XtNwidth,&ldwidth, NULL);
  ldheight = POPUP_HEIGHT;
  popup_x = top_x + 20 + ran();
  popup_y = top_y + 72 + ran();
  if (popup_x+ldwidth > root_width)
        popup_x = root_width - ldwidth - 20;
  if (popup_y+ldheight > root_height)
        popup_y = root_height - POPUP_HEIGHT - 20;
  ldwidth += 100-4;

  popup_load = XtVaCreatePopupShell(popupname, transientShellWidgetClass,
               toplevel, XtNx,popup_x, XtNy,popup_y, XtNheight,ldheight, NULL);
  load_d = XtVaCreateManagedWidget("load_dialog",dialogWidgetClass,
           popup_load, XtNbackground,bgcolor, XtNresize,True,
           XtNvalue,"", NULL);
          /* We set XtNvalue because we need load_t to exist now so it
           * can be given the correct size.
           */
  load_t = XtNameToWidget(load_d, "value");
  load_l = XtNameToWidget(load_d, "label");
  load_ok = XtVaCreateManagedWidget("OK",commandWidgetClass,load_d, NULL);
  XawDialogAddButton(load_d, "add", popdownAddALL, (XtPointer)ld);
  XawDialogAddButton(load_d, "Cancel", popdownCB, (XtPointer)ld);
  load_b = XtVaCreateManagedWidget("load_button",toggleWidgetClass,load_d,
          XtNforeground,togglecolor, XtNbackground,buttonbgcolor, NULL);

  cwd_l = XtVaCreateManagedWidget("cwd_label",labelWidgetClass,
             load_d, XtNlabel,basepath, XtNborderWidth,0,
             XtNfromVert,load_b, XtNwidth,ldwidth, XtNheight,INFO_HEIGHT,
             XtNbackground,text2bgcolor, XtNresize,False, NULL);
  load_pane = XtVaCreateManagedWidget("pane",panedWidgetClass,load_d,
             XtNfromVert,cwd_l, XtNwidth,ldwidth, XtNheight,PANE_HEIGHT,
             XtNorientation,XtorientHorizontal, NULL);
  load_vportdir = XtVaCreateManagedWidget("vdport",viewportWidgetClass,
             load_pane, XtNallowHoriz,True, XtNallowVert,True,
             XtNbackground,textbgcolor, XtNpreferredPaneSize,ldwidth/5, NULL);
  load_vport = XtVaCreateManagedWidget("vport",viewportWidgetClass,
             load_pane, XtNallowHoriz,True, XtNallowVert,True,
             XtNbackground,textbgcolor, XtNpreferredPaneSize,ldwidth*4/5, NULL);
  load_dlist = XtVaCreateManagedWidget("dirs",listWidgetClass,load_vportdir,
             XtNverticalList,True, XtNforceColumns,True,
             XtNbackground,textbgcolor, XtNdefaultColumns,1, NULL);
  load_flist = XtVaCreateManagedWidget("files",listWidgetClass,load_vport,
             XtNverticalList,True, XtNforceColumns,False,
             XtNbackground,textbgcolor, XtNdefaultColumns,3, NULL);
  load_info = XtVaCreateManagedWidget("cwd_info",labelWidgetClass,
             load_d, XtNborderWidth,0, XtNwidth,ldwidth,
             XtNheight,INFO_HEIGHT, XtNresize,False,
             XtNbackground,text2bgcolor, XtNfromVert,load_pane, NULL);
  XtVaSetValues(load_t, XtNwidth,(int)(ldwidth/1.5), NULL);
  XtVaSetValues(load_l, XtNwidth,(int)(ldwidth/1.5), NULL);

  XtAddCallback(load_flist, XtNcallback,
                 (XtCallbackProc)setFileLoadCB, (XtPointer)ld); 
  XtAddCallback(load_dlist, XtNcallback,
                 (XtCallbackProc)setDirLoadCB, (XtPointer)ld); 
  XtAddCallback(load_b, XtNcallback,callFilterDirList, (XtPointer)ld);
  XtInstallAccelerators(load_t, load_b);
  XtAddEventHandler(popup_load, FocusChangeMask, False,
                    restoreLDPointer, (XtPointer)ld);
}

static void
popupDialog(Widget w, char *Popname, String *title,
            XtCallbackProc OKfunc, ldStorePointer ldS) {
  ldPointer ldlocal;

  ldlocal = getldsPointer(ldS, Popname)->ld;
  ld = ldlocal;
  if (popup_load == NULL) createDialog(w, ldlocal);

  XtRemoveAllCallbacks(load_ok,XtNcallback);
  XtAddCallback(load_ok, XtNcallback,OKfunc, (XtPointer)ldlocal);
  if (title != NULL) XtVaSetValues(popup_load, XtNtitle,*title, NULL);

  setDirList(ldlocal, basepath);
  XtVaSetValues(cwd_l, XtNlabel,basepath, NULL);

  setupWindow(popup_load, "MenuPopdown()", False, False);
  /* uses MenuPopdown() (see intrinsics p.88) becuase FocusIn events
   * are not always recieved on pressing the X button so ld can be
   * incorrect for do-popdown().
   */

  XtVaSetValues(load_flist, XtNwidth,0, XtNheight,0, NULL);
  XtVaSetValues(load_dlist, XtNwidth,0, XtNheight,0, NULL);
  /* Neccesary to bypass Xaw8 bug: The listwidget will always inherit
   * width and height regardless of original setting. This should be
   * harmless on other implementations.
   */
}

static void
popdownCB(Widget w, XtPointer client_data, XtPointer call_data) {
  XtPopdown(popup_load); /* uses global ld */
}

static void
popdownAddALLACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  popdownAddALL(w, ld, NULL); /* uses global ld */
}

static void
popdownAddALL(Widget w, XtPointer client_data, XtPointer call_data) {
  char *p;
  ldPointer ld = (ldPointer)client_data;
  String *filelist = fdirlist;
  Boolean toggle;

  XtVaGetValues(load_b, XtNstate,&toggle, NULL);
  if ((toggle == False) || (filelist == NULL)) {
    a_pipe_write("X %s/", basepath);
    /*
     * Speed shortcut - timidity internals can get a directory name, and
     * get the midi files from it. Otherwise, all the filenames in the
     * directory would have been passed through the pipe which
     * can be slow for a very large amount of files. The alternative would
     * involve something like:
     * filelist = fulldirlist; (but the case of filelist == NULL can be more
     * complicated).
     */
    goto addallpopdown;
  }
  while ((p = *(filelist++)) != NULL) {
    a_pipe_write("X %s/%s", basepath, p);
  }
addallpopdown:
  XtPopdown(popup_load);
}

static void
popdownLoadfile(Widget w, XtPointer client_data, XtPointer call_data) {
  char *p, *p2;
  ldPointer ld = (ldPointer)client_data;

  p = XawDialogGetValueString(load_d);
  if ((!strncmp(p, "http:", 5)) || (!strncmp(p, "ftp:", 4))) goto lfiledown;
  if ((p2 = expandDir(p, NULL, basepath)) != NULL) p = p2;
  if (!IsEffectiveFile(p)) {
    char *s = strrchr(p, '/');

    if (s == NULL) return;
    else s++;
    p2 = s;
    while ((*s) != '\0') {
      if ((*s == '*') || (*s == '?')) {
        strlcpy(filter, p2, sizeof(filter));
        XtVaSetValues(load_b, XtNstate,True, NULL);
        filterDirList(w, ld, True);
        return;
      }
      s++;
    }
  }
lfiledown:
  a_pipe_write("X %s", p);
#ifdef CLEARVALUE
  clearValue(load_d);
#endif /* CLEARVALUE */
  XtVaSetValues(load_d, XtNvalue,"", NULL);
  XtPopdown(popup_load);
}

static void
popdownSavefile(Widget w, XtPointer client_data, XtPointer call_data) {
  char *p;
  char lbuf[PIPE_LENGTH];
  struct stat st;
  ldPointer ld = (ldPointer)client_data;

  p = XawDialogGetValueString(XtParent(w));
  snprintf(lbuf, sizeof(lbuf), "%s/%s", basepath, p);
  if (stat(lbuf, &st) != -1) {
    if (st.st_mode & S_IFMT & (S_IFREG|S_IFLNK)) {
      if (confirmCB(popup_load, "warnoverwrite", True) != OK) return;
    }
    else return;
  }

  saveformatDialog(popup_load, lbuf, ld);
}

static void
popdownLoadPL(Widget w, XtPointer client_data, XtPointer call_data) {
  char *p, *p2;
  ldPointer ld = (ldPointer)client_data;

  p = XawDialogGetValueString(load_d);
  if ((p2 = expandDir(p, NULL, basepath)) != NULL) p = p2;
  if ( (IsEffectiveFile(p)) && (!readPlaylist(p)) ) {
#ifdef CLEARVALUE
    clearValue(load_d);
#endif /* CLEARVALUE */
    XtVaSetValues(load_d, XtNvalue,"", NULL);
    XtPopdown(popup_load);
  } else {
    char *s = strrchr(p, '/');

    if (s == NULL) return;
    else s++;
    p2 = s;
    while ((*s) != '\0') {
      if ((*s == '*') || (*s == '?')) {
        strlcpy(filter, p2, sizeof(filter));
        XtVaSetValues(load_b, XtNstate,True, NULL);
        filterDirList(w, ld, True);
        return;
      }
      s++;
    }
  }
}

static void
popdownSavePL(Widget w, XtPointer client_data, XtPointer call_data) {
  char *p;
  char lbuf[PIPE_LENGTH];
  struct stat st;
  ldPointer ld = (ldPointer)client_data;

  p = XawDialogGetValueString(XtParent(w));
  snprintf(lbuf, sizeof(lbuf), "%s/%s", basepath, p);
  if (stat(lbuf, &st) != -1) {
    if (st.st_mode & S_IFMT & (S_IFREG|S_IFLNK)) {
      if (confirmCB(popup_load, "warnoverwrite", True) != OK) return;
    }
    else return;
  }
  a_pipe_write("s %s", lbuf);
#ifdef CLEARVALUE
  clearValue(XtParent(w));
#endif /* CLEARVALUE */
  XtVaSetValues(XtParent(w), XtNvalue,"", NULL);
  XtPopdown(popup_load);
}

static void
saveformatDialog(Widget parent, char *lbuf, ldPointer ld) {
  Widget popup_sform, popup_sformat, popup_slabel, sbox_rbox, sbox_ratelabel,
         sbox_ratetext, popup_sbuttons, popup_sok, popup_scancel, lowBox;

  if ((recording == True) ||
      ((popup_sformat = XtNameToWidget(parent, "popup_sformat")) != NULL)) {
    warnCB(parent, "warnrecording", True);
    return;
  }

  popup_sformat = XtVaCreatePopupShell("popup_sformat",
            transientShellWidgetClass,parent, NULL);
  popup_sform = XtVaCreateManagedWidget("popup_sform",formWidgetClass,
            popup_sformat, XtNbackground,bgcolor, XtNwidth,200, NULL);
  popup_slabel = XtVaCreateManagedWidget("popup_slabel",labelWidgetClass,
            popup_sform, XtNbackground,menubcolor, NULL);

  lowBox = createOutputSelectionWidgets(popup_sformat, popup_sform,
                                             popup_slabel, record, False);

  sbox_rbox = XtVaCreateManagedWidget("sbox_rbox",boxWidgetClass,popup_sform,
                                           XtNorientation,XtorientVertical,
                                           XtNbackground,bgcolor,
                                           XtNfromVert,lowBox,
                                           XtNborderWidth,0, NULL);
  sbox_ratelabel = XtVaCreateManagedWidget("sbox_ratelabel",labelWidgetClass,
                                              sbox_rbox, XtNborderWidth,0,
                                              XtNforeground,textcolor,
                                              XtNbackground,bgcolor, NULL);
  sbox_ratetext = XtVaCreateManagedWidget("sbox_ratetext",asciiTextWidgetClass,
                                              sbox_rbox,
                                              XtNdisplayNonprinting,False,
                                              XtNfromHoriz,sbox_ratelabel,
                                              XtNstring,(String)S(DEFAULT_RATE),
                                              XtNbackground,textbgcolor,
                                              XtNforeground,textcolor,
                                              XtNeditType,XawtextEdit, NULL);
  XtCallActionProc(sbox_ratetext, (String)"end-of-line", NULL, NULL, ZERO);
  XtInstallAccelerators(sbox_ratetext, record->formatGroup);

  popup_sbuttons = XtVaCreateManagedWidget("popup_sbuttons",boxWidgetClass,
                                           popup_sform, XtNbackground,bgcolor,
                                           XtNorientation,XtorientHorizontal,
                                           XtNfromVert,sbox_rbox,
                                           XtNborderWidth,0, NULL);
  popup_sok = XtVaCreateManagedWidget("OK",commandWidgetClass,popup_sbuttons,
                    XtNbackground,buttonbgcolor, XtNresize,False,
                    XtNfromVert,sbox_rbox, XtNwidth,90, NULL);
  popup_scancel = XtVaCreateManagedWidget("Cancel",commandWidgetClass,
                    popup_sbuttons, XtNbackground,buttonbgcolor,
                    XtNresize,False, XtNfromVert,sbox_rbox,
                    XtNfromHoriz,popup_sok, XtNwidth,90, NULL);

  record->lbuf = safe_strdup(lbuf);
  XtAddCallback(popup_sok, XtNcallback,recordCB, (XtPointer)sbox_ratetext);
  XtAddCallback(popup_scancel, XtNcallback,closeWidgetCB,
                (XtPointer)popup_sformat);

  setupWindow(popup_sformat, "do-closeparent()", False, True);
  XtSetKeyboardFocus(popup_sformat, sbox_ratetext);
}

static void
recordCB(Widget w, XtPointer client_data, XtPointer call_data) {
  String rate;
  Widget warning;
  id_list *result;
  int i;

  if (client_data != NULL) w = (Widget)client_data;
  result = (id_list *)XawToggleGetCurrent(record->formatGroup);
  XtVaGetValues(w, XtNstring,&rate, NULL);
  i = atoi(rate);
  if ((i < MIN_OUTPUT_RATE) || (i > MAX_OUTPUT_RATE)) return;
  if (recording == True) {
    warnCB(toplevel, "warnrecording", True);
    return;
  }
  recording = True;
  snprintf(local_buf, sizeof(local_buf), "W%c%d %s", result->id_char,
           i, record->lbuf);
  w = seekTransientShell(w);
  XtPopdown(XtParent(w));
  XtPopdown(w);

  stopCB(NULL, NULL, NULL);
  warning = warnCB(toplevel, "waitforwav", False);
  a_pipe_write("%s", local_buf);
  while (strncmp(local_buf, "Z1", 2)) {
    XtAppProcessEvent(app_con, XtIMAll);
  }
  if (*(local_buf + 2) == 'E') goto savend;
#ifdef CLEARVALUE
  clearValue(load_d);
#endif /* CLEARVALUE */
  XtVaSetValues(load_d, XtNvalue,"", NULL);
  a_pipe_write("P");
  while (strncmp(local_buf, "Z2", 2)) {
    XtAppProcessEvent(app_con, XtIMAll);
  }
savend:
  XtDestroyWidget(warning);
  a_pipe_write("w");
  nextCB(NULL, NULL, NULL);
  stopCB(NULL, NULL, NULL);
  recording = False;
}

static void
recordACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  recordCB(w, NULL, NULL);
}

static void
scrollListACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  int i = atoi(*v);
  Widget scrollbar;
  int x, y;
  Window win;

  XTranslateCoordinates(disp, XtWindow(w), XtWindow(XtParent(w)),
                        e->xbutton.x, e->xbutton.y, &x, &y, &win);

  scrollbar = XtNameToWidget(XtParent(w), "vertical");
  if (scrollbar != NULL) {
    e->xbutton.y = y;
  } else {
    scrollbar = XtNameToWidget(XtParent(w), "horizontal");
    if (scrollbar == NULL) return;
    e->xbutton.x = x;
  }

  if (i > 0) {
    String arg[1];
    arg[0] = XtNewString("Forward");
    XtCallActionProc(scrollbar, (String)"StartScroll", e, arg, ONE);
    XtFree(arg[0]);
    if (use_own_start_scroll) {
      XtCallActionProc(scrollbar, (String)"NotifyThumb", e, NULL, ZERO);
    } else {
      arg[0] = XtNewString("Proportional");
      XtCallActionProc(scrollbar, (String)"NotifyScroll", e, arg, ONE);
      XtFree(arg[0]);
    }
    XtCallActionProc(scrollbar, (String)"EndScroll", e, NULL, ZERO);
  } else {
    String arg[1];
    arg[0] = XtNewString("Backward");
    XtCallActionProc(scrollbar, (String)"StartScroll", e, arg, ONE);
    XtFree(arg[0]);
    if (use_own_start_scroll) {
      XtCallActionProc(scrollbar, (String)"NotifyThumb", e, NULL, ZERO);
    } else {
      arg[0] = XtNewString("Proportional");
      XtCallActionProc(scrollbar, (String)"NotifyScroll", e, arg, ONE);
      XtFree(arg[0]);
    }
    XtCallActionProc(scrollbar, (String)"EndScroll", e, NULL, ZERO);
  }
}

static void
toggleMark(Widget w, Boolean value) {
  XtVaSetValues(w, XtNleftBitmap,(value == True)?check_mark:None, NULL);
}

static void
filemenuACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  int i = atoi(*v);

  filemenuCB(file_menu[i-100].widget, (XtPointer)&file_menu[i-100].id, NULL);
}

static void
popupfilemenuACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  XtCallActionProc(file_mb, (String)"reset", e, NULL, ZERO);
  XtCallActionProc(file_mb, (String)"PopupMenu", e, NULL, ZERO);
}

static void
popdownfilemenuACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  XtCallActionProc(file_mb, (String)"reset", e, NULL, ZERO);
  XtCallActionProc(file_sm, (String)"MenuPopdown", e, NULL, ZERO);
}

static void
filemenuCB(Widget w, XtPointer client_data, XtPointer call_data) {
  int *id = (int *)client_data;

  switch (*id) {
    case ID_LOAD:
      popupDialog(w, "dialog_lfile", NULL, popdownLoadfile, ldSstart);
      break;
    case ID_SAVE:
      if ((record == NULL) || (max_files == 0)) return;
      popupDialog(w, "dialog_sfile", NULL, popdownSavefile, ldSstart);
      break;
    case ID_LOAD_PLAYLIST:
      popupDialog(w, LISTDIALOGBASENAME, 
                &app_resources.load_LISTDIALOGBASENAME_title,
                popdownLoadPL, ldSstart);
      break;
    case ID_SAVE_PLAYLIST:
      popupDialog(w, LISTDIALOGBASENAME, 
                 &app_resources.save_LISTDIALOGBASENAME_title,
                 popdownSavePL, ldSstart);
      break;
    case ID_AUTOSTART:
      Cfg.autostart ^= True;
      toggleMark(w, Cfg.autostart);
      break;
    case ID_AUTOQUIT:
      Cfg.autoexit ^= True;
      toggleMark(w, Cfg.autoexit);
      a_pipe_write("q");
      break;
    case ID_HIDETRACE:
      XawFormDoLayout(base_f, False);
      if (!ctl->trace_playing) {
        Boolean s;

        XtVaSetValues(b_box, XtNleft,XawRubber, XtNright,XawRubber, NULL);
        createTraceWidgets();
       /*
        * trace_vport should be unmanaged before calling XResizeWindow, with
        * XtNtop set to XawChainTop else xaw tends to place it on the wrong
        * place (typically where lyric_t used to be before calling
        * XResizeWindow). After the resize, we'll remanage trace_vport, and
        * set XtNtop to XawChainBottom (useful in case of resizing).
        */
        XtUnmanageChild(trace_vport);
        callInitTrace();
        ctl->trace_playing = 1;
#ifdef HAVE_TIP
        xawTipSet(Cfg.tooltips);
#endif /* HAVE_TIP */
       /*
        * In the case lyric_t is not managed, the maximum height has been 
        * defined to curr_height by setSizeHints. We need to increase it 
        * otherwise the trace may not be shown - the WM may cap
        * XConfigureEvent->height to
        * maximum_height - WINDOW_DECORATIONS_HEIGHT.
        */
        if (!XtIsManaged(lyric_t)) setSizeHints(root_height);
        XResizeWindow(disp, XtWindow(toplevel),
                      TRACE_WIDTH+8, curr_height + trace_v_height);
        XtVaGetValues(play_b, XtNstate,&s, NULL);
        if (s == True) a_pipe_write("tR");
        else a_pipe_write("t");
        toggleMark(w, True);
        return;
      }
     /*
      * We must do this via XResizeWindow, as we are unable to calculate
      * lyric_height correctly here in the case
      * (trace_v_height + curr_height) is too big.
      *
      * lyric_height = curr_height - base_height -
      * (XtIsManaged(trace_vport))?trace_v_height:0.
      *
      * curr_height should be capped to max_height - WINDOW_DECORATIONS_HEIGHT
      *
      * We do not know WDH, so we cannot avoid overflow leading to
      * display artifacts when (trace_v_height + curr_height)
      * is larger than the cap.
      * However, most of these WMs will not return an XConfigureEvent->
      * height greater than that cap, so resizeToplevelAction can
      * calculate a correct lyric_height.
      */
      if (XtIsManaged(trace_vport)) {
        XtUnmanageChild(trace_vport);
        XtUnmanageChild(fast_b); XtUnmanageChild(slow_b);
        XtUnmanageChild(keyup_b); XtUnmanageChild(keydown_b);
        XtVaSetValues(b_box, XtNleft,XawChainLeft, XtNright,XawChainLeft, NULL);
        XResizeWindow(disp, XtWindow(toplevel), DEFAULT_REG_WIDTH,
                      curr_height - trace_v_height);
        Cfg.disptrace = False;
      } else {
        XtVaSetValues(trace_vport, XtNtop,XawChainTop, NULL);
        if (!XtIsManaged(lyric_t)) setSizeHints(root_height);
        XtVaSetValues(b_box, XtNleft,XawRubber, XtNright,XawRubber, NULL);
        XtManageChild(fast_b); XtManageChild(slow_b);
        XtManageChild(keyup_b); XtManageChild(keydown_b);
        XResizeWindow(disp, XtWindow(toplevel),
                      TRACE_WIDTH+8, curr_height + trace_v_height);
        Cfg.disptrace = True;
      }
      toggleMark(w, !Cfg.disptrace);
      break;
    case ID_HIDETXT:
      XawFormDoLayout(base_f, False);
      if (Cfg.disptrace == True) {
        XtUnmanageChild(trace_vport);
        XtVaSetValues(trace_vport, XtNtop,XawChainTop, NULL);
      }
      if (XtIsManaged(lyric_t)) {
        if (ctl->trace_playing)
          XtVaSetValues(trace_vport, XtNfromVert,t_box, NULL);
        XtUnmanageChild(lyric_t);
        XResizeWindow(disp, XtWindow(toplevel),
                      curr_width, curr_height - lyric_height);
        Cfg.disptext = False;
      } else {
        setSizeHints(curr_height + lyric_height);
        if (ctl->trace_playing)
          XtVaSetValues(trace_vport, XtNfromVert,lyric_t, NULL);
        XtManageChild(lyric_t);
        XResizeWindow(disp, XtWindow(toplevel),
                      curr_width, curr_height + lyric_height);
        Cfg.disptext = True;
      }
      toggleMark(w, !Cfg.disptext);
      break;
    case ID_SAVECONFIG:
      a_saveconfig(dotfile, Cfg.save_list);
      break;
    case ID_SHUFFLE:
      Cfg.shuffle ^= True;
      randomCB(NULL, (XtPointer)&Cfg.shuffle, NULL);
      break;
    case ID_REPEAT:
      Cfg.repeat ^= True;
      repeatCB(NULL, (XtPointer)&Cfg.repeat, NULL);
      break;
    case ID_OPTIONS:
      optionspopupACT(w, NULL, NULL, NULL);
      break;
    case ID_FILELIST:
      flistpopupACT(w, NULL, NULL, NULL);
      break;
    case ID_ABOUT:
      aboutACT(w, NULL, NULL, NULL);
      break;
    case ID_QUIT:
      quitCB(w, NULL, NULL);
      break;
  }
}

#ifdef WIDGET_IS_LABEL_WIDGET
static void
a_print_msg(Widget w) {
  size_t i, msglen;

  a_pipe_nread((char *)&msglen, sizeof(size_t));
  while (msglen > 0) {
    i = msglen;
    if (i > sizeof(local_buf)-1) i = sizeof(local_buf)-1;
    a_pipe_nread(local_buf, i);
    local_buf[i] = '\0';
    XtVaSetValues(w, XtNlabel,local_buf, NULL);
    msglen -= i;
  }
}
#else
static void
a_print_msg(Widget w) {
  size_t i, msglen;
  XawTextPosition pos;
  XawTextBlock tb;

  tb.firstPos = 0;
  tb.ptr = local_buf;
  tb.format = XawFmt8Bit;
  pos = XawTextGetInsertionPoint(w);

  a_pipe_nread((char *)&msglen, sizeof(size_t));
  while (msglen > 0) {
    i = msglen;
    if (i > sizeof(local_buf)) i = sizeof(local_buf);
    a_pipe_nread(local_buf, i);
    tb.length = i;
    XawTextReplace(w, pos, pos, &tb);
    pos += i;
    XawTextSetInsertionPoint(w, pos+1);
    msglen -= i;
  }
#ifdef BYPASSTEXTSCROLLBUG
  XtCallActionProc(lyric_t, (String)"redraw-display", NULL, NULL, ZERO);
#endif /* BYPASSTEXTSCROLLBUG */
}
#endif /* WIDGET_IS_LABEL_WIDGET */

static void
free_vars(void) {
  Cardinal n;
  int i = 0;
  WidgetList wl;
  Pixmap bm_Pixmap;

#if 0
  free_ldS(ldSstart);
  free(Cfg.DefaultDir);
  free(home); free(dotfile);
  free(psmenu);
  if (record != NULL) {
    for (i = 0; i < record->max; i++)
      free(record->output_list[i].id_name); 
    free(record->output_list);
    free(record);
  }
  if (play != NULL) {
    for (i = 0; i < play->max; i++)
      free(play->output_list[i].id_name); 
    free(play->output_list);
    free(play);
  }
  free_ptr_list(flist, max_files);
#endif

  XtUnmapWidget(toplevel);
  if (ctl->trace_playing) uninitTrace(True);
  XFreePixmap(disp, check_mark); XFreePixmap(disp, arrow_mark);
  XFreePixmap(disp, on_mark); XFreePixmap(disp, off_mark);
  XtVaGetValues(b_box, XtNchildren,&wl, XtNnumChildren,&n, NULL);
  for (i = 0; i < n; i++) {
    XtVaGetValues(wl[i], XtNbitmap,&bm_Pixmap, NULL);
    XFreePixmap(disp, bm_Pixmap);
  }
  XtDestroyApplicationContext(app_con);
}

static void
handle_input(XtPointer data, int *source, XtInputId *id) {
  char s[16];
  int n; long i;
  barfloat thumb;

  a_pipe_read(local_buf, sizeof(local_buf));
  switch (local_buf[0]) {
  case 't':
    curr_time = n = atoi(local_buf+2);
    if (halt) break;
    i = n % 60; n /= 60;
    sprintf(s, "%d:%02ld", n, i);
    XtVaSetValues(tune_l0, XtNlabel,s, NULL);
    if (total_time > 0) {
      thumb.f = (float)curr_time / (float)total_time;
      setThumb(tune_bar, thumb);
    }
    break;
  case 'T':
    n = atoi(local_buf+2);
    if (n > 0) {
      total_time = n;
      snprintf(s, sizeof(s), "/%2d:%02d", n/60, n%60);
      XtVaSetValues(tune_l, XtNlabel,s, NULL);
    }
    break;
  case 'E':
    {
      char *name;
      name = strchr(local_buf+2, ' ');
      current_n_displayed = n = atoi(local_buf+2);
      lockevents = False;
      if (IsRealized(popup_file))
        XawListHighlight(file_list, n-1);
      if (name == NULL) break;
      name++;
      XtVaSetValues(title_mb, XtNlabel,name, NULL);
      snprintf(window_title, sizeof(window_title), "%s : %s",
               APP_CLASS, local_buf+2);
      XtVaSetValues(toplevel, XtNtitle,window_title, NULL);
    }
    break;
  case 'e':
    if (app_resources.arrange_title) {
      char *p = local_buf+2;
      if (!strcmp(p, "(null)")) p = (char *)app_resources.tracecfg.untitled;
      snprintf(window_title, sizeof(window_title), "%s : %s", APP_CLASS, p);
      XtVaSetValues(toplevel, XtNtitle,window_title, NULL);
    }
    snprintf(window_title, sizeof(window_title), "%s", local_buf+2);
    break;
  case 'O' :
    thumb.f = 0;
    setThumb(tune_bar, thumb);
    XtVaSetValues(tune_l0, XtNlabel,"0:00", NULL);
    offPlayButton();
    break;
  case 'L':
    a_print_msg(lyric_t);
    break;
  case 'Q':
    free_vars();
    exit(0);
  case 'V':
    if (lockevents == True) return;
    amplitude = atoi(local_buf+2);
    snprintf(s, sizeof(s), "%d", amplitude);
    XtVaSetValues(vol_l, XtNlabel,s, NULL);
    thumb.f = (float)amplitude / (float)MAXVOLUME;
    setThumb(vol_bar, thumb);
    break;
  case 'g':
  case '\0':
  case 'Z':
    break;
  case 'X':
    n = max_files;
    max_files += atoi(local_buf+2);
    if ((max_files > 0) && (record != NULL))
      XtVaSetValues(file_menu[ID_SAVE - 100].widget, XtNsensitive,True, NULL);
    for (i=n;i<max_files;i++) {
      a_pipe_read(local_buf, sizeof(local_buf));
      addOneFile(max_files, i, local_buf);
      addFlist(local_buf, i);
    }
    if (IsRealized(popup_file)) {
      XawListReturnStruct *lr = XawListShowCurrent(file_list);

      XawListChange(file_list, flist, max_files, 0, True);
      if ((lr != NULL) && (lr->list_index != XAW_LIST_NONE))
         XawListHighlight(file_list, lr->list_index);
      else
        if (max_files > 0) XawListHighlight(file_list, 0);
    }
    break;
  case 'm':
    n = atoi(local_buf+1);
    switch (n) {
    case GM_SYSTEM_MODE:
      snprintf(s, sizeof(s), "%d:%02d / GM", total_time/60, total_time%60);
      break;
    case GS_SYSTEM_MODE:
      snprintf(s, sizeof(s), "%d:%02d / GS", total_time/60, total_time%60);
      break;
    case XG_SYSTEM_MODE:
      snprintf(s, sizeof(s), "%d:%02d / XG", total_time/60, total_time%60);
      break;
    default:
      snprintf(s, sizeof(s), "%d:%02d", total_time/60, total_time%60);
      break;
    }
    XtVaSetValues(time_l, XtNlabel,s, NULL);
    break;
  case 's':
    savePlaylist(local_buf);
    break;
  case 'o':         /* pitch offset */
    if (IsTracePlaying()) {
      XtCallActionProc(keyup_b, (String)"unset", NULL, NULL, ZERO);
      XtCallActionProc(keydown_b, (String)"unset", NULL, NULL, ZERO);
#ifdef XAWPLUS
      XtCallActionProc(keyup_b, (String)"unhighlight", NULL, NULL, ZERO);
      XtCallActionProc(keydown_b, (String)"unhighlight", NULL, NULL, ZERO);
#endif /* XAWPLUS */
      (void)handleTraceinput(local_buf);
    }
    break;
  case 'p':         /* pitch */
    if (IsTracePlaying()) (void)handleTraceinput(local_buf);
    break;
  case 'q':         /* quotient, ratio */
    if (IsTracePlaying()) {
      XtCallActionProc(fast_b, (String)"unset", NULL, NULL, ZERO);
      XtCallActionProc(slow_b, (String)"unset", NULL, NULL, ZERO);
#ifdef XAWPLUS
      XtCallActionProc(fast_b, (String)"unhighlight", NULL, NULL, ZERO);
      XtCallActionProc(slow_b, (String)"unhighlight", NULL, NULL, ZERO);
#endif /* XAWPLUS */
      (void)handleTraceinput(local_buf);
    }
    break;
  case 'r':         /* rhythem, tempo */
    if (IsTracePlaying()) (void)handleTraceinput(local_buf);
    break;
  default :
    if ((lockevents == True) || (((ctl->trace_playing) &&
         (!handleTraceinput(local_buf)))))
      return;
    fprintf(stderr, "Unkown message '%s' from CONTROL\n", local_buf);
  }
}


static int
configcmp(char *s, int *num) {
  int i;
  char *p;

  for (i = 0; i < CFGITEMSNUMBER; i++) {
    if (strncasecmp(s, cfg_items[i], strlen(cfg_items[i])) == 0) {
      p = s + strlen(cfg_items[i]);
      while ((*p == ' ') || (*p == '\t')) p++;
      if ((i == S_MidiFile) || (i == S_DefaultDirectory))
        *num = p - s + SPLEN;
      else
        *num = atoi(p);
      return i;
    }
  }
  return -1;
}

static char *
strmatch(char *s1, char *s2) {
  char *p = s1;

  while ((*p != '\0') && (*p == *s2++)) p++;
  *p = '\0';
  return s1;
}

/* Canonicalize by removing /. and /foo/.. if they appear. */
static char *
canonicalize_path(char *path)
{
  char *o, *p, *target;
  int abspath;

  o = p = path;
  while (*p) {
    if ((p[0] == '/') && (p[1] == '/')) p++;
    else *o++ = *p++;
  }
  while ((path < o-1) && (path[o - path - 1] == '/')) o--;
  path[o - path] = '\0';

  if ((p = strchr(path, '/')) == NULL) return path;
  abspath = (p == path);

  o = target = p;
  while (*p) {
    if (*p != '/') *o++ = *p++;
    else if ((p[0] == '/') && (p[1] == '.') && ((p[2] == '/')||(p[2] == '\0')))
    {
      /* If "/." is the entire filename, keep the "/".  Otherwise,
       *  just delete the whole "/.".
       */
      if ((o == target) && (p[2] == '\0')) *o++ = *p;
      p += 2;
    }
    else if ((p[0] == '/') && (p[1] == '.') && (p[2] == '.') &&
    /* `/../' is the "superroot" on certain file systems.  */
             (o != target) && (p[3] == '/' || p[3] == '\0'))
    {
      while ((o != target) && (--o) && (*o != '/'));
      p += 3;
      if ((o == target) && (!abspath)) o = target = p;
    }
    else *o++ = *p++;
  }

  target[o - target] = '\0';
  if (*path == '\0') strcpy(path, "/");
  return path;
}

static char *
expandDir(char *path, DirPath *full, char *bpath) {
  static char newfull[PATH_MAX];
  char tmp[PATH_MAX];
  char *p, *tail;

  p = path;
  if (path == NULL) {
    strcpy(tmp, "/");
    strcpy(newfull, "/");
    if (full != NULL) {
      full->basename = NULL;
      full->dirname = newfull;
    }
    return newfull;
  } else 
    if ((*p != '~') && ((tail = strrchr(path, '/')) == NULL) 
         && (strcmp(p, ".")) && (strcmp(p, "..")) )
  {
    p = tmp;
    strlcpy(p, bpath, PATH_MAX);
    if (full != NULL) full->dirname = p;
    while (*p++ != '\0') ;
    strlcpy(p, path, PATH_MAX - (p - tmp));
    snprintf(newfull, sizeof(newfull), "%s/%s", bpath, path);
    if (full != NULL) full->basename = p;
    return newfull;
  }
  if (*p  == '/') {
    strlcpy(tmp, path, PATH_MAX);
  } else {
    if (*p == '~') {
      struct passwd *pw;

      p++;
      if ((*p == '/') || (*p == '\0')) {
        if (home == NULL) return NULL;
        while (*p == '/') p++;
        snprintf(tmp, sizeof(tmp), "%s/%s", home, p);
      } else {
        char buf[80], *bp = buf;

        while ((*p != '/') && (*p != '\0')) *bp++ = *p++;
        *bp = '\0';
        pw = getpwnam(buf);
        if (pw == NULL) {
          ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
                    "I tried to expand a non-existant user's homedir!");
          return NULL;
        }
        else home = pw->pw_dir;
        while (*p == '/') p++;
        snprintf(tmp, sizeof(tmp), "%s/%s", home, p);
      }
    } else {    /* *p != '~' */
      snprintf(tmp, sizeof(tmp), "%s/%s", bpath, path);
    }
  }
  p = canonicalize_path(tmp);
  tail = strrchr(p, '/'); *tail++ = '\0';
  if (full != NULL) {
    full->dirname = p;
    full->basename = tail;
  }
  snprintf(newfull, sizeof(newfull), "%s/%s", p, tail);
  return newfull;
}

static void
setDirACT(Widget w, XEvent *e, String *v, Cardinal *n) {
/* uses global ld */
  char *p, *p2;
  struct stat st;

  p = XawDialogGetValueString(load_d);
  if ((p2 = expandDir(p, NULL, basepath)) != NULL) p = p2;
  if (stat(p, &st) == -1) XtCallCallbacks(load_ok, XtNcallback, (XtPointer)ld);
  else if (S_ISDIR(st.st_mode)) {
    p2 = strrchr(p, '/');
    if ((*(p2+1) == '\0') && (p2 != p)) *p2 = '\0';
    if (!setDirList(ld, p) ) {
      strlcpy(basepath, p, sizeof(basepath));
      XtVaSetValues(cwd_l, XtNlabel,basepath, NULL);
#ifdef CLEARVALUE
      clearValue(load_d);
#endif /* CLEARVALUE */
      XtVaSetValues(load_d, XtNvalue,"", NULL);
    }
  }
  else XtCallCallbacks(load_ok, XtNcallback,(XtPointer)ld);
}

static void
callFilterDirList(Widget w, XtPointer client_data, XtPointer call_data) {
  Boolean toggle;

  ldPointer ld = (ldPointer)client_data;
  XtVaGetValues(load_b, XtNstate,&toggle, NULL);
  filterDirList(w, ld, toggle);
}

static void
filterDirList(Widget w, ldPointer ld, Boolean toggle) {
/* can use global ld when called from SetDirAction via SetDirList */
  String *fulllist = fulldirlist, *filelist;
  StringTable strtab;
  char *filename;
  char mbuf[35];
  unsigned int f_num = 0;

  if (toggle == False) {
    XawListChange(load_flist, fulldirlist, fullfilenum, 0, True);
    XtVaSetValues(load_flist, XtNwidth,0, XtNheight,0, NULL);
    snprintf(mbuf, sizeof(mbuf), "%d Directories, %d Files",
             dirnum, fullfilenum);
    XtVaSetValues(load_info, XtNlabel,mbuf, NULL);
    return;
  }
  else if ( (fdirlist != NULL) &&
            (!(strncmp(cur_filter, filter, sizeof(cur_filter)))) )
  {
    XawListChange(load_flist, fdirlist, filenum, 0, True);
    XtVaSetValues(load_flist, XtNwidth,0, XtNheight,0, NULL);
    snprintf(mbuf, sizeof(mbuf), "%d Directories, %d Files", dirnum, filenum);
    XtVaSetValues(load_info, XtNlabel,mbuf, NULL);
    return;
  }
  if (!strcmp(filter, "SetDirList")) strcpy(filter, cur_filter);
  init_string_table(&strtab);
  while ((filename = *(fulllist++)) != NULL) {
    if (!(arc_case_wildmat(filename, filter))) continue;
    f_num++;
    put_string_table(&strtab, filename, strlen(filename));
  }
  filenum = f_num;
  if (f_num > 0)
    filelist = (String *)make_string_array(&strtab);
  else {
    filelist = (String *)safe_malloc(sizeof(String));
    *filelist = '\0';
  }
  XawListChange(load_flist, filelist, f_num, 0, True);
  free(fdirlist);
  fdirlist = filelist;
  XtVaSetValues(load_flist, XtNwidth,0, XtNheight,0, NULL);
  strlcpy(cur_filter, filter, sizeof(cur_filter));

  snprintf(mbuf, sizeof(mbuf), "%d Directories, %d Files", dirnum, filenum);
  XtVaSetValues(load_info, XtNlabel,mbuf, NULL);
}

static int
cmpstringp(const void *p1, const void *p2) {
  return strcmp(* (char * const *) p1, * (char * const *) p2);
}

static int
setDirList(ldPointer ld, char *curr_dir) {
/* uses global ld when called from SetDirAction */
  URL dirp;
  struct stat st;
  char filename[PATH_MAX];
  unsigned int d_num = 0, f_num = 0;
  Boolean toggle;

  if ((dirp = url_dir_open(curr_dir)) != NULL) {
    char *fullpath;
    MBlockList pool;
    StringTable strftab, strdtab;
    String *olddirlist = ddirlist, *oldfullfilelist = fulldirlist;
    char lbuf[50];

    init_mblock(&pool);
    XtVaGetValues(load_b, XtNstate,&toggle, NULL);

    init_string_table(&strftab); init_string_table(&strdtab);
    while (url_gets(dirp, filename, sizeof(filename)) != NULL) {
      fullpath = (char *)new_segment(&pool, strlen(curr_dir) + 
                                     strlen(filename) + 2);
      sprintf(fullpath, "%s/%s", curr_dir, filename);
      if (filename[0] == '.') {
        if (filename[1] == '\0') continue;
        if ((filename[1] == '.') && (filename[2] == '\0')) {
          if ((curr_dir[0] == '/') && (curr_dir[1] == '\0')) continue;
        }
        else if (!Cfg.showdotfiles) continue;
      }
      if (stat(fullpath, &st) == -1) continue;
      if (S_ISDIR(st.st_mode)) {
        strcat(filename, "/"); d_num++;
        put_string_table(&strdtab, filename, strlen(filename));
      } else {
        f_num++;
        put_string_table(&strftab, filename, strlen(filename));
      }
    }
    if (d_num > 0) {
      ddirlist = (String *)make_string_array(&strdtab);
      qsort (ddirlist, d_num, sizeof (char *), cmpstringp);
    } else {
      ddirlist = (String *)safe_malloc(sizeof(String));
      *ddirlist = '\0';
    }
    if (f_num > 0) {
      fulldirlist = (String *)make_string_array(&strftab);
      qsort (fulldirlist, f_num, sizeof (char *), cmpstringp);
    } else {
      fulldirlist = (String *)safe_malloc(sizeof(String));
      *fulldirlist = '\0';
    }
    fullfilenum = f_num; dirnum = d_num;

    XawListChange(load_dlist, ddirlist, d_num, 0, True);
    XtVaSetValues(load_dlist, XtNwidth,0, XtNheight,0, NULL);
    free(olddirlist);
    /* According to Xt CLI p. 49, we must keep the list usable
     * until it is replaced, so we use a local pointer first,
     * and free the previous list only after it has been replaced.
     */
    if (toggle == True) {
      strcpy(filter, "SetDirList");
      filterDirList(load_d, ld, True);
      if (oldfullfilelist) free(oldfullfilelist);
      return 0;
    }
    XawListChange(load_flist, fulldirlist, f_num, 0, True);
    XtVaSetValues(load_flist, XtNwidth,0, XtNheight,0, NULL);
    /* Same reason as in popupDialog */
    free(fdirlist);
    fdirlist = NULL;
    free(oldfullfilelist);
    snprintf(lbuf, sizeof(lbuf), "%d Directories, %d Files",
             d_num, f_num);
    XtVaSetValues(load_info, XtNlabel,lbuf, NULL);
    return 0;
  } else {
    fprintf(stderr, "Can't read directory\n");
    return 1;
  }
}

static void
setFileLoadCB(Widget list, XtPointer client_data, XawListReturnStruct *lrs) {
  ldPointer ld = (ldPointer)client_data;

#ifdef CLEARVALUE
  clearValue(load_d);
#endif /* CLEARVALUE */
  Widget Text = XtNameToWidget(load_d, "value");
  XtVaSetValues(Text, XtNstring,lrs->string, NULL);
  XtVaSetValues(Text, XtNinsertPosition,strlen(lrs->string), NULL);
  return;
}

static void
setDirLoadCB(Widget list, XtPointer client_data, XawListReturnStruct *lrs) {
  ldPointer ld = (ldPointer)client_data;
  struct stat st;
  char curr_dir[PATH_MAX];

  snprintf(curr_dir, sizeof(curr_dir)-1, "%s/%s", basepath, lrs->string);
  canonicalize_path(curr_dir);
  if (stat(curr_dir, &st) == -1) return;
  if (!setDirList(ld, curr_dir) ) {
    strcpy(basepath, curr_dir);
    XtVaSetValues(cwd_l, XtNlabel,basepath, NULL);
  }
  return;
}

static void
toggleTraceACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  if (ctl->trace_playing && ((e->xbutton.button == 1) || (e->type == KeyPress)))
    toggleTracePlane(IsTracePlaying());
}

static void
muteChanACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  int c;

  if (e->xbutton.y <= TRACE_HEADER) return;
  c = (e->xbutton.y - TRACE_FOOT - BAR_SPACE/2)/BAR_SPACE;
  if ((c > getVisibleChanNum()-1) || (c < 0)) return;
  else a_pipe_write("M %d", c+getLowestVisibleChan());
}

static void
scrollTraceACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  int i = atoi(*v);

  if (!ctl->trace_playing) return;
  if (i>0) scrollTrace(1);
  else scrollTrace(-1);
}

static void
exchgWidthACT(Widget w, XEvent *e, String *v, Cardinal *n) {
#define MEDIAN (TRACE_WIDTH+8 + DEFAULT_REG_WIDTH)/2
  if (curr_width < DEFAULT_REG_WIDTH)
    XResizeWindow(disp, XtWindow(toplevel), DEFAULT_REG_WIDTH, curr_height);
  else if (curr_width < MEDIAN)
    XResizeWindow(disp, XtWindow(toplevel), MEDIAN, curr_height);
  else if (curr_width < TRACE_WIDTH+8)
    XResizeWindow(disp, XtWindow(toplevel), TRACE_WIDTH+8, curr_height);
  else
    XResizeWindow(disp, XtWindow(toplevel), DEFAULT_REG_WIDTH, curr_height);
#undef MEDIAN
}

static void
redrawACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  if (e->xexpose.count == 0) callRedrawTrace(IsTracePlaying());
}

static void
redrawCaptionACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  if (e->type == EnterNotify) redrawCaption(True);
  else redrawCaption(False);
}

static void
completeDirACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  char *p;
  DirPath full;
  Widget load_dialog_widget = XtParent(w);  /* XtParent = ld->ld_load_d */

  p = XawDialogGetValueString(load_dialog_widget);
  if (!expandDir(p, &full, basepath)) { /* uses global ld */
    ctl->cmsg(CMSG_WARNING, VERB_NORMAL, "something wrong with getting path.");
    return;
  }
  if (full.basename != NULL) {
    int lenb, lend, match = 0;
    char filename[PATH_MAX], matchstr[PATH_MAX], *path = "/";
    char *fullpath;
    URL dirp;

    lenb = strlen(full.basename);
    lend = strlen(full.dirname);
    if (lend != 0) path = full.dirname;
    if ((dirp = url_dir_open(path)) != NULL) {
      MBlockList pool;
      struct stat st;
      init_mblock(&pool);

      while (url_gets(dirp, filename, sizeof(filename)) != NULL) {
        if (!strncmp(full.basename, filename, lenb)) {

          fullpath = (char *)new_segment(&pool,
                            lend + strlen(filename) + 2);
          sprintf(fullpath, "%s/%s", full.dirname, filename);

          if (stat(fullpath, &st) == -1)
            continue;
          if (!match)
            strlcpy(matchstr, filename, PATH_MAX);
          else
            strmatch(matchstr, filename);
          match++;
          if (S_ISDIR(st.st_mode) && (!strcmp(filename, full.basename))) {
            lenb = strlcpy(matchstr, filename, PATH_MAX);
            if (lenb >= PATH_MAX) lenb = PATH_MAX - 1;
            strncat(matchstr, "/", PATH_MAX - 1 - lenb);
            match = 1;
            break;
          }
        }
      }
      url_close(dirp);
      reuse_mblock(&pool);

      if (match) {
#ifdef CLEARVALUE
        clearValue(XtParent(w));
#endif /* CLEARVALUE */
        snprintf(filename, sizeof(filename), "%s/%s", full.dirname, matchstr);
        XtVaSetValues(load_dialog_widget, XtNvalue,filename, NULL); 
      }
    }
  }
}

static int
readPlaylist(char *p) {
  FILE *Playlist;
  char fname[SLINELEN], *pp;
  int ret = 1;

  if ((Playlist = fopen(p, "r")) == NULL) {
     fprintf(stderr, "Can't open %s for reading.\n", p);
     return 2;
  }
  if ((pp = fgets(fname, SLINELEN, Playlist)) == NULL) goto out;
  if (memcmp(fname, "\0timidity playlist:\n", 20)) goto out;
  while (fgets(fname, SLINELEN, Playlist) != NULL) {
    if ((pp = strchr(fname, '\n')) != NULL) *pp = '\0';
    a_pipe_write("X %s", fname);
  }
  ret = 0;
out:
  fclose(Playlist);
  return ret;
}

static void
savePlaylist(char *p) {
  int i, n;
  char *file, *space;
  FILE *fp;

  space = strchr(p, ' ');
  file = space + 1;
  *space = '\0';
  n = atoi(p+1);
  if (n <= 0) return;
  if (!strcmp(file, dotfile)) {
    char prefix[10];

    snprintf(prefix, sizeof(prefix), SPREFIX "%s ", cfg_items[S_MidiFile]);
    if ((fp = fopen(dotfile, "a")) != NULL) {
      for(i=0; i<n; i++) {
        a_pipe_read(local_buf, sizeof(local_buf));
        if (fprintf(fp,"%s%s\n", prefix, local_buf) < 0) goto error;
      }
      fclose(fp);
    }
    else goto error;
  } else {
    if ((fp = fopen(file, "w")) != NULL) {
      if (fputc('\0', fp) == EOF) goto error;
      if (fprintf(fp, "timidity playlist:\n") < 0) goto error;
      for(i=0; i<n; i++) {
        a_pipe_read(local_buf, sizeof(local_buf));
        if (fprintf(fp,"%s\n", local_buf) < 0) goto error;
      }
      fclose(fp);
    }
    else goto error;
  }
  return;
error:
  if (fp != NULL) fclose(fp);
  for(i=0; i<n; i++) a_pipe_read(local_buf, sizeof(local_buf));
  warnCB(toplevel, "saveplaylisterror", True);
  return;
}

static void
a_readconfig (Config *Cfg, char **home) {
  char s[SLINELEN];
  char *p, *pp;
  int k;
  struct stat st;
  FILE *fp;

  *home = get_user_home_dir();
  if (*home != NULL) {
    dotfile = (char *)safe_malloc(sizeof(char) * PATH_MAX);
    snprintf(dotfile, PATH_MAX, "%s/%s", *home, INITIAL_CONFIG);
    if ( (fp = fopen(dotfile, "r")) != NULL) {
      dotfile_flist = (char **)safe_malloc(sizeof(char *) * INIT_FLISTNUM);
      while ((p = fgets(s, SLINELEN, fp)) != NULL) {
        if ((pp = strchr(p, '\n')) != NULL) *pp = '\0';
        if (strncasecmp(s, SPREFIX, SPLEN) != 0) continue;
        switch (configcmp(s+SPLEN, &k)) {
        case S_ConfirmExit:
          Cfg->confirmexit = (Boolean)k; break;
        case S_RepeatPlay:
          Cfg->repeat = (Boolean)k; break;
        case S_AutoStart:
          Cfg->autostart = (Boolean)k; break;
        case S_AutoExit:
          Cfg->autoexit = (Boolean)k; break;
        case S_DispText:
          Cfg->disptext = (Boolean)k;
          break;
        case S_ShufflePlay:
          Cfg->shuffle = (Boolean)k; break;
        case S_DispTrace:
          Cfg->disptrace = (Boolean)k; break;
        case S_CurVol:
          Cfg->amplitude = (int)k;
          if (Cfg->amplitude > MAXVOLUME) Cfg->amplitude = MAXVOLUME;
          else if (Cfg->amplitude < 0) Cfg->amplitude = 0;
          break;
        case S_ExtOptions:
          Cfg->extendopt = (int)k; break;
        case S_ChorusOption:
          Cfg->chorusopt = (int)k; break;
        case S_Tooltips:
          Cfg->tooltips = (Boolean)k; break;
        case S_Showdotfiles:
          Cfg->showdotfiles = (Boolean)k; break;
        case S_SaveList:
          Cfg->save_list = (Boolean)k; break;
        case S_DefaultDirectory:
          p = s+k;
          pp = expandDir(p, NULL, "/");
          if ((stat(pp, &st) == -1) || (!(S_ISDIR(st.st_mode)))) {
            fprintf(stderr, "Default directory %s doesn't exist!\n", p);
            Cfg->DefaultDir = NULL;
          } else {
            Cfg->DefaultDir = safe_strdup(pp);
          }
          break;
        case S_MidiFile:
          p = s+k;
          if (dot_nfile < INIT_FLISTNUM) {
            dotfile_flist[dot_nfile] = safe_strdup(p);
            dot_nfile++;
          }
          dotfile_flist[dot_nfile] = NULL;
          break;
        }
      }
      fclose(fp);
      if (Cfg->DefaultDir == NULL) Cfg->DefaultDir = safe_strdup(*home);
    } else {
      Cfg->DefaultDir = safe_strdup(*home);
      a_saveconfig(dotfile, False);
    }
  } else {
    Cfg->DefaultDir = safe_strdup("/");
  }

  if (ctl->flags & CTLF_AUTOSTART) Cfg->autostart = True;
  if (ctl->flags & CTLF_AUTOEXIT) Cfg->autoexit = True;
  if (ctl->flags & CTLF_LIST_LOOP) Cfg->repeat = True;
  if (ctl->flags & CTLF_LIST_RANDOM) Cfg->shuffle = True;
}

static void
a_saveconfig(char *file, Boolean save_list) {
  FILE *fp;

  if (*file == '\0') return;
  if ((fp = fopen(file, "w")) == NULL) {
    fprintf(stderr, "cannot open initializing file '%s'.\n", file);
    return;
  }
  fprintf(fp, SPREFIX "%s %d\n", cfg_items[S_RepeatPlay], Cfg.repeat?1:0);
  fprintf(fp, SPREFIX "%s %d\n", cfg_items[S_ShufflePlay], Cfg.shuffle?1:0);
  fprintf(fp, SPREFIX "%s %d\n", cfg_items[S_ExtOptions], init_options);
  fprintf(fp, SPREFIX "%s %d\n", cfg_items[S_ChorusOption], init_chorus);
  fprintf(fp, SPREFIX "%s %d\n", cfg_items[S_CurVol], amplitude);
  fprintf(fp, SPREFIX "%s %d\n", cfg_items[S_Showdotfiles],
          Cfg.showdotfiles?1:0);
  fprintf(fp, SPREFIX "%s %s\n", cfg_items[S_DefaultDirectory], Cfg.DefaultDir);
  fprintf(fp, SPREFIX "%s %d\n", cfg_items[S_DispTrace], Cfg.disptrace?1:0);
  fprintf(fp, SPREFIX "%s %d\n", cfg_items[S_DispText], Cfg.disptext?1:0);
  fprintf(fp, SPREFIX "%s %d\n", cfg_items[S_Tooltips], Cfg.tooltips?1:0);
  fprintf(fp, SPREFIX "%s %d\n", cfg_items[S_AutoStart], Cfg.autostart?1:0);
  fprintf(fp, SPREFIX "%s %d\n", cfg_items[S_AutoExit], Cfg.autoexit?1:0);
  fprintf(fp, SPREFIX "%s %d\n", cfg_items[S_ConfirmExit], Cfg.confirmexit?1:0);
  fprintf(fp, SPREFIX "%s %d\n", cfg_items[S_SaveList], Cfg.save_list?1:0);
  fclose(fp);
  if (save_list) {
    /* TODO: We're doing this dance via the pipe because no structure
     * in xaw_i.c contains full filenames (including paths).
     * Changing this would require changing xaw_c.c as well.
     */
    a_pipe_write("s %s", dotfile);
  }
}

#ifdef OFFIX
static void
FileDropedHandler(Widget widget, XtPointer data, XEvent *event, Boolean *b) {
  char *filename;
  unsigned char *Data;
  unsigned long Size;
  char local_buffer[PATH_MAX];
  int i;
  static const int AcceptType[] = {DndFile, DndFiles, DndDir, DndLink,
                                   DndExe, DndURL, DndNotDnd};
  int Type;

  Type = DndDataType(event);
  for(i=0;AcceptType[i] != DndNotDnd;i++) {
    if (AcceptType[i] == Type)
      goto OK;
  }
  fprintf(stderr, "NOT ACCEPT\n");
  /*Not Acceptable,so Do Nothing*/
  return;
OK:
  DndGetData(&Data, &Size);
  if (Type == DndFiles) {
    filename = Data;
    while (filename[0] != '\0') {
      a_pipe_write("X %s", filename);
      filename = filename + strlen(filename) + 1;
    }
  } else {
    a_pipe_write("X %s%s", Data, (Type == DndDir)?"/":"");
  }
  return;
}
#endif /* OFFIX */

static void
leaveSubmenuACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  XLeaveWindowEvent *leave_event = (XLeaveWindowEvent *)e;
  Dimension height;

  XtVaGetValues(w, XtNheight,&height, NULL);
  if ((leave_event->x <= 0) || (leave_event->y <= 0) ||
      (leave_event->y >= height))
    XtPopdown(w);
}

#ifdef TimNmenu
static void
checkRightAndPopupSubmenuACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  XLeaveWindowEvent *leave_ev = (XLeaveWindowEvent *)e;
  Dimension nheight, height, width;
  Position x, y;
  int i;

  if (maxentry_on_a_menu == 0) return;

  if (e == NULL) i = *(int *)n;
  else i = atoi(*v);
  if (w != title_sm) {
    if ((leave_ev->x <= 0) || (leave_ev->y < 0)) {
      XtPopdown(w);
      return;
    }
  } else {
    if ((leave_ev->x <= 0) || (leave_ev->y <= 0)) return;
  }
  if (psmenu[i] == NULL) return;
  XtVaGetValues(psmenu[i], XtNheight,&height, NULL);

  /* neighbor menu height */
  XtVaGetValues((i > 0)? psmenu[i-1]:title_sm, XtNwidth,&width,
                         XtNheight,&nheight, XtNx,&x, XtNy,&y, NULL);
  if ((leave_ev->x > 0) && (leave_ev->y > nheight - 22)) {
    XtVaSetValues(psmenu[i], XtNx,x+80, NULL);
    XtPopup(psmenu[i], XtGrabNone);
    XtVaGetValues(psmenu[i], XtNheight,&height, NULL);
    XtVaSetValues(psmenu[i], XtNy,y +((height)? nheight-height:0), NULL);
  }
}
#endif /* TimNmenu */

static void
popdownSubmenuCB(Widget w, XtPointer client_data, XtPointer call_data) {
  long i = (long)client_data;

  if (i < 0) i = submenu_n -1;
  while (i >= 0) XtPopdown(psmenu[i--]);
}

static void
popdownSubmenuACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  int i = atoi(*v);

  while (i >= 0) XtPopdown(psmenu[i--]);
}

static void
closeParentACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  XtPopdown(seekTransientShell(w));
}

static void
flistMoveACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  int i = atoi(*v);
  XawListReturnStruct *lr;

  if (max_files == 0) return;
  lr = XawListShowCurrent(file_list);
  if (XtIsRealized(popup_file)) {
    if ((lr != NULL) && (lr->list_index != XAW_LIST_NONE)) {
      Dimension listheight, vportheight;
      Widget file_vport = XtParent(file_list);
      int perpage;

      XtVaGetValues(file_list, XtNheight,&listheight, NULL);
      XtVaGetValues(file_vport, XtNheight,&vportheight, NULL);
      perpage = ceil((max_files * vportheight) / listheight - 0.5);
      if (*(int *)n == 1) {
        i += lr->list_index;
      } else if (*(int *)n == 2) {
        i = lr->list_index + i*perpage;
      } else {
        if (i > 0) i = max_files-1;
        else i = 0;
      }
      if (i < 0) i = 0;
      else if (i >= max_files) i = max_files-1;
      if (listheight > vportheight) {
        Widget scrollbar; barfloat thumb; int covered;

        scrollbar = XtNameToWidget(file_vport, "vertical");
        if (scrollbar == NULL) return;
        XtVaGetValues(scrollbar, XtNtopOfThumb,&thumb.f, NULL);
        covered = thumb.f*max_files;

        if ( ((i - 1) < covered) || ((i + 1) > (covered + perpage)) ) {
          String arg[1];

          if ((i - 1) < covered) {
            if (i > perpage/2)
              thumb.f = (float)(i - perpage/2) / (float)max_files;
            else thumb.f = 0;
          }
          else thumb.f = (float) (i - perpage/2) / (float)max_files;
          arg[0] = XtNewString("Continuous");
          XtCallActionProc(scrollbar, (String)"StartScroll", e, arg, ONE);
          XtFree(arg[0]);
          setThumb(scrollbar, thumb);
          XtCallActionProc(scrollbar, (String)"NotifyThumb", e, NULL, ZERO);
          XtCallActionProc(scrollbar, (String)"EndScroll", e, NULL, ZERO);
        }
      }
      XawListHighlight(file_list, i);
    } else {
/* should never be reached */
      XawListHighlight(file_list, (i<0)?max_files-1:0);
    }
  }
}

static void
addFlist(char *fname, long current_n) {
  if (max_num < current_n+1) {
    max_num += 64;
    flist = (String *)safe_realloc(flist, (max_num+1)*sizeof(String));
  }
  free(flist[current_n]);
  flist[current_n] = safe_strdup(fname);
  flist[current_n+1] = NULL;
}

static void
addOneFile(int max_files, long curr_num, char *fname) {
  static Dimension tmpi;
  static int menu_height;
  static Widget tmpw;
  static int jjj;
  Widget pbox;
  char sbuf[256];

  if (curr_num == 0) {
    jjj = 0; tmpi = 0; menu_height = 0; tmpw = title_sm;
  }
  if (menu_height + tmpi*2 > root_height) {
    if (maxentry_on_a_menu == 0) {
      jjj = curr_num;
      maxentry_on_a_menu = jjj;
      XtAddCallback(title_sm, XtNpopdownCallback,popdownSubmenuCB,
                         (XtPointer)(submenu_n -1));
    }
    if (jjj >= maxentry_on_a_menu) {
      if (!psmenu) {
        psmenu = (Widget *)safe_malloc(sizeof(Widget)
                              * ((int)(max_files / curr_num) + 2));
#ifndef TimNmenu
       sb = (String *)safe_malloc(sizeof(String));
#endif
      } else {
        psmenu = (Widget *)safe_realloc((char *)psmenu,
                    sizeof(Widget)*(submenu_n + 2));
#ifndef TimNmenu
        sb = (String *)safe_realloc(sb, sizeof(String)*(submenu_n + 2));
#endif
      }
      snprintf(sbuf, sizeof(sbuf), "morebox%ld", submenu_n);
      pbox = XtVaCreateManagedWidget(sbuf,smeBSBObjectClass,tmpw,
               XtNlabel,app_resources.more_text,
               XtNbackground,textbgcolor, XtNforeground,capcolor,
               XtNrightBitmap,arrow_mark,
               XtNfontSet,app_resources.label_font, NULL);

      snprintf(sbuf, sizeof(sbuf), "psmenu%ld", submenu_n);
      psmenu[submenu_n] = XtVaCreatePopupShell(sbuf,simpleMenuWidgetClass,
                                 title_sm, XtNforeground,textcolor,
                                 XtNbackingStore,NotUseful, XtNsaveUnder,False,
                                 XtNwidth,app_resources.menu_width,
                                 XtNbackground,textbgcolor, NULL);
#ifndef TimNmenu
      sb[submenu_n] = safe_strdup(sbuf);
      XtVaSetValues(pbox, XtNmenuName,sb[submenu_n], NULL);
#else
      snprintf(sbuf, sizeof(sbuf),
              "<LeaveWindow>: unhighlight() \
                  checkRightAndPopupSubmenu(%ld)", submenu_n);
      XtOverrideTranslations(tmpw, XtParseTranslationTable(sbuf));
      snprintf(sbuf, sizeof(sbuf), "<BtnUp>: popdownSubmenu(%ld) \
                                      notify() unhighlight()\n\
        <EnterWindow>: unhighlight()\n\
        <LeaveWindow>: leaveSubmenu(%ld) unhighlight()", submenu_n, submenu_n);
      XtOverrideTranslations(psmenu[submenu_n], XtParseTranslationTable(sbuf));
#endif /* !TimNmenu */
      tmpw = psmenu[submenu_n++]; psmenu[submenu_n] = NULL;
      jjj = 0;
    }
  }
  if (maxentry_on_a_menu != 0) jjj++;
  bsb = XtVaCreateManagedWidget(fname,smeBSBObjectClass,tmpw, NULL);
  XtAddCallback(bsb, XtNcallback,menuCB, (XtPointer)curr_num);
  XtVaGetValues(bsb, XtNheight,&tmpi, NULL);
  if (maxentry_on_a_menu == 0) menu_height += tmpi;
  else psmenu[submenu_n] = NULL;
}

static void
createOptions(void) {
  Widget modul_b, modul_bb, modul_l, porta_b, porta_bb, porta_l, nrpnv_b,
         nrpnv_bb, nrpnv_l, reverb_b, reverb_bb, reverb_l, chorus_bb, chorus_l,
         chpressure_b, chpressure_bb, chpressure_l, overlapv_b, overlapv_bb,
         overlapv_l, txtmeta_b, txtmeta_bb, txtmeta_l,
         popup_optform, lowBox, popup_olabel, popup_ook, popup_ocancel;
  Dimension x, y;

  popup_opt = XtVaCreatePopupShell("popup_option",transientShellWidgetClass,
                       toplevel, NULL);
  XtVaGetValues(toplevel, XtNx,&x, XtNy,&y, NULL);
  XtVaSetValues(popup_opt, XtNx,x+DEFAULT_REG_WIDTH, XtNy,y, NULL);

  popup_optform = XtVaCreateManagedWidget("popup_optform",formWidgetClass,
                       popup_opt, XtNorientation,XtorientVertical,
                       XtNbackground,bgcolor, NULL);
  modul_bb = XtVaCreateManagedWidget("modul_box",boxWidgetClass,popup_optform,
                       XtNbackground,bgcolor,
                       XtNorientation,XtorientHorizontal, NULL);
  modul_b = XtVaCreateManagedWidget("modul_button",toggleWidgetClass,modul_bb,
                       XtNbitmap,off_mark, XtNforeground,togglecolor,
                       XtNbackground,buttonbgcolor, NULL);
  modul_l = XtVaCreateManagedWidget("modul_lbl",labelWidgetClass,modul_bb,
                       XtNforeground,textcolor, XtNbackground,bgcolor, NULL);
  porta_bb = XtVaCreateManagedWidget("porta_box",boxWidgetClass,popup_optform,
                       XtNfromVert,modul_bb, 
                       XtNorientation,XtorientHorizontal,
                       XtNbackground,bgcolor, NULL);
  porta_b = XtVaCreateManagedWidget("porta_button",toggleWidgetClass,porta_bb,
                       XtNforeground,togglecolor, XtNbackground,buttonbgcolor,
                       XtNbitmap,off_mark, XtNfromVert,modul_b, NULL);
  porta_l = XtVaCreateManagedWidget("porta_lbl",labelWidgetClass,porta_bb,
                       XtNforeground,textcolor, XtNbackground,bgcolor, NULL);
  nrpnv_bb = XtVaCreateManagedWidget("nrpnv_box",boxWidgetClass,popup_optform,
                       XtNfromVert,porta_bb, 
                       XtNorientation,XtorientHorizontal,
                       XtNbackground,bgcolor, NULL);
  nrpnv_b = XtVaCreateManagedWidget("nrpnv_button",toggleWidgetClass,nrpnv_bb,
                       XtNforeground,togglecolor, XtNbackground,buttonbgcolor,
                       XtNbitmap,off_mark, XtNfromVert,porta_b, NULL);
  nrpnv_l = XtVaCreateManagedWidget("nrpnv_lbl",labelWidgetClass,nrpnv_bb,
                       XtNforeground,textcolor, XtNbackground,bgcolor, NULL);
  reverb_bb = XtVaCreateManagedWidget("reverb_box",boxWidgetClass,
                       popup_optform, XtNfromVert,nrpnv_bb,
                       XtNorientation,XtorientHorizontal,
                       XtNbackground,bgcolor, NULL);
  reverb_b = XtVaCreateManagedWidget("reverb_button",toggleWidgetClass,
                       reverb_bb, XtNbitmap,off_mark, XtNfromVert,nrpnv_b,
                       XtNforeground,togglecolor, XtNbackground,buttonbgcolor,
                       NULL);
  reverb_l = XtVaCreateManagedWidget("reverb_lbl",labelWidgetClass,reverb_bb,
                       XtNforeground,textcolor, XtNbackground,bgcolor, NULL);
  chorus_bb = XtVaCreateManagedWidget("chorus_box",boxWidgetClass,
                       popup_optform, XtNorientation,XtorientHorizontal,
                       XtNfromVert,reverb_bb, 
                       XtNbackground,bgcolor, NULL);
  chorus_b = XtVaCreateManagedWidget("chorus_button",toggleWidgetClass,
                       chorus_bb, XtNbitmap,off_mark, XtNfromVert,reverb_b,
                       XtNforeground,togglecolor, XtNbackground,buttonbgcolor,
                       NULL);
  chorus_l = XtVaCreateManagedWidget("chorus_lbl",labelWidgetClass,chorus_bb,
                       XtNforeground,textcolor, XtNbackground,bgcolor, NULL);
  chpressure_bb = XtVaCreateManagedWidget("chpressure_box",boxWidgetClass,
                       popup_optform, XtNorientation,XtorientHorizontal,
                       XtNfromVert,chorus_bb, 
                       XtNbackground,bgcolor, NULL);
  chpressure_b = XtVaCreateManagedWidget("chpressure_button",
                       toggleWidgetClass,chpressure_bb, XtNbitmap,off_mark,
                       XtNfromVert,chorus_b, XtNforeground,togglecolor,
                       XtNbackground,buttonbgcolor, NULL);
  chpressure_l = XtVaCreateManagedWidget("chpressure_lbl",labelWidgetClass,
                       chpressure_bb, XtNforeground,textcolor,
                       XtNbackground,bgcolor, NULL);
  overlapv_bb = XtVaCreateManagedWidget("overlapvoice_box",boxWidgetClass,
                       popup_optform, XtNorientation,XtorientHorizontal,
                       XtNfromVert,chpressure_bb, 
                       XtNbackground,bgcolor, NULL);
  overlapv_b = XtVaCreateManagedWidget("overlapvoice_button",
                       toggleWidgetClass,overlapv_bb, XtNbitmap,off_mark, 
                       XtNfromVert,chpressure_b, XtNforeground,togglecolor,
                       XtNbackground,buttonbgcolor, NULL);
  overlapv_l = XtVaCreateManagedWidget("overlapv_lbl",labelWidgetClass,
                       overlapv_bb, XtNforeground,textcolor,
                       XtNbackground,bgcolor, NULL);
  txtmeta_bb = XtVaCreateManagedWidget("txtmeta_box",boxWidgetClass,
                       popup_optform, XtNorientation,XtorientHorizontal,
                       XtNfromVert,overlapv_bb, 
                       XtNbackground,bgcolor, NULL);
  txtmeta_b = XtVaCreateManagedWidget("txtmeta_button",toggleWidgetClass,
                       txtmeta_bb, XtNbitmap,off_mark, XtNfromVert,overlapv_b,
                       XtNforeground,togglecolor, XtNbackground,buttonbgcolor,
                       NULL);
  txtmeta_l = XtVaCreateManagedWidget("txtmeta_lbl",labelWidgetClass,
                       txtmeta_bb, XtNforeground,textcolor,
                       XtNbackground,bgcolor, NULL);


  popup_olabel = XtVaCreateManagedWidget("popup_olabel",labelWidgetClass,
            popup_optform, XtNbackground,menubcolor, XtNfromVert,txtmeta_bb,
            NULL);
  lowBox = createOutputSelectionWidgets(popup_opt, popup_optform,
                                        popup_olabel, play, True);

  popup_ook = XtVaCreateManagedWidget("closebutton",commandWidgetClass,
                                      popup_optform, XtNfromVert,lowBox,
                                      XtNwidth,80, NULL);
  popup_ocancel = XtVaCreateManagedWidget("Cancel",commandWidgetClass,
                                          popup_optform, XtNfromVert,lowBox,
                                          XtNfromHoriz,popup_ook, XtNwidth,80,
                                          NULL);
  XtAddCallback(popup_ook, XtNcallback,optionscloseCB, NULL);
  XtAddCallback(popup_ocancel, XtNcallback,closeWidgetCB, (XtPointer)popup_opt);
  XtAddCallback(modul_b, XtNcallback,optionsCB, NULL);
  XtAddCallback(porta_b, XtNcallback,optionsCB, NULL);
  XtAddCallback(nrpnv_b, XtNcallback,optionsCB, NULL);
  XtAddCallback(reverb_b, XtNcallback,optionsCB, NULL);
  XtAddCallback(chpressure_b, XtNcallback,optionsCB, NULL);
  XtAddCallback(overlapv_b, XtNcallback,optionsCB, NULL);
  XtAddCallback(txtmeta_b, XtNcallback,optionsCB, NULL);
  XtAddCallback(chorus_b, XtNcallback,optionsCB, NULL);
  option_num[MODUL_N].widget = modul_b;
  option_num[PORTA_N].widget = porta_b;
  option_num[NRPNV_N].widget = nrpnv_b;
  option_num[REVERB_N].widget = reverb_b;
  option_num[CHPRESSURE_N].widget = chpressure_b;
  option_num[OVERLAPV_N].widget = overlapv_b;
  option_num[TXTMETA_N].widget = txtmeta_b;
  XtSetKeyboardFocus(popup_opt, popup_optform);
  if (init_options != 0) {
    int i;

    for(i=0; i<MAX_OPTION_N; i++) {
      if (init_options & option_num[i].bit)
        XtVaSetValues(option_num[i].widget, XtNstate,True,
                      XtNbitmap,on_mark, NULL);
    }
  }
  if (init_chorus != 0)
    XtVaSetValues(chorus_b, XtNstate,True, XtNbitmap,on_mark, NULL);
}

static void
fselectCB(Widget w, XtPointer client_data, XtPointer call_data) {
  XawListReturnStruct *lr = XawListShowCurrent(file_list);

  if ((lr != NULL) && (lr->list_index != XAW_LIST_NONE)) {
    onPlayOffPause();
    a_pipe_write("L %d", lr->list_index + 1);
  }
}

static void
fdeleteCB(Widget w, XtPointer client_data, XtPointer call_data) {
  XawListReturnStruct *lr = XawListShowCurrent(file_list);
  int n; long i;
  char *p;

  if ((lr == NULL) || (lr->list_index == XAW_LIST_NONE)) return;
  if (max_files == 1) {
    fdelallCB(w, NULL, NULL);
    return;
  }
  n = lr->list_index;
  if (n+1 < current_n_displayed) current_n_displayed--;
  else if (n+1 == current_n_displayed) {
    stopCB(w, NULL, NULL);
    XtVaSetValues(tune_l, XtNlabel,"/ 00:00", NULL);
    if (n+1<max_files) p = strchr(flist[n+1], ' ');
    else {
      p = strchr(flist[n-1], ' ');
      current_n_displayed--;
    }
    if (p != NULL) XtVaSetValues(title_mb, XtNlabel,p+1, NULL);
    else fprintf(stderr, "No space character in flist!\n"); 
         /* Should never happen */
  }
  a_pipe_write("d %d", n);
  --max_files;
  free(flist[n]);
  for(i=n; i<max_files; i++) {
    p = strchr(flist[i+1], '.');
    snprintf(flist[i+1], strlen(flist[i+1])+1, "%ld%s", i+1, p);
    flist[i] = flist[i+1];
  }
  flist[max_files] = NULL;
  if (XtIsRealized(popup_file)) {
    XawListChange(file_list, flist, max_files, 0, True);
    if (n >= max_files) --n;
    XawListHighlight(file_list, n);
  }
  if (psmenu != NULL) { 
    free(psmenu);
    psmenu = NULL;
#ifndef TimNmenu
    if (sb != NULL) {
      free(sb);
      sb = NULL;
    }
#endif /* !TimNmenu */
  }
  XtDestroyWidget(title_sm);
  maxentry_on_a_menu = 0;
  submenu_n = 0;
  title_sm = XtVaCreatePopupShell("title_simplemenu",simpleMenuWidgetClass,
                                title_mb, XtNforeground,textcolor,
                                XtNbackground,textbgcolor, XtNsaveUnder,False,
                                XtNbackingStore,NotUseful, NULL);
  for(i=0; i<max_files; i++)
    addOneFile(max_files, i, flist[i]);
}

static void
fdelallCB(Widget w, XtPointer client_data, XtPointer call_data) {
  int i; char lbuf[50];

  stopCB(w, NULL, NULL);
  a_pipe_write("A");
  for(i=1; i<max_files; i++) free(flist[i]);
  /* We keep a single member in flist. Otherwise, Xaw will display the
   * widget's name as a member, also letting the user "delete" it (with a
   * crash to follow).
   */
  max_files = 0;
  current_n_displayed = 0;
  if (*flist == NULL) *flist = (String)safe_malloc(sizeof(char *));
  **flist = '\0';
  if (XtIsRealized(popup_file))
    XawListChange(file_list, flist, max_files?max_files:1, 0, True);
  XtVaSetValues(file_menu[ID_SAVE - 100].widget, XtNsensitive,False, NULL);
  if (psmenu != NULL) {
    free(psmenu);
    psmenu = NULL;
#ifndef TimNmenu
    if (sb != NULL) {
      free(sb);
      sb = NULL;
    }
#endif /* !TimNmenu */
  }
  XtDestroyWidget(title_sm);
  maxentry_on_a_menu = 0; submenu_n = 0;
  title_sm = XtVaCreatePopupShell("title_simplemenu",simpleMenuWidgetClass,
                                title_mb, XtNforeground,textcolor,
                                XtNbackground,textbgcolor, XtNsaveUnder,False,
                                XtNbackingStore,NotUseful, NULL);
  bsb = XtVaCreateManagedWidget("dummyfile",smeLineObjectClass,title_sm, NULL);
  snprintf(lbuf, sizeof(lbuf), "TiMidity++ %s", timidity_version);
  XtVaSetValues(title_mb, XtNlabel,lbuf, NULL);
  snprintf(window_title, sizeof(window_title), "%s : %s",
           APP_CLASS, app_resources.no_playing);
  XtVaSetValues(toplevel, XtNtitle,window_title, NULL);
  XtVaSetValues(tune_l, XtNlabel,"/ ----", NULL);
#ifndef WIDGET_IS_LABEL_WIDGET
  deleteTextACT(w, NULL, NULL, NULL);
#endif
}

static void
backspaceACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  XawTextPosition begin, end;
  XawTextBlock tb;

  XawTextGetSelectionPos(w, &begin, &end);
  if (begin == end) return;
  tb.firstPos = 0;
  tb.ptr = ".";
  tb.format = XawFmt8Bit;
  tb.length = strlen(tb.ptr);
  XawTextReplace(w, begin, end, &tb);
  XawTextSetInsertionPoint(w, begin+1);
}

#ifndef WIDGET_IS_LABEL_WIDGET
static void
deleteTextACT(Widget w, XEvent *e, String *v, Cardinal *n) {
#ifdef CLEARVALUE
  Widget TextSrc;

  XtVaGetValues(lyric_t, XtNtextSource,&TextSrc, NULL); 
  XawAsciiSourceFreeString(TextSrc);
#endif /* CLEARVALUE */
  XtVaSetValues(lyric_t, XtNstring,(String)"<< TiMidity Messages >>\n", NULL);
}
#endif

static void
scrollTextACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  int i, l = atoi(*v);

  for (i=0; i<l; i++)
    XtCallActionProc(lyric_t, "scroll-one-line-up", NULL, NULL, ZERO);
  for (i=0; i>l; i--)
    XtCallActionProc(lyric_t, "scroll-one-line-down", NULL, NULL, ZERO);
}

static void
createFlist(void) {

  if (popup_file == NULL) {
    Widget popup_fform, flist_cmdbox, popup_fplay, popup_fdelete,
           popup_fdelall, popup_fclose;

    popup_file = XtVaCreatePopupShell("popup_file",transientShellWidgetClass,
                    toplevel, NULL);
    popup_fform = XtVaCreateManagedWidget("popup_fform",formWidgetClass,
                    popup_file, XtNbackground,bgcolor,
                    XtNorientation,XtorientVertical, NULL);
    file_vport = XtVaCreateManagedWidget("file_vport",viewportWidgetClass,
                    popup_fform, XtNallowHoriz,True, XtNallowVert,True,
                    XtNleft,XawChainLeft, XtNright,XawChainRight,
                    XtNbottom,XawChainBottom, XtNbackground,textbgcolor, NULL);
    file_list = XtVaCreateManagedWidget("filelist",listWidgetClass,file_vport,
                    XtNbackground,textbgcolor, XtNverticalList,True,
                    XtNdefaultColumns,1, XtNforceColumns,True, NULL);
    flist_cmdbox = XtVaCreateManagedWidget("flist_cmdbox",boxWidgetClass,
                    popup_fform, XtNfromVert,file_vport, XtNright,XawChainLeft,
                    XtNbottom,XawChainBottom, XtNtop,XawChainBottom,
                    XtNorientation,XtorientHorizontal, XtNbackground,bgcolor,
                    NULL);
    popup_fplay = XtVaCreateManagedWidget("fplaybutton",commandWidgetClass,
                    flist_cmdbox, XtNfontSet,app_resources.volume_font, NULL);
    popup_fdelete = XtVaCreateManagedWidget("fdeletebutton",commandWidgetClass,
                    flist_cmdbox, XtNfontSet,app_resources.volume_font, NULL);
    popup_fdelall = XtVaCreateManagedWidget("fdelallbutton",commandWidgetClass,
                    flist_cmdbox, XtNfontSet,app_resources.volume_font, NULL);
    popup_fclose = XtVaCreateManagedWidget("closebutton",commandWidgetClass,
                    flist_cmdbox, XtNfontSet,app_resources.volume_font, NULL);
    XtAddCallback(popup_fclose, XtNcallback,closeWidgetCB,
                  (XtPointer)popup_file);
    XtAddCallback(popup_fplay, XtNcallback,fselectCB, NULL);
    XtAddCallback(popup_fdelete, XtNcallback,fdeleteCB, NULL);
    XtAddCallback(popup_fdelall, XtNcallback,fdelallCB, NULL);
    XtSetKeyboardFocus(popup_file, popup_fform);
#ifdef XawTraversal
    XtSetKeyboardFocus(file_list, popup_fform);
#endif /* XawTraversal */
    XawListChange(file_list, flist, 0, 0, True);
    if (max_files != 0) XawListHighlight(file_list, 0);
  }
}

#ifdef XDND
static void
xdnd_file_drop_handler(char *filename) {
  char local_buffer[PIPE_LENGTH], *fp;
  struct stat st;

  fp = filename;
  if (!strncmp(fp, "file:", 5)) {
    fp += 5;
    if (!strncmp(fp, "//", 2)) {
      fp += 2;
      if (*fp == '\0') return; /* path given was file :// */
      if (*fp != '/') {
        if (!strncmp(fp, "localhost/", 10)) {
          fp += 10;
        } else return;
       /*
        * This is either a remote URL (which we don't support)
        * or a malformed file://text URL
        */
      }
    }
  }

  if ((*fp == '~') && ((*(fp+1) == '/') || (*(fp+1) == '\0'))) {
    if (home == NULL) return;
    fp++;
    snprintf(local_buffer, sizeof(local_buffer), "X %s%s",
             home, fp);
  }
  else if ( (*fp == '/') && (*(fp+1) == '~') &&
            ((*(fp+2) == '/') || (*(fp+2) == '\0')) ) {
    if (home == NULL) return;
    fp += 2;
    snprintf(local_buffer, sizeof(local_buffer), "X %s%s",
             home, fp);
  }
  else
    snprintf(local_buffer, sizeof(local_buffer), "X %s",
             fp);

  if (stat(local_buffer + 2, &st) == -1) return;
  if (S_ISDIR(st.st_mode))
     strlcat(local_buffer, "/", sizeof(local_buffer));
  a_pipe_write("%s", local_buffer);
}

static void
a_dnd_init(void) {
  init_dnd(disp, dnd);
  enable_dnd_for_widget(dnd, toplevel, xdnd_file_drop_handler);
}

static void
enable_dnd_for_widget(DndClass *dnd, Widget widget, dnd_callback_t cb) {
  make_window_dnd_aware(dnd, XtWindow(widget), cb);
  XtAddEventHandler(widget, ClientMessage, True, 
                     xdnd_listener, (XtPointer)dnd);
  XtAddEventHandler(widget, SelectionNotify, True, 
                     xdnd_listener, (XtPointer)dnd);
}

static void
xdnd_listener(Widget w, XtPointer client_data, XEvent *ev, Boolean *dummy) {
  DndClass *dnd = (DndClass *)client_data;
  dnd->win = XtWindow(w);
  process_client_dnd_message(dnd, ev); 
}
#endif /* XDND */

static void
destroyWidgetCB(Widget w, XtPointer client_data, XtPointer call_data) {
  w = (Widget)client_data;
  if (XtIsRealized(XtParent(w))) {
    XWindowAttributes war;
    Window win;

    win = XtWindow(XtParent(w));
    XGetWindowAttributes(disp, win, &war);
    if (war.map_state == IsViewable)
      XSetInputFocus(disp, win, RevertToParent, CurrentTime);
  }
#ifdef RECFMTGROUPDESTHACK
  if (seekTransientShell(record->formatGroup) == w) {
    XtDestroyWidget(XtParent(XtParent(record->formatGroup)));
    record->formatGroup = NULL;
  }
#endif /* RECFMTGROUPDESTHACK */
  XtDestroyWidget(w);
}

static void
setupWindow(Widget w, String action, Boolean xdnd,
            Boolean destroyWidgetOnPopdown) {
  char s[255];

  snprintf(s, sizeof(s), "<Message>WM_PROTOCOLS: %s", action);
  XtOverrideTranslations(w, XtParseTranslationTable(s));
  if (destroyWidgetOnPopdown == True)
    XtAddCallback(w, XtNpopdownCallback,destroyWidgetCB, (XtPointer)w);
  XtPopup(w, XtGrabNone);
  XSetWMProtocols(disp, XtWindow(w), &wm_delete_window, 1);
#ifdef HAVE_GETPID
  XChangeProperty(disp, XtWindow(w), net_wm_pid, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&pid, 1);
  /* The WM spec requires setting up WM_CLIENT_MACHINE along with
   * _NET_WM_PID. Xt sets it up automatically for us.
   */
#endif
#ifdef XDND
  if (xdnd == True) enable_dnd_for_widget(dnd, w, xdnd_file_drop_handler);
#endif /* XDND */
}

static void
callRedrawTrace(Boolean draw) {
  if ((ctl->trace_playing) && (XtIsRealized(trace))) redrawTrace(draw);
}

static void
createTraceWidgets(void) {

  if ((app_resources.tracecfg.trace_height > TRACE_HEIGHT_WITH_FOOTER) ||
      (app_resources.tracecfg.trace_height < BAR_SPACE+TRACE_FOOT+TRACE_HEADER))
    trace_v_height = TRACE_HEIGHT_WITH_FOOTER;
  else
    trace_v_height = app_resources.tracecfg.trace_height;
  trace_vport = XtVaCreateManagedWidget("trace_vport",viewportWidgetClass,
          base_f, XtNbottom,XawChainBottom, XtNtop,XawChainTop,
          XtNleft,XawChainLeft, XtNright,XawChainLeft,
          XtNfromVert,(XtIsManaged(lyric_t))?lyric_t:t_box,
          XtNallowHoriz,True, XtNallowVert,True,
          XtNwidth,TRACE_WIDTH+8, XtNheight,trace_v_height, NULL);
  trace = XtVaCreateManagedWidget("trace",widgetClass,trace_vport,
          XtNheight,app_resources.tracecfg.trace_height,
          XtNwidth,app_resources.tracecfg.trace_width, NULL);
  Cfg.disptrace = True;

  XtSetKeyboardFocus(trace, base_f);

  fast_b = XtVaCreateManagedWidget("fast_button",commandWidgetClass,b_box,
              XtNforeground,togglecolor, XtNbackground,buttonbgcolor,
              XtNbitmap,GET_BITMAP(fast), XtNfromHoriz,repeat_b, NULL);
  slow_b = XtVaCreateManagedWidget("slow_button",commandWidgetClass,b_box,
                XtNforeground,togglecolor, XtNbackground,buttonbgcolor,
                XtNbitmap,GET_BITMAP(slow), XtNfromHoriz,fast_b, NULL);
  keyup_b = XtVaCreateManagedWidget("keyup_button",commandWidgetClass,b_box,
              XtNforeground,togglecolor, XtNbackground,buttonbgcolor,
              XtNbitmap,GET_BITMAP(keyup), XtNfromHoriz,slow_b, NULL);
  keydown_b = XtVaCreateManagedWidget("keydown_button",commandWidgetClass,b_box,
                XtNforeground,togglecolor, XtNbackground,buttonbgcolor,
                XtNbitmap,GET_BITMAP(keydown), XtNfromHoriz,keyup_b, NULL);

  XtAddCallback(fast_b, XtNcallback,tempoCB, (XtPointer)TRUE);
  XtAddCallback(slow_b, XtNcallback,tempoCB, (XtPointer)FALSE);
  XtAddCallback(keyup_b, XtNcallback,pitchCB, (XtPointer)TRUE);
  XtAddCallback(keydown_b, XtNcallback,pitchCB, (XtPointer)FALSE);

#ifndef XawTraversal
  XtSetKeyboardFocus(fast_b, base_f);
  XtSetKeyboardFocus(slow_b, base_f);
  XtSetKeyboardFocus(keyup_b, base_f);
  XtSetKeyboardFocus(keydown_b, base_f);
#endif /* !XawTraversal */
}

static void
callInitTrace(void) {
  initTrace(disp, XtWindow(trace), window_title, &(app_resources.tracecfg));
}

static void
createBars(void) {
  int i, j, k, l, vol_hd, tune_hd;
#ifdef SCROLLBARLENGTHBUG
  int m, m2;
#endif /* !SCROLLBARLENGTHBUG */
  barfloat thumb;

  Cardinal num_actions;
  XtActionList action_list;
  static XtActionsRec startscroll_act[] = {
    {"StartScroll", StartScrollACT}
  };

  v_box = XtVaCreateManagedWidget("volume_box",formWidgetClass,base_f,
            XtNorientation,XtorientHorizontal, XtNheight,36,
            XtNtop,XawChainTop, XtNbottom,XawChainTop,
            XtNleft,XawChainLeft, XtNright,XawChainLeft,
            XtNfromVert,b_box, XtNbackground,bgcolor, NULL);
  t_box = XtVaCreateManagedWidget("tune_box",formWidgetClass,base_f,
            XtNorientation,XtorientHorizontal,
            XtNtop,XawChainTop, XtNbottom,XawChainTop,
            XtNleft,XawChainLeft, XtNright,XawChainLeft,
            XtNheight,36, XtNfromVert,v_box, XtNbackground,bgcolor, NULL);
  i = XmbTextEscapement(app_resources.volume_font, "Volume ", 7)+8;
  j = snprintf(local_buf, sizeof(local_buf), "%3d  ", amplitude);
  j = XmbTextEscapement(app_resources.volume_font, local_buf, j)+8;
  k = XmbTextEscapement(app_resources.volume_font, " 0:00", 5)+8;
  l = XmbTextEscapement(app_resources.volume_font, "/ 00:00", 7)+8;
  if (k+l > i+j) {
    vol_hd = k+l-i-j;
    tune_hd = 0;
  } else {
    tune_hd = i+j-k-l;
    vol_hd = 0;
  }
  vol_l0 = XtVaCreateManagedWidget("volume_label0",labelWidgetClass,v_box,
            XtNwidth,i, XtNfontSet,app_resources.volume_font,
            XtNlabel,"Volume", XtNborderWidth,0, XtNforeground,textcolor,
            XtNleft,XawChainLeft, XtNright,XawChainLeft,
            XtNbackground,bgcolor, NULL);
  vol_l = XtVaCreateManagedWidget("volume_label",labelWidgetClass,v_box,
            XtNwidth,j, XtNborderWidth,0, XtNfromHoriz,vol_l0,
            XtNfontSet,app_resources.volume_font,
            XtNorientation,XtorientHorizontal, XtNforeground,textcolor,
            XtNleft,XawChainLeft, XtNright,XawChainLeft,
            XtNbackground,bgcolor, XtNlabel,local_buf, NULL);
  /* VOLUME_LABEL_WIDTH = i+30+j */
  vol_bar = XtVaCreateManagedWidget("volume_bar",scrollbarWidgetClass,v_box,
            XtNorientation,XtorientHorizontal, XtNfromHoriz,vol_l,
            XtNwidth,DEFAULT_REG_WIDTH - (i+30+j+vol_hd),
            XtNhorizDistance,vol_hd,
            XtNleft,XawChainLeft, XtNright,XawChainLeft,
            XtNbackground,textbgcolor, NULL);
  thumb.f = (float)amplitude / (float)MAXVOLUME;
  setThumb(vol_bar, thumb);
  tune_l0 = XtVaCreateManagedWidget("tune_label0",labelWidgetClass,t_box,
            XtNwidth,k, XtNlabel,"0:00",
            XtNfontSet,app_resources.volume_font,
            XtNborderWidth,0, XtNforeground,textcolor,
            XtNleft,XawChainLeft, XtNright,XawChainLeft,
            XtNbackground,bgcolor, NULL);
  tune_l = XtVaCreateManagedWidget("tune_label",labelWidgetClass,t_box,
            XtNwidth,l, XtNfontSet,app_resources.volume_font,
            XtNfromHoriz,tune_l0, XtNborderWidth,0, XtNforeground,textcolor,
            XtNleft,XawChainLeft, XtNright,XawChainLeft,
            XtNbackground,bgcolor, NULL);
  tune_bar = XtVaCreateManagedWidget("tune_bar",scrollbarWidgetClass,t_box,
            XtNwidth,DEFAULT_REG_WIDTH-(k+l+30+tune_hd),
            XtNbackground,textbgcolor,
            XtNhorizDistance,tune_hd, XtNfromHoriz,tune_l,
            XtNleft,XawChainLeft, XtNright,XawChainLeft,
            XtNorientation,XtorientHorizontal, NULL);
#ifdef SCROLLBARLENGTHBUG
  XtVaGetValues(vol_bar, XtNminimumThumb,&m, NULL);
  XtVaGetValues(tune_bar, XtNminimumThumb,&m2, NULL);
  XtVaSetValues(vol_bar,
                  XtNlength,DEFAULT_REG_WIDTH-(i+j+30+vol_hd)-m, NULL);
  XtVaSetValues(tune_bar,
                  XtNlength,DEFAULT_REG_WIDTH-(k+l+30+tune_hd)-m2, NULL);
#endif /* SCROLLBARLENGTHBUG */

 /*
  * StartScroll does not exist when Xaw3d is compiled with XAW_ARROW_SCROLLBARS,
  * or when neXtaw or XawPlus is used. Thus, a runtime check sees if it exists.
  * If it does not, A replacement is defined to avoid warnings, and to allow
  * for scrolling with the wheel buttons.
  * Note that this check could work only after the scrollbar class is inited.
  */
  XtGetActionList(scrollbarWidgetClass, &action_list, &num_actions);
  j = (int)num_actions;
  for (i = 0; i < j; i++)
    if (!strcasecmp(action_list[i].string, "StartScroll")) return;
  XtAppAddActions(app_con, startscroll_act, XtNumber(startscroll_act));
  use_own_start_scroll = True;
}

static void
createButtons(void) {
  play_b = XtVaCreateManagedWidget("play_button",toggleWidgetClass,b_box,
            XtNbitmap,GET_BITMAP(play), XtNforeground,buttoncolor,
            XtNbackground,buttonbgcolor, NULL);
  pause_b = XtVaCreateManagedWidget("pause_button",toggleWidgetClass,b_box,
            XtNbitmap,GET_BITMAP(pause), XtNfromHoriz,play_b,
            XtNforeground,togglecolor, XtNbackground,buttonbgcolor, NULL);
  stop_b = XtVaCreateManagedWidget("stop_button",commandWidgetClass,b_box,
            XtNbitmap,GET_BITMAP(stop), XtNforeground,buttoncolor,
            XtNbackground,buttonbgcolor, XtNfromHoriz,pause_b, NULL);
  prev_b = XtVaCreateManagedWidget("prev_button",commandWidgetClass,b_box,
            XtNbitmap,GET_BITMAP(prev), XtNforeground,buttoncolor,
            XtNbackground,buttonbgcolor, XtNfromHoriz,stop_b, NULL);
  back_b = XtVaCreateManagedWidget("back_button",commandWidgetClass,b_box,
            XtNbitmap,GET_BITMAP(back), XtNforeground,buttoncolor,
            XtNbackground,buttonbgcolor, XtNfromHoriz,prev_b, NULL);
  fwd_b = XtVaCreateManagedWidget("fwd_button",commandWidgetClass,b_box,
            XtNbitmap,GET_BITMAP(fwrd), XtNforeground,buttoncolor,
            XtNbackground,buttonbgcolor, XtNfromHoriz,back_b, NULL);
  next_b = XtVaCreateManagedWidget("next_button",commandWidgetClass,b_box,
            XtNbitmap,GET_BITMAP(next), XtNforeground,buttoncolor,
            XtNbackground,buttonbgcolor, XtNfromHoriz,fwd_b, NULL);
  quit_b = XtVaCreateManagedWidget("quit_button",commandWidgetClass,b_box,
            XtNbitmap,GET_BITMAP(quit), XtNforeground,buttoncolor,
            XtNbackground,buttonbgcolor, XtNfromHoriz,next_b, NULL);
  random_b = XtVaCreateManagedWidget("random_button",toggleWidgetClass,b_box,
            XtNbitmap,GET_BITMAP(random), XtNfromHoriz,quit_b,
            XtNforeground,togglecolor, XtNbackground,buttonbgcolor, NULL);
  repeat_b = XtVaCreateManagedWidget("repeat_button",toggleWidgetClass,b_box,
            XtNbitmap,GET_BITMAP(repeat2), XtNfromHoriz,random_b,
            XtNforeground,togglecolor, XtNbackground,buttonbgcolor, NULL);

  XtAddCallback(quit_b, XtNcallback,quitCB, NULL);
  XtAddCallback(play_b, XtNcallback,playCB, NULL);
  XtAddCallback(pause_b, XtNcallback,pauseCB, NULL);
  XtAddCallback(stop_b, XtNcallback,stopCB, NULL);
  XtAddCallback(prev_b, XtNcallback,prevCB, NULL);
  XtAddCallback(next_b, XtNcallback,nextCB, NULL);
  XtAddCallback(fwd_b, XtNcallback,forwardCB, NULL);
  XtAddCallback(back_b, XtNcallback,backCB, NULL);
  XtAddCallback(random_b, XtNcallback,randomCB, NULL);
  XtAddCallback(repeat_b, XtNcallback,repeatCB, NULL);

#ifndef XawTraversal
  XtSetKeyboardFocus(play_b, base_f);
  XtSetKeyboardFocus(stop_b, base_f);
  XtSetKeyboardFocus(prev_b, base_f);
  XtSetKeyboardFocus(back_b, base_f);
  XtSetKeyboardFocus(fwd_b, base_f);
  XtSetKeyboardFocus(next_b, base_f);
  XtSetKeyboardFocus(quit_b, base_f);
  XtSetKeyboardFocus(random_b, base_f);
  XtSetKeyboardFocus(repeat_b, base_f);
#endif /* !XawTraversal */
}

static void
init_output_lists(void) {
  int i = 0, j = 0;

  play = (outputs *)safe_malloc(sizeof(outputs));
  record = (outputs *)safe_malloc(sizeof(outputs));
  play->output_list = (id_list *)safe_malloc(sizeof(id_list) * 7);
  record->output_list = (id_list *)safe_malloc(sizeof(id_list) * 7);
  play->current = play->def = 0; play->formatGroup = NULL;
  record->current = record->def = 0; record->formatGroup = NULL;
  while (strcmp(local_buf, "Z0")) {
    a_pipe_read(local_buf, sizeof(local_buf));
    if ((*local_buf == 'F') || (*local_buf == 'f')) {
      record->output_list[i].id_char = *(local_buf + 1);
      record->output_list[i].id_name = safe_strdup(local_buf + 3);
      if (((i+1) % 7) == 0) record->output_list = (id_list *)safe_realloc(
                                                          record->output_list,
                                                          sizeof(id_list) *
                                                          (7 * ((i+1)/7 + 1)));
      i++;
    } else if ((*local_buf == 'O') || (*local_buf == 'o')) {
      play->output_list[j].id_char = *(local_buf + 1);
      play->output_list[j].id_name = safe_strdup(local_buf + 3);
      if (((j+1) % 7) == 0) play->output_list = (id_list *)safe_realloc(
                                                         play->output_list,
                                                         sizeof(id_list) *
                                                         (7 * ((j+1)/7 + 1)));
      if (*local_buf == 'o') play->def = play->current = j;
      j++;
    }
  }
  if (i == 0) {
    free(record->output_list);
    free(record);
    record = NULL;
    XtVaSetValues(file_menu[ID_SAVE - 100].widget, XtNsensitive,False, NULL);
  } else {
    record->output_list = (id_list *)safe_realloc(record->output_list,
                                                sizeof(id_list) * i);
    record->max = i;
    record->lbuf = NULL;
  }
  if (j == 0) {
    free(play->output_list);
    free(play);
    play = NULL;
  } else {
    play->output_list = (id_list *)safe_realloc(play->output_list,
                                               sizeof(id_list) * j);
    play->max = j;
    play->lbuf = NULL;
  }
}

static void
a_init_interface(int pipe_in) {
  static XtActionsRec actions[] = {
    {"do-quit", (XtActionProc)quitCB},
    {"show-menu", popupfilemenuACT},
    {"hide-menu", popdownfilemenuACT},
#ifdef OLDXAW
    {"fix-menu", (XtActionProc)filemenuCB},
#endif
    {"do-menu", filemenuACT},
    {"do-addall", popdownAddALLACT},
    {"do-complete", completeDirACT},
    {"do-chgdir", setDirACT},
    {"draw-trace", redrawACT},
    {"do-exchange", exchgWidthACT},
    {"do-toggletrace", toggleTraceACT},
    {"do-mutechan", muteChanACT},
    {"do-revcaption", redrawCaptionACT},
    {"do-popdown", (XtActionProc)popdownCB},
    {"do-play", (XtActionProc)playCB},
    {"do-sndspec", sndspecACT},
    {"do-pause", pauseACT},
    {"do-stop", (XtActionProc)stopCB},
    {"do-next", (XtActionProc)nextCB},
    {"do-prev", (XtActionProc)prevCB},
    {"do-forward", (XtActionProc)forwardCB},
    {"do-back", (XtActionProc)backCB},
    {"do-key", soundkeyACT},
    {"do-speed", speedACT},
    {"do-voice", voiceACT},
    {"do-volupdown", volupdownACT},
    {"do-tuneset", tunesetACT},
    {"do-resize", resizeToplevelACT},
    {"do-scroll", scrollListACT},
#ifndef WIDGET_IS_LABEL_WIDGET
    {"do-deltext", deleteTextACT},
#endif
    {"do-scroll-lyrics", scrollTextACT},
#ifdef TimNmenu
    {"checkRightAndPopupSubmenu", checkRightAndPopupSubmenuACT},
#endif
    {"leaveSubmenu", leaveSubmenuACT},
    {"popdownSubmenu", popdownSubmenuACT},
    {"do-options", optionspopupACT},
    {"do-optionsclose", (XtActionProc)optionscloseCB},
    {"do-filelist", flistpopupACT},
    {"do-about", aboutACT},
    {"do-toggle-tooltips", xawtipsetACT},
    {"do-closeparent", closeParentACT},
    {"do-flistmove", flistMoveACT},
    {"do-fselect", (XtActionProc)fselectCB},
    {"do-fdelete", (XtActionProc)fdeleteCB},
    {"do-fdelall", (XtActionProc)fdelallCB},
    {"do-backspace", backspaceACT},
    {"do-cancel", cancelACT},
    {"do-ok", okACT},
    {"do-up", upACT},
    {"do-down", downACT},
    {"do-record", recordACT},
    {"changetrace", scrollTraceACT}
  };

  XtResource xaw_resources[] = {
#define offset(entry) XtOffset(struct _app_resources*, entry)
#define toffset(entry) XtOffset(struct _app_resources*, tracecfg.entry)
  {"arrangeTitle", "ArrangeTitle", XtRBoolean, sizeof(Boolean),
   offset(arrange_title), XtRImmediate, (XtPointer)False},
  {"gradientBar", "GradientBar", XtRBoolean, sizeof(Boolean),
   toffset(gradient_bar), XtRImmediate, (XtPointer)False},
#ifdef WIDGET_IS_LABEL_WIDGET
  {"textLHeight", "TextLHeight", XtRShort, sizeof(short),
   offset(text_height), XtRImmediate, (XtPointer)30},
#else
  {"textHeight", "TextHeight", XtRShort, sizeof(short),
   offset(text_height), XtRImmediate, (XtPointer)120},
#endif /* WIDGET_IS_LABEL_WIDGET */
  {"traceWidth", "TraceWidth", XtRShort, sizeof(short),
   toffset(trace_width), XtRImmediate, (XtPointer)TRACE_WIDTH},
  {"traceHeight", "TraceHeight", XtRShort, sizeof(short),
   toffset(trace_height), XtRImmediate, (XtPointer)TRACE_HEIGHT_WITH_FOOTER},
  {"menuWidth", "MenuWidth", XtRShort, sizeof(Dimension),
   offset(menu_width), XtRImmediate, (XtPointer)200},
  {"foreground", XtCForeground, XtRPixel, sizeof(Pixel),
   toffset(common_fgcolor), XtRString, "black"},
  {"background", XtCBackground, XtRPixel, sizeof(Pixel),
   offset(common_bgcolor), XtRString, COMMON_BGCOLOR},
  {"menubutton", "MenuButtonBackground", XtRPixel, sizeof(Pixel),
   offset(menub_bgcolor), XtRString, "#CCFF33"},
  {"textbackground", "TextBackground", XtRPixel, sizeof(Pixel),
   toffset(text_bgcolor), XtRString, TEXTBG_COLOR},
  {"text2background", "Text2Background", XtRPixel, sizeof(Pixel),
   offset(text2_bgcolor), XtRString, "gray80"},
  {"toggleforeground", "ToggleForeground", XtRPixel, sizeof(Pixel),
   offset(toggle_fgcolor), XtRString, "MediumBlue"},
  {"buttonforeground", "ButtonForeground", XtRPixel, sizeof(Pixel),
   offset(button_fgcolor), XtRString, "blue"},
  {"buttonbackground", "ButtonBackground", XtRPixel, sizeof(Pixel),
   offset(button_bgcolor), XtRString, COMMANDBUTTON_COLOR},
  {"velforeground", "VelForeground", XtRPixel, sizeof(Pixel),
   toffset(velocity_color), XtRString, "orange"},
  {"veldrumforeground", "VelDrumForeground", XtRPixel, sizeof(Pixel),
   toffset(drumvelocity_color), XtRString, "red"},
  {"volforeground", "VolForeground", XtRPixel, sizeof(Pixel),
   toffset(volume_color), XtRString, "LightPink"},
  {"expforeground", "ExpForeground", XtRPixel, sizeof(Pixel),
   toffset(expr_color), XtRString, "aquamarine"},
  {"panforeground", "PanForeground", XtRPixel, sizeof(Pixel),
   toffset(pan_color), XtRString, "blue"},
  {"tracebackground", "TraceBackground", XtRPixel, sizeof(Pixel),
   toffset(trace_bgcolor), XtRString, "gray90"},
  {"rimcolor", "RimColor", XtRPixel, sizeof(Pixel),
   toffset(rim_color), XtRString, "gray20"},
  {"boxcolor", "BoxColor", XtRPixel, sizeof(Pixel),
   toffset(box_color), XtRString, "gray76"},
  {"captioncolor", "CaptionColor", XtRPixel, sizeof(Pixel),
   toffset(caption_color), XtRString, "DarkSlateGrey"},
  {"sustainedkeycolor", "SustainedKeyColor", XtRPixel, sizeof(Pixel),
   toffset(sus_color), XtRString, "red4"},
  {"whitekeycolor", "WhiteKeyColor", XtRPixel, sizeof(Pixel),
   toffset(white_key_color), XtRString, "white"},
  {"blackkeycolor", "BlackKeyColor", XtRPixel, sizeof(Pixel),
   toffset(black_key_color), XtRString, "black"},
  {"playingkeycolor", "PlayingKeyColor", XtRPixel, sizeof(Pixel),
   toffset(play_color), XtRString, "maroon1"},
  {"reverbcolor", "ReverbColor", XtRPixel, sizeof(Pixel),
   toffset(rev_color), XtRString, "PaleGoldenrod"},
  {"choruscolor", "ChorusColor", XtRPixel, sizeof(Pixel),
   toffset(cho_color), XtRString, "yellow"},
  {"labelfont", XtCFontSet, XtRFontSet, sizeof(XFontSet *),
   offset(label_font), XtRString,
       "-adobe-helvetica-bold-r-*-*-14-*-75-75-*-*-*-*,*"},
  {"volumefont", XtCFontSet, XtRFontSet, sizeof(XFontSet *),
   offset(volume_font), XtRString,
       "-adobe-helvetica-bold-r-*-*-12-*-75-75-*-*-*-*,*"},
  {"textfontset", XtCFontSet, XtRFontSet, sizeof(XFontSet),
   offset(text_font), XtRString, "-*-*-medium-r-normal--14-*-*-*-*-*-*-*,*"},
  {"ttitlefont", XtCFontSet, XtRFontSet, sizeof(XFontSet),
   toffset(ttitle_font), XtRString,
       "-*-fixed-medium-r-normal--14-*-*-*-*-*-*-*,*"},
  {"tracefont", XtCFontSet, XtRFontSet, sizeof(XFontSet *),
   toffset(trace_font), XtRString, "7x14,*"},
  {"labelfile", "LabelFile", XtRString, sizeof(String),
   offset(file_text), XtRString, "file..."},
  {"popup_confirm_title", XtCString, XtRString, sizeof(String),
   offset(popup_confirm), XtRString, "Dialog"},
  {"moreString", XtCString, XtRString, sizeof(String),
   offset(more_text), XtRString, "More..."},
  {"noplaying", XtCString, XtRString, sizeof(String),
   offset(no_playing), XtRString, "[ No Playing File ]"},
  {"untitled", XtCString, XtRString, sizeof(String),
   toffset(untitled), XtRString, "<No Title>"},
  {"load_" LISTDIALOGBASENAME ".title", XtCString, XtRString,
   sizeof(String), offset(load_LISTDIALOGBASENAME_title), XtRString,
   "TiMidity <Load Playlist>"},
  {"save_" LISTDIALOGBASENAME ".title", XtCString, XtRString,
   sizeof(String), offset(save_LISTDIALOGBASENAME_title), XtRString,
   "TiMidity <Save Playlist>"}
#undef offset
#undef toffset
  };

  String fallback_resources[] = {
    "*international: True",
    "*Label*fontSet: -adobe-helvetica-bold-o-*-*-14-*-*-*-*-*-*-*,*",
    "*MenuButton*fontSet: -adobe-helvetica-bold-r-*-*-14-*-*-*-*-*-*-*,*",
    "*Command*fontSet: -adobe-helvetica-bold-r-*-*-12-*-*-*-*-*-*-*,*",
    "*List*fontSet: -adobe-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*,*",
    "*Dialog*List*fontSet: -adobe-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*,*",
    "*Dialog*fontSet: -adobe-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*,*",
    "*Text*fontSet: -misc-fixed-medium-r-normal--14-*-*-*-*-*-*-*,*",
    "*Text*background: " TEXTBG_COLOR "",
    "*Text*scrollbar*background: " TEXTBG_COLOR "",
    "*Scrollbar*background: " TEXTBG_COLOR "",
    "*Label.foreground: black",
    "*Label.background: #CCFF33",
    "*Command.background: " COMMANDBUTTON_COLOR "",
    "*Tip.fontSet: -misc-fixed-medium-r-normal-*-10-*-*-*-*-*-*-*",
    "*Tip.background: white",
    "*Tip.foreground: black",
    "*Tip.borderColor: black",
    "*Dialog.Command.background: " COMMANDBUTTON_COLOR "",
    "*Dialog.Text.background: " TEXTBG_COLOR "",
    "*fontSet: -*--14-*",
    "*load_dialog.label.background: " COMMON_BGCOLOR "",
    "*Command.fontSet: -adobe-helvetica-bold-r-*-*-12-*-*-*-*-*-*-*,*",
    "*Toggle.fontSet: -adobe-helvetica-medium-o-*-*-12-*-*-*-*-*-*-*,*",
#ifdef OLDXAW
    "*MenuButton.translations:<EnterWindow>:    highlight()\\n\
        <LeaveWindow>:  reset()\\n\
        Any<BtnDown>:   reset() fix-menu() PopupMenu()",
#endif
    "*menu_box.borderWidth: 0",
    "*button_box.borderWidth: 0",
    "*button_box.horizDistance: 4",
    "*file_menubutton.menuName: file_simplemenu",
    "*file_menubutton.width: 60",
    "*file_menubutton.height: 28",
    "*file_menubutton.horizDistance: 6",
    "*file_menubutton.vertDistance: 4",
    "*file_menubutton.file_simplemenu*fontSet: -*--14-*",
    "*title_menubutton.title_simplemenu*fontSet: \
         -adobe-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*,*",
    "*title_menubutton*SmeBSB.fontSet: \
         -adobe-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*,*",
    "*title_menubutton.menuName: title_simplemenu",
    "*OK.label: OK",
    "*Cancel.label: Cancel",
    "*title_menubutton.width: 210",
    "*title_menubutton.height: 28",
    "*title_menubutton.resize: false",
    "*title_menubutton.horizDistance: 6",
    "*title_menubutton.vertDistance: 4",
    "*title_menubutton.fromHoriz: file_menubutton",
    "*time_label.width: 92",
    "*time_label.height: 26",
    "*time_label.resize: false",
    "*time_label.fromHoriz: title_menubutton",
    "*time_label.horizDistance: 1",
    "*time_label.vertDistance: 4",
    "*time_label.label: time / mode",
    "*button_box.height: 40",
    "*button_box*Command.width: 32",
    "*button_box*Command.height: 32",
    "*button_box*Toggle.width: 32",
    "*button_box*Toggle.height: 32",
    "*button_box*Command.horizDistance: 1",
    "*button_box*Command.vertDistance: 1",
    "*button_box*Toggle.horizDistance: 1",
    "*button_box*Toggle.vertDistance: 1",
    "*random_button.horizDistance: 4",
    "*play_button.vertDistance: 9",
    "*play_button.tip: Play",
    "*pause_button.tip: Pause",
    "*stop_button.tip: Stop",
    "*prev_button.tip: Previous",
    "*back_button.tip: Back",
    "*fwd_button.tip: Forward",
    "*next_button.tip: Next",
    "*quit_button.tip: Quit",
    "*random_button.tip: Shuffle",
    "*repeat_button.tip: Repeat",
    "*fast_b.tip: Increase tempo",
    "*slow_b.tip: Decrease Tempo",
    "*keyup_b.tip: Raise pitch",
    "*keydown_b.tip: Lower pitch",
    "*repeat_button.tip: Repeat",
    "*repeat_button.tip: Repeat",
    "*repeat_button.tip: Repeat",
    "*repeat_button.tip: Repeat",
    "*lyric_text.fromVert: tune_box",
    "*lyric_text.borderWidth: 1" ,
    "*lyric_text.vertDistance: 4",
    "*lyric_text.horizDistance: 6",
#ifndef WIDGET_IS_LABEL_WIDGET
    "*lyric_text.height: 120",
    "*lyric_text.scrollVertical: Always",
    "*lyric_text.translations: #override\\n\
        <Btn2Down>:	do-deltext()\\n\
        <Btn4Down>:	do-scroll-lyrics(-1)\\n\
        <Btn5Down>:	do-scroll-lyrics(1)",
#else
    "*lyric_text.height: 30",
    "*lyric_text.label: MessageWindow",
    "*lyric_text.resize: true",
#endif
    "*volume_box*fontSet: -adobe-helvetica-bold-r-*-*-12-*-*-*-*-*-*-*,*",
    "*tune_box*fontSet: -adobe-helvetica-bold-r-*-*-12-*-*-*-*-*-*-*,*",
    "*volume_box*horizDistance: 0",
    "*volume_box*vertDistance: 0",
    "*volume_box.vertDistance: 2",
    "*volume_box.borderWidth: 0",
    "*volume_label.borderWidth: 0",
    "*tune_box.borderWidth: 0",
    "*tune_label.label: / ----",
    "*tune_box*horizDistance: 0",
    "*tune_box*vertDistance: 0",
    "*tune_box.vertDistance: 2",
    "*popup_option.title: TiMidity <Extend Modes>",
    "*popup_file.title: TiMidity <File List>",
    "*dialog_lfile.title: TiMidity <Load File>",
    "*dialog_sfile.title: TiMidity <Save File>",
    "*popup_about.title: Information",
    "*popup_warning.title: Information",
    "*popup_sformat.title: Dialog",
    "*popup_slabel.label: Select output format",
    "*popup_olabel.label: Output device",
    "*load_dialog.label: File Name",
    "*load_dialog.borderWidth: 0",
    "*load_dialog.height: 400",
    "*trace.vertDistance: 2",
    "*trace.borderWidth: 1",
    "*trace_vport.borderWidth: 1",
    "*trace_vport.useRight: True",
    "*popup_optform*Box*borderWidth: 0",
    "*load_dialog.label.fontSet: -*--14-*",
    "*popup_abox*fontSet: -adobe-helvetica-bold-o-*-*-14-*-*-*-*-*-*-*,*",
    "*popup_cform*fontSet: -adobe-helvetica-bold-o-*-*-14-*-*-*-*-*-*-*,*",
    "*popup_optform*fontSet: -adobe-helvetica-bold-o-*-*-14-*-*-*-*-*-*-*,*",
    "*popup_sform*fontSet: -adobe-helvetica-bold-o-*-*-14-*-*-*-*-*-*-*,*",
    "*popup_wbox*fontSet: -adobe-helvetica-bold-o-*-*-14-*-*-*-*-*-*-*,*",
    "*cwd_label.fontSet: -adobe-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*,*",
    "*time_label*cwd_info.fontSet: \
           -adobe-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*,*",
    "*time_label.fontSet: -adobe-helvetica-bold-r-*-*-14-*-*-*-*-*-*-*,*",
    "*volume_bar.translations: #override\\n\
        ~Ctrl Shift<Btn1Down>: do-volupdown(-50)\\n\
        ~Ctrl Shift<Btn3Down>: do-volupdown(50)\\n\
        ~Ctrl Shift<Btn2Down>: do-volupdown(1)\\n\
        Ctrl ~Shift<Btn2Down>: do-volupdown(-1)\\n\
        Ctrl ~Shift<Btn1Down>: do-volupdown(-5)\\n\
        Ctrl ~Shift<Btn3Down>: do-volupdown(5)\\n\
        <BtnDown>: StartScroll(Continuous) MoveThumb() NotifyThumb()\\n\
        <BtnUp>: NotifyScroll(FullLength) EndScroll()",
    "*tune_bar.translations: #override\\n\
        <BtnDown>:	StartScroll(Continuous) MoveThumb() NotifyThumb()\\n\
        <BtnMotion>:	MoveThumb() NotifyThumb()\\n\
        <BtnUp>:	do-tuneset() NotifyScroll(FullLength) EndScroll()",
    "*file_simplemenu.load.label: Load (Meta-N)",
    "*file_simplemenu.load.underline: 0",
    "*file_simplemenu.save.label: Save (Ctrl-V)",
    "*file_simplemenu.save.underline: 2",
    "*file_simplemenu.load_playlist.label: Load Playlist (Meta-L)",
    "*file_simplemenu.load_playlist.underline: 3",
    "*file_simplemenu.save_playlist.label: Save Playlist (Meta-P)",
    "*file_simplemenu.save_playlist.underline: 5",
    "*file_simplemenu.saveconfig.label: Save Config (Meta-S)",
    "*file_simplemenu.saveconfig.underline: 0",
    "*file_simplemenu.hidetext.label: (Un)Hide Messages (Ctrl-M)",
    "*file_simplemenu.hidetext.underline: 9",
    "*file_simplemenu.hidetrace.label: (Un)Hide Trace (Ctrl-T)",
    "*file_simplemenu.hidetrace.underline: 9",
    "*file_simplemenu.shuffle.label: Shuffle (Ctrl-S)",
    "*file_simplemenu.shuffle.underline: 1",
    "*file_simplemenu.repeat.label: Repeat (Ctrl-R)",
    "*file_simplemenu.repeat.underline: 0",
    "*file_simplemenu.autostart.label: Auto Start",
    "*file_simplemenu.autostart.underline: 1",
    "*file_simplemenu.autoquit.label: Auto Exit",
    "*file_simplemenu.autoquit.underline: 5",
    "*file_simplemenu.filelist.label: File List (Ctrl-F)",
    "*file_simplemenu.filelist.underline: 0",
    "*file_simplemenu.modes.label: Extend Modes (Ctrl-O)",
    "*file_simplemenu.modes.underline: 8",
    "*file_simplemenu.about.label: About",
    "*file_simplemenu.about.underline: 0",
    "*file_simplemenu.quit.label: Quit (Meta-Q, Q)",
    "*file_simplemenu.quit.underline: 0",
    "*file_simplemenu.translations: #override\\n\
        <Key>n:		MenuPopdown() do-menu(" S(ID_LOAD) ")\\n\
        ~Meta<Key>l:	MenuPopdown() do-menu(" S(ID_LOAD) ")\\n\
        <Key>v:		MenuPopdown() do-menu(" S(ID_SAVE) ")\\n\
        Meta<Key>l:	MenuPopdown() do-menu(" S(ID_LOAD_PLAYLIST) ")\\n\
        <Key>d:		MenuPopdown() do-menu(" S(ID_LOAD_PLAYLIST) ")\\n\
        <Key>p:		MenuPopdown() do-menu(" S(ID_SAVE_PLAYLIST) ")\\n\
        ~Ctrl<Key>s:	MenuPopdown() do-menu(" S(ID_SAVECONFIG) ")\\n\
        <Key>h:		MenuPopdown() do-menu(" S(ID_SHUFFLE) ")\\n\
        Ctrl<Key>s:	MenuPopdown() do-menu(" S(ID_SHUFFLE) ")\\n\
        <Key>r:		MenuPopdown() do-menu(" S(ID_REPEAT) ")\\n\
        <Key>m:		MenuPopdown() do-menu(" S(ID_HIDETXT) ")\\n\
        <Key>t:		MenuPopdown() do-menu(" S(ID_HIDETRACE) ")\\n\
        <Key>u:		MenuPopdown() do-menu(" S(ID_AUTOSTART) ")\\n\
        <Key>e:		MenuPopdown() do-menu(" S(ID_AUTOQUIT) ")\\n\
        ~Meta<Key>f:	MenuPopdown() do-filelist()\\n\
        <Key>o:		MenuPopdown() do-options()\\n\
        <Key>a:		MenuPopdown() do-about()\\n\
        <Key>q:		MenuPopdown() do-quit()\\n\
        <Key>Escape:	MenuPopdown()\\n\
        <Motion>:	highlight()",
    "*load_dialog.add.label: Add ALL",
    "*load_dialog.load_button.label: Filter",
    "*flist_cmdbox.fplaybutton.label: Play",
    "*flist_cmdbox.fdeletebutton.label: Delete",
    "*flist_cmdbox.fdelallbutton.label: Delete ALL",
    "*closebutton.label: Close",
    "*modul_box.modul_lbl.label: Modulation control",
    "*porta_box.porta_lbl.label: Portamento control",
    "*nrpnv_box.nrpnv_lbl.label: NRPN Vibration",
    "*reverb_box.reverb_lbl.label: Reverb control",
    "*chorus_box.chorus_lbl.label: Chorus control",
    "*chpressure_box.chpressure_lbl.label: Channel Pressure control",
    "*overlapvoice_box.overlapv_lbl.label: Allow Multiple Same Notes",
    "*txtmeta_box.txtmeta_lbl.label: Tracing All Text Meta Events",
    "*sbox_ratelabel.label: Rate",
    "*base_form.translations: #override\\n\
        ~Ctrl Meta<Key>n:	do-menu(" S(ID_LOAD) ")\\n\
        Ctrl ~Shift<Key>v:	do-menu(" S(ID_SAVE) ")\\n\
        Meta<Key>l:		do-menu(" S(ID_LOAD_PLAYLIST) ")\\n\
        Meta<Key>p:		do-menu(" S(ID_SAVE_PLAYLIST) ")\\n\
        ~Ctrl Meta<Key>s:	do-menu(" S(ID_SAVECONFIG) ")\\n\
        Ctrl<Key>r:		do-menu(" S(ID_REPEAT) ")\\n\
        Ctrl<Key>s:		do-menu(" S(ID_SHUFFLE) ")\\n\
        Ctrl<Key>t:		do-menu(" S(ID_HIDETRACE) ")\\n\
        Ctrl<Key>m:		do-menu(" S(ID_HIDETXT) ")\\n\
        ~Ctrl<Key>q:		do-quit()\\n\
        ~Ctrl<Key>r:		do-play()\\n\
        <Key>Return:		do-play()\\n\
        <Key>KP_Enter:		do-play()\\n\
        ~Ctrl<Key>g:		do-sndspec()\\n\
        ~Ctrl<Key>space:	do-pause()\\n\
        ~Ctrl<Key>s:		do-stop()\\n\
        ~Meta<Key>p:		do-prev()\\n\
        <Key>Left:		do-prev()\\n\
        ~Meta<Key>n:		do-next()\\n\
        <Key>Right:		do-next()\\n\
        ~Ctrl ~Meta<Key>f:	do-forward()\\n\
        ~Ctrl<Key>b:		do-back()\\n\
        ~Ctrl<Key>plus:		do-key()\\n\
        ~Shift<Key>-:		do-key(1)\\n\
        <Key>KP_Add:		do-key()\\n\
        <Key>KP_Subtract:	do-key(1)\\n\
        ~Ctrl<Key>greater:	do-speed()\\n\
        ~Ctrl<Key>less:		do-speed(1)\\n\
        ~Ctrl ~Shift<Key>o:	do-voice()\\n\
        ~Ctrl Shift<Key>o:	do-voice(1)\\n\
        Ctrl<Key>o:		do-options()\\n\
        ~Meta Ctrl<Key>f:	do-filelist()\\n\
        ~Meta<Key>l:		do-filelist()\\n\
        <Key>a:			do-about()\\n\
        Ctrl<Key>d:		do-toggle-tooltips(-1)\\n\
        ~Ctrl ~Shift<Key>v:	do-volupdown(-10)\\n\
        ~Ctrl Shift<Key>v:	do-volupdown(10)\\n\
        <Key>Down:		do-volupdown(-10)\\n\
        <Key>Up:		do-volupdown(10)\\n\
        ~Ctrl<Key>x:		do-exchange()\\n\
        ~Ctrl<Key>t:		do-toggletrace()\\n\
        ~Ctrl Meta <Key>f:	show-menu()\\n\
        <Key>z:			show-menu()\\n\
        <Key>j:			changetrace(1)\\n\
        <BtnDown>:		hide-menu()\\n\
        <ConfigureNotify>:	do-resize()",

    "*Viewport.useBottom: True",
    "*List.baseTranslations: #override\\n\
        <Btn4Down>:   do-scroll(-1)\\n\
        <Btn5Down>:   do-scroll(1)",
    "*Scrollbar.baseTranslations: #override\\n\
        <Btn4Down>:   StartScroll(Backward)\\n\
        <Btn5Down>:   StartScroll(Forward)",
    "*Text.baseTranslations: #override\\n\
        ~Shift<Key>Delete:	delete-next-character()\\n\
        Ctrl<Key>V:		insert-selection(CLIPBOARD)",
    "*TransientShell.Box.baseTranslations: #override\\n\
        ~Ctrl<Key>c:	do-closeparent()\\n\
        <Key>Escape:	do-closeparent()\\n\
        <Key>KP_Enter:	do-closeparent()\\n\
        <Key>Return:	do-closeparent()",
    "*load_dialog.value.translations: #override\\n\
        ~Ctrl Meta<Key>KP_Enter:	do-addall()\\n\
        ~Ctrl Meta<Key>Return:		do-addall()\\n\
        ~Ctrl ~Meta<Key>KP_Enter:	do-chgdir()\\n\
        ~Ctrl ~Meta<Key>Return:		do-chgdir()\\n\
        ~Ctrl ~Meta<Key>Tab:		do-complete() end-of-line()\\n\
        Ctrl ~Shift<Key>g:		do-popdown()\\n\
        <Key>BackSpace:		do-backspace() delete-previous-character()\\n\
        Ctrl<Key>H:		do-backspace() delete-previous-character()\\n\
        <Key>Escape:		do-popdown()",
    "*load_dialog.load_button.accelerators: #override\\n\
        Ctrl<KeyPress>`: toggle() notify()",
    "*dialog_sfile*load_dialog.add.Sensitive: False",
    "*" LISTDIALOGBASENAME "*load_dialog.add.Sensitive: False",
    "*trace.translations: #override\\n\
        <Btn1Down>:	do-toggletrace()\\n\
        <Btn3Down>:	do-mutechan()\\n\
        <Btn4Down>:	changetrace(-1)\\n\
        <Btn5Down>:	changetrace(1)\\n\
        <EnterNotify>:	do-revcaption()\\n\
        <LeaveNotify>:	do-revcaption()\\n\
        <Expose>:	draw-trace()",
    "*time_label.translations: #override\\n\
        <Btn2Down>:	do-menu(" S(ID_HIDETRACE) ")\\n\
        <Btn3Down>:	do-exchange()",
    "*popup_optform.translations: #override\\n\
        ~Ctrl<Key>c:	do-closeparent()\\n\
        ~Ctrl<Key>q:	do-quit()\\n\
        <Key>Escape:	do-closeparent()\\n\
        <Key>KP_Enter:	do-optionsclose()\\n\
        <Key>Return:	do-optionsclose()",
    "*popup_file*filelist.translations: #override\\n\
        <Btn1Up>(2+):	do-fselect()\\n\
        <Btn3Up>:	do-stop()",
    "*flist_cmdbox.width: 272",
    "*flist_cmdbox.height: 24",
    "*flist_cmdbox.borderWidth: 0",
    "*file_vport.width: 272",
    "*file_vport.height: 336",
    "*file_vport.borderWidth: 1",
    "*popup_fform.translations: #override\\n\
        ~Ctrl<Key>c:	do-closeparent()\\n\
        <Key>Escape:	do-closeparent()\\n\
        <Key>Home:	do-flistmove(-1, 0, 0)\\n\
        <Key>Prior:	do-flistmove(-1, 0)\\n\
        <Key>Right:	do-flistmove(-5)\\n\
        <Key>Up:	do-flistmove(-1)\\n\
        <Key>p:		do-flistmove(-1)\\n\
        ~Ctrl<Key>r:	do-fselect()\\n\
        <Key>Return:	do-fselect()\\n\
        <Key>KP_Enter:	do-fselect()\\n\
        Ctrl<Key>m:	do-fselect()\\n\
        <Key>space:	do-pause()\\n\
        <Key>s:		do-stop()\\n\
        <Key>Down:	do-flistmove(1)\\n\
        <Key>n:		do-flistmove(1)\\n\
        <Key>Left:	do-flistmove(5)\\n\
        <Key>Next:	do-flistmove(1, 0)\\n\
        <Key>End:	do-flistmove(1, 0, 0)\\n\
        <Key>d:		do-fdelete()\\n\
        :<Key>A:	do-fdelall()\\n\
        ~Shift<Key>v:	do-volupdown(-10)\\n\
        Shift<Key>v:	do-volupdown(10)\\n\
        ~Ctrl<Key>f:	do-forward()\\n\
        ~Ctrl<Key>b:	do-back()\\n\
        ~Ctrl<Key>q:	do-quit()",
    "*popup_cform.translations: #override\\n\
        ~Ctrl<Key>c:	do-cancel()\\n\
        <Key>Escape:	do-cancel()\\n\
        <Key>KP_Enter:	do-ok()\\n\
        <Key>Return:	do-ok()",
    "*sbox_ratetext.translations: #override\\n\
        <Key>Escape:	do-closeparent()\\n\
        <Key>KP_Enter:	do-record()\\n\
        <Key>Return:	do-record()\\n\
        <Key>BackSpace:	delete-previous-character()\\n\
        Shift<Key>:	no-op()\\n\
        ~Ctrl<Key>0:	insert-char()\\n\
        ~Ctrl<Key>KP_0:	insert-char()\\n\
        ~Ctrl<Key>1:	insert-char()\\n\
        ~Ctrl<Key>KP_1:	insert-char()\\n\
        ~Ctrl<Key>2:	insert-char()\\n\
        ~Ctrl<Key>KP_2:	insert-char()\\n\
        ~Ctrl<Key>3:	insert-char()\\n\
        ~Ctrl<Key>KP_3:	insert-char()\\n\
        ~Ctrl<Key>4:	insert-char()\\n\
        ~Ctrl<Key>KP_4:	insert-char()\\n\
        ~Ctrl<Key>5:	insert-char()\\n\
        ~Ctrl<Key>KP_5:	insert-char()\\n\
        ~Ctrl<Key>6:	insert-char()\\n\
        ~Ctrl<Key>KP_6:	insert-char()\\n\
        ~Ctrl<Key>7:	insert-char()\\n\
        ~Ctrl<Key>KP_7:	insert-char()\\n\
        ~Ctrl<Key>8:	insert-char()\\n\
        ~Ctrl<Key>KP_8:	insert-char()\\n\
        ~Ctrl<Key>9:	insert-char()\\n\
        ~Ctrl<Key>KP_9:	insert-char()\\n\
        <Key>Home:	beginning-of-file()\\n\
        :<Key>KP_Home:	beginning-of-file()\\n\
        <Key>End:	end-of-file()\\n\
        :<Key>KP_End:	end-of-file()\\n\
        <Key>Right:	forward-character()\\n\
        :<Key>KP_Right:	forward-character()\\n\
        <Key>Left:	backward-character()\\n\
        :<Key>KP_Left:	backward-character()\\n\
        Hyper<Key>:	no-op()\\n\
        Super<Key>:	no-op()\\n\
        None<Key>:	no-op()\\n\
        Alt<Key>:	no-op()\\n\
        Meta<Key>:	no-op()\\n\
        Lock<Key>:	no-op()",
    "*fbox_toggle0.accelerators: #override\\n\
        <Key>Up:	do-up()\\n\
        <Key>Down:	do-down()\\n\
        <Btn4Down>:	do-up()\\n\
        <Btn5Down>:	do-down()",
    "*confirmexit.label: Do you wish to exit?",
    "*warnoverwrite.label: Do you wish to overfile this file?",
    "*saveplaylisterror.label: Could not save playlist!",
    "*waitforwav.label: Please wait. This may take several minutes.",
    "*warnrecording.label: Cannot record - a file is already being recorded",
    NULL
  };
  int argc = 1, i;
  char *argv = APP_NAME;

  xaw_vendor_setup();

  XtSetLanguageProc(NULL, NULL, NULL);
  toplevel = XtVaAppInitialize(&app_con, APP_CLASS, NULL, ZERO, &argc, &argv,
                         fallback_resources, NULL);
  XtGetApplicationResources(toplevel, (XtPointer)&app_resources,
                            xaw_resources, XtNumber(xaw_resources), NULL, 0);
  umask(022);
  a_readconfig(&Cfg, &home);
  if (Cfg.disptrace) ctl->trace_playing = 1;
  amplitude =
    (amplification == DEFAULT_AMPLIFICATION)?Cfg.amplitude:amplification;
  disp = XtDisplay(toplevel);
  root_height = DisplayHeight(disp, DefaultScreen(disp));
  root_width = DisplayWidth(disp, DefaultScreen(disp));
  XtVaSetValues(toplevel, XtNiconPixmap,GET_BITMAP(timidity), NULL);

  check_mark = GET_BITMAP(check);
  arrow_mark = GET_BITMAP(arrow);
  on_mark = GET_BITMAP(on);
  off_mark = GET_BITMAP(off);

  ldSstart = init_ldS();

#ifdef OFFIX
  DndInitialize(toplevel);
  DndRegisterOtherDrop(FileDropedHandler);
  DndRegisterIconDrop(FileDropedHandler);
#endif /* OFFIX */
  XtAppAddActions(app_con, actions, XtNumber(actions));

  base_f = XtVaCreateManagedWidget("base_form",formWidgetClass,toplevel,
            XtNbackground,bgcolor, NULL);
#ifdef XawTraversal
  XawFocusInstallActions(app_con);
  XawFocusInstall(base_f, False);
#endif /* XawTraversal */
  m_box = XtVaCreateManagedWidget("menu_box",boxWidgetClass,base_f,
            XtNtop,XawChainTop, XtNbottom,XawChainTop,
            XtNleft,XawChainLeft, XtNright,XawChainLeft,
            XtNorientation,XtorientHorizontal, XtNbackground,bgcolor, NULL);
  file_mb = XtVaCreateManagedWidget("file_menubutton",menuButtonWidgetClass,
            m_box, XtNbackground,menubcolor,
            XtNfontSet,app_resources.label_font,
            XtNlabel,app_resources.file_text, NULL);
  file_sm = XtVaCreatePopupShell("file_simplemenu",simpleMenuWidgetClass,
            file_mb, XtNforeground,textcolor, XtNbackground,textbgcolor,
            XtNresize,False, XtNbackingStore,NotUseful, XtNsaveUnder,False,
            XtNwidth,app_resources.menu_width+100, NULL);
  for (i = 0; i < (int)XtNumber(file_menu); i++) {
    bsb = XtVaCreateManagedWidget(file_menu[i].name,
            *(file_menu[i].class), file_sm,XtNleftBitmap,None,
             XtNleftMargin,24, NULL);
    XtAddCallback(bsb, XtNcallback,filemenuCB, (XtPointer)&file_menu[i].id);
    file_menu[i].widget = bsb;
  }
  snprintf(local_buf, sizeof(local_buf), "TiMidity++ %s", timidity_version);
  title_mb = XtVaCreateManagedWidget("title_menubutton",menuButtonWidgetClass,
            m_box, XtNbackground,menubcolor, XtNlabel,local_buf,
            XtNfontSet,app_resources.label_font, NULL);
  title_sm = XtVaCreatePopupShell("title_simplemenu",simpleMenuWidgetClass,
            title_mb, XtNforeground,textcolor, XtNbackground,textbgcolor,
            XtNbackingStore,NotUseful, XtNsaveUnder,False, NULL);
  time_l = XtVaCreateManagedWidget("time_label",commandWidgetClass,m_box,
            XtNfontSet,app_resources.label_font,
            XtNbackground,menubcolor, NULL);
  b_box = XtVaCreateManagedWidget("button_box",boxWidgetClass,base_f,
            XtNorientation,XtorientHorizontal,
            XtNtop,XawChainTop, XtNbottom,XawChainTop,
            XtNleft,XawChainLeft, XtNright,XawChainLeft,
            XtNbackground,bgcolor, XtNfromVert,m_box, NULL);

  createButtons();

  createBars();

#ifndef WIDGET_IS_LABEL_WIDGET
  lyric_t = XtVaCreateWidget("lyric_text",asciiTextWidgetClass,base_f,
            XtNwrap,XawtextWrapWord, XtNeditType,XawtextAppend,
            XtNwidth,(ctl->trace_playing)?TRACE_WIDTH+8:DEFAULT_REG_WIDTH,
#else
  lyric_t = XtVaCreateWidget("lyric_text",labelWidgetClass,base_f,
            XtNforeground,textcolor, XtNbackground,menubcolor,
#endif
            XtNborderWidth,1, XtNfontSet,app_resources.text_font,
            XtNtop,XawChainTop, XtNright,XawChainRight,
            XtNheight,app_resources.text_height, XtNfromVert,t_box, NULL);
  if (Cfg.disptext == True) XtManageChild(lyric_t);

  if (ctl->trace_playing) createTraceWidgets();

  XtAddCallback(vol_bar, XtNjumpProc,volsetCB, NULL);
  XtAddCallback(tune_bar, XtNjumpProc,tuneslideCB, NULL);
  XtAddCallback(time_l, XtNcallback,filemenuCB,
                (XtPointer)&file_menu[ID_HIDETXT-100].id);
  XtAppAddInput(app_con, pipe_in,
                (XtPointer)XtInputReadMask, handle_input, NULL);

  wm_delete_window = XInternAtom(disp, "WM_DELETE_WINDOW", False);
#ifdef HAVE_GETPID
  net_wm_pid = XInternAtom(disp, "_NET_WM_PID", False);
  pid = getpid();
#endif
  setupWindow(toplevel, "do-quit()", False, False);

  XtVaGetValues(toplevel, XtNheight,&curr_height,
                          XtNwidth,&curr_width, NULL);
  XtVaGetValues(lyric_t, XtNheight,&lyric_height, NULL);
  base_height = curr_height;

  XtSetKeyboardFocus(base_f, base_f);
  XtSetKeyboardFocus(lyric_t, base_f);
#ifdef XDND
  a_dnd_init();
#endif /* XDND */
  snprintf(window_title, sizeof(window_title), "%s : %s",
           APP_CLASS, app_resources.no_playing);
  XtVaSetValues(toplevel, XtNtitle,window_title, NULL);

  a_print_text(lyric_t, strcpy(local_buf, "<< TiMidity Messages >>"));
  a_pipe_write("READY");

  while (1) {
    a_pipe_read(local_buf, sizeof(local_buf));
    if (local_buf[0] < 'A') break;
    a_print_text(lyric_t, local_buf+2);
  }
  bsb = XtVaCreateManagedWidget("dummyfile",smeLineObjectClass,title_sm,
                                 XtNforeground,textbgcolor, NULL);
  init_options = atoi(local_buf);
  a_pipe_read(local_buf, sizeof(local_buf));
  init_chorus = atoi(local_buf);

  init_output_lists();
  a_pipe_read(local_buf, sizeof(local_buf));
  max_files = atoi(local_buf);
  flist = (String *)safe_malloc((INIT_FLISTNUM+1)*sizeof(char *));
  *flist = NULL;
  for (i=0; i<max_files; i++) {
    a_pipe_read(local_buf, sizeof(local_buf));
    addOneFile(max_files, i, local_buf);
    addFlist(local_buf, i);
  }
  for (i=0; i<dot_nfile; i++) {
    a_pipe_write("X %s", dotfile_flist[i]);
    free(dotfile_flist[i]);
  }
  free(dotfile_flist);

  if (Cfg.disptext == False)
    XtVaSetValues(file_menu[ID_HIDETXT - 100].widget,
                  XtNleftBitmap,check_mark, NULL);
  else base_height -= lyric_height;

  if (ctl->trace_playing) {
    callInitTrace();
    a_pipe_write("t");
    base_height -= trace_v_height;
    toggleMark(file_menu[ID_HIDETRACE-100].widget, False);
    resizeToplevelACT(toplevel, NULL, NULL, NULL);
  } else {
    toggleMark(file_menu[ID_HIDETRACE-100].widget, True);
#ifdef XAWPLUS
    resizeToplevelACT(toplevel, NULL, NULL, NULL);
#endif /* XAWPLUS */
  }
#ifdef HAVE_TIP
  if (Cfg.tooltips == True) xawTipSet(True);
#endif /* HAVE_TIP */

  a_pipe_write("V %d", amplitude);
  if (init_options == DEFAULT_OPTIONS) init_options = Cfg.extendopt;
  if (init_chorus == DEFAULT_CHORUS) init_chorus = Cfg.chorusopt;
  else Cfg.chorusopt = init_chorus;
  a_pipe_write("E %03d", init_options);
  a_pipe_write("C %03d", init_chorus);

  if (Cfg.autostart)
    XtVaSetValues(file_menu[ID_AUTOSTART - 100].widget,
                  XtNleftBitmap,check_mark, NULL);
  if (Cfg.autoexit) {
    XtVaSetValues(file_menu[ID_AUTOQUIT - 100].widget,
                  XtNleftBitmap,check_mark, NULL);
    a_pipe_write("q");
  }

  if (Cfg.repeat) {
    XtVaSetValues(file_menu[ID_REPEAT - 100].widget,
                  XtNleftBitmap,check_mark, NULL);
    repeatCB(NULL, &Cfg.repeat, NULL);
  }
  if (Cfg.shuffle) {
    XtVaSetValues(file_menu[ID_SHUFFLE - 100].widget,
                  XtNleftBitmap,check_mark, NULL);
    randomCB(NULL, &Cfg.shuffle, NULL);
  }
  if (Cfg.autostart) {
    if (max_files != 0) playCB(NULL, NULL, NULL);
    else if (dot_nfile != 0) {
      onPlayOffPause();
      a_pipe_write("B");
    }
  }
  else stopCB(NULL, NULL, NULL);
  if ((max_files == 0) && (dot_nfile == 0))
    XtVaSetValues(file_menu[ID_SAVE - 100].widget, XtNsensitive,False, NULL);
}

void a_start_interface(int pipe_in) {
  a_init_interface(pipe_in);
  XtAppMainLoop(app_con);
}

static char *
get_user_home_dir(void) {
  char *home, *p;
  struct passwd *pw;

  home = getenv("HOME");
  if (home == NULL) home = getenv("home");
  if (home == NULL) {
    pw = getpwuid(getuid());
    if (pw == NULL) {
      ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
                 "I cannot find the current user's home directory!");
      return NULL;
    }
    home = pw->pw_dir;
  }
  p = safe_strdup(home);
  return p;
}

#include "interface.h"
#if defined(IA_MOTIF)
/*
 * Switch -lXm's vendorShellWidgetClass to -lXaw's vendorShellWidgetClass
 */
#define vendorShellClassRec xaw_vendorShellClassRec
#define vendorShellWidgetClass xaw_vendorShellWidgetClass
#include "xaw_redef.c"
#undef vendorShellClassRec
#undef vendorShellWidgetClass
extern WidgetClass vendorShellWidgetClass;
extern VendorShellClassRec vendorShellClassRec;
static void
xaw_vendor_setup(void) {
  memcpy(&vendorShellClassRec, &xaw_vendorShellClassRec,
         sizeof(VendorShellClassRec));
  vendorShellWidgetClass = (WidgetClass)&xaw_vendorShellWidgetClass;
}
#else
static void
xaw_vendor_setup(void) { }
#endif

#ifdef CLEARVALUE
static void
clearValue(Widget w) {
  Widget TextSrc;

  XtVaGetValues(XtNameToWidget(w, "value"), XtNtextSource, &TextSrc, NULL); 
  XawAsciiSourceFreeString(TextSrc);
}
#endif /* CLEARVALUE */

static void
simulateArrowsCB(Widget w, XtPointer client_data, XtPointer call_data) {
  long offset = (long)call_data;
  XEvent *e = (XEvent *)client_data;
  barfloat thumb;
  Dimension len;

  XtVaGetValues(w, XtNtopOfThumb,&thumb.f, XtNlength,&len, NULL);
  if (abs(offset) >= len) return;
  thumb.f += (float)offset/(float)len;
  if (thumb.f < 0) thumb.f = 0;
  else if (thumb.f > 1) thumb.f = 1;
  setThumb(w, thumb);
  XtCallActionProc(w, (String)"NotifyThumb", e, NULL, ZERO);
  e->xmotion.same_screen = 0;
}

static void
StartScrollACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  XtOrientation Orientation;
  long call_data;

  if ((e->type != ButtonPress) && (e->type != ButtonRelease)) return;
  XtVaGetValues(w, XtNorientation,&Orientation, NULL);
  if (Orientation == XtorientHorizontal) call_data = e->xbutton.x;
  else call_data = e->xbutton.y;
  if (!strcasecmp("Continuous", *v)) {
    XtAddCallback(w, XtNscrollProc,simulateArrowsCB, (XtPointer)e);
    XtCallActionProc(w, (String)"NotifyScroll", e, NULL, ZERO);
    XtRemoveCallback(w, XtNscrollProc,simulateArrowsCB, (XtPointer)e);
    return;
  } else if (!strcasecmp("Backward", *v)) {
    call_data = -call_data;
  }
  XtCallCallbacks(w, XtNscrollProc, (XtPointer)call_data);
}

/* XawScrollbarSetThumb is buggy. */
static void
setThumb(Widget w, barfloat u)
{
  if (sizeof(float) > sizeof(XtArgVal)) {
    XtVaSetValues(w, XtNtopOfThumb,&u.f, NULL);
  } else {
    XtVaSetValues(w, XtNtopOfThumb,u.x, NULL);
  }
}

static Widget seekTransientShell(Widget w) {
  if (w == NULL) return NULL;
  while (w != toplevel) {
    if (XtIsTransientShell(w)) return w;
    w = XtParent(w);
  }
  return toplevel;
}

/* The following implements a series of toggles, to be used for output
 * selection either when saving a file, or changing output in options.
 */

static void
freevarCB(Widget w, XtPointer client_data, XtPointer call_data) {
  outputs *out = (outputs *)client_data;

  free(out->lbuf);
  free(out->toggleGroup);
}

static void
restoreDefaultOSelectionCB(Widget w, XtPointer client_data,
                           XtPointer call_data) {
  outputs *out = (outputs *)client_data;

  XawToggleSetCurrent(out->formatGroup,
                      (XtPointer)(out->output_list + out->def));
}

static void
tnotifyCB(Widget w, XtPointer client_data, XtPointer call_data) {
  outputs *out;
  id_list *result;
  int i;
  Boolean s;

  XtVaGetValues(w, XtNstate,&s, NULL);
  if (s == False) return;
  if ((Widget)client_data == play->formatGroup) out = play;
  else out = record;
  result = (id_list *)XawToggleGetCurrent(out->formatGroup);
  for (i=0; i<out->max; i++)
    if (result->id_char == (out->output_list[i].id_char)) break;
  out->current = i;
}

static void
upACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  char s[20];
  Widget w1, w2;
  outputs *out;
  id_list *result;
  int i;

  if (w == play->formatGroup) out = play;
  else out = record;
  result = (id_list *)XawToggleGetCurrent(out->formatGroup);
  for (i=0; i<out->max; i++)
    if (result->id_char == (out->output_list[i].id_char)) break;
  if (i == 0) i = out->max-1;
  else i--;
  snprintf(s, sizeof(s), "sbox_fbox%d", i);
  w1 = XtNameToWidget(XtParent(XtParent(w)), s);
  snprintf(s, sizeof(s), "fbox_toggle%d", i);
  w2 = XtNameToWidget(w1, s);
  XtVaSetValues(w2, XtNstate,True, NULL);
  out->current = i;
}

static void
downACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  char s[20];
  Widget w1, w2;
  outputs *out;
  id_list *result;
  int i;

  if (w == play->formatGroup) out = play;
  else out = record;
  result = (id_list *)XawToggleGetCurrent(out->formatGroup);
  for (i=0; i<out->max; i++)
    if (result->id_char == (out->output_list[i].id_char)) break;
  if (i >= out->max-1) i = 0;
  else i++;
  snprintf(s, sizeof(s), "sbox_fbox%d", i);
  w1 = XtNameToWidget(XtParent(XtParent(w)), s);
  snprintf(s, sizeof(s), "fbox_toggle%d", i);
  w2 = XtNameToWidget(w1, s);
  XtVaSetValues(w2, XtNstate,True, NULL);
  out->current = i;
}

static Widget
createOutputSelectionWidgets(Widget popup, Widget parent,
                             Widget fromVert, outputs *out,
                             Boolean restoreDefault) {
  Widget *sbox_fbox, *fbox_toggle, *fbox_label, *pw, formatGroup;
  char s[20];
  int i = out->max, j;
  id_list *list;
  char defaultToggleTranslations[] =
    "<EnterWindow>:         highlight(Always)\n\
    <LeaveWindow>:         unhighlight()\n\
    <Btn1Down>,<Btn1Up>:   set() notify()";
  XtTranslations ToggleTrans;

  if (out == NULL) return None;
  list = out->output_list;
  pw = (Widget *)safe_malloc(sizeof(Widget) * 3 * i);
  out->toggleGroup = pw;

  sbox_fbox = pw;
  fbox_toggle = pw + i;
  fbox_label = pw + 2*i;
  ToggleTrans = XtParseTranslationTable(defaultToggleTranslations);

  sbox_fbox[0] = XtVaCreateManagedWidget("sbox_fbox0",boxWidgetClass,parent,
                                         XtNorientation,XtorientHorizontal,
                                         XtNbackground,bgcolor,
                                         XtNfromVert,fromVert,
                                         XtNborderWidth,0, NULL);
  fbox_toggle[0] = XtVaCreateManagedWidget("fbox_toggle0",toggleWidgetClass,
                                      sbox_fbox[0], XtNlabel,"",
                                      XtNtranslations,ToggleTrans,
                                      XtNbackground,buttonbgcolor,
                                      XtNforeground,togglecolor,
                                      XtNradioGroup,NULL, XtNborderWidth,1,
                                      XtNradioData,(XtPointer)list,
#ifndef DONTUSEOVALTOGGLES
                                      XtNshapeStyle,XmuShapeOval,
#endif /* !DONTUSEOVALTOGGLES */
                                      XtNborderColor,togglecolor,
                                      XtNinternalWidth,3, XtNinternalHeight,1,
                                      XtNwidth,17, XtNheight,17, NULL);
  fbox_label[0] = XtVaCreateManagedWidget("fbox_label0",labelWidgetClass,
                                            sbox_fbox[0], XtNbackground,bgcolor,
                                            XtNlabel,list[0].id_name,
                                            XtNforeground,textcolor,
                                            XtNfromHoriz,fbox_toggle[0],
                                            XtNborderWidth,0, NULL);
  out->formatGroup = formatGroup = fbox_toggle[0];
  XtAddCallback(fbox_toggle[0], XtNcallback,tnotifyCB,
                (XtPointer)formatGroup);

  for (j = 1; j < i; j++) {
    snprintf(s, sizeof(s), "sbox_fbox%d", j);
    sbox_fbox[j] = XtVaCreateManagedWidget(s,boxWidgetClass,parent,
                                           XtNorientation,XtorientHorizontal,
                                           XtNfromVert,sbox_fbox[j-1],
                                           XtNbackground,bgcolor,
                                           XtNborderWidth,0, NULL);
    snprintf(s, sizeof(s), "fbox_toggle%d", j);
    fbox_toggle[j] = XtVaCreateManagedWidget(s,toggleWidgetClass,sbox_fbox[j],
                                        XtNbackground,buttonbgcolor,
                                        XtNforeground,togglecolor,
                                        XtNradioData,(XtPointer)(list + j),
                                        XtNradioGroup,formatGroup,
                                        XtNfromVert,fbox_toggle[j-1],
#ifndef DONTUSEOVALTOGGLES
                                        XtNshapeStyle,XmuShapeOval,
#endif /* !DONTUSEOVALTOGGLES */
                                        XtNinternalWidth,3, XtNinternalHeight,1,
                                        XtNwidth,17, XtNheight,17, XtNlabel,"",
					XtNtranslations,ToggleTrans,
                                        XtNborderColor,togglecolor,
                                        XtNborderWidth,1, NULL);
    XtAddCallback(fbox_toggle[j], XtNcallback,tnotifyCB,
                  (XtPointer)formatGroup);
    snprintf(s, sizeof(s), "fbox_label%d", j);
    fbox_label[j] = XtVaCreateManagedWidget(s,labelWidgetClass,sbox_fbox[j],
                                              XtNfromHoriz,fbox_toggle[j],
                                              XtNlabel,list[j].id_name,
                                              XtNforeground,textcolor,
                                              XtNbackground,bgcolor,
                                              XtNjustify,XtJustifyLeft,
                                              XtNborderWidth,0, NULL);
  }
  XtCallActionProc(fbox_toggle[out->def], (String)"set", NULL, NULL, ZERO);

  XtAddCallback(popup, XtNdestroyCallback,freevarCB,
                (XtPointer)out);
  if (restoreDefault == True)
    XtAddCallback(popup, XtNpopdownCallback,restoreDefaultOSelectionCB,
                  (XtPointer)out);
  XtInstallAccelerators(parent, formatGroup);
  XtInstallAccelerators(popup, formatGroup);

  return sbox_fbox[i-1];
}

/* End output selection routines */

#ifdef HAVE_TIP
static void
TipEnable(Widget w, String s) {
#ifdef XAW3D
  XawTipEnable(w, s);
#elif defined(XAWPLUS)
  XtVaSetValues(w, XtNhelpText,s, XtNuseHelp,True, NULL);
#else
  XawTipEnable(w);
#endif /* !XAW3D */
}

static void
TipDisable(Widget w) {
#ifdef XAWPLUS
  XtVaSetValues(w, XtNuseHelp,False, NULL);
#else
  XawTipDisable(w);
#endif /* XAWPLUS */
}

static void
xawtipsetACT(Widget w, XEvent *e, String *v, Cardinal *n) {
  int state = atoi(*v);

  if (state == -1) Cfg.tooltips ^= True;
  else Cfg.tooltips = (Boolean)state;
  xawTipSet(Cfg.tooltips);
}

static void
xawTipSet(Boolean tooltips) {
  if (tooltips == True) {
    TipEnable(play_b, "Play");
    TipEnable(pause_b, "Pause");
    TipEnable(stop_b, "Stop");
    TipEnable(prev_b, "Previous");
    TipEnable(back_b, "Back");
    TipEnable(fwd_b, "Forward");
    TipEnable(next_b, "Next");
    TipEnable(quit_b, "Quit");
    TipEnable(random_b, "Shuffle");
    TipEnable(repeat_b, "Repeat");
    if (ctl->trace_playing) {
      TipEnable(fast_b, "Increase tempo");
      TipEnable(slow_b, "Decrease Tempo");
      TipEnable(keyup_b, "Raise pitch");
      TipEnable(keydown_b, "Lower pitch");
    }
  } else {
    TipDisable(play_b);
    TipDisable(pause_b);
    TipDisable(stop_b);
    TipDisable(prev_b);
    TipDisable(back_b);
    TipDisable(fwd_b);
    TipDisable(next_b);
    TipDisable(quit_b);
    TipDisable(random_b);
    TipDisable(repeat_b);
    if (ctl->trace_playing) {
      TipDisable(fast_b);
      TipDisable(slow_b);
      TipDisable(keyup_b);
      TipDisable(keydown_b);
    }
  }
}
#else
static void
xawtipsetACT(Widget w, XEvent *e, String *v, Cardinal *n) { }
#endif /* HAVE_TIP */
