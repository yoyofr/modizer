/*
 *                                sc68 - API
 *         Copyright (C) 2001 Ben(jamin) Gerard <ben@sashipa.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <string.h>

#include "config_package68.h"
#include "config_option68.h"

#include "api68.h"
#include "mixer68.h"
#include "conf68.h"
#include "emu68.h"
#include "init68.h"
#include "error68.h"
#include "string68.h"
#include "alloc68.h"
#include "file68.h"
#include "rsc68.h"
#include "debugmsg68.h"

#include "emu68.h"
#include "ioplug68.h"
#include "io68.h"

#define TRAP_14_ADDR 0x600
/* ST xbios function emulator.#
 *  Just 4 SUPEREXEC in LOGICAL & some other music files
 */
static u8 trap14[] = {
  0x0c,0x6f,0x00,0x26,0x00,0x06,0x67,0x00,0x00,0x22,0x0c,0x6f,0x00,0x1f,0x00,
  0x06,0x67,0x00,0x00,0x28,0x0c,0x6f,0x00,0x22,0x00,0x06,0x67,0x00,0x00,0x86,
  0x0c,0x6f,0x00,0x0e,0x00,0x06,0x67,0x00,0x00,0xec,0x4e,0x73,0x48,0xe7,0xff,
  0xfe,0x20,0x6f,0x00,0x44,0x4e,0x90,0x4c,0xdf,0x7f,0xff,0x4e,0x73,0x48,0xe7,
  0xff,0xfe,0x41,0xef,0x00,0x44,0x4c,0x98,0x00,0x07,0x02,0x40,0x00,0x03,0xd0,
  0x40,0x16,0x38,0xfa,0x17,0x02,0x43,0x00,0xf0,0xe5,0x4b,0x36,0x7b,0x00,0x42,
  0xd6,0xc3,0x76,0x00,0x45,0xf8,0xfa,0x1f,0xd4,0xc0,0x43,0xf8,0xfa,0x19,0xd2,
  0xc0,0xe2,0x48,0x04,0x00,0x00,0x02,0x6b,0x1a,0x57,0xc3,0x18,0x03,0x0a,0x03,
  0x00,0xf0,0x44,0x04,0x48,0x84,0xd8,0x44,0x43,0xf1,0x40,0xfe,0xd8,0x44,0x02,
  0x41,0x00,0x0f,0xe9,0x69,0xc7,0x11,0x14,0x82,0x26,0x90,0x83,0x11,0x4c,0xdf,
  0x7f,0xff,0x4e,0x73,0x00,0x34,0x00,0x20,0x00,0x14,0x00,0x10,0x48,0xe7,0x00,
  0xc0,0x43,0xfa,0x00,0x20,0x70,0x00,0x20,0x7b,0x00,0x3e,0x41,0xfb,0x80,0x5e,
  0x23,0x88,0x00,0x00,0x58,0x40,0xb0,0x7c,0x00,0x24,0x66,0xec,0x20,0x09,0x4c,
  0xdf,0x03,0x00,0x4e,0x73,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x4e,0x75,0xc1,0x88,0x41,0xfa,0x00,0x0c,0x21,0x48,0x00,0x04,0x58,
  0x48,0xc1,0x88,0x4e,0x73,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,
};

/* paulaemul.c */
extern int paula_interpol;

struct _api68_s {
  int            version;    /**< sc68 version.                       */
  reg68_t        reg;        /**< 68k registers.                      */
  disk68_t     * disk;       /**< Current loaded disk.                */
  music68_t    * mus;        /**< Current playing music.              */
  int            track;      /**< Current playing track.              */
  int            track_to;   /**< Track to set (0:n/a).               */
  int            loop_to;    /**< Loop to set (-1:default 0:infinite) */
  int            force_loop; /**< Loop to set if default (-1:music)   */
  int            track_here; /**< Force first track here.             */
  unsigned int   playaddr;   /**< Current play address in 68 memory.  */
  int            seek_to;    /**< Seek to this time (-1:n/a)          */
  SC68config_t * config;     /**< Config.                             */

  /** Playing time info. */
  struct {
    unsigned int def_ms;     /**< default time in ms.             */
    unsigned int length_ms;  /**< current track length in ms.     */
    unsigned int elapsed_ms; /**< number of elapsed ms.           */
    unsigned int total;      /**< total sec so far !  .           */
    unsigned int total_ms;   /**< total ms correction.            */
  } time;

