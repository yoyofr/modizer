/* Test of ioctl() function.
   Copyright (C) 2011-2020 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include <sys/ioctl.h>

#include "signature.h"
SIGNATURE_CHECK (ioctl, int, (int, int, ...));

#include <errno.h>
#include <unistd.h>

#include "macros.h"

int
main (void)
{
#ifdef FIONREAD
  /* Test behaviour for invalid file descriptors.  */
  {
    int value;
    errno = 0;
    ASSERT (ioctl (-1, FIONREAD, &value) == -1);
    ASSERT (errno == EBADF);
  }
  {
    int value;
    close (99);
    errno = 0;
    ASSERT (ioctl (99, FIONREAD, &value) == -1);
    ASSERT (errno == EBADF);
  }
#endif

  return 0;
}
