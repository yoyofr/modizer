/*
 *                         sc68 - "sc68" url functions
 *       Copyright (C) 2001-2003 Benjamin Gerard <ben@sashipa.com>
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
#include <ctype.h>

#include "string68.h"
#include "url68.h"
#include "debugmsg68.h"

#include "istream68_null.h"
#include "istream68_file.h"
#include "istream68_fd.h"

int url68_get_protocol(char * protocol, int max, const char *url)
{
  int i, c;

  if (!url || !protocol || max < 4) {
    return -1;
  }

  *protocol = 0;
  for (c=i=0; i<max && (c=url[i], isalnum(c)); ++i)
    ;
  if (i < 2 || i >= max
      || c != ':' || url[i+1] != '/' || url[i+2] != '/') {
    return -1;
  }
  memcpy(protocol, url, i);
  protocol[i] = 0;
  return 0;
}

int url68_local_protocol(const char * protocol)
{
  int i;

  static const char * local_proto[] = {
    "", "FILE","LOCAL","NULL"
    /* , "STDIN", "STDOUT" remove this (not seekable) */
  };
  const int n_proto = sizeof(local_proto)/sizeof(*local_proto);

  i = 0;
  if (protocol) {
    for (; i<n_proto && SC68strcmp(protocol, local_proto[i]); ++i)
      ;
  }
  return i < n_proto;
}

istream_t * url68_stream_create(const char * url, int mode)
{
  char protocol[16];
  istream_t * isf = 0;
  int use_curl = 0;

  debugmsg68("Create URL stream [%s] %C\n",url,mode==1?'R':'W');

  if (!url68_get_protocol(protocol, sizeof(protocol), url)) {
    if (!SC68strcmp(protocol, "RSC68")) {
      return 0;
    } else if (!SC68strcmp(protocol, "SC68")) {
      return 0;
    } else if (!SC68strcmp(protocol, "FILE")) {
      url += 4+3;
    } else if (!SC68strcmp(protocol, "LOCAL")) {
      url += 5+3;
    } else if (!SC68strcmp(protocol, "NULL")) {
      return istream_null_create(url);
    } else if (!SC68strcmp(protocol, "STDIN")) {
      if (mode != 1) return 0;
      isf = istream_fd_create("stdin://",0,1);
      url = "/dev/stdin"; /* fallback */
    } else if (!SC68strcmp(protocol, "STDOUT")) {
      if (mode != 2) return 0;
      isf = istream_fd_create("stdout://",1,2);
      url = "/dev/stdout"; /* fallback */
    } else if (!SC68strcmp(protocol, "STDERR")) {
      if (mode != 2) return 0;
      isf = istream_fd_create("stderr://",2,2);
      url = "/dev/stderr"; /* fallback */
    } else {
      use_curl = 1;
    }
  }

  if (use_curl) {
    return 0;
  }

  /* Default open as FILE */
  if (!isf) {
    isf = istream_file_create(url,mode);
  }
  /* Fallback to file descriptor */
  if (!isf) {
    isf = istream_fd_create(url,-1,mode);
  }
  
  return isf;
}
