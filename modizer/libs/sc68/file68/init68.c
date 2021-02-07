/*
 *                         sc68 - "sc68" file functions
 *         Copyright (C) 2001 Benjamin Gerard <ben@sashipa.com>
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



#include <stdlib.h>
#include <string.h>

#include "init68.h"
#include "debugmsg68.h"
#include "error68.h"
#include "alloc68.h"
#include "registry68.h"
#include "rsc68.h"

static char * mygetenv(const char *name)
{
#ifndef HAVE_GETENV
  return 0;
#else
  char tmp[128];
  int i, c;

  for (i=0; i<sizeof(tmp)-1 && (c = *name++ & 255); ) {
    tmp[i++] = (c >= 'a' && c <= 'z') ? (c+'A'-'a') : c;
  }
  tmp[i] = 0;
  return getenv(tmp);
#endif
}

static const char * get_str_arg(const char *arg, const char *name)
{
  if (strstr(arg,name) == arg) {
    int l = strlen(name);
    if (arg[l] == '=') {
      return arg + l + 1;
    }
  }
  return 0;
}

/* Get path from registry, converts '\' to '/' and adds missing trailing '/'.
 *
 * @return pointer to the end of string
 * @retval 0 error
 */
static char * get_reg_path(registry68_key_t key, char * kname,
			   char * buffer, int buflen)
{
  int i = registry68_gets(key,kname,buffer,buflen);
  buffer[buflen-1] = 0;
  if (i < 0) {
    buffer[0] = 0;
    return 0;
  } else {
    char *e;
    for (e=buffer; *e; ++e) {
      if (*e == '\\') *e = '/';
    }
    if (e > buffer && e[-1] != '/') {
      *e++ = '/';
      *e=0;
    }
    return e;
  }
}

int file68_init(char **argv, int argc)
{
  int i, j;
  char tmp[1024];

  struct {
    const char * name;
    const char * value;
    const char * org;
  } vars[] = {
    { "sc68_data"  , 0, 0 },
    { "sc68_home"  , 0, 0 },
    { "sc68_music" , 0, 0 },
    { "sc68_rmusic", 0, 0 }
  };
  const int nvar = sizeof(vars) / sizeof(*vars);

  /* Get current pathes */
  rsc68_get_path(&vars[0].org, &vars[1].org, &vars[2].org, &vars[3].org);

  /* Parse arguments */
  for (i=1; i<argc; ++i) {
    const char * s;

    /* Check if arg */
    if (argv[i][0] != '-' || argv[i][1] != '-') {
      continue;
    }
    if (!strcmp(argv[i],"--")) {
      break;
    }

    /* Scan this arg */
    for (s=0, j=0; !s && j<nvar; ++j) {
      s = get_str_arg(argv[i]+2, vars[j].name);
      if (s && !vars[j].org) {
	vars[j].value = s;
	break;
      }
    }

    if (s) {
      /* Shift argv */
      for (j=i, --argc; j<argc; ++j) {
	argv[j] = argv[j+1];
      }
      --i;
    }
  }

  /* Get enviromment */
  for (i=0; i<nvar; ++i) {
    if (!vars[i].org && !vars[i].value) {
      vars[i].value = mygetenv(vars[i].name);
    }
  }
  
  /* Get share path from registry */
  if (!vars[0].org && !vars[0].value) {
    char * e;
    const char path[] = "Resources";
    e = get_reg_path(registry68_LMK,
		     "SOFTWARE/sashipa/sc68/Install_Dir",
		     tmp, sizeof(tmp));
    if (e && (e+sizeof(path) < tmp+sizeof(tmp))) {
      memcpy(e, path, sizeof(path));
      vars[0].value = tmp;
    }
  }

/*   if (!vars[0].value && default_share_path && default_share_path[0]) { */
/*     vars[0].value = default_share_path; */
/*   } */

  /* Setup new shared path */
  if (vars[0].value) {
    rsc68_set_share(vars[0].value);
  }

  /* Get user path from registry */
  if (!vars[1].org && !vars[1].value) {
    char * e;
    const char path[] = "sc68";
    e = get_reg_path(registry68_CUK,
		     "Volatile Environment/APPDATA",
		     tmp, sizeof(tmp));
    if (e && (e+sizeof(path) < tmp+sizeof(tmp))) {
      memcpy(e, path, sizeof(path));
      vars[1].value = tmp;
    }
  }

  /* Get user path from HOME */
  if (!vars[1].org && !vars[1].value) {
    const char path[] = "/.sc68";
    vars[1].value = mygetenv("HOME");
    if(vars[1].value && strlen(vars[1].value)+sizeof(path) < sizeof(tmp)) {
      strcpy(tmp,vars[1].value);
      strcat(tmp,path);
      vars[1].value = tmp;
    }
  }

  /* Setup new user path */
  if (vars[1].value) {
    rsc68_set_user(vars[1].value);
  }

  /* Setup new music path */
  if (vars[2].value) {
    rsc68_set_music(vars[2].value);
  }

  /* Setup new remote path */
  if (vars[3].value) {
    rsc68_set_remote_music(vars[3].value);
  }

  return argc;
}

void file68_shutdown(void)
{
  rsc68_set_user(0);
  rsc68_set_share(0);
  rsc68_set_music(0);
  rsc68_set_remote_music(0);
}
