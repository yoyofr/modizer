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

    xaw_c.c - XAW Interface from
        Tomokazu Harada <harada@prince.pe.u-tokyo.ac.jp>
        Yoshishige Arai <ryo2@on.rim.or.jp>
        Yair Kalvariski <cesium2@gmail.com>
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#if defined(TIME_WITH_SYS_TIME)
#include <sys/time.h>
#include <time.h>
#elif defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif /* TIME_WITH_SYS_TIME */
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif /* NO_STRING_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "output.h"
#include "controls.h"
#include "miditrace.h"
#include "timer.h"
#include "xaw.h"

static int cmsg(int, int, char *, ...);
static int ctl_blocking_read(int32 *);
static void ctl_close(void);
static void ctl_event(CtlEvent *);
static int ctl_read(int32 *);
static int ctl_open(int, int);
static int ctl_pass_playing_list(int, char **);
ControlMode *interface_a_loader(void);

static void a_pipe_open(void);
static int a_pipe_ready(void);
int a_pipe_read(char *, size_t);
int a_pipe_nread(char *, size_t);
void a_pipe_sync(void);
void a_pipe_write(const char *, ...);
static void a_pipe_write_buf(char *, int);
static void a_pipe_write_msg(char *);
static void a_pipe_write_msg_nobr(char *);
extern void a_start_interface(int);

static void ctl_current_time(int, int);
static void ctl_drumpart(int, int);
static void ctl_event(CtlEvent *);
static void ctl_expression(int, int);
static void ctl_keysig(int);
static void ctl_key_offset(int);
static void ctl_lyric(int);
static void ctl_master_volume(int);
static void ctl_mute(int, int);
static void ctl_note(int, int, int, int);
static void ctl_panning(int, int);
static void ctl_pitch_bend(int, int);
static void ctl_program(int, int, void *);
static void ctl_reset(void);
static void ctl_sustain(int, int);
static void ctl_tempo(int);
static void ctl_timeratio(int);
static void ctl_total_time(int);
static void ctl_refresh(void);
static void ctl_volume(int, int);
static void set_otherinfo(int, int, char);
static void shuffle(int, int *);
static void query_outputs(void);
static void update_indicator(void);
static void xaw_add_midi_file(char *);
static void xaw_delete_midi_file(int);
static void xaw_output_flist(char *);

static double indicator_last_update = 0;
#define EXITFLG_QUIT 1
#define EXITFLG_AUTOQUIT 2
static int reverb_type;
/*
 * Unfortunate hack, forced because opt_reverb_control has been overloaded,
 * and is no longer 0 or 1. Now it's in the range 0..3, so when reenabling
 * reverb and disabling it, we need to know the original value.
 */
static int exitflag = 0, randomflag = 0, repeatflag = 0, selectflag = 0,
           current_no = 0, xaw_ready = 0, number_of_files, *file_table;
static int pipe_in_fd, pipe_out_fd;
static char **list_of_files, **titles;
static int active[MAX_XAW_MIDI_CHANNELS];
/*
 * When a midi channel has played a note, this is set to 1. Otherwise, it's 0.
 * This is used to screen out channels which are not used from the GUI.
 */
extern int amplitude;
extern PlayMode *play_mode, *target_play_mode;
static PlayMode *olddpm = NULL;

/**********************************************/
/* export the interface functions */

#define ctl xaw_control_mode

ControlMode ctl = {
    "XAW interface", 'a',
    "xaw",
    1,0,0,
    0,
    ctl_open,
    ctl_close,
    ctl_pass_playing_list,
    ctl_read,
    NULL,
    cmsg,
    ctl_event
};

static char local_buf[PIPE_LENGTH];

/***********************************************************************/
/* Put controls on the pipe                                            */
/***********************************************************************/
#define CMSG_MESSAGE 16

static int cmsg(int type, int verbosity_level, char *fmt, ...) {
  va_list ap;
  char *buff;
  MBlockList pool;

  if (((type == CMSG_TEXT) || (type == CMSG_INFO) || (type == CMSG_WARNING)) &&
      (ctl.verbosity<verbosity_level))
    return 0;

  va_start(ap, fmt);

  if (!xaw_ready) {
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, NLS);
    va_end(ap);
    return 0;
  }

  init_mblock(&pool);
  buff = (char *)new_segment(&pool, MIN_MBLOCK_SIZE);
  vsnprintf(buff, MIN_MBLOCK_SIZE, fmt, ap);
  a_pipe_write_msg(buff);
  reuse_mblock(&pool);

  va_end(ap);
  return 0;
}

