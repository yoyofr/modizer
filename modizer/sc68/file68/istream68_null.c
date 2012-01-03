/*
 *                         sc68 - Null stream
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



/* define this if you don't want NULL support. */
#ifndef ISTREAM_NO_NULL

#include <string.h>

#include "istream68_null.h"
#include "istream68_def.h"
#include "alloc68.h"

/** istream file structure. */
typedef struct {
  istream_t istream; /**< istream function.           */
  int size;          /**< max pos R or W.             */
  int pos;           /**< current position.           */
  int open;          /**< has been opened.            */
  char name[1];      /**< filename (null://filename). */
} istream_null_t;

static const char * ism_name(istream_t * istream)
{
  istream_null_t * ism = (istream_null_t *)istream;

  return (!ism || !ism->name[0])
    ? 0
    : ism->name;
}

static int ism_open(istream_t * istream)
{
  istream_null_t * ism = (istream_null_t *)istream;

  if (!ism || ism->open) {
    return -1;
  }
  ism->open = 1;
  ism->pos = 0;
  ism->size = 0;
  return 0;
}

static int ism_close(istream_t * istream)
{
  istream_null_t * ism = (istream_null_t *)istream;

  if (!ism || !ism->open) {
    return -1;
  }
  ism->open = 0;
  return 0;
}

static int ism_read_or_write(istream_t * istream, int n)
{
  istream_null_t * ism = (istream_null_t *)istream;

  if (!ism || !ism->open || n < 0) {
    return -1;
  }
  if (n) {
    /* No op: do not update size */
    ism->pos += n;
    if (ism->pos > ism->size) {
      ism->size = ism->pos;
    }
  }
  return n;
}

static int ism_read(istream_t * istream, void * data, int n)
{
  return ism_read_or_write(istream, n);
}

static int ism_write(istream_t * istream, const void * data, int n)
{
  return ism_read_or_write(istream, n);
}

static int ism_length(istream_t * istream)
{
  istream_null_t * ism = (istream_null_t *)istream;

  return (ism) ? ism->size : -1;
}

static int ism_tell(istream_t * istream)
{
  istream_null_t * ism = (istream_null_t *)istream;

  return (!ism || !ism->open)
    ? -1 
    : ism->pos;
}

static int ism_seek(istream_t * istream, int offset)
{
  istream_null_t * ism = (istream_null_t *)istream;

  if (ism) {
    offset += ism->pos;
    if (offset >= 0) {
      ism->pos = offset;
      return 0;
    }
  }
  return -1;
}

static void ism_destroy(istream_t * istream)
{
  SC68free(istream);
}

static const istream_t istream_null = {
  ism_name,
  ism_open, ism_close,
  ism_read, ism_write,
  ism_length, ism_tell, ism_seek, ism_seek,
  ism_destroy
};

istream_t * istream_null_create(const char * name)
{
  istream_null_t *ism;
  int size;
  static const char hd[] = "null://";

  if (!name) {
    name = "default";
  }

  size = sizeof(istream_null_t) + sizeof(hd)-1 + strlen(name);

  ism = SC68alloc(size);
  if (!ism) {
    return 0;
  }

  ism->istream = istream_null;
  ism->size = 0;
  ism->pos = 0;
  ism->open = 0;
  strcpy(ism->name,hd);
  strcat(ism->name,name);

  return &ism->istream;
}

#else /* #ifndef ISTREAM_NO_FILE */

/* istream mem must not be include in this package. Anyway the creation
 * still exist but it always returns error.
 */

#include "istream68_null.h"

istream_t * istream_null_create(const char * name)
{
  return 0;
}

#endif