  /** Mixer info struture. */
  struct
  {
    unsigned int   rate;         /**< Sampling rate in hz.              */
    unsigned int * buffer;       /**< Current PCM position.             */
    int            buflen;       /**< PCM count in buffer.              */
    int            stdbuflen;    /**< Default number of PCM per pass.   */
    unsigned int   cycleperpass; /**< Number of 68K cycles per pass.    */
    int            amiga_blend;  /**< Amiga LR blend factor [0..65536]. */
    unsigned int   sample_cnt;   /**< Number of mixed PCM.              */
    unsigned int   pass_cnt;     /**< Current pass.                     */
    unsigned int   pass_total;   /**< Total number of pass.             */
    unsigned int   loop_cnt;     /**< Loop counter.                     */
    unsigned int   loop_total;   /**< Total # of loop (0:infinite)      */
    int            stp;          /**< pitch frq (fixed int 8).          */
    int            max_stp;      /**< max pitch frq (0:no seek).        */
  } mix;

};

/* Currently only one API is possible. */
static api68_t * api68;

static int stream_read_68k(unsigned int dest, istream_t * is, unsigned int sz)
{
  if (EMU68_memvalid(dest, sz)) {
    return SC68error_add("68Kread_stream() : %s",EMU68error_get());
  }
  return (istream_read(is, reg68.mem+dest, sz) == sz) ? 0 : -1;
}

static int init68k(api68_t * api, const int mem68_size)
{
  u8 * mem = 0;
  int alloc_size;

  api68_debug("init_68k() : enter\n");

  /* Calculate 68k memory buffer size. See EMU68_init() doc for more info. */
  alloc_size = mem68_size;

  if (EMU68_debugmode()) {
    api68_debug("init_68k() : debug mode detected\n");
    alloc_size <<= 1;
  } else {
    api68_debug("init_68k() : fast mode detected\n");
    alloc_size += 3;
  }

  /* Allocate 68k memory buffer. */
  api68_debug("init_68k() : alloc 68 memory (%dkb)\n", alloc_size >> 10);
  mem = SC68alloc(alloc_size);
  if (!mem) {
    goto error;
  }

  /* Do initialization. */
  api68_debug("init_68k() : 68K emulator init\n");
  if (EMU68_init(mem, mem68_size) < 0) {
    SC68error_add(EMU68error_get());
    goto error;
  }

  api->reg = reg68;
  api->reg.sr = 0x2000;
  api->reg.a[7] = mem68_size - 4;

  /* Initialize chipset */
  api68_debug("init_68k() : chipset init\n");

  if (YM_init() < 0) {
    SC68error_add("Yamaha 2149 emulator init failed");
    goto error;
  }

  if (MW_init() < 0) {
    SC68error_add("Microwire emulator init failed");
    goto error;
  }

  if (MFP_init() < 0) {
    SC68error_add("MFP emulator init failed");
    goto error;
  }

  if (PL_init() < 0) {
    SC68error_add("Paula emulator init failed");
    goto error;
  }

  api68_debug("init_68k() : Success\n");
  return 0;

 error:
  api68_debug("init_68k() : Failed\n");

  SC68free(mem);
  return -1;
}

static int myconfig_get_int(SC68config_t * c,
			    const char * name,
			    int def)
{
  SC68config_type_t type;
  int v = def;
  if (c) {
    v = -1;
    type = SC68config_get(c, &v, &name);  
    if (type != SC68CONFIG_INT) {
      v = def;
    }
  }
  return v;
}

static const char * myconfig_get_str(SC68config_t * c,
				     const char * name,
				     const char * def)
{
  SC68config_type_t type;
  const char * r = def;
  if (c) {
    int idx = -1;
    type = SC68config_get(c, &idx, &name);
    if (type == SC68CONFIG_STRING) {
      r = name;
    }
  }
  return r;
}


static void set_config(api68_t * api)
{
  SC68config_t * c = api->config;
  const char * lmusic, * rmusic;

  api->version         = myconfig_get_int(c, "version",       VERSION68_NUM);
  api->mix.amiga_blend = myconfig_get_int(c, "amiga_blend",   0x4000);
  paula_interpol       = myconfig_get_int(c, "amiga_interpol",1);
  api->track_here      = myconfig_get_int(c, "force_track",   1);
  api->time.def_ms     = myconfig_get_int(c, "default_time",  3*60) * 1000;
  api->mix.rate        = myconfig_get_int(c, "sampling_rate", 44100);
  api->force_loop      = myconfig_get_int(c, "force_loop",    -1);
  api->mix.max_stp     = myconfig_get_int(c, "seek_speed",    0xF00);
  api->time.total      = myconfig_get_int(c, "total_time",    0);
  api->time.total_ms   = 0;

  rsc68_get_path(0,0,&lmusic, &rmusic);
  if (!lmusic) {
    lmusic = myconfig_get_str(c, "music_path", 0);
    rsc68_set_music(lmusic);
  }
  if (!rmusic) {
    rmusic = myconfig_get_str(c, "remote_music_path", 0);
    rsc68_set_remote_music(rmusic);
  }

}

