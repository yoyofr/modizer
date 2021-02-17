/* Decomposition of Unicode strings.
   Copyright (C) 2009-2020 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2009.

   This program is free software: you can redistribute it and/or
   modify it under the terms of either:

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at your
       option) any later version.

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at your
       option) any later version.

   or both in parallel, as here.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <stddef.h>

#include "unitypes.h"

/* Variant of uc_decomposition that does not produce the 'tag'.  */
extern int
       uc_compat_decomposition (ucs4_t uc, ucs4_t *decomposition);

/* A Unicode character together with its canonical combining class.  */
struct ucs4_with_ccc
{
  ucs4_t code;
  int ccc;      /* range 0..255 */
};

/* Stable-sort an array of 'struct ucs4_with_ccc'.  */
extern void
       gl_uninorm_decompose_merge_sort_inplace (struct ucs4_with_ccc *src, size_t n,
                                                struct ucs4_with_ccc *tmp);
