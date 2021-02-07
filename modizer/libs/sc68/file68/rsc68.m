/*
 *                         sc68 - Resource access
 *         Copyright (C) 2001-2003 Benjamin Gerard <ben@sashipa.com>
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



#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "error68.h"
#include "rsc68.h"
#include "url68.h"
#include "string68.h"
#include "alloc68.h"
#include "istream68_file.h"
#include "debugmsg68.h"
#import <Foundation/Foundation.h>

static const char * share_path  = 0; /* Shared resource path. */
static const char * user_path   = 0; /* User resource path.   */
static const char * lmusic_path = 0; /* Local music path.     */
static const char * rmusic_path = 0; /* Remote music path.    */

static istream_t * default_open(rsc68_t type, const char *name,
				int mode, rsc68_info_t * info);

static rsc68_handler_t rsc68 = default_open;

#ifndef SC68_SHARED_DATA_PATH
# define SC68_SHARED_DATA_PATH 0
#endif

#ifndef PACKAGE68_URL
# define SC68_RMUSIC_PATH 0
#else
# define SC68_RMUSIC_PATH PACKAGE68_URL "/Download/Music"
#endif

static struct {
  rsc68_t type;
  const char * name;
  const char * path;
  const char * ext;
} rsc68_table[rsc68_last];

static void rsc68_init_table(void)
{
  static int init = 0;
  if (init) {
    return;
  }
  init = 1;
  memset(rsc68_table, 0, sizeof(rsc68_table));

  rsc68_table[rsc68_replay].type = rsc68_replay	;
  rsc68_table[rsc68_replay].name = "replay";
  rsc68_table[rsc68_replay].path = "/Replay/";
  rsc68_table[rsc68_replay].ext  = ".bin";

  rsc68_table[rsc68_config].type = rsc68_config;
  rsc68_table[rsc68_config].name = "config";
  rsc68_table[rsc68_config].path = "/";
  rsc68_table[rsc68_config].ext  = ".txt";

  rsc68_table[rsc68_sample].type = rsc68_sample;
  rsc68_table[rsc68_sample].name = "sample";
  rsc68_table[rsc68_sample].path = "/";//"/Sample/";
  rsc68_table[rsc68_sample].ext  = ".sc68";


  rsc68_table[rsc68_dll].type = rsc68_dll;
  rsc68_table[rsc68_dll].name = "dll";
  rsc68_table[rsc68_dll].path = "/Dll/";
  rsc68_table[rsc68_dll].ext  = 0;

  rsc68_table[rsc68_music].type = rsc68_music;
  rsc68_table[rsc68_music].name = "music";
  rsc68_table[rsc68_music].path = "/Music/";
  rsc68_table[rsc68_music].ext  = ".sc68";
}

static const char *default_share_path(void)
{
	return [[[NSBundle mainBundle] bundlePath] cStringUsingEncoding:NSUTF8StringEncoding];
}

static const char *default_rmusic_path(void)
{
  static const char * def = SC68_RMUSIC_PATH;
  return rmusic_path
    ? rmusic_path
    : (def && def[0] ? def : 0);
}

static const char * rsc_set_any(const char ** any, const char * path)
{
  SC68free((char *)*any);
  return *any = SC68strdup(path) ;
}

const char * rsc68_set_share(const char *path)
{
  return rsc_set_any(&share_path, path);
}

const char * rsc68_set_user(const char *path)
{
  return rsc_set_any(&user_path, path);
}

const char * rsc68_set_music(const char *path)
{
  return rsc_set_any(&lmusic_path, path);
}

const char * rsc68_set_remote_music(const char *path)
{
  return rsc_set_any(&rmusic_path, path);
}

void rsc68_get_path(const char **share,
		    const char **user,
		    const char **local_music,
		    const char **remote_music)
{
  if (share) *share = share_path;
  if (user) *user = user_path;
  if (local_music) *local_music = lmusic_path;
  if (remote_music) *remote_music = rmusic_path;
}

/* Returns char or -1 to skip it */
typedef int (*char_cv_t)(int);

/* Runs the convert chain */
static int convert_chain(int c, ...)
{
  va_list list;
  char_cv_t cv;

  va_start(list,c);
  while (cv = va_arg(list,char_cv_t), cv) {
    c = cv(c);
  }
  va_end(list);
  return c;
}

/* No conversion */
static int cv_none(int c) {
  return c;
}

/* Convert upper to lower */
static int cv_lower(int c) {
  return (c>='A' && c <= 'Z')
    ? c - 'A' + 'a'
    : c;
}

static int cv_from_tables(int c, const char * acc, const char * con)
{
  const char * s;
  if (s=strchr(acc,c), s) {
    c = con[s-acc];
  }
  return c;
}


