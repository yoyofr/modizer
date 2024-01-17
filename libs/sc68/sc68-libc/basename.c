/*
 * @file    basename.c
 * @brief   replacement for missing or faulty basename()
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (c) 1998-2016 Benjamin Gerard
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_BASENAME
# error "compiling basename.c but already have a basename() function"
#endif

#include "libc68.h"

#ifndef PATH_SEP
# ifdef WIN32
#  define PATH_SEP { '/' , '\\' }
# else
#  define PATH_SEP { '/' }
# endif
#endif

/**
 * @warning  Sloppy replacement. In many cases this function won't
 *           behave as basename() should.
 */
char *basename(char *path)
{
  static const char sep[] = PATH_SEP;
  int i,c,j,k;

#ifdef WIN32
  c = path[0] | 0x20;
  if (c >= 'a' && c <= 'z' && path[1] == ':')
    path += 2;                          /* skip drive letter */
#endif
  for (i=j=0; c = path[i], c; ++i)
    for (k=0; k < sizeof(sep); ++k)
      if (c == sep[k]) {
        j = i+1;
        break;
      }
  return path + j;
}