/*ARGSUSED*/
static int tt_i;

static void
ctl_current_time(int sec, int v) {

  static int previous_sec = -1, last_voices = -1, last_v = -1, last_time = -1;

  if (sec != previous_sec) {
    previous_sec = sec;
    a_pipe_write("t %d", sec);
  }
  if (last_time != tt_i) {
    last_time = tt_i;
    a_pipe_write("T %d", tt_i);
  }
  if (!ctl.trace_playing || midi_trace.flush_flag) return;
  if (last_voices != voices) {
    last_voices = voices;
    a_pipe_write("vL%d", voices);
  }
  if (last_v != v) {
    last_v = v;
    a_pipe_write("vl%d", v);
  }
}

static void
ctl_total_time(int tt) {
    tt_i = tt / play_mode->rate;
    ctl_current_time(0, 0);
    a_pipe_write("m%d", play_system_mode);
}

static void
ctl_master_volume(int mv) {
  amplitude = atoi(local_buf + 2);
  if (amplitude < 0) amplitude = 0;
  if (amplitude > MAXVOLUME) amplitude = MAXVOLUME;
  a_pipe_write("V %03d", mv);
}

static void
ctl_volume(int ch, int val) {
  if ((!ctl.trace_playing) || (ch >= MAX_XAW_MIDI_CHANNELS)) return;
  a_pipe_write("PV%c%d", ch+'A', val);
}

static void
ctl_expression(int ch, int val) {
  if ((!ctl.trace_playing) || (ch >= MAX_XAW_MIDI_CHANNELS)) return;
  a_pipe_write("PE%c%d", ch+'A', val);
}

static void
ctl_panning(int ch, int val) {
  if ((!ctl.trace_playing) || (ch >= MAX_XAW_MIDI_CHANNELS)) return;
  a_pipe_write("PA%c%d", ch+'A', val);
}

static void
ctl_sustain(int ch, int val) {
  if ((!ctl.trace_playing) || (ch >= MAX_XAW_MIDI_CHANNELS)) return;
  a_pipe_write("PS%c%d", ch+'A', val);
}

static void
ctl_pitch_bend(int ch, int val) {
  if ((!ctl.trace_playing) || (ch >= MAX_XAW_MIDI_CHANNELS)) return;
  a_pipe_write("PB%c%d", ch+'A', val);
}

#ifdef WIDGET_IS_LABEL_WIDGET
static void
ctl_lyric(int lyricid) {
  char *lyric;
  static int lyric_col = 0;
  static char lyric_buf[PIPE_LENGTH];

  lyric = event2string(lyricid);
  if (lyric != NULL) {
    if (lyric[0] == ME_KARAOKE_LYRIC) {
      if ((lyric[1] == '/') || (lyric[1] == '\\')) {
        strlcpy(lyric_buf, lyric + 2, sizeof(lyric_buf));
        a_pipe_write_msg(lyric_buf);
        lyric_col = strlen(lyric_buf);
      }
      else if (lyric[1] == '@') {
        if (lyric[2] == 'L')
          snprintf(lyric_buf, sizeof(lyric_buf), "Language: %s", lyric + 3);
        else if (lyric[2] == 'T')
          snprintf(lyric_buf, sizeof(lyric_buf), "Title: %s", lyric + 3);
        else
          strlcpy(lyric_buf, lyric + 1, sizeof(lyric_buf));
        a_pipe_write_msg(lyric_buf);
        lyric_col = 0;
      }
      else {
        strlcpy(lyric_buf + lyric_col, lyric + 1,
                  sizeof(lyric_buf) - lyric_col);
        a_pipe_write_msg(lyric_buf);
        lyric_col += strlen(lyric + 1);
      }
    }
    else {
      lyric_col = 0;
      a_pipe_write_msg_nobr(lyric + 1);
    }
  }
}
#else
static void
ctl_lyric(int lyricid) {
  char *lyric;

  lyric = event2string(lyricid);
  if (lyric != NULL) {
    if (lyric[0] == ME_KARAOKE_LYRIC) {
      if ((lyric[1] == '/') || (lyric[1] == '\\')) { 
        lyric[1] = '\n';
        a_pipe_write_msg_nobr(lyric + 1);
      }
      else if (lyric[1] == '@') {
        if (lyric[2] == 'L')
          snprintf(local_buf, sizeof(local_buf), "Language: %s", lyric + 3);
        else if (lyric[2] == 'T')
          snprintf(local_buf, sizeof(local_buf), "Title: %s", lyric + 3);
        else
          strlcpy(local_buf, lyric + 1, sizeof(local_buf));
        a_pipe_write_msg(local_buf);
      }
      else a_pipe_write_msg_nobr(lyric + 1);
    }
    else a_pipe_write_msg_nobr(lyric + 1);
  }
}
#endif /* WIDGET_IS_LABEL_WIDGET */