/* Convert accented */
static int cv_accent(int c)
{
  return cv_from_tables(c,
			"áâàãäëéêèïíîìöõóôòüúûùüç",
			"aaaaaeeeeiiiiooooouuuuuc");
}

/* Convert to minus */
static int cv_minus(int c)
{
  if (strchr("\\/&$",c)) c = '-';
  return c;
}

/* Convert to "skip" */
static int cv_skip(int c)
{
  if (strchr(":?<>",c)) c = -1;
  return c;
}

/* Convert to "skip" */
static int cv_skip2(int c)
{
  if (strchr("<>",c)) c = -1;
  return c;
}

/* Convert for filename in local filesystem */
static int cv_name_local(int c)
{
  c = cv_accent(cv_minus(cv_skip2(c)));
  return c;
}

static int cv_path_local(int c)
{
  if (c == '\\') c = '/';
  if (c != '/') {
    c = cv_name_local(c);
  }
  return c;
}

static int cv_name_remote(int c)
{
  c = cv_accent(cv_minus(cv_skip2(c)));
  if (c == ' ') c = '_';
  else if (c == '#') c = '0';
  return c;
}

static int cv_path_remote(int c)
{
  if (c == '\\') c = '/';
  if (c != '/') {
    c = cv_name_remote(c);
  }
  return c;
}


static int copy_path(char *d, int max,
		     const char * s,
		     char_cv_t cv1,
		     char_cv_t cv2,
		     int brk)
{
  int i;

  for (i=0; i<max; ) {
    int c = (*s++) & 255, c2;
    if (!c) break;
    c2 = convert_chain(c, cv1, cv2, 0);
    if (c2 != -1) {
      d[i++] = c2;
    }
    if (c == brk) break;
  }
  if (i < max) {
    d[i] = 0;
  } else {
    i = -1;
  }
  return i;
}

const char * rsc68_get_music_params(rsc68_info_t *info, const char *name)
{
  if (info) {
    info->type = rsc68_last;
  }
  if (name && name[0] == ':') {
    int tinfo[3] = { -1,-1,-1 };
    int i, c;

    for (i=0, c=(*name)&255; i<3 && c == ':'; ++i) {
      c = (*++name) & 255;
      if (isdigit(c)) {
	int v = 0;
	do {
	  v = v*10 + c-'0';
	  c = (*++name) & 255;
	} while (isdigit(c));
	tinfo[i] = v;
      }
    }
    while (c && c != '/') {
      c = *++name;
    }

    if (info) {
      info->type = rsc68_music;
      info->data.music.track = tinfo[0];
      info->data.music.loop  = tinfo[1];
      info->data.music.time  = tinfo[2];
    }
  }

  return name;
}



/* author/hw/title[/:track:loop:time] */
static char * convert_music_path(char * newname, int max,
				 const char *name,
				 rsc68_info_t * info)
{
  int len, c;
  char * nname = newname;
  char * ename = nname+max;

  /* Author */
  len = copy_path(nname, ename - nname, name, 0, 0, '/');
  if (len <= 0) goto error;
  nname += len;
  name  += len;

  /* Hardware */
  c = (*name++) & 255;

  if (c == '0') {
    len = copy_path(nname, ename - nname, "Atari ST/", 0, 0, 0);
  } else if (c == '1') {
    len = copy_path(nname, ename - nname, "Amiga/", 0, 0, 0);
  } else {
    len = -1;
  }
  if (len <= 0) goto error;
  nname += len;
  c = (*name++) & 255;
  if (c != '/') goto error;

  /* Title */
  len = copy_path(nname, ename - nname, name, 0, 0, '/');
  if (len <= 0) goto error;
  nname += len;
  name  += len;

  if (nname[-1] == '/') {
    --nname;
  }
  *nname = 0;

  /* Optional track # */
  name = rsc68_get_music_params(info, name);

  return newname;

 error:
  *newname = 0;
  return 0;
}