static void myconfig_set_int(SC68config_t * c,
			    const char * name,
			    int v)
{
  if (c) {
    SC68config_set(c, -1, name, v, 0);
  }
}

static void get_config(api68_t * api)
{
  SC68config_t * c = api->config;

  myconfig_set_int(c, "version",       VERSION68_NUM);
  myconfig_set_int(c, "amiga_blend",   api->mix.amiga_blend);
  myconfig_set_int(c, "amiga_interpol",!!paula_interpol);
  myconfig_set_int(c, "force_track",   api->track_here);
  myconfig_set_int(c, "default_time",  (api->time.def_ms+999)/1000u);
  myconfig_set_int(c, "sampling_rate", api->mix.rate);
  myconfig_set_int(c, "force_loop",    api->force_loop);
  myconfig_set_int(c, "seek_speed",    api->mix.max_stp);
  myconfig_set_int(c, "total_time",api->time.total+api->time.total_ms/1000u);
}

int api68_config_load(api68_t * api)
{
  int err = -1;

  if (api) {
    if (!api->config) {
      api->config = SC68config_create(0);
    }
    err = SC68config_load(api->config);
    set_config(api);
  }
  return err;
}

int api68_config_save(api68_t * api)
{
  int err = -1;
  if (api) {
    get_config(api);
    err = SC68config_save(api->config);
  }
  return err;
}

int api68_config_idx(api68_t * api, const char * name)
{
  return SC68config_get_idx(api ? api->config : 0, name);
}

SC68config_type_t api68_config_range(api68_t * api, int idx,
                                     int * min, int * max, int * def)
{
  return SC68config_range(api ? api->config : 0, idx, min, max, def);
}

SC68config_type_t api68_config_get(api68_t * api,
				   int * idx, const char ** name)
{
  return api
    ? SC68config_get(api->config, idx, name)
    : SC68CONFIG_ERROR;
}

SC68config_type_t api68_config_set(api68_t * api,
				   int idx,
				   const char * name,
				   int v,
				   const char * s)
{
  return api
    ? SC68config_set(api->config,idx,name,v,s)
    : SC68CONFIG_ERROR;
}

void api68_config_apply(api68_t * api)
{
  if (api) {
    SC68config_valid(api->config);
    set_config(api);
  }
}


api68_t * api68_init(api68_init_t * init)
{
  const int mem68_size = (512<<10); /* $$$ should be retrieve form EMU68. */
  api68_t *api = 0;

  /* First things to do to have some debug messages. */
  if (init) {
    /* Set debug handler. */
    debugmsg68_set_handler(init->debug);
    debugmsg68_set_cookie(init->debug_cookie);
  }

  api68_debug("api68_init(): enter\n");

  if (api68) {
    api68_debug("api68_init(): already initialized\n");
    return 0;
  }

  if (!init) {
    SC68error_add("api68_init() : missing init struct");
    return 0;
  }

  /* Set dynamic memory handler. */
  if (!init->alloc || !init->free) {
    SC68error_add("api68_init() : missing dynamic memory handler(s)");
    return 0;
  }
  SC68set_alloc(init->alloc);
  SC68set_free(init->free);

  /* Set resource pathes. */
  if (init->user_path) {
    api68_debug("api68_init(): user resource [%s]\n", init->user_path);
    rsc68_set_user(init->user_path);
  }
  if (init->shared_path) {
    api68_debug("api68_init(): shared resource [%s]\n", init->shared_path);
    rsc68_set_share(init->shared_path);
  }
  if (init->lmusic_path) {
    api68_debug("api68_init(): local musics [%s]\n", init->lmusic_path);
    rsc68_set_music(init->lmusic_path);
  }
  if (init->rmusic_path) {
    api68_debug("api68_init(): remote musics [%s]\n", init->rmusic_path);
    rsc68_set_music(init->rmusic_path);
  }

  /* Intialize file68. */
  init->argc = file68_init(init->argv, init->argc);

  /* Get the all pathes. */
  rsc68_get_path(&init->shared_path,
		   &init->user_path,
		   &init->lmusic_path,
		   &init->rmusic_path);

  /* Alloc API struct. */
  api = SC68alloc(sizeof(api68_t));
  if (!api) {
    goto error;
  }
  memset(api, 0, sizeof(*api));

  /* Load config file */
  api68_config_load(api);

  /* Override config. */
  if (init->sampling_rate) {
    api->mix.rate = init->sampling_rate;
  }
  if (!api->mix.rate) {
    api->mix.rate = SAMPLING_RATE_DEF;
  }
  init->sampling_rate = api68_sampling_rate(api, api->mix.rate);

  if (init68k(api, mem68_size)) {
    goto error;
  }

  /* Finally gets all pathes. */
  rsc68_get_path(&init->shared_path,
		   &init->user_path,
		   &init->lmusic_path,
		   &init->rmusic_path);

  debugmsg68("api68: shared-path=[%s]\n",init->shared_path);
  debugmsg68("api68: user-path=[%s]\n",init->user_path);
  debugmsg68("api68: music-path=[%s]\n",init->lmusic_path);
  debugmsg68("api68: music-rpath=[%s]\n",init->rmusic_path);
    
  api68 = api;
  api68_debug("api68_init(): Success\n");
  return api;

 error:
  api68_shutdown(api);
  api68_debug("api68_init(): Failed\n");
  return 0;
}