/*ARGSUSED*/
static int
ctl_open(int using_stdin, int using_stdout) {
  ctl.opened = 1;

  set_trace_loop_hook(update_indicator);

  /* The child process won't come back from this call  */
  a_pipe_open();

  return 0;
}

static void
ctl_close(void) {
  if (ctl.opened) {
    a_pipe_write("Q");
    ctl.opened = 0;
    xaw_ready = 0;
  }
}

static void
xaw_add_midi_file(char *additional_path) {
  char *files[1], **ret;
  int i, nfiles, nfit, *local_len = NULL;
  char *p;

  files[0] = additional_path;
  nfiles = 1;
  ret = expand_file_archives(files, &nfiles);
  if (ret == NULL) return;
  titles = (char **)safe_realloc(titles,
             (number_of_files + nfiles) * sizeof(char *));
  list_of_files = (char **)safe_realloc(list_of_files,
                   (number_of_files + nfiles) * sizeof(char *));
  if (nfiles > 0) local_len = (int *)safe_malloc(nfiles * sizeof(int));
  for (i=0, nfit=0;i<nfiles;i++) {
      if (check_midi_file(ret[i]) >= 0) {
          p = strrchr(ret[i], '/');
          if (p == NULL) p = ret[i]; else p++;
          titles[number_of_files+nfit] =
            (char *)safe_malloc(sizeof(char) * (strlen(p) +  9));
          list_of_files[number_of_files + nfit] = safe_strdup(ret[i]);
          local_len[nfit] = sprintf(titles[number_of_files + nfit],
                                  "%d. %s", number_of_files + nfit + 1, p);
          nfit++;
      }
  }
  if (nfit > 0) {
      file_table = (int *)safe_realloc(file_table,
                                     (number_of_files + nfit) * sizeof(int));
      for(i = number_of_files; i < number_of_files + nfit; i++)
          file_table[i] = i;
      number_of_files += nfit;
      a_pipe_write("X %d", nfit);
      for (i=0;i<nfit;i++)
          a_pipe_write_buf(titles[number_of_files - nfit + i], local_len[i]);
  }
  free(local_len);
  free(ret[0]);
  free(ret);
}

static void
xaw_delete_midi_file(int delete_num) {
    int i;
    char *p;

    if (delete_num < 0) {
        for(i=0;i<number_of_files;i++){
            free(list_of_files[i]);
            free(titles[i]);
        }
        list_of_files = NULL; titles = NULL;
        file_table = (int *)safe_realloc(file_table, 1*sizeof(int));
        file_table[0] = 0;
        number_of_files = 0;
        current_no = 0;
    } else {
        free(titles[delete_num]); titles[delete_num] = NULL;
        for(i=delete_num; i<number_of_files-1; i++) {
            list_of_files[i] = list_of_files[i+1];
            p = strchr(titles[i+1], '.');
            titles[i] = (char *)safe_realloc(titles[i],
                                  strlen(titles[i+1]) * sizeof(char));
            sprintf(titles[i], "%d%s", i+1, p);
        }
        if (number_of_files > 0) number_of_files--;
        if ((current_no >= delete_num) && (delete_num)) current_no--;
    }
}

static void
xaw_output_flist(char *filename) {
  int i;
  char *temp = safe_strdup(filename);

  a_pipe_write("s%d %s", number_of_files, temp);
  free(temp);
  for(i=0;i<number_of_files;i++) {
    a_pipe_write("%s", list_of_files[i]);
  }
}

