/* Test of pthread_sigmask in a single-threaded program.
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

/* Written by Bruno Haible <bruno@clisp.org>, 2011.  */

#include <config.h>

#include <signal.h>

#include "signature.h"
SIGNATURE_CHECK (pthread_sigmask, int, (int, const sigset_t *, sigset_t *));

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "macros.h"

#if !(defined _WIN32 && !defined __CYGWIN__)

static volatile int sigint_occurred;

static void
sigint_handler (int sig)
{
  sigint_occurred++;
}

int
main (int argc, char *argv[])
{
  sigset_t set;
  int pid = getpid ();
  char command[80];

  signal (SIGINT, sigint_handler);

  sigemptyset (&set);
  sigaddset (&set, SIGINT);

  /* Check error handling.  */
  ASSERT (pthread_sigmask (1729, &set, NULL) == EINVAL);

  /* Block SIGINT.  */
  ASSERT (pthread_sigmask (SIG_BLOCK, &set, NULL) == 0);

  /* Request a SIGINT signal from outside.  */
  sprintf (command, "sh -c 'sleep 1; kill -%d %d' &", SIGINT, pid);
  ASSERT (system (command) == 0);

  /* Wait.  */
  sleep (2);

  /* The signal should not have arrived yet, because it is blocked.  */
  ASSERT (sigint_occurred == 0);

  /* Unblock SIGINT.  */
  ASSERT (pthread_sigmask (SIG_UNBLOCK, &set, NULL) == 0);

  /* The signal should have arrived now, because POSIX says
       "If there are any pending unblocked signals after the call to
        pthread_sigmask(), at least one of those signals shall be delivered
        before the call to pthread_sigmask() returns."  */
  ASSERT (sigint_occurred == 1);

  return 0;
}

#else

/* On native Windows, getpid() values and the arguments that are passed to
   the (Cygwin?) 'kill' program are not necessarily related.  */

int
main ()
{
  fputs ("Skipping test: native Windows platform\n", stderr);
  return 77;
}

#endif