void api68_shutdown(api68_t * api)
{
  u8 * mem = reg68.mem;
  api68_debug("api68_shutdown(): enter\n");

  api68_config_save(api);

  file68_shutdown();
  EMU68_kill();
  SC68free(mem);
  SC68free(api);

  api68_debug("api68_shutdown(): leave\n");
  api68 = 0;
}

unsigned int api68_sampling_rate(api68_t * api, unsigned int f)
{
  if (!f) {
    f = api ? api->mix.rate : MW_sampling_rate(0);
  } else {
    f = YM_sampling_rate(f);
    PL_sampling_rate(f);
    MW_sampling_rate(f);
    if (api) {
      api->mix.rate = f;
    }
  }
  api68_debug("api68_sampling_rate() : %u hz\n", f);
  return f;
}

void api68_set_share(api68_t * api, const char * path)
{
  rsc68_set_share(path);
}

void api68_set_user(api68_t * api, const char * path)
{
  rsc68_set_user(path);
}

static unsigned int calc_disk_time(api68_t * api, disk68_t * d)
{
  return (api && api->disk == d && d->nb_six == 1 && d->mus == api->mus)
    ? api->time.length_ms
    : d->time_ms;
}

static int calc_loop_total(int force_loop, int loop_to, int mus_loop)
{
  return
    (loop_to == -1)
    ? (force_loop == -1 ? mus_loop : force_loop)
    : loop_to;
}

static unsigned int fr_to_ms(unsigned int frames, unsigned int hz)
{
  u64 ms;

  ms = frames;
  ms *= 1000u;
  ms /= hz;

  return (unsigned int) ms;
}

static unsigned int ms_to_fr(unsigned int ms, unsigned int hz)
{
  u64 fr;

  fr =  ms;
  fr *= hz;
  fr /= 1000u;

  return (unsigned int ) fr;
}



/** Start current music of current disk.
 */