/*ARGSUSED*/
static int
ctl_blocking_read(int32 *valp) {
  int n;

  a_pipe_read(local_buf, sizeof(local_buf));
  for (;;) {
    switch (local_buf[0]) {
      case 'X':
        xaw_add_midi_file(local_buf + 2);
        return RC_NONE;
      case 'A' : xaw_delete_midi_file(-1);
        return RC_QUIT;
      case 'B' : return RC_REALLY_PREVIOUS;
      case 'b': *valp = (int32)(play_mode->rate * 10);
        return RC_BACK;
      case 'C': n = atoi(local_buf + 2);
        opt_chorus_control = n;
        return RC_QUIT;
      case 'D': randomflag = atoi(local_buf + 2); return RC_QUIT;
      case 'd': n = atoi(local_buf + 2);
        xaw_delete_midi_file(atoi(local_buf + 2));
        return RC_NONE;
      case 'E': n = atoi(local_buf + 2);
        opt_modulation_wheel = n & MODUL_BIT;
        opt_portamento = n & PORTA_BIT;
        opt_nrpn_vibrato = n & NRPNV_BIT;
        opt_reverb_control = !!(n & REVERB_BIT) * reverb_type;
        opt_channel_pressure = n & CHPRESSURE_BIT;
        opt_overlap_voice_allow = n & OVERLAPV_BIT;
        opt_trace_text_meta_event = n & TXTMETA_BIT;
        return RC_QUIT;
      case 'F': 
      case 'L': selectflag = atoi(local_buf + 2); return RC_QUIT;
      case 'f': *valp = (int32)(play_mode->rate * 10);
        return RC_FORWARD;
      case 'g': return RC_TOGGLE_SNDSPEC;
      case 'P': return RC_LOAD_FILE;
      case 'U': return RC_TOGGLE_PAUSE;
      case 'S': return RC_QUIT;
      case 'N': return RC_NEXT;
      case 'R': repeatflag = atoi(local_buf + 2); return RC_NONE;
      case 'T':
        n = atoi(local_buf + 2); *valp = n * play_mode->rate;
        return RC_JUMP;
      case 'M':
        n = atoi(local_buf + 2);
        *valp = (int32)n;
        return RC_TOGGLE_MUTE;
      case 'O': *valp = (int32)1; return RC_VOICEDECR;
      case 'o': *valp = (int32)1; return RC_VOICEINCR;
      case 'q': exitflag ^= EXITFLG_AUTOQUIT; return RC_NONE;
      case 's':
        xaw_output_flist(local_buf + 2); return RC_NONE;
      case 't':
        ctl.trace_playing = 1;
        if (local_buf[1] == 'R') ctl_reset();
        return RC_NONE;
      case 'V': 
        amplification = atoi(local_buf + 2); *valp = (int32)0;
        return RC_CHANGE_VOLUME;
      case '+': *valp = (int32)1; return RC_KEYUP;
      case '-': *valp = (int32)-1; return RC_KEYDOWN;
      case '>': *valp = (int32)1; return RC_SPEEDUP;
      case '<': *valp = (int32)1; return RC_SPEEDDOWN;
      case 'W':
        if (olddpm == NULL) {
          PlayMode **ii;
          char id = *(local_buf + 1), *p;

          target_play_mode = NULL;
          for (ii = play_mode_list; *ii != NULL; ii++)
            if ((*ii)->id_character == id) 
              target_play_mode = *ii;
          if (target_play_mode == NULL) goto z1error;
          p = strchr(local_buf, ' ');
          target_play_mode->name = safe_strdup(p + 1);
          target_play_mode->rate = atoi(local_buf + 2);
          if (target_play_mode->open_output() == -1) {
            free(target_play_mode->name);
            goto z1error;
          }
          play_mode->close_output();
          olddpm = play_mode;
          play_mode = target_play_mode;
          /* playmidi.c won't change play_mode when a file is not played,
           * so to be certain, we do this ourselves.
           */
          a_pipe_write("Z1");
          return RC_OUTPUT_CHANGED;
        }
z1error:
        a_pipe_write("Z1E");
        return RC_NONE;
      case 'w':
        if (olddpm != NULL) {
          play_mode->close_output();
          if (*(local_buf + 1) == 'S') a_pipe_write("Z2S");
          target_play_mode = olddpm;
          if (target_play_mode->open_output() == -1) return RC_NONE;
          free(play_mode->name);
          play_mode = target_play_mode;
          olddpm = NULL;
          return RC_OUTPUT_CHANGED;
        }
        return RC_NONE;
      case 'p':
       {
          PlayMode **ii;
          char id = *(local_buf + 1);

          if (play_mode->id_character == id) goto z3error;
          if (olddpm != NULL) goto z3error;

          target_play_mode = NULL;
          for (ii = play_mode_list; *ii != NULL; ii++)
            if ((*ii)->id_character == id)
              target_play_mode = *ii;
          if (target_play_mode == NULL) goto z3error;
          play_mode->close_output();
          if (target_play_mode->open_output() == -1) {
            play_mode->open_output();
            goto z3error;
          }
          play_mode = target_play_mode;
          a_pipe_write("Z3");
          return RC_OUTPUT_CHANGED;
       }
z3error:
       a_pipe_write("Z3E");
       return RC_NONE;
      case 'Q':
        free(file_table);
        for (n=0; n<number_of_files; n++) {
          free(titles[n]); free(list_of_files[n]);
        }
      default : exitflag |= EXITFLG_QUIT; return RC_QUIT;
    }
  }
}