static istream_t * default_open(rsc68_t type, const char *name,
				int mode, rsc68_info_t * info)
{
  istream_t * is = 0;
  int err = -1;
  const char *subdir = 0, *ext = 0;
  char tmp[1024], * apath = 0;
  char tmpname[512];
  int alen = 0;
/*   void * (*mycpy)(void *, const void *, size_t) = memcpy; */

  char_cv_t cv_path=0, cv_extra=0;

  struct {
    const char * path, * sdir, * ext;
    int curl;
  } pathes[4];
  int ipath, npath = 0;
  const char * share_path = default_share_path();
  const char * rmusic_path = default_rmusic_path();

  /* default to invalid type. */
  if (info) {
    info->type = rsc68_last;
  }

  if ( (int) type < 0 || (int)type >= rsc68_last) {
    return 0;
  }

  memset(pathes,0,sizeof(pathes));

  /* Build default pathes list */
  if (user_path) {
    pathes[npath++].path = user_path;
  }

  if (mode == 1 && share_path) {
    pathes[npath++].path = share_path;
  }

  subdir = rsc68_table[type].path;
  ext    = rsc68_table[type].ext;

  debugmsg68("rsc68: open %c [rsc68://%s/%s%s]\n",
		(mode==1)?'R':'W',rsc68_table[type].name, name, ext?ext:"");

  /* Any specific stuff. */
  switch (type) {
  case rsc68_replay:
    cv_extra = cv_lower; /* $$$ transform replay name to lower case. */
    break;

  case rsc68_music:
    if (lmusic_path) {
      pathes[npath].path = lmusic_path;
      pathes[npath].sdir = "/";
      ++npath;
    }
    if (mode == 1 && rmusic_path) {
      pathes[npath].path = rmusic_path;
      pathes[npath].sdir = "/";
      pathes[npath].curl = 1;
      ++npath;
    }
    name = convert_music_path(tmpname, sizeof(tmpname), name, info);
    break;

  default:
    break;
  }

  for (ipath=0; name && ipath < npath; ++ipath) {
    const char *cpath, * cdir, * cext;
    char *p, *pe, *path;
    int len, l;

    cpath = pathes[ipath].path;
    cdir  = pathes[ipath].sdir ? pathes[ipath].sdir : subdir;
    cext  = pathes[ipath].ext ? pathes[ipath].ext : ext;

    len = 1
      + strlen(cpath)
      + strlen(cdir)
      + strlen(name)
      + (cext ? strlen(cext) : 0);

    debugmsg68("default_open() : trying '%s'\n", cpath);

    if (len <= alen) {
      path = apath;
    } else if (len  <= sizeof(tmp)) {
      path = tmp;
    } else {
      SC68free(apath);
      apath = SC68alloc(len);
      alen = apath ? len : 0;
      path = apath;
    }

    if (!path) {
      continue;
    }

    p = path;
    pe = path + len;

    cv_path = pathes[ipath].curl
      ? cv_path_remote
      : cv_path_local;

    /* Build path. */
    l = copy_path(p, pe-p, cpath, cv_path, 0 , 0);
    p += l;
    l = copy_path(p, pe-p, cdir, cv_path, 0, 0);
    p += l;
    l = copy_path(p, pe-p, name, cv_path, cv_extra, 0);
    p += l;
    if (cext) {
      l = copy_path(p, pe-p, cext, 0, 0 ,0);
      p += l;
    }
    debugmsg68("default_open() : path '%s'\n", path);

    if (pathes[ipath].curl) {
			return 0;
    } else {
      is = istream_file_create(path, mode);
    }
    err = istream_open(is);
    if (!err) {
      debugmsg68("default_open() : ok for '%s'\n", path);
      break;
    } else {
      debugmsg68("default_open() : failed '%s'\n", path);
      istream_destroy(is);
      is = 0;
    }
  }

  if (apath != tmp) {
    SC68free(apath);
  }
  if (err) {
    istream_destroy(is);
    is = 0;
  }

  if (is && info) {
    info->type = type;
  }
  return is;
}


rsc68_handler_t rsc68_set_handler(rsc68_handler_t fct)
{
  rsc68_handler_t old;

  old = rsc68;
  if (fct) {
    rsc68 = fct;
  }
  return old;
}

istream_t * rsc68_open_url(const char *url, int mode, rsc68_info_t * info)
{
  int i;
  char protocol[16];

  if (info) {
    info->type = rsc68_last;
  }
  if (!rsc68 || mode < 1 || mode > 2) {
    return 0;
  }

  if (url68_get_protocol(protocol, sizeof(protocol), url)) {
    return 0;
  }
  if (SC68strcmp(protocol,"RSC68")) {
    return 0;
  }
  url += 5+3; /* Skip "RSC68://" */

  /* Get resource type. */
  for (i=0; i<sizeof(protocol)-1; ++i) {
    int c = *url++ & 255;
    if (!c) {
      return 0;
    }
    if (c == '/') {
      break;
    }
    protocol[i]=c;
  }
  protocol[i]=0;

  rsc68_init_table();
  for (i=0; i<rsc68_last && SC68strcmp(rsc68_table[i].name, protocol); ++i)
    ;
  return rsc68(i, url, mode, info);
}

istream_t * rsc68_open(rsc68_t type, const char *name, int mode,
			 rsc68_info_t * info)
{
  if (info) {
    info->type = rsc68_last;
  }
  if (!rsc68 || mode < 1 || mode > 2) {
    return 0;
  }
  rsc68_init_table();
  return rsc68(type, name, mode, info);
}

#ifdef __cplusplus
}
#endif
