/*
 *                         sc68 - dynamic memory
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

#include "string.h"
#include "alloc68.h"
#include "error68.h"

static sc68_alloc_t sc68_alloc;
static sc68_free_t sc68_free;

/** Allocate dynamic memory. */
void * SC68alloc(unsigned int n)
{
  void * addr;

  if (!sc68_alloc) {
    SC68error_add("dynamic memory allocation handler not set.");
    addr = 0;
  } else {
    addr = sc68_alloc(n);
      memset(addr,0,n);
    if (!addr) {
      SC68error_add("dynamic memory allocation error.");
    }
  }
  return addr;
}

/** Allocate and clean buffer ($$$ OPTIMIZE ME) */
void * SC68calloc(unsigned int n)
{
  void * buffer = SC68alloc(n);
  if (buffer) {
    char * b = buffer;
    while (n--) {
      *b++ = 0;
    }
  }
  return buffer;
}

/** Free dynamic memory. */
void SC68free(void * data)
{
  if (!data) {
    return;
  }
  if (!sc68_free) {
    SC68error_add("dynamic memory free handler not set.");
  } else {
    sc68_free(data);
  }
}

/** Set/get dynamic memory allocation handler. */
sc68_alloc_t SC68set_alloc(sc68_alloc_t alloc)
{
  sc68_alloc_t old = sc68_alloc;

  if (alloc) {
    sc68_alloc = alloc;
  }
  return old;
}

/** Set/get dynamic memory free handler. */
sc68_free_t SC68set_free(sc68_free_t free)
{
  sc68_free_t old = sc68_free;

  if (free) {
    sc68_free = free;
  }
  return old;
}