static int
ctl_read(int32 *valp) {
  if (a_pipe_ready() <= 0) return RC_NONE;
  return ctl_blocking_read(valp);
}

static void
shuffle(int n, int *a) {
  int i, j, tmp;

  for (i=0;i<n;i++) {
    j = int_rand(n);
    tmp = a[i];
    a[i] = a[j];
    a[j] = tmp;
  }
}

static void
query_outputs(void) {
  PlayMode **ii; unsigned char c;

  for (ii = play_mode_list; *ii != NULL; ii++) {
    if ((*ii)->flag & PF_FILE_OUTPUT) c = 'F';
    else c = 'O';
    if (*ii == play_mode) c += 32;
    a_pipe_write("%c%c %s", c, (*ii)->id_character, (*ii)->id_name);
  }
  a_pipe_write("Z0");
}

static int
ctl_pass_playing_list(int init_number_of_files, char **init_list_of_files) {
  int command = RC_NONE, i, j;
  int32 val;
  char *p;

  for (i=0; i<MAX_XAW_MIDI_CHANNELS; i++) active[i] = 0;
  reverb_type = opt_reverb_control?opt_reverb_control:DEFAULT_REVERB;
  /* Wait prepare 'interface' */
  a_pipe_read(local_buf, sizeof(local_buf));
  if (strcmp("READY", local_buf)) return 0;
  xaw_ready = 1;

  a_pipe_write("%d",
  (opt_modulation_wheel<<MODUL_N)
     | (opt_portamento<<PORTA_N)
     | (opt_nrpn_vibrato<<NRPNV_N)
     | (!!(opt_reverb_control)<<REVERB_N)
     | (opt_channel_pressure<<CHPRESSURE_N)
     | (opt_overlap_voice_allow<<OVERLAPV_N)
     | (opt_trace_text_meta_event<<TXTMETA_N));
  a_pipe_write("%d", opt_chorus_control);

  query_outputs();

  /* Make title string */
  titles = (char **)safe_malloc(init_number_of_files * sizeof(char *));
  list_of_files = (char **)safe_malloc(init_number_of_files * sizeof(char *));
  for (i=0, j=0;i<init_number_of_files;i++) {
    if (check_midi_file(init_list_of_files[i]) >= 0) {
      p = strrchr(init_list_of_files[i], '/');
      if (p == NULL) p = safe_strdup(init_list_of_files[i]);
      else p++;
      list_of_files[j] = safe_strdup(init_list_of_files[i]);
      titles[j] = (char *)safe_malloc(sizeof(char) * (strlen(p) +  9));
      sprintf(titles[j], "%d. %s", j+1, p);
      j++;
    }
  }
  number_of_files = j;
  titles = (char **)safe_realloc(titles, number_of_files * sizeof(char *));
  list_of_files = (char **)safe_realloc(list_of_files,
                                         number_of_files * sizeof(char *));

  /* Send title string */
  a_pipe_write("%d", number_of_files);
  for (i=0;i<number_of_files;i++)
    a_pipe_write("%s", titles[i]);

  /* Make the table of play sequence */
  file_table = (int *)safe_malloc(number_of_files * sizeof(int));
  for (i=0;i<number_of_files;i++) file_table[i] = i;

  /* Draw the title of the first file */
  current_no = 0;
  if (number_of_files != 0) {
    a_pipe_write("E %s", titles[file_table[0]]);
    command = ctl_blocking_read(&val);
  }

  /* Main loop */
  for (;;) {
    /* Play file */
    if ((command == RC_LOAD_FILE) && (number_of_files != 0)) {
      char *title;
      a_pipe_write("E %s", titles[file_table[current_no]]);
      if ((title = get_midi_title(list_of_files[file_table[current_no]]))
            == NULL)
      title = list_of_files[file_table[current_no]];
      a_pipe_write("e %s", title);
      command = play_midi_file(list_of_files[file_table[current_no]]);
    } else {
      if (command == RC_CHANGE_VOLUME) amplitude += val;
      if (command == RC_JUMP) ;
      if (command == RC_TOGGLE_SNDSPEC) ;
      /* Quit timidity*/
      if (exitflag & EXITFLG_QUIT) return 0;
      /* Stop playing */
      if (command == RC_QUIT) {
        a_pipe_write("T 00:00");
        /* Shuffle the table */
        if (randomflag) {
          if (number_of_files == 0) {
            randomflag = 0;
            continue;
          }
          current_no = 0;
          if (randomflag == 1) {
            shuffle(number_of_files, file_table);
            randomflag = 0;
            command = RC_LOAD_FILE;
            continue;
          }
          randomflag = 0;
          for (i=0;i<number_of_files;i++) file_table[i] = i;
          a_pipe_write("E %s", titles[file_table[current_no]]);
        }
        /* Play the selected file */
        if (selectflag) {
          for (i=0;i<number_of_files;i++)
            if (file_table[i] == selectflag-1) break;
          if (i != number_of_files) current_no = i;
          selectflag = 0;
          command = RC_LOAD_FILE;
          continue;
        }
        /* After the all file played */
      } else if ((command == RC_TUNE_END) || (command == RC_ERROR)) {
        if (current_no+1 < number_of_files) {
          if (olddpm != NULL) {
            command = RC_QUIT;
            a_pipe_write("Z2");
          } else {
            current_no++;
            command = RC_LOAD_FILE;
            continue;
          }
        } else if (exitflag & EXITFLG_AUTOQUIT) {
          return 0;
          /* Repeat */
        } else if (repeatflag) {
          current_no = 0;
          command = RC_LOAD_FILE;
          continue;
          /* Off the play button */
        } else {
          if (olddpm != NULL) a_pipe_write("Z2");
          a_pipe_write("O");
        }
        /* Play the next */
      } else if (command == RC_NEXT) {
        if (current_no+1 < number_of_files) current_no++;
        command = RC_LOAD_FILE;
        continue;
        /* Play the previous */
      } else if (command == RC_REALLY_PREVIOUS) {
        if (current_no > 0) current_no--;
        command = RC_LOAD_FILE;
        continue;
      }
      command = ctl_blocking_read(&val);
    }
  }
}