static int apply_change_track(api68_t * api)
{
  u32 a0;
  disk68_t * d;
  music68_t * m;
  int track;

  if (!api || !api->disk) {
    return API68_MIX_ERROR;
  }
  if (track = api->track_to, !track) {
    return API68_MIX_OK;
  }

  api->track_to = 0;
  api->mix.loop_cnt = 0;

  /* -1 : stop */
  if (track == -1) {
    api68_debug("apply_change_track() : stop\n");
    api->mus = 0;
    api->track = 0;
    api->time.total += api->time.elapsed_ms / 1000u;
    api->time.total_ms += api->time.elapsed_ms % 1000u;
    api->time.elapsed_ms = 0;
    api->time.length_ms = 0;
    return API68_END | API68_CHANGE;
  }

  api68_debug("apply_change_track(%d) : enter\n", track);

  d = api->disk;
  if (track < 1 || track > d->nb_six) {
    SC68error_add("track [%d] out of range [%d]", track, d->nb_six);
    return API68_MIX_ERROR;
  }
  m = d->mus + track - 1;

  api68_debug(" -> Starting track #%02d - [%s]:\n", track, m->name);

  /* ReInit 68K & IO */
  EMU68memory_reset();
  EMU68ioplug_unplug_all();
  if (m->hwflags.bit.amiga) {
    api68_debug(" -> Add Paula hardware\n");
    EMU68ioplug(&paula_io);
    EMU68_set_interrupt_io(0);
  }
  if (m->hwflags.bit.ym) {
    api68_debug(" -> Add YM hardware\n");
    EMU68ioplug(&shifter_io);
    EMU68ioplug(&ym_io);
    EMU68ioplug(&mfp_io);
    EMU68_set_interrupt_io(&mfp_io);
  }
  if (m->hwflags.bit.ste) {
    api68_debug(" -> Add STE hardware\n");
    EMU68ioplug(&mw_io);
  }
  EMU68_reset();

  /* Init exceptions */
  memset(reg68.mem, 0, reg68.memsz);
  reg68.mem[0] = 0x4e;
  reg68.mem[1] = 0x73;
  reg68.mem[0x41a] = 0;         /* Zound Dragger */
  reg68.mem[0x41b] = 0x10;      /* Zound Dragger */
  reg68.mem[TRAP_VECTOR(14) + 0] = (u8) (TRAP_14_ADDR >> 24);
  reg68.mem[TRAP_VECTOR(14) + 1] = (u8) (TRAP_14_ADDR >> 16);
  reg68.mem[TRAP_VECTOR(14) + 2] = (u8) (TRAP_14_ADDR >> 8);
  reg68.mem[TRAP_VECTOR(14) + 3] = (u8) (TRAP_14_ADDR);
  memcpy(reg68.mem + TRAP_14_ADDR, trap14, sizeof(trap14));

  /* Address in 68K memory : default $8000 */
  api->playaddr = a0 = (!m->a0) ? 0x8000 : m->a0;
  api68_debug(" -> play address %06x\n", api->playaddr);

  /* Check external replay */
  if (m->replay) {
    int err;
    int size = 0;
    istream_t * is;

    api68_debug(" -> external replay '%s'\n", m->replay);
    is = rsc68_open(rsc68_replay, m->replay, 1, 0);
    err = !is || (size = istream_length(is), size < 0);
    err = err || stream_read_68k(a0, is, size);
    istream_destroy(is);
    if (err) {
      return API68_MIX_ERROR;
    }
    api68_debug(" -> external replay [%06x-%06x]\n", a0, a0+size);
    a0 = a0 + ((size + 1) & -2);
  }

  /* Copy Data into 68K memory */
  if (EMU68_memput(a0, (u8 *)m->data, m->datasz)) {
    api68_debug(" -> Failed music data [%06x-%06x]\n", a0, a0+m->datasz);

    SC68error_add(EMU68error_get());
    return API68_MIX_ERROR;
  }

  api68_debug(" -> music data [%06x-%06x)\n", a0, a0+m->datasz);

  api->mix.rate = 44100;
  api->mix.buffer = 0;
  api->mix.buflen = 0;
  api->mix.sample_cnt = 0;
  api->mix.pass_cnt = 0;
  api->time.elapsed_ms = 0;
  api->mix.loop_total = calc_loop_total(api->force_loop,api->loop_to,m->loop);

  api68_debug(" -> loop            : %u\n", api->mix.loop_total);

  if (!m->frames) {
    api->mix.pass_total = ms_to_fr(api->time.def_ms, m->frq);
    api->time.length_ms = api->time.def_ms;
    api68_debug(" -> default time ms : %u\n", api->time.def_ms);
  } else {
    api->mix.pass_total = m->frames;
    api->time.length_ms = m->time_ms;
    api68_debug(" -> time ms         : %u\n", m->time_ms);
  }
  api68_debug(" -> length ms       : %u\n", api->time.length_ms);
  api68_debug(" -> frames          : %u\n", api->mix.pass_total);
  api->mix.cycleperpass = (m->frq == 50) ? 160256 : (8000000 / m->frq);
  /* make cycleperpass multiple of 32 (can't remember why :) ) */
  api->mix.cycleperpass = (api->mix.cycleperpass+31) & ~31;
  api68_debug(" -> cycle/frame     : %u\n", api->mix.cycleperpass);

  {
    u64 len;
    len = api->mix.rate;
    len *= api->mix.cycleperpass;
    len /= 8000000;
    api->mix.stdbuflen = (int) len;
  }
  api68_debug(" -> buffer length   : %u\n", api->mix.stdbuflen);

  /* Set 68K register value for INIT */
  api68_debug(" -> Running music init code...\n");
  api->reg.d[0] = m->d0;
  api->reg.d[1] = !m->hwflags.bit.ste;
  api->reg.d[2] = m->datasz;
  api->reg.a[0] = a0;
  api->reg.a[7] = reg68.memsz - 16;
  api->reg.pc = api->playaddr;
  api->reg.sr = 0x2300;
  if (m->frq == 60 && m->hwflags.bit.ym) {
    api68_debug(" -> Force shifter to 60Hz...\n");
    shifter_io.Wfunc[0](0xFF820a,0xFC,0);
  }
  EMU68_set_registers(&api->reg);
  reg68.cycle = 0;
  EMU68_level_and_interrupt(0);
  api68_debug(" -> OK\n");

  /* Set 68K PC register to music play address */
  EMU68_get_registers(&api->reg);
  api->reg.pc = api->playaddr+8;
  api->reg.sr = 0x2300;
  EMU68_set_registers(&api->reg);

  reg68.cycle = 0;

  api->mus = m;
  api->track = track;

  return API68_CHANGE;
}


