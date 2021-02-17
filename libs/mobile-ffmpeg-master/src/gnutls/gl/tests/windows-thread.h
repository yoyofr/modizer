/* Creating and controlling threads (native Windows implementation).
   Copyright (C) 2005-2020 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2005.
   Based on GCC's gthr-win32.h.  */

#ifndef _WINDOWS_THREAD_H
#define _WINDOWS_THREAD_H

#define WIN32_LEAN_AND_MEAN  /* avoid including junk */
#include <windows.h>

/* The glwthread_thread_t is a pointer to a structure in memory.
   Why not the thread handle?  If it were the thread handle, it would be hard
   to implement glwthread_thread_self() (since GetCurrentThread () returns a
   pseudo-handle, DuplicateHandle (GetCurrentThread ()) returns a handle that
   must be closed afterwards, and there is no function for quickly retrieving
   a thread handle from its id).
   Why not the thread id?  I tried it.  It did not work: Sometimes ids appeared
   that did not belong to running threads, and glthread_join failed with ESRCH.
 */
typedef struct glwthread_thread_struct *glwthread_thread_t;

#ifdef __cplusplus
extern "C" {
#endif

/* attr is a bit mask, consisting of the following bits: */
#define GLWTHREAD_ATTR_DETACHED 1
extern int glwthread_thread_create (glwthread_thread_t *threadp,
                                    unsigned int attr,
                                    void * (*func) (void *), void *arg);
extern int glwthread_thread_join (glwthread_thread_t thread, void **retvalp);
extern int glwthread_thread_detach (glwthread_thread_t thread);
extern glwthread_thread_t glwthread_thread_self (void);
extern int glwthread_thread_exit (void *retval);

#ifdef __cplusplus
}
#endif

#endif /* _WINDOWS_THREAD_H */