/* ------ Pipe handlers ----- */

static void
a_pipe_open(void) {
  int cont_inter[2], inter_cont[2];

  if ((pipe(cont_inter) < 0) || (pipe(inter_cont) < 0)) exit(1);

  if (fork() == 0) {
    close(cont_inter[1]);
    close(inter_cont[0]);
    pipe_in_fd = cont_inter[0];
    pipe_out_fd = inter_cont[1];
    a_start_interface(pipe_in_fd);
  }
  close(cont_inter[0]);
  close(inter_cont[1]);
  pipe_in_fd = inter_cont[0];
  pipe_out_fd = cont_inter[1];
}

void
a_pipe_write(const char *fmt, ...) {
  /* char local_buf[PIPE_LENGTH]; */
  int len;
  va_list ap;

  va_start(ap, fmt);
  len = vsnprintf(local_buf, sizeof(local_buf), fmt, ap);
  if ((len < 0) || (len > PIPE_LENGTH))
    write(pipe_out_fd, local_buf, PIPE_LENGTH);
  else write(pipe_out_fd, local_buf, len);
  write(pipe_out_fd, "\n", 1);
  va_end(ap);
}

static void
a_pipe_write_buf(char *buf, int len) {
  if ((len < 0) || (len > PIPE_LENGTH)) write(pipe_out_fd, buf, PIPE_LENGTH);
  else write(pipe_out_fd, buf, len);
  write(pipe_out_fd, "\n", 1);
}

static int
a_pipe_ready(void) {
  fd_set fds;
  static struct timeval tv;
  int cnt;

  FD_ZERO(&fds);
  FD_SET(pipe_in_fd, &fds);
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  if ((cnt = select(pipe_in_fd+1, &fds, NULL, NULL, &tv)) < 0) return -1;
  return (cnt > 0) && (FD_ISSET(pipe_in_fd, &fds) != 0);
}

int
a_pipe_read(char *buf, size_t bufsize) {
  int i;

  bufsize--;
  for (i=0;i<bufsize;i++) {
    ssize_t len = read(pipe_in_fd, buf+i, 1);
    if (len != 1) {
      perror("CONNECTION PROBLEM WITH XAW PROCESS");
      exit(1);
    }
    if (buf[i] == '\n') break;
  }
  buf[i] = '\0';
  return 0;
}