static unsigned int calc_current_ms(api68_t * api)
{
  u64 ms;

  ms = api->mix.pass_cnt + (api->mix.loop_cnt * api->mix.pass_total);
  ms *= api->mix.cycleperpass;
  ms /= 8000u;
  return api->time.elapsed_ms = (unsigned int) ms;
}

int api68_process(api68_t * api, void * buf16st, int n)
{
  int ret = API68_IDLE;
  u32 * buffer;
  int buflen;
  //  const unsigned int sign = MIXER68_CHANGE_SIGN;

  if (!api || n < 0) {
    return API68_MIX_ERROR;
  }

  if (!api->mus) {
    ret = apply_change_track(api);
  }

  if (!api->mus) {
    return API68_MIX_ERROR;
  }

  if (n==0) {
    return ret;
  }

  if (!buf16st) {
    return API68_MIX_ERROR;
  }

  buffer = api->mix.buffer;
  buflen = api->mix.buflen;

/*   api68_debug("Enter process: n:%d len:%d\n", n, buflen); */

  while (n > 0) {
    int code;

    code = apply_change_track(api);
    if (code & API68_MIX_ERROR) {
      return code;
    }
    ret |= code;
    if (code & API68_END) {
      break;
    }

    if (buflen <= 0) {
      /* Not idle */
      ret &= ~API68_IDLE;

      /* Run the emulator. */
      EMU68_level_and_interrupt(api->mix.cycleperpass);

      /* Always used YM buffer, even if no YM required. */
      buffer = YM_get_buffer();

      if (api->mus->hwflags.bit.amiga) {
	/* Amiga - Paula */
	buflen = api->mix.stdbuflen;
        PL_mix(buffer, api->reg.mem, buflen);
        SC68mixer_blend_LR(buffer, buffer, buflen,api->mix.amiga_blend, 0, 0);
      } else {
	/* Buffer length depends on YM mixer. */
	buflen = (api->mus->hwflags.bit.ym)
	  ? YM_mix(api->mix.cycleperpass)
	  : api->mix.stdbuflen;

      /* $$$macosx bug */
      if (0) {
	int i;
	api68_debug("Pass #%u, len:%d cpp:%d\n",
		    api->mix.pass_cnt, buflen, api->mix.cycleperpass);
	if (0) {
	  for (i=0; i<buflen; ++i) {
	    api68_debug("%08X ", buffer[i]);
	  }
	  api68_debug("\n");
	}
      }


	if (api->mus->hwflags.bit.ste) {
	  /* STE - MicroWire */
	  MW_mix(buffer, api->reg.mem, buflen);
	} else {
	  /* No STE, process channel duplication. */
	  SC68mixer_dup_L_to_R(buffer, buffer, buflen, 0);
	}
      }

      /* Advance time */
      calc_current_ms(api);
      ++api->mix.pass_cnt;
      api->mix.sample_cnt += buflen;

      /* Reach end of track */
      if (api->mix.pass_total && api->mix.pass_cnt >= api->mix.pass_total) {
	int next_track;
	api->mix.pass_cnt = 0;
	ret |= API68_LOOP;
	++api->mix.loop_cnt;
	if (api->mix.loop_total && api->mix.loop_cnt >= api->mix.loop_total) {
	  next_track = api->track+1;
	  api->track_to = (next_track > api->disk->nb_six) ? -1 : next_track;
	  api->seek_to = -1;
	}
      }
    }

    /* Copy to destination buffer. */
    if (buflen > 0) {
      u32 * b = (u32 *)buf16st;

      int len = buflen;
      int seek_to = api->seek_to;
      int stp = 0;

      /* Do speed up */
      if (seek_to != -1) {
	unsigned int ms = api->time.elapsed_ms;
	if (ms >= (unsigned int)seek_to) {
	  api->seek_to = -1;
	} else {
	  const int minstp = 0x010;
	  const int maxstp = api->mix.max_stp;
	  int newstp;

	  if (maxstp < minstp) {
	    /* no max stp : just discard this buffer */
	    buflen = 0;
	    continue;
	  }

	  stp = api->mix.stp;  /* Current stp */
	  
	  /* Got number of ms to seek */
	  ms = seek_to - ms;
	  newstp = (ms >> 4);
	  if (stp < newstp) {
	    stp += 2;
	  } if (stp > newstp) {
	    stp -= 2;
	  }
	  if (stp < minstp) {
	    stp = minstp;
	  } else if (stp > maxstp) {
	    stp = maxstp;
	  }

	}
      }

      api->mix.stp = stp;
      stp += 0x100;

      if (stp == 0x100) {
	/* Speed copy */
	if (len > n) {
	  len = n;
	}
	n -= len;
	buflen -= len;
	do {
	  *b++ = *buffer++;
	} while (--len);
      } else {
	/* Slow man ! */
	int j = 0;
	len <<= 8;
	do {
	  *b++ = buffer[j>>8];
	  j += stp;
	} while (--n && j < len );
	j >>= 8;
	buffer += j;
	buflen -= j;
	if (buflen < 0) {
	  buflen = 0;
	}
      }
      buf16st = b;
    }
  }

  /* Fill buffer with null PCM */
  if (n > 0) {
    u32 * b = (u32 *)buf16st;

/*     api68_debug("Process finishing with %d 0\n", n); */

    do {
      *b++ = 0;
    } while (--n);
  }

  api->mix.buffer = buffer;
  api->mix.buflen = buflen;

/*   api68_debug("Leave Process : %x\n", ret); */

  return ret;
}