int
a_pipe_nread(char *buf, size_t n) {
    ssize_t i, j = 0;

    if (n <= 0) return 0;
    while ((i = read(pipe_in_fd, buf + j, n - j)) > 0) j += i;
    return j;
}

void
a_pipe_sync(void) {
  fsync(pipe_out_fd);
  fsync(pipe_in_fd);
  usleep(100000);
}

static void
a_pipe_write_msg(char *msg) {
    size_t msglen;
    char buf[2 + sizeof(size_t)], *p, *q;

    /* convert '\r' to '\n', but strip '\r' from '\r\n' */
    p = q = msg;
    while (*q != '\0') {
      if (*q != '\r') *p++ = *q++;
      else if (*(++q) != '\n') *p++ = '\n';
    }
    *p = '\0';

    msglen = strlen(msg) + 1; /* +1 for '\n' */
    buf[0] = 'L';
    buf[1] = '\n';

    memcpy(buf + 2, &msglen, sizeof(size_t));
    write(pipe_out_fd, buf, sizeof(buf));
    write(pipe_out_fd, msg, msglen - 1);
    write(pipe_out_fd, "\n", 1);
}

static void
a_pipe_write_msg_nobr(char *msg) {
    size_t msglen;
    char buf[2 + sizeof(size_t)], *p, *q;

    /* convert '\r' to '\n', but strip '\r' from '\r\n' */
    p = q = msg;
    while (*q != '\0') {
      if (*q != '\r') *p++ = *q++;
      else if (*(++q) != '\n') *p++ = '\n';
    }
    *p = '\0';

    msglen = strlen(msg);
    buf[0] = 'L';
    buf[1] = '\n';

    memcpy(buf + 2, &msglen, sizeof(size_t));
    write(pipe_out_fd, buf, sizeof(buf));
    write(pipe_out_fd, msg, msglen);
}

static void
ctl_note(int status, int ch, int note, int velocity) {
  char c;

  if (ch >= MAX_XAW_MIDI_CHANNELS) return;
  if(!ctl.trace_playing || midi_trace.flush_flag) return;

  switch (status) {
  case VOICE_ON:
    c = '*';
    break;
  case VOICE_SUSTAINED:
    c = '&';
    break;
  case VOICE_FREE:
  case VOICE_DIE:
  case VOICE_OFF:
  default:
    c = '.';
    break;
  }
  a_pipe_write("Y%c%c%03d%d", ch+'A', c, (unsigned char)note, velocity);

  if (active[ch] == 0) {
    active[ch] = 1;
    ctl_program(ch, channel[ch].program, channel_instrum_name(ch));
  }
}

static void
ctl_program(int ch, int val, void *comm) {
  if ((!ctl.trace_playing) || (ch >= MAX_XAW_MIDI_CHANNELS)) return;

  if ((channel[ch].program == DEFAULT_PROGRAM) && (active[ch] == 0) &&
      (!ISDRUMCHANNEL(ch))) return;
  active[ch] = 1;
  if (!IS_CURRENT_MOD_FILE) val += progbase;
  a_pipe_write("PP%c%d", ch+'A', val);
  if (comm != NULL) {
    a_pipe_write("I%c%s", ch+'A', (!strlen((char *)comm) &&
                 (ISDRUMCHANNEL(ch)))? "<drum>":(char *)comm);
  }
}

static void
ctl_drumpart(int ch, int is_drum) {
  if ((!ctl.trace_playing) || (ch >= MAX_XAW_MIDI_CHANNELS)) return;

  a_pipe_write("i%c%c", ch+'A', is_drum+'A');
}