int api68_verify(istream_t * is)
{
  return SC68file_verify(is);
}

int api68_verify_file(const char * url)
{
  return SC68file_verify_file(url);
}

int api68_verify_mem(const void * buffer, int len)
{
  return SC68file_verify_mem(buffer,len);
}

int api68_is_our_file(const char * url, const char *exts, int * is_remote)
{
  return SC68file_is_our_file(url,exts,is_remote);
}

static int load_disk(api68_t * api, disk68_t * d)
{
  int track;
  if (!api || !d) {
    goto error;
  }

  if (api->disk) {
    SC68error_add("disk is already loaded");
    goto error;
  }

  api->disk = d;
  api->track = 0;
  api->mus = 0;

  track = api->track_here;
  if (track > d->nb_six) {
    track = d->default_six;
  }

  return api68_play(api, track, -1);

 error:
  SC68free(d);
  return -1;
}

int api68_load(api68_t * api, istream_t * is)
{
  return load_disk(api, SC68file_load(is));
}

int api68_load_file(api68_t * api, const char * url)
{
  return load_disk(api, SC68file_load_file(url));
}

int api68_load_mem(api68_t * api, const void * buffer, int len)
{
  return load_disk(api,SC68file_load_mem(buffer, len));
}


api68_disk_t api68_load_disk(istream_t * is)
{
  return (api68_disk_t) SC68file_load(is);
}

api68_disk_t api68_load_disk_file(const char * url)
{
  return (api68_disk_t) SC68file_load_file(url);
}

api68_disk_t api68_disk_load_mem(const void * buffer, int len)
{
  return (api68_disk_t) SC68file_load_mem(buffer, len);
}


int api68_open(api68_t * api, api68_disk_t disk)
{
  if (!disk) {
    api68_close(api);
    return -1; /* Not an error but notifiy no disk has been loaded */
  }
  if (!api) {
    return -1;
  }
  return load_disk(api, disk);
}

void api68_close(api68_t * api)
{
  if (!api || !api->disk) {
    return;
  }

  api68_debug("api68_close() : enter\n");

  SC68free(api->disk);
  api->disk = 0;
  api->mus = 0;
  api->track = 0;
  api->track_to = 0;
  api->seek_to = -1;

  api68_debug("api68_close() : close\n");
}

int api68_tracks(api68_t * api)
{
  return (api && api->disk) ? api->disk->nb_six : -1;
}

int api68_play(api68_t * api, int track, int loop)
{
  disk68_t * d;
  api68_debug("api68_play(%d) : enter\n", track);

  if (!api) {
    return -1;
  }
  d = api->disk;
  if (!d) {
    return -1;
  }

  /* -1 : read current track ot loop. */
  if (track == -1) {
    return loop 
      ? api->mix.loop_cnt
      : api->track;
  }
  /* 0 : set disk default. */
  if (track == 0) {
    track = d->default_six + 1;
  }
  /* Check track range. */
  if (track <= 0 || track > d->nb_six) {
    return SC68error_add("track [%d] out of range [%d]", track, d->nb_six);
  }

  /* Set change track. Real track loading occurs during process thread to
     avoid multi-threading bug. */
  api->track_to = track;
  api->seek_to = -1;
  api->loop_to = loop;

  return 0;
}



int api68_stop(api68_t * api)
{
  if (!api || !api->disk) {
    return -1;
  }
  api->track_to = -1;
  api->seek_to = -1;
  return 0;
}