static void
ctl_event(CtlEvent *e) {
  switch(e->type) {
    case CTLE_LOADING_DONE:
      break;
    case CTLE_CURRENT_TIME:
      ctl_current_time((int)e->v1, (int)e->v2);
      break;
    case CTLE_PLAY_START:
      ctl_total_time((int)e->v1);
      break;
    case CTLE_PLAY_END:
      break;
    case CTLE_TEMPO:
      ctl_tempo((int)e->v1);
      break;
    case CTLE_TIME_RATIO:
      ctl_timeratio((int)e->v1);
      break;
    case CTLE_METRONOME:
      break;
    case CTLE_NOTE:
      ctl_note((int)e->v1, (int)e->v2, (int)e->v3, (int)e->v4);
      break;
    case CTLE_PROGRAM:
      ctl_program((int)e->v1, (int)e->v2, (char *)e->v3);
      break;
    case CTLE_DRUMPART:
      ctl_drumpart((int)e->v1, (int)e->v2);
      break;
    case CTLE_VOLUME:
      ctl_volume((int)e->v1, (int)e->v2);
      break;
    case CTLE_EXPRESSION:
      ctl_expression((int)e->v1, (int)e->v2);
      break;
    case CTLE_PANNING:
      ctl_panning((int)e->v1, (int)e->v2);
      break;
    case CTLE_SUSTAIN:
      ctl_sustain((int)e->v1, (int)e->v2);
      break;
    case CTLE_PITCH_BEND:
      ctl_pitch_bend((int)e->v1, (int)e->v2);
      break;
    case CTLE_MOD_WHEEL:
      ctl_pitch_bend((int)e->v1, e->v2 ? -1 : 0x2000);
      break;
    case CTLE_CHORUS_EFFECT:
      set_otherinfo((int)e->v1, (int)e->v2, 'c');
      break;
    case CTLE_REVERB_EFFECT:
      set_otherinfo((int)e->v1, (int)e->v2, 'r');
      break;
    case CTLE_LYRIC:
      ctl_lyric((int)e->v1);
      break;
    case CTLE_MASTER_VOLUME:
      ctl_master_volume((int)e->v1);
      break;
    case CTLE_REFRESH:
      ctl_refresh();
      break;
    case CTLE_RESET:
      ctl_reset();
      break;
    case CTLE_MUTE:
      ctl_mute((int)e->v1, (int)e->v2);
      break;
    case CTLE_KEYSIG:
      ctl_keysig((int)e->v1);
      break;
    case CTLE_KEY_OFFSET:
      ctl_key_offset((int)e->v1);
      break;
  }
}

static void
ctl_refresh(void) { }

static void
set_otherinfo(int ch, int val, char c) {
  if ((!ctl.trace_playing) || (ch >= MAX_XAW_MIDI_CHANNELS)) return;
  a_pipe_write("P%c%c%d", c, ch+'A', val);
}

static void
ctl_reset(void) {
  int i;

  if (!ctl.trace_playing) return;

  indicator_last_update = get_current_calender_time();
  ctl_tempo((int)current_play_tempo);
  ctl_timeratio((int)(100 / midi_time_ratio + 0.5));
  ctl_keysig((int)current_keysig);
  ctl_key_offset(note_key_offset);
  for (i=0; i<MAX_XAW_MIDI_CHANNELS; i++) {
    active[i] = 0;
    if (ISDRUMCHANNEL(i)) {
      if (opt_reverb_control) set_otherinfo(i, get_reverb_level(i), 'r');
    } else {
      set_otherinfo(i, channel[i].bank, 'b');
      if (opt_reverb_control) set_otherinfo(i, get_reverb_level(i), 'r');
      if (opt_chorus_control) set_otherinfo(i, get_chorus_level(i), 'c');
    }
    ctl_program(i, channel[i].bank, channel_instrum_name(i));
    ctl_volume(i, channel[i].volume);
    ctl_expression(i, channel[i].expression);
    ctl_panning(i, channel[i].panning);
    ctl_sustain(i, channel[i].sustain);
    if ((channel[i].pitchbend == 0x2000) && (channel[i].mod.val > 0))
      ctl_pitch_bend(i, -1);
    else
      ctl_pitch_bend(i, channel[i].pitchbend);
  }
  a_pipe_write("R");
}

static void
update_indicator(void) {
  double t, diff;

  if (!ctl.trace_playing) return;
  t = get_current_calender_time();
  diff = t - indicator_last_update;
  if (diff > XAW_UPDATE_TIME) {
     a_pipe_write("U");
     indicator_last_update = t;
  }
}

static void
ctl_mute(int ch, int mute) {
  if ((!ctl.trace_playing) || (ch >= MAX_XAW_MIDI_CHANNELS)) return;
  a_pipe_write("M%c%d", ch+'A', mute);
  return;
}

static void
ctl_tempo(int tempo) {
  a_pipe_write("r%d", tempo);
  return;
}


static void
ctl_timeratio(int ratio) {
  a_pipe_write("q%d", ratio);
  return;
}

static void
ctl_keysig(int keysig) {
  a_pipe_write("p %d", keysig);
  return;
}

static void
ctl_key_offset(int offset) {
  a_pipe_write("o %d", offset);
  return;
}

/*
 * interface_<id>_loader();
 */
ControlMode *
interface_a_loader(void) {
    return &ctl;
}