/** $$$ loop stuff is broken if loop differs some tracks */
int api68_seek(api68_t * api, int time_ms, int * is_seeking) {
	disk68_t * d;
	
	if (!api || (d=api->disk, !d)) {
		return -1;
	}
	
	if (time_ms == -1) {
		/* Read current */
		if (is_seeking) {
			*is_seeking = api->seek_to != -1;
		}
		if (!api->mus) {
			return -1;
		} else {
			//int loop = calc_loop_total(api->force_loop, api->loop_to, api->mus->loop);
			return /*(api->mus->start_ms * loop) +*/ api->time.elapsed_ms;
		}
	} else {
		unsigned int ms = (unsigned int) time_ms;
			
		if (ms > api->time.elapsed_ms) {
			/* real seek forward */
			if (is_seeking) *is_seeking = 1;
			api->seek_to = ms;
		} else {
			if (is_seeking) *is_seeking = 1;
			api->track_to = api->track;
			apply_change_track(api);
			
			api->seek_to = ms;//-1
		}
		return (int) api->time.elapsed_ms;
	}
}

int api68_music_info(api68_t * api, api68_music_info_t * info, int track,
		     api68_disk_t disk)
{
  disk68_t * d;
  music68_t * m = 0;
  int hw;
  hwflags68_t hwf;
  int loop, force_loop, loop_to;

  static const char * hwtable[8] = {
    "none",
    "Yamaha-2149",
    "MicroWire (STE)",
    "Yamaha-2149 & MicroWire (STE)",
    "Amiga/Paula",
    "Yamaha-2149  & Amiga/Paula",
    "MicroWire (STE) & Amiga/Paula",
    "Yamaha-2149 & MicroWire (STE) & Amiga/Paula",
  };

  if ((!api && !disk) || !info) {
    return -1;
  }

  d = disk ? disk : api->disk;
  if (!d) {
    return -1;
  }

  /* -1 : use current track or default track (depending if disk was given) */
  if (track == -1) {
    track = disk ? d->default_six+1 : api->track;
  }

  if (track < 0 || track > d->nb_six) {
    return SC68error_add("track [%d] out of range [%d]", track, d->nb_six);
  }

  m = d->mus + ((!track) ? d->default_six : (track - 1));
  info->track = track;
  info->tracks = d->nb_six;
  force_loop = api?api->force_loop:-1;
  loop_to = api?api->loop_to:-1;
  loop = calc_loop_total(force_loop, loop_to, m->loop);

  if (!track) {
    /* disk info */
    info->title = d->name;
    info->replay = 0;
    info->time_ms = calc_disk_time(api,d);
    info->start_ms = 0;
    hwf.all = d->hwflags.all;
    track = info->tracks;
  } else {
    /* track info */
    info->title = m->name;
    info->replay = m->replay;
    info->time_ms = (api && m == api->mus) ? api->time.length_ms : m->time_ms;
    info->start_ms = m->start_ms;
    hwf.all = m->hwflags.all;
  }
  loop = calc_loop_total(force_loop,
			 (api && m==api->mus) ? loop_to : 1,
			 m->loop);
  info->time_ms  *= loop;
  info->start_ms *= loop;

  /* api68_debug("mus:%p %p, loop:%d , time:%d\n",m,api->mus,loop,info->time_ms); */

  /* If there is a track remapping always use it. */
  if (m->track) {
    track = info->track = m->track;
  }

  info->author = m->author;
  info->composer = m->composer;
  info->converter = m->converter;
  info->ripper = m->ripper;
  if (!info->replay) {
    info->replay = "built-in";
  }

  hw = 7 &
    (  (hwf.bit.ym ? SC68_YM : 0)
     | (hwf.bit.ste ? SC68_STE : 0)
     | (hwf.bit.amiga ? SC68_AMIGA : 0)
       )
;

  info->hw.ym = hwf.bit.ym;
  info->hw.ste = hwf.bit.ste;
  info->hw.amiga = hwf.bit.amiga;
  info->rate = m->frq;
  info->addr = m->a0;
  info->hwname = hwtable[hw];
  SC68time_str(info->time, track, (info->time_ms+999u)/1000u);

  return 0;
}

const char * api68_error(void)
{
  return SC68error_get();
}

void * api68_alloc(unsigned int n)
{
  return SC68alloc(n);
}

void api68_free(void * data)
{
  SC68free(data);
}

void api68_debug(const char * fmt, ...)
{
  va_list list;
  va_start(list,fmt);
  vdebugmsg68(fmt,list);
  va_end(list);
}
