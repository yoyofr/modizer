/* Implement the most essential subset of POSIX 1003.1-2008 pthread.h.

   Copyright (C) 2009-2020 Free Software Foundation, Inc.

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

/* Written by Paul Eggert, Glen Lenker, and Bruno Haible.  */

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif
@PRAGMA_COLUMNS@

#if defined _GL_ALREADY_INCLUDING_PTHREAD_H
/* Special invocation convention:
   On Android, we have a sequence of nested includes
   <pthread.h> -> <time.h> -> <sys/time.h> -> <sys/select.h> ->
   <signal.h> -> <pthread.h>.
   In this situation, PTHREAD_COND_INITIALIZER is not yet defined,
   therefore we should not attempt to define PTHREAD_MUTEX_NORMAL etc.  */

#@INCLUDE_NEXT@ @NEXT_PTHREAD_H@

#else
/* Normal invocation convention.  */

#ifndef _@GUARD_PREFIX@_PTHREAD_H_

#if @HAVE_PTHREAD_H@

# define _GL_ALREADY_INCLUDING_PTHREAD_H

/* The include_next requires a split double-inclusion guard.  */
# @INCLUDE_NEXT@ @NEXT_PTHREAD_H@

# undef _GL_ALREADY_INCLUDING_PTHREAD_H

#endif

#ifndef _@GUARD_PREFIX@_PTHREAD_H_
#define _@GUARD_PREFIX@_PTHREAD_H_

#define __need_system_stdlib_h
#include <stdlib.h>
#undef __need_system_stdlib_h


/* The pthreads-win32 <pthread.h> defines a couple of broken macros.  */
#undef asctime_r
#undef ctime_r
#undef gmtime_r
#undef localtime_r
#undef rand_r
#undef strtok_r

#include <errno.h>
#include <sched.h>
#include <sys/types.h>
#include <time.h>

/* The definitions of _GL_FUNCDECL_RPL etc. are copied here.  */

/* The definition of _Noreturn is copied here.  */

/* The definition of _GL_ARG_NONNULL is copied here.  */

/* The definition of _GL_WARN_ON_USE is copied here.  */

/* =========== Thread types and macros =========== */

#if (defined _WIN32 && ! defined __CYGWIN__) && USE_WINDOWS_THREADS
# if @GNULIB_PTHREAD_THREAD@
#  include "windows-thread.h"
#  if @HAVE_PTHREAD_T@
#   define pthread_t rpl_pthread_t
#   define pthread_attr_t rpl_pthread_attr_t
#  endif
#  if !GNULIB_defined_pthread_thread_types
typedef glwthread_thread_t pthread_t;
typedef unsigned int pthread_attr_t;
#   define GNULIB_defined_pthread_thread_types 1
#  endif
# else
#  if @HAVE_PTHREAD_T@
#   define pthread_t rpl_pthread_t
#   define pthread_attr_t rpl_pthread_attr_t
#  endif
#  if !GNULIB_defined_pthread_thread_types
typedef int pthread_t;
typedef unsigned int pthread_attr_t;
#   define GNULIB_defined_pthread_thread_types 1
#  endif
# endif
# undef PTHREAD_CREATE_JOINABLE
# undef PTHREAD_CREATE_DETACHED
# define PTHREAD_CREATE_JOINABLE 0
# define PTHREAD_CREATE_DETACHED 1
#else
# if !@HAVE_PTHREAD_T@
#  if !GNULIB_defined_pthread_thread_types
typedef int pthread_t;
typedef unsigned int pthread_attr_t;
#   define GNULIB_defined_pthread_thread_types 1
#  endif
# endif
# if !@HAVE_PTHREAD_CREATE_DETACHED@
#  define PTHREAD_CREATE_JOINABLE 0
#  define PTHREAD_CREATE_DETACHED 1
# endif
#endif

/* =========== Once-only control (initialization) types and macros ========== */

#if (defined _WIN32 && ! defined __CYGWIN__) && USE_WINDOWS_THREADS
# if @GNULIB_PTHREAD_ONCE@
#  include "windows-once.h"
#  if @HAVE_PTHREAD_T@
#   define pthread_once_t rpl_pthread_once_t
#  endif
#  if !GNULIB_defined_pthread_once_types
typedef glwthread_once_t pthread_once_t;
#   define GNULIB_defined_pthread_once_types 1
#  endif
#  undef PTHREAD_ONCE_INIT
#  define PTHREAD_ONCE_INIT GLWTHREAD_ONCE_INIT
# else
#  if @HAVE_PTHREAD_T@
#   define pthread_once_t rpl_pthread_once_t
#  endif
#  if !GNULIB_defined_pthread_once_types
typedef int pthread_once_t;
#   define GNULIB_defined_pthread_once_types 1
#  endif
#  undef PTHREAD_ONCE_INIT
#  define PTHREAD_ONCE_INIT { 0 }
# endif
#else
# if !@HAVE_PTHREAD_T@
#  if !GNULIB_defined_pthread_once_types
typedef int pthread_once_t;
#   define GNULIB_defined_pthread_once_types 1
#  endif
#  undef PTHREAD_ONCE_INIT
#  define PTHREAD_ONCE_INIT { 0 }
# endif
#endif

/* =========== Mutex types and macros =========== */

#if (defined _WIN32 && ! defined __CYGWIN__) && USE_WINDOWS_THREADS
# if @GNULIB_PTHREAD_MUTEX@
#  include "windows-timedmutex.h"
#  include "windows-timedrecmutex.h"
#  if @HAVE_PTHREAD_T@
#   define pthread_mutex_t rpl_pthread_mutex_t
#   define pthread_mutexattr_t rpl_pthread_mutexattr_t
#  endif
#  if !GNULIB_defined_pthread_mutex_types
typedef struct
        {
          int type;
          union
            {
              glwthread_timedmutex_t    u_timedmutex;
              glwthread_timedrecmutex_t u_timedrecmutex;
            }
          u;
        }
        pthread_mutex_t;
typedef unsigned int pthread_mutexattr_t;
#   define GNULIB_defined_pthread_mutex_types 1
#  endif
#  undef PTHREAD_MUTEX_INITIALIZER
#  define PTHREAD_MUTEX_INITIALIZER { 1, { GLWTHREAD_TIMEDMUTEX_INIT } }
# else
#  if @HAVE_PTHREAD_T@
#   define pthread_mutex_t rpl_pthread_mutex_t
#   define pthread_mutexattr_t rpl_pthread_mutexattr_t
#  endif
#  if !GNULIB_defined_pthread_mutex_types
typedef int pthread_mutex_t;
typedef unsigned int pthread_mutexattr_t;
#   define GNULIB_defined_pthread_mutex_types 1
#  endif
#  undef PTHREAD_MUTEX_INITIALIZER
#  define PTHREAD_MUTEX_INITIALIZER { 0 }
# endif
# undef PTHREAD_MUTEX_DEFAULT
# undef PTHREAD_MUTEX_NORMAL
# undef PTHREAD_MUTEX_ERRORCHECK
# undef PTHREAD_MUTEX_RECURSIVE
# define PTHREAD_MUTEX_DEFAULT PTHREAD_MUTEX_NORMAL
# define PTHREAD_MUTEX_NORMAL 0
# define PTHREAD_MUTEX_ERRORCHECK 1
# define PTHREAD_MUTEX_RECURSIVE 2
# undef PTHREAD_MUTEX_STALLED
# undef PTHREAD_MUTEX_ROBUST
# define PTHREAD_MUTEX_STALLED 0
# define PTHREAD_MUTEX_ROBUST 1
#else
# if !@HAVE_PTHREAD_T@
#  if !GNULIB_defined_pthread_mutex_types
typedef int pthread_mutex_t;
typedef unsigned int pthread_mutexattr_t;
#   define GNULIB_defined_pthread_mutex_types 1
#  endif
#  undef PTHREAD_MUTEX_INITIALIZER
#  define PTHREAD_MUTEX_INITIALIZER { 0 }
# endif
# if !@HAVE_PTHREAD_MUTEX_RECURSIVE@
#  define PTHREAD_MUTEX_DEFAULT PTHREAD_MUTEX_NORMAL
#  define PTHREAD_MUTEX_NORMAL 0
#  define PTHREAD_MUTEX_ERRORCHECK 1
#  define PTHREAD_MUTEX_RECURSIVE 2
# endif
# if !@HAVE_PTHREAD_MUTEX_ROBUST@
#  define PTHREAD_MUTEX_STALLED 0
#  define PTHREAD_MUTEX_ROBUST 1
# endif
#endif

/* =========== Read-write lock types and macros =========== */

#if (defined _WIN32 && ! defined __CYGWIN__) && USE_WINDOWS_THREADS
# if @GNULIB_PTHREAD_RWLOCK@
#  include "windows-timedrwlock.h"
#  if @HAVE_PTHREAD_T@
#   define pthread_rwlock_t rpl_pthread_rwlock_t
#   define pthread_rwlockattr_t rpl_pthread_rwlockattr_t
#  endif
#  if !GNULIB_defined_pthread_rwlock_types
typedef glwthread_timedrwlock_t pthread_rwlock_t;
typedef unsigned int pthread_rwlockattr_t;
#   define GNULIB_defined_pthread_rwlock_types 1
#  endif
#  undef PTHREAD_RWLOCK_INITIALIZER
#  define PTHREAD_RWLOCK_INITIALIZER GLWTHREAD_TIMEDRWLOCK_INIT
# else
#  if @HAVE_PTHREAD_T@
#   define pthread_rwlock_t rpl_pthread_rwlock_t
#   define pthread_rwlockattr_t rpl_pthread_rwlockattr_t
#  endif
#  if !GNULIB_defined_pthread_rwlock_types
typedef int pthread_rwlock_t;
typedef unsigned int pthread_rwlockattr_t;
#   define GNULIB_defined_pthread_rwlock_types 1
#  endif
#  undef PTHREAD_RWLOCK_INITIALIZER
#  define PTHREAD_RWLOCK_INITIALIZER { 0 }
# endif
#elif @GNULIB_PTHREAD_RWLOCK@ && @REPLACE_PTHREAD_RWLOCK_INIT@ /* i.e. PTHREAD_RWLOCK_UNIMPLEMENTED */
# if @HAVE_PTHREAD_T@
#  define pthread_rwlock_t rpl_pthread_rwlock_t
#  define pthread_rwlockattr_t rpl_pthread_rwlockattr_t
# endif
# if !GNULIB_defined_pthread_rwlock_types
typedef struct
        {
          pthread_mutex_t lock; /* protects the remaining fields */
          pthread_cond_t waiting_readers; /* waiting readers */
          pthread_cond_t waiting_writers; /* waiting writers */
          unsigned int waiting_writers_count; /* number of waiting writers */
          int runcount; /* number of readers running, or -1 when a writer runs */
        }
        pthread_rwlock_t;
typedef unsigned int pthread_rwlockattr_t;
#  define GNULIB_defined_pthread_rwlock_types 1
# endif
# undef PTHREAD_RWLOCK_INITIALIZER
# define PTHREAD_RWLOCK_INITIALIZER \
   { PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 0 }
#else
# if @HAVE_PTHREAD_T@
#  if !defined PTHREAD_RWLOCK_INITIALIZER && defined PTHREAD_RWLOCK_INITIALIZER_NP /* z/OS */
#   define PTHREAD_RWLOCK_INITIALIZER PTHREAD_RWLOCK_INITIALIZER_NP
#  endif
# else
#  if !GNULIB_defined_pthread_rwlock_types
typedef int pthread_rwlock_t;
typedef unsigned int pthread_rwlockattr_t;
#   define GNULIB_defined_pthread_rwlock_types 1
#  endif
#  undef PTHREAD_RWLOCK_INITIALIZER
#  define PTHREAD_RWLOCK_INITIALIZER { 0 }
# endif
#endif

/* =========== Condition variable types and macros =========== */

#if (defined _WIN32 && ! defined __CYGWIN__) && USE_WINDOWS_THREADS
# if @GNULIB_PTHREAD_COND@
#  include "windows-cond.h"
#  if @HAVE_PTHREAD_T@
#   define pthread_cond_t rpl_pthread_cond_t
#   define pthread_condattr_t rpl_pthread_condattr_t
#  endif
#  if !GNULIB_defined_pthread_cond_types
typedef glwthread_cond_t pthread_cond_t;
typedef unsigned int pthread_condattr_t;
#   define GNULIB_defined_pthread_cond_types 1
#  endif
#  undef PTHREAD_COND_INITIALIZER
#  define PTHREAD_COND_INITIALIZER GLWTHREAD_COND_INIT
# else
#  if @HAVE_PTHREAD_T@
#   define pthread_cond_t rpl_pthread_cond_t
#   define pthread_condattr_t rpl_pthread_condattr_t
#  endif
#  if !GNULIB_defined_pthread_cond_types
typedef int pthread_cond_t;
typedef unsigned int pthread_condattr_t;
#   define GNULIB_defined_pthread_cond_types 1
#  endif
#  undef PTHREAD_COND_INITIALIZER
#  define PTHREAD_COND_INITIALIZER { 0 }
# endif
#else
# if !@HAVE_PTHREAD_T@
#  if !GNULIB_defined_pthread_cond_types
typedef int pthread_cond_t;
typedef unsigned int pthread_condattr_t;
#   define GNULIB_defined_pthread_cond_types 1
#  endif
#  undef PTHREAD_COND_INITIALIZER
#  define PTHREAD_COND_INITIALIZER { 0 }
# endif
#endif

/* =========== Thread-specific storage types and macros =========== */

#if (defined _WIN32 && ! defined __CYGWIN__) && USE_WINDOWS_THREADS
# if @GNULIB_PTHREAD_TSS@
#  include "windows-tls.h"
#  if @HAVE_PTHREAD_T@
#   define pthread_key_t rpl_pthread_key_t
#  endif
#  if !GNULIB_defined_pthread_tss_types
typedef glwthread_tls_key_t pthread_key_t;
#   define GNULIB_defined_pthread_tss_types 1
#  endif
#  undef PTHREAD_DESTRUCTOR_ITERATIONS
#  define PTHREAD_DESTRUCTOR_ITERATIONS GLWTHREAD_DESTRUCTOR_ITERATIONS
# else
#  if @HAVE_PTHREAD_T@
#   define pthread_key_t rpl_pthread_key_t
#  endif
#  if !GNULIB_defined_pthread_tss_types
typedef void ** pthread_key_t;
#   define GNULIB_defined_pthread_tss_types 1
#  endif
#  undef PTHREAD_DESTRUCTOR_ITERATIONS
#  define PTHREAD_DESTRUCTOR_ITERATIONS 0
# endif
#else
# if !@HAVE_PTHREAD_T@
#  if !GNULIB_defined_pthread_tss_types
typedef void ** pthread_key_t;
#   define GNULIB_defined_pthread_tss_types 1
#  endif
#  undef PTHREAD_DESTRUCTOR_ITERATIONS
#  define PTHREAD_DESTRUCTOR_ITERATIONS 0
# endif
#endif

/* =========== Spinlock types and macros =========== */

#if (defined _WIN32 && ! defined __CYGWIN__) && USE_WINDOWS_THREADS
# if @GNULIB_PTHREAD_SPIN@
#  include "windows-spin.h"
#  if @HAVE_PTHREAD_T@
#   define pthread_spinlock_t rpl_pthread_spinlock_t
#  endif
#  if !GNULIB_defined_pthread_spin_types
typedef glwthread_spinlock_t pthread_spinlock_t;
#   define GNULIB_defined_pthread_spin_types 1
#  endif
# else
#  if @HAVE_PTHREAD_T@
#   define pthread_spinlock_t rpl_pthread_spinlock_t
#  endif
#  if !GNULIB_defined_pthread_spin_types
typedef pthread_mutex_t pthread_spinlock_t;
#   define GNULIB_defined_pthread_spin_types 1
#  endif
# endif
# undef PTHREAD_PROCESS_PRIVATE
# undef PTHREAD_PROCESS_SHARED
# define PTHREAD_PROCESS_PRIVATE 0
# define PTHREAD_PROCESS_SHARED 1
#else
# if !@HAVE_PTHREAD_SPINLOCK_T@
/* Approximate spinlocks with mutexes.  */
#  if !GNULIB_defined_pthread_spin_types
typedef pthread_mutex_t pthread_spinlock_t;
#   define GNULIB_defined_pthread_spin_types 1
#  endif
# endif
# if !@HAVE_PTHREAD_PROCESS_SHARED@
#  define PTHREAD_PROCESS_PRIVATE 0
#  define PTHREAD_PROCESS_SHARED 1
# endif
#endif

/* =========== Other types and macros =========== */

#if !@HAVE_PTHREAD_T@
# if !GNULIB_defined_other_pthread_types
typedef int pthread_barrier_t;
typedef unsigned int pthread_barrierattr_t;
#  define GNULIB_defined_other_pthread_types 1
# endif
#endif

#if !defined PTHREAD_CANCELED

# define PTHREAD_BARRIER_SERIAL_THREAD (-1)

# define PTHREAD_CANCEL_DEFERRED 0
# define PTHREAD_CANCEL_ASYNCHRONOUS 1

# define PTHREAD_CANCEL_ENABLE 0
# define PTHREAD_CANCEL_DISABLE 1

# define PTHREAD_CANCELED ((void *) -1)

# define PTHREAD_INHERIT_SCHED 0
# define PTHREAD_EXPLICIT_SCHED 1

# define PTHREAD_PRIO_NONE 0
# define PTHREAD_PRIO_INHERIT 1
# define PTHREAD_PRIO_PROTECT 2

# define PTHREAD_SCOPE_SYSTEM 0
# define PTHREAD_SCOPE_PROCESS 1

#endif

/* =========== Thread functions =========== */

#if @GNULIB_PTHREAD_THREAD@
# if @REPLACE_PTHREAD_CREATE@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_create
#   define pthread_create rpl_pthread_create
#  endif
_GL_FUNCDECL_RPL (pthread_create, int,
                  (pthread_t *threadp, const pthread_attr_t *attr,
                   void * (*mainfunc) (void *), void *arg)
                  _GL_ARG_NONNULL ((1, 3)));
_GL_CXXALIAS_RPL (pthread_create, int,
                  (pthread_t *threadp, const pthread_attr_t *attr,
                   void * (*mainfunc) (void *), void *arg));
# else
#  if !@HAVE_PTHREAD_CREATE@
_GL_FUNCDECL_SYS (pthread_create, int,
                  (pthread_t *threadp, const pthread_attr_t *attr,
                   void * (*mainfunc) (void *), void *arg)
                  _GL_ARG_NONNULL ((1, 3)));
#  endif
_GL_CXXALIAS_SYS_CAST (pthread_create, int,
                       (pthread_t *threadp, const pthread_attr_t *attr,
                        void * (*mainfunc) (void *), void *arg));
# endif
_GL_CXXALIASWARN (pthread_create);
#elif defined GNULIB_POSIXCHECK
# undef pthread_create
# if HAVE_RAW_DECL_PTHREAD_CREATE
_GL_WARN_ON_USE (pthread_create, "pthread_create is not portable - "
                 "use gnulib module pthread-thread for portability");
# endif
#endif

#if @GNULIB_PTHREAD_THREAD@
# if @REPLACE_PTHREAD_ATTR_INIT@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_attr_init
#   define pthread_attr_init rpl_pthread_attr_init
#  endif
_GL_FUNCDECL_RPL (pthread_attr_init, int, (pthread_attr_t *attr)
                                          _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_attr_init, int, (pthread_attr_t *attr));
# else
#  if !@HAVE_PTHREAD_ATTR_INIT@
_GL_FUNCDECL_SYS (pthread_attr_init, int, (pthread_attr_t *attr)
                                          _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_attr_init, int, (pthread_attr_t *attr));
# endif
_GL_CXXALIASWARN (pthread_attr_init);
#elif defined GNULIB_POSIXCHECK
# undef pthread_attr_init
# if HAVE_RAW_DECL_PTHREAD_ATTR_INIT
_GL_WARN_ON_USE (pthread_attr_init, "pthread_attr_init is not portable - "
                 "use gnulib module pthread-thread for portability");
# endif
#endif

#if @GNULIB_PTHREAD_THREAD@
# if @REPLACE_PTHREAD_ATTR_GETDETACHSTATE@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_attr_getdetachstate
#   define pthread_attr_getdetachstate rpl_pthread_attr_getdetachstate
#  endif
_GL_FUNCDECL_RPL (pthread_attr_getdetachstate, int,
                  (const pthread_attr_t *attr, int *detachstatep)
                  _GL_ARG_NONNULL ((1, 2)));
_GL_CXXALIAS_RPL (pthread_attr_getdetachstate, int,
                  (const pthread_attr_t *attr, int *detachstatep));
# else
#  if !@HAVE_PTHREAD_ATTR_GETDETACHSTATE@
_GL_FUNCDECL_SYS (pthread_attr_getdetachstate, int,
                  (const pthread_attr_t *attr, int *detachstatep)
                  _GL_ARG_NONNULL ((1, 2)));
#  endif
_GL_CXXALIAS_SYS (pthread_attr_getdetachstate, int,
                  (const pthread_attr_t *attr, int *detachstatep));
# endif
_GL_CXXALIASWARN (pthread_attr_getdetachstate);
#elif defined GNULIB_POSIXCHECK
# undef pthread_attr_getdetachstate
# if HAVE_RAW_DECL_PTHREAD_ATTR_GETDETACHSTATE
_GL_WARN_ON_USE (pthread_attr_getdetachstate, "pthread_attr_getdetachstate is not portable - "
                 "use gnulib module pthread-thread for portability");
# endif
#endif

#if @GNULIB_PTHREAD_THREAD@
# if @REPLACE_PTHREAD_ATTR_SETDETACHSTATE@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_attr_setdetachstate
#   define pthread_attr_setdetachstate rpl_pthread_attr_setdetachstate
#  endif
_GL_FUNCDECL_RPL (pthread_attr_setdetachstate, int,
                  (pthread_attr_t *attr, int detachstate)
                  _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_attr_setdetachstate, int,
                  (pthread_attr_t *attr, int detachstate));
# else
#  if !@HAVE_PTHREAD_ATTR_SETDETACHSTATE@
_GL_FUNCDECL_SYS (pthread_attr_setdetachstate, int,
                  (pthread_attr_t *attr, int detachstate)
                  _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_attr_setdetachstate, int,
                  (pthread_attr_t *attr, int detachstate));
# endif
_GL_CXXALIASWARN (pthread_attr_setdetachstate);
#elif defined GNULIB_POSIXCHECK
# undef pthread_attr_setdetachstate
# if HAVE_RAW_DECL_PTHREAD_ATTR_SETDETACHSTATE
_GL_WARN_ON_USE (pthread_attr_setdetachstate, "pthread_attr_setdetachstate is not portable - "
                 "use gnulib module pthread-thread for portability");
# endif
#endif

#if @GNULIB_PTHREAD_THREAD@
# if @REPLACE_PTHREAD_ATTR_DESTROY@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_attr_destroy
#   define pthread_attr_destroy rpl_pthread_attr_destroy
#  endif
_GL_FUNCDECL_RPL (pthread_attr_destroy, int, (pthread_attr_t *attr)
                                             _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_attr_destroy, int, (pthread_attr_t *attr));
# else
#  if !@HAVE_PTHREAD_ATTR_DESTROY@
_GL_FUNCDECL_SYS (pthread_attr_destroy, int, (pthread_attr_t *attr)
                                             _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_attr_destroy, int, (pthread_attr_t *attr));
# endif
_GL_CXXALIASWARN (pthread_attr_destroy);
#elif defined GNULIB_POSIXCHECK
# undef pthread_attr_destroy
# if HAVE_RAW_DECL_PTHREAD_ATTR_DESTROY
_GL_WARN_ON_USE (pthread_attr_destroy, "pthread_attr_destroy is not portable - "
                 "use gnulib module pthread-thread for portability");
# endif
#endif

#if @GNULIB_PTHREAD_THREAD@
# if @REPLACE_PTHREAD_SELF@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_self
#   define pthread_self rpl_pthread_self
#  endif
_GL_FUNCDECL_RPL (pthread_self, pthread_t, (void) _GL_ATTRIBUTE_PURE);
_GL_CXXALIAS_RPL (pthread_self, pthread_t, (void));
# else
#  if !@HAVE_PTHREAD_SELF@
_GL_FUNCDECL_SYS (pthread_self, pthread_t, (void) _GL_ATTRIBUTE_PURE);
#  endif
_GL_CXXALIAS_SYS (pthread_self, pthread_t, (void));
# endif
_GL_CXXALIASWARN (pthread_self);
#elif defined GNULIB_POSIXCHECK
# undef pthread_self
# if HAVE_RAW_DECL_PTHREAD_SELF
_GL_WARN_ON_USE (pthread_self, "pthread_self is not portable - "
                 "use gnulib module pthread-thread for portability");
# endif
#endif

#if @GNULIB_PTHREAD_THREAD@
# if @REPLACE_PTHREAD_EQUAL@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_equal
#   define pthread_equal rpl_pthread_equal
#  endif
_GL_FUNCDECL_RPL (pthread_equal, int, (pthread_t thread1, pthread_t thread2));
_GL_CXXALIAS_RPL (pthread_equal, int, (pthread_t thread1, pthread_t thread2));
# else
#  if !@HAVE_PTHREAD_EQUAL@
_GL_FUNCDECL_SYS (pthread_equal, int, (pthread_t thread1, pthread_t thread2));
#  endif
_GL_CXXALIAS_SYS (pthread_equal, int, (pthread_t thread1, pthread_t thread2));
# endif
_GL_CXXALIASWARN (pthread_equal);
#elif defined GNULIB_POSIXCHECK
# undef pthread_equal
# if HAVE_RAW_DECL_PTHREAD_EQUAL
_GL_WARN_ON_USE (pthread_equal, "pthread_equal is not portable - "
                 "use gnulib module pthread-thread for portability");
# endif
#endif

#if @GNULIB_PTHREAD_THREAD@
# if @REPLACE_PTHREAD_DETACH@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_detach
#   define pthread_detach rpl_pthread_detach
#  endif
_GL_FUNCDECL_RPL (pthread_detach, int, (pthread_t thread));
_GL_CXXALIAS_RPL (pthread_detach, int, (pthread_t thread));
# else
#  if !@HAVE_PTHREAD_DETACH@
_GL_FUNCDECL_SYS (pthread_detach, int, (pthread_t thread));
#  endif
_GL_CXXALIAS_SYS (pthread_detach, int, (pthread_t thread));
# endif
_GL_CXXALIASWARN (pthread_detach);
#elif defined GNULIB_POSIXCHECK
# undef pthread_detach
# if HAVE_RAW_DECL_PTHREAD_DETACH
_GL_WARN_ON_USE (pthread_detach, "pthread_detach is not portable - "
                 "use gnulib module pthread-thread for portability");
# endif
#endif

#if @GNULIB_PTHREAD_THREAD@
# if @REPLACE_PTHREAD_JOIN@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_join
#   define pthread_join rpl_pthread_join
#  endif
_GL_FUNCDECL_RPL (pthread_join, int, (pthread_t thread, void **valuep));
_GL_CXXALIAS_RPL (pthread_join, int, (pthread_t thread, void **valuep));
# else
#  if !@HAVE_PTHREAD_JOIN@
_GL_FUNCDECL_SYS (pthread_join, int, (pthread_t thread, void **valuep));
#  endif
_GL_CXXALIAS_SYS (pthread_join, int, (pthread_t thread, void **valuep));
# endif
_GL_CXXALIASWARN (pthread_join);
#elif defined GNULIB_POSIXCHECK
# undef pthread_join
# if HAVE_RAW_DECL_PTHREAD_JOIN
_GL_WARN_ON_USE (pthread_join, "pthread_join is not portable - "
                 "use gnulib module pthread-thread for portability");
# endif
#endif

#if @GNULIB_PTHREAD_THREAD@
# if @REPLACE_PTHREAD_EXIT@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_exit
#   define pthread_exit rpl_pthread_exit
#  endif
_GL_FUNCDECL_RPL (pthread_exit, _Noreturn void, (void *value));
_GL_CXXALIAS_RPL (pthread_exit, void, (void *value));
# else
#  if !@HAVE_PTHREAD_EXIT@
_GL_FUNCDECL_SYS (pthread_exit, _Noreturn void, (void *value));
#  endif
/* Need to cast because of AIX with xlclang++.  */
_GL_CXXALIAS_SYS_CAST (pthread_exit, void, (void *value));
# endif
_GL_CXXALIASWARN (pthread_exit);
#elif defined GNULIB_POSIXCHECK
# undef pthread_exit
# if HAVE_RAW_DECL_PTHREAD_EXIT
_GL_WARN_ON_USE (pthread_exit, "pthread_exit is not portable - "
                 "use gnulib module pthread-thread for portability");
# endif
#endif

/* =========== Once-only control (initialization) functions =========== */

#if @GNULIB_PTHREAD_ONCE@
# if @REPLACE_PTHREAD_ONCE@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_once
#   define pthread_once rpl_pthread_once
#  endif
_GL_FUNCDECL_RPL (pthread_once, int,
                  (pthread_once_t *once_control, void (*initfunction) (void))
                  _GL_ARG_NONNULL ((1, 2)));
_GL_CXXALIAS_RPL (pthread_once, int,
                  (pthread_once_t *once_control, void (*initfunction) (void)));
# else
#  if !@HAVE_PTHREAD_ONCE@
_GL_FUNCDECL_SYS (pthread_once, int,
                  (pthread_once_t *once_control, void (*initfunction) (void))
                   _GL_ARG_NONNULL ((1, 2)));
#  endif
_GL_CXXALIAS_SYS_CAST (pthread_once, int,
                       (pthread_once_t *once_control,
                        void (*initfunction) (void)));
# endif
_GL_CXXALIASWARN (pthread_once);
#elif defined GNULIB_POSIXCHECK
# undef pthread_once
# if HAVE_RAW_DECL_PTHREAD_ONCE
_GL_WARN_ON_USE (pthread_once, "pthread_once is not portable - "
                 "use gnulib module pthread-once for portability");
# endif
#endif

/* =========== Mutex functions =========== */

#if @GNULIB_PTHREAD_MUTEX@
# if @REPLACE_PTHREAD_MUTEX_INIT@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_mutex_init
#   define pthread_mutex_init rpl_pthread_mutex_init
#  endif
_GL_FUNCDECL_RPL (pthread_mutex_init, int,
                  (pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
                  _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_mutex_init, int,
                  (pthread_mutex_t *mutex, const pthread_mutexattr_t *attr));
# else
#  if !@HAVE_PTHREAD_MUTEX_INIT@
_GL_FUNCDECL_SYS (pthread_mutex_init, int,
                  (pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
                  _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_mutex_init, int,
                  (pthread_mutex_t *mutex, const pthread_mutexattr_t *attr));
# endif
_GL_CXXALIASWARN (pthread_mutex_init);
#elif defined GNULIB_POSIXCHECK
# undef pthread_mutex_init
# if HAVE_RAW_DECL_PTHREAD_MUTEX_INIT
_GL_WARN_ON_USE (pthread_mutex_init, "pthread_mutex_init is not portable - "
                 "use gnulib module pthread-mutex for portability");
# endif
#endif

#if @GNULIB_PTHREAD_MUTEX@
# if @REPLACE_PTHREAD_MUTEXATTR_INIT@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_mutexattr_init
#   define pthread_mutexattr_init rpl_pthread_mutexattr_init
#  endif
_GL_FUNCDECL_RPL (pthread_mutexattr_init, int, (pthread_mutexattr_t *attr)
                                               _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_mutexattr_init, int, (pthread_mutexattr_t *attr));
# else
#  if !@HAVE_PTHREAD_MUTEXATTR_INIT@
_GL_FUNCDECL_SYS (pthread_mutexattr_init, int, (pthread_mutexattr_t *attr)
                                               _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_mutexattr_init, int, (pthread_mutexattr_t *attr));
# endif
_GL_CXXALIASWARN (pthread_mutexattr_init);
#elif defined GNULIB_POSIXCHECK
# undef pthread_mutexattr_init
# if HAVE_RAW_DECL_PTHREAD_MUTEXATTR_INIT
_GL_WARN_ON_USE (pthread_mutexattr_init, "pthread_mutexattr_init is not portable - "
                 "use gnulib module pthread-mutex for portability");
# endif
#endif

#if @GNULIB_PTHREAD_MUTEX@
# if @REPLACE_PTHREAD_MUTEXATTR_GETTYPE@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_mutexattr_gettype
#   define pthread_mutexattr_gettype rpl_pthread_mutexattr_gettype
#  endif
_GL_FUNCDECL_RPL (pthread_mutexattr_gettype, int,
                  (const pthread_mutexattr_t *attr, int *typep)
                  _GL_ARG_NONNULL ((1, 2)));
_GL_CXXALIAS_RPL (pthread_mutexattr_gettype, int,
                  (const pthread_mutexattr_t *attr, int *typep));
# else
#  if !@HAVE_PTHREAD_MUTEXATTR_GETTYPE@
_GL_FUNCDECL_SYS (pthread_mutexattr_gettype, int,
                  (const pthread_mutexattr_t *attr, int *typep)
                  _GL_ARG_NONNULL ((1, 2)));
#  endif
/* Need to cast, because on FreeBSD the first parameter is
                        pthread_mutexattr_t *attr.  */
_GL_CXXALIAS_SYS_CAST (pthread_mutexattr_gettype, int,
                       (const pthread_mutexattr_t *attr, int *typep));
# endif
_GL_CXXALIASWARN (pthread_mutexattr_gettype);
#elif defined GNULIB_POSIXCHECK
# undef pthread_mutexattr_gettype
# if HAVE_RAW_DECL_PTHREAD_MUTEXATTR_GETTYPE
_GL_WARN_ON_USE (pthread_mutexattr_gettype, "pthread_mutexattr_gettype is not portable - "
                 "use gnulib module pthread-mutex for portability");
# endif
#endif

#if @GNULIB_PTHREAD_MUTEX@
# if @REPLACE_PTHREAD_MUTEXATTR_SETTYPE@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_mutexattr_settype
#   define pthread_mutexattr_settype rpl_pthread_mutexattr_settype
#  endif
_GL_FUNCDECL_RPL (pthread_mutexattr_settype, int,
                  (pthread_mutexattr_t *attr, int type) _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_mutexattr_settype, int,
                  (pthread_mutexattr_t *attr, int type));
# else
#  if !@HAVE_PTHREAD_MUTEXATTR_SETTYPE@
_GL_FUNCDECL_SYS (pthread_mutexattr_settype, int,
                  (pthread_mutexattr_t *attr, int type) _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_mutexattr_settype, int,
                  (pthread_mutexattr_t *attr, int type));
# endif
_GL_CXXALIASWARN (pthread_mutexattr_settype);
#elif defined GNULIB_POSIXCHECK
# undef pthread_mutexattr_settype
# if HAVE_RAW_DECL_PTHREAD_MUTEXATTR_SETTYPE
_GL_WARN_ON_USE (pthread_mutexattr_settype, "pthread_mutexattr_settype is not portable - "
                 "use gnulib module pthread-mutex for portability");
# endif
#endif

#if @GNULIB_PTHREAD_MUTEX@
# if @REPLACE_PTHREAD_MUTEXATTR_GETROBUST@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_mutexattr_getrobust
#   define pthread_mutexattr_getrobust rpl_pthread_mutexattr_getrobust
#  endif
_GL_FUNCDECL_RPL (pthread_mutexattr_getrobust, int,
                  (const pthread_mutexattr_t *attr, int *robustp)
                  _GL_ARG_NONNULL ((1, 2)));
_GL_CXXALIAS_RPL (pthread_mutexattr_getrobust, int,
                  (const pthread_mutexattr_t *attr, int *robustp));
# else
#  if !@HAVE_PTHREAD_MUTEXATTR_GETROBUST@
_GL_FUNCDECL_SYS (pthread_mutexattr_getrobust, int,
                  (const pthread_mutexattr_t *attr, int *robustp)
                  _GL_ARG_NONNULL ((1, 2)));
#  endif
/* Need to cast, because on FreeBSD the first parameter is
                        pthread_mutexattr_t *attr.  */
_GL_CXXALIAS_SYS_CAST (pthread_mutexattr_getrobust, int,
                       (const pthread_mutexattr_t *attr, int *robustp));
# endif
# if __GLIBC__ >= 2
_GL_CXXALIASWARN (pthread_mutexattr_getrobust);
# endif
#elif defined GNULIB_POSIXCHECK
# undef pthread_mutexattr_getrobust
# if HAVE_RAW_DECL_PTHREAD_MUTEXATTR_GETROBUST
_GL_WARN_ON_USE (pthread_mutexattr_getrobust, "pthread_mutexattr_getrobust is not portable - "
                 "use gnulib module pthread-mutex for portability");
# endif
#endif

#if @GNULIB_PTHREAD_MUTEX@
# if @REPLACE_PTHREAD_MUTEXATTR_SETROBUST@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_mutexattr_setrobust
#   define pthread_mutexattr_setrobust rpl_pthread_mutexattr_setrobust
#  endif
_GL_FUNCDECL_RPL (pthread_mutexattr_setrobust, int,
                  (pthread_mutexattr_t *attr, int robust)
                  _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_mutexattr_setrobust, int,
                  (pthread_mutexattr_t *attr, int robust));
# else
#  if !@HAVE_PTHREAD_MUTEXATTR_SETROBUST@
_GL_FUNCDECL_SYS (pthread_mutexattr_setrobust, int,
                  (pthread_mutexattr_t *attr, int robust)
                  _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_mutexattr_setrobust, int,
                  (pthread_mutexattr_t *attr, int robust));
# endif
# if __GLIBC__ >= 2
_GL_CXXALIASWARN (pthread_mutexattr_setrobust);
# endif
#elif defined GNULIB_POSIXCHECK
# undef pthread_mutexattr_setrobust
# if HAVE_RAW_DECL_PTHREAD_MUTEXATTR_SETROBUST
_GL_WARN_ON_USE (pthread_mutexattr_setrobust, "pthread_mutexattr_setrobust is not portable - "
                 "use gnulib module pthread-mutex for portability");
# endif
#endif

#if @GNULIB_PTHREAD_MUTEX@
# if @REPLACE_PTHREAD_MUTEXATTR_DESTROY@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_mutexattr_destroy
#   define pthread_mutexattr_destroy rpl_pthread_mutexattr_destroy
#  endif
_GL_FUNCDECL_RPL (pthread_mutexattr_destroy, int, (pthread_mutexattr_t *attr)
                                                  _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_mutexattr_destroy, int, (pthread_mutexattr_t *attr));
# else
#  if !@HAVE_PTHREAD_MUTEXATTR_DESTROY@
_GL_FUNCDECL_SYS (pthread_mutexattr_destroy, int, (pthread_mutexattr_t *attr)
                                                  _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_mutexattr_destroy, int, (pthread_mutexattr_t *attr));
# endif
_GL_CXXALIASWARN (pthread_mutexattr_destroy);
#elif defined GNULIB_POSIXCHECK
# undef pthread_mutexattr_destroy
# if HAVE_RAW_DECL_PTHREAD_MUTEXATTR_DESTROY
_GL_WARN_ON_USE (pthread_mutexattr_destroy, "pthread_mutexattr_destroy is not portable - "
                 "use gnulib module pthread-mutex for portability");
# endif
#endif

#if @GNULIB_PTHREAD_MUTEX@
# if @REPLACE_PTHREAD_MUTEX_LOCK@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_mutex_lock
#   define pthread_mutex_lock rpl_pthread_mutex_lock
#  endif
_GL_FUNCDECL_RPL (pthread_mutex_lock, int, (pthread_mutex_t *mutex)
                                           _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_mutex_lock, int, (pthread_mutex_t *mutex));
# else
#  if !@HAVE_PTHREAD_MUTEX_LOCK@
_GL_FUNCDECL_SYS (pthread_mutex_lock, int, (pthread_mutex_t *mutex)
                                           _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_mutex_lock, int, (pthread_mutex_t *mutex));
# endif
# if __GLIBC__ >= 2
_GL_CXXALIASWARN (pthread_mutex_lock);
# endif
#elif defined GNULIB_POSIXCHECK
# undef pthread_mutex_lock
# if HAVE_RAW_DECL_PTHREAD_MUTEX_LOCK
_GL_WARN_ON_USE (pthread_mutex_lock, "pthread_mutex_lock is not portable - "
                 "use gnulib module pthread-mutex for portability");
# endif
#endif

#if @GNULIB_PTHREAD_MUTEX@
# if @REPLACE_PTHREAD_MUTEX_TRYLOCK@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_mutex_trylock
#   define pthread_mutex_trylock rpl_pthread_mutex_trylock
#  endif
_GL_FUNCDECL_RPL (pthread_mutex_trylock, int, (pthread_mutex_t *mutex)
                                              _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_mutex_trylock, int, (pthread_mutex_t *mutex));
# else
#  if !@HAVE_PTHREAD_MUTEX_TRYLOCK@
_GL_FUNCDECL_SYS (pthread_mutex_trylock, int, (pthread_mutex_t *mutex)
                                              _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_mutex_trylock, int, (pthread_mutex_t *mutex));
# endif
_GL_CXXALIASWARN (pthread_mutex_trylock);
#elif defined GNULIB_POSIXCHECK
# undef pthread_mutex_trylock
# if HAVE_RAW_DECL_PTHREAD_MUTEX_TRYLOCK
_GL_WARN_ON_USE (pthread_mutex_trylock, "pthread_mutex_trylock is not portable - "
                 "use gnulib module pthread-mutex for portability");
# endif
#endif

#if @GNULIB_PTHREAD_MUTEX_TIMEDLOCK@
# if @REPLACE_PTHREAD_MUTEX_TIMEDLOCK@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_mutex_timedlock
#   define pthread_mutex_timedlock rpl_pthread_mutex_timedlock
#  endif
_GL_FUNCDECL_RPL (pthread_mutex_timedlock, int,
                  (pthread_mutex_t *mutex, const struct timespec *abstime)
                  _GL_ARG_NONNULL ((1, 2)));
_GL_CXXALIAS_RPL (pthread_mutex_timedlock, int,
                  (pthread_mutex_t *mutex, const struct timespec *abstime));
# else
#  if !@HAVE_PTHREAD_MUTEX_TIMEDLOCK@
_GL_FUNCDECL_SYS (pthread_mutex_timedlock, int,
                  (pthread_mutex_t *mutex, const struct timespec *abstime)
                  _GL_ARG_NONNULL ((1, 2)));
#  endif
_GL_CXXALIAS_SYS (pthread_mutex_timedlock, int,
                  (pthread_mutex_t *mutex, const struct timespec *abstime));
# endif
_GL_CXXALIASWARN (pthread_mutex_timedlock);
#elif defined GNULIB_POSIXCHECK
# undef pthread_mutex_timedlock
# if HAVE_RAW_DECL_PTHREAD_MUTEX_TIMEDLOCK
_GL_WARN_ON_USE (pthread_mutex_timedlock, "pthread_mutex_timedlock is not portable - "
                 "use gnulib module pthread_mutex_timedlock for portability");
# endif
#endif

#if @GNULIB_PTHREAD_MUTEX@
# if @REPLACE_PTHREAD_MUTEX_UNLOCK@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_mutex_unlock
#   define pthread_mutex_unlock rpl_pthread_mutex_unlock
#  endif
_GL_FUNCDECL_RPL (pthread_mutex_unlock, int, (pthread_mutex_t *mutex)
                                             _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_mutex_unlock, int, (pthread_mutex_t *mutex));
# else
#  if !@HAVE_PTHREAD_MUTEX_UNLOCK@
_GL_FUNCDECL_SYS (pthread_mutex_unlock, int, (pthread_mutex_t *mutex)
                                             _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_mutex_unlock, int, (pthread_mutex_t *mutex));
# endif
_GL_CXXALIASWARN (pthread_mutex_unlock);
#elif defined GNULIB_POSIXCHECK
# undef pthread_mutex_unlock
# if HAVE_RAW_DECL_PTHREAD_MUTEX_UNLOCK
_GL_WARN_ON_USE (pthread_mutex_unlock, "pthread_mutex_unlock is not portable - "
                 "use gnulib module pthread-mutex for portability");
# endif
#endif

#if @GNULIB_PTHREAD_MUTEX@
# if @REPLACE_PTHREAD_MUTEX_DESTROY@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_mutex_destroy
#   define pthread_mutex_destroy rpl_pthread_mutex_destroy
#  endif
_GL_FUNCDECL_RPL (pthread_mutex_destroy, int, (pthread_mutex_t *mutex)
                                              _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_mutex_destroy, int, (pthread_mutex_t *mutex));
# else
#  if !@HAVE_PTHREAD_MUTEX_DESTROY@
_GL_FUNCDECL_SYS (pthread_mutex_destroy, int, (pthread_mutex_t *mutex)
                                              _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_mutex_destroy, int, (pthread_mutex_t *mutex));
# endif
_GL_CXXALIASWARN (pthread_mutex_destroy);
#elif defined GNULIB_POSIXCHECK
# undef pthread_mutex_destroy
# if HAVE_RAW_DECL_PTHREAD_MUTEX_DESTROY
_GL_WARN_ON_USE (pthread_mutex_destroy, "pthread_mutex_destroy is not portable - "
                 "use gnulib module pthread-mutex for portability");
# endif
#endif

/* =========== Read-write lock functions =========== */

#if @GNULIB_PTHREAD_RWLOCK@
# if @REPLACE_PTHREAD_RWLOCK_INIT@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_rwlock_init
#   define pthread_rwlock_init rpl_pthread_rwlock_init
#  endif
_GL_FUNCDECL_RPL (pthread_rwlock_init, int,
                  (pthread_rwlock_t *lock, const pthread_rwlockattr_t *attr)
                  _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_rwlock_init, int,
                  (pthread_rwlock_t *lock, const pthread_rwlockattr_t *attr));
# else
#  if !@HAVE_PTHREAD_RWLOCK_INIT@
_GL_FUNCDECL_SYS (pthread_rwlock_init, int,
                  (pthread_rwlock_t *lock, const pthread_rwlockattr_t *attr)
                  _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_rwlock_init, int,
                  (pthread_rwlock_t *lock, const pthread_rwlockattr_t *attr));
# endif
_GL_CXXALIASWARN (pthread_rwlock_init);
#elif defined GNULIB_POSIXCHECK
# undef pthread_rwlock_init
# if HAVE_RAW_DECL_PTHREAD_RWLOCK_INIT
_GL_WARN_ON_USE (pthread_rwlock_init, "pthread_rwlock_init is not portable - "
                 "use gnulib module pthread-rwlock for portability");
# endif
#endif

#if @GNULIB_PTHREAD_RWLOCK@
# if @REPLACE_PTHREAD_RWLOCKATTR_INIT@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_rwlockattr_init
#   define pthread_rwlockattr_init rpl_pthread_rwlockattr_init
#  endif
_GL_FUNCDECL_RPL (pthread_rwlockattr_init, int, (pthread_rwlockattr_t *attr)
                                                _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_rwlockattr_init, int, (pthread_rwlockattr_t *attr));
# else
#  if !@HAVE_PTHREAD_RWLOCKATTR_INIT@
_GL_FUNCDECL_SYS (pthread_rwlockattr_init, int, (pthread_rwlockattr_t *attr)
                                                _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_rwlockattr_init, int, (pthread_rwlockattr_t *attr));
# endif
_GL_CXXALIASWARN (pthread_rwlockattr_init);
#elif defined GNULIB_POSIXCHECK
# undef pthread_rwlockattr_init
# if HAVE_RAW_DECL_PTHREAD_RWLOCKATTR_INIT
_GL_WARN_ON_USE (pthread_rwlockattr_init, "pthread_rwlockattr_init is not portable - "
                 "use gnulib module pthread-rwlock for portability");
# endif
#endif

#if @GNULIB_PTHREAD_RWLOCK@
# if @REPLACE_PTHREAD_RWLOCKATTR_DESTROY@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_rwlockattr_destroy
#   define pthread_rwlockattr_destroy rpl_pthread_rwlockattr_destroy
#  endif
_GL_FUNCDECL_RPL (pthread_rwlockattr_destroy, int,
                  (pthread_rwlockattr_t *attr) _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_rwlockattr_destroy, int,
                  (pthread_rwlockattr_t *attr));
# else
#  if !@HAVE_PTHREAD_RWLOCKATTR_DESTROY@
_GL_FUNCDECL_SYS (pthread_rwlockattr_destroy, int,
                  (pthread_rwlockattr_t *attr) _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_rwlockattr_destroy, int,
                  (pthread_rwlockattr_t *attr));
# endif
_GL_CXXALIASWARN (pthread_rwlockattr_destroy);
#elif defined GNULIB_POSIXCHECK
# undef pthread_rwlockattr_destroy
# if HAVE_RAW_DECL_PTHREAD_RWLOCKATTR_DESTROY
_GL_WARN_ON_USE (pthread_rwlockattr_destroy, "pthread_rwlockattr_destroy is not portable - "
                 "use gnulib module pthread-rwlock for portability");
# endif
#endif

#if @GNULIB_PTHREAD_RWLOCK@
# if @REPLACE_PTHREAD_RWLOCK_RDLOCK@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_rwlock_rdlock
#   define pthread_rwlock_rdlock rpl_pthread_rwlock_rdlock
#  endif
_GL_FUNCDECL_RPL (pthread_rwlock_rdlock, int, (pthread_rwlock_t *lock)
                                              _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_rwlock_rdlock, int, (pthread_rwlock_t *lock));
# else
#  if !@HAVE_PTHREAD_RWLOCK_RDLOCK@
_GL_FUNCDECL_SYS (pthread_rwlock_rdlock, int, (pthread_rwlock_t *lock)
                                              _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_rwlock_rdlock, int, (pthread_rwlock_t *lock));
# endif
_GL_CXXALIASWARN (pthread_rwlock_rdlock);
#elif defined GNULIB_POSIXCHECK
# undef pthread_rwlock_rdlock
# if HAVE_RAW_DECL_PTHREAD_RWLOCK_RDLOCK
_GL_WARN_ON_USE (pthread_rwlock_rdlock, "pthread_rwlock_rdlock is not portable - "
                 "use gnulib module pthread-rwlock for portability");
# endif
#endif

#if @GNULIB_PTHREAD_RWLOCK@
# if @REPLACE_PTHREAD_RWLOCK_WRLOCK@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_rwlock_wrlock
#   define pthread_rwlock_wrlock rpl_pthread_rwlock_wrlock
#  endif
_GL_FUNCDECL_RPL (pthread_rwlock_wrlock, int, (pthread_rwlock_t *lock)
                                              _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_rwlock_wrlock, int, (pthread_rwlock_t *lock));
# else
#  if !@HAVE_PTHREAD_RWLOCK_WRLOCK@
_GL_FUNCDECL_SYS (pthread_rwlock_wrlock, int, (pthread_rwlock_t *lock)
                                              _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_rwlock_wrlock, int, (pthread_rwlock_t *lock));
# endif
_GL_CXXALIASWARN (pthread_rwlock_wrlock);
#elif defined GNULIB_POSIXCHECK
# undef pthread_rwlock_wrlock
# if HAVE_RAW_DECL_PTHREAD_RWLOCK_WRLOCK
_GL_WARN_ON_USE (pthread_rwlock_wrlock, "pthread_rwlock_wrlock is not portable - "
                 "use gnulib module pthread-rwlock for portability");
# endif
#endif

#if @GNULIB_PTHREAD_RWLOCK@
# if @REPLACE_PTHREAD_RWLOCK_TRYRDLOCK@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_rwlock_tryrdlock
#   define pthread_rwlock_tryrdlock rpl_pthread_rwlock_tryrdlock
#  endif
_GL_FUNCDECL_RPL (pthread_rwlock_tryrdlock, int, (pthread_rwlock_t *lock)
                                                 _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_rwlock_tryrdlock, int, (pthread_rwlock_t *lock));
# else
#  if !@HAVE_PTHREAD_RWLOCK_TRYRDLOCK@
_GL_FUNCDECL_SYS (pthread_rwlock_tryrdlock, int, (pthread_rwlock_t *lock)
                                                 _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_rwlock_tryrdlock, int, (pthread_rwlock_t *lock));
# endif
_GL_CXXALIASWARN (pthread_rwlock_tryrdlock);
#elif defined GNULIB_POSIXCHECK
# undef pthread_rwlock_tryrdlock
# if HAVE_RAW_DECL_PTHREAD_RWLOCK_TRYRDLOCK
_GL_WARN_ON_USE (pthread_rwlock_tryrdlock, "pthread_rwlock_tryrdlock is not portable - "
                 "use gnulib module pthread-rwlock for portability");
# endif
#endif

#if @GNULIB_PTHREAD_RWLOCK@
# if @REPLACE_PTHREAD_RWLOCK_TRYWRLOCK@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_rwlock_trywrlock
#   define pthread_rwlock_trywrlock rpl_pthread_rwlock_trywrlock
#  endif
_GL_FUNCDECL_RPL (pthread_rwlock_trywrlock, int, (pthread_rwlock_t *lock)
                                                 _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_rwlock_trywrlock, int, (pthread_rwlock_t *lock));
# else
#  if !@HAVE_PTHREAD_RWLOCK_TRYWRLOCK@
_GL_FUNCDECL_SYS (pthread_rwlock_trywrlock, int, (pthread_rwlock_t *lock)
                                                 _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_rwlock_trywrlock, int, (pthread_rwlock_t *lock));
# endif
_GL_CXXALIASWARN (pthread_rwlock_trywrlock);
#elif defined GNULIB_POSIXCHECK
# undef pthread_rwlock_trywrlock
# if HAVE_RAW_DECL_PTHREAD_RWLOCK_TRYWRLOCK
_GL_WARN_ON_USE (pthread_rwlock_trywrlock, "pthread_rwlock_trywrlock is not portable - "
                 "use gnulib module pthread-rwlock for portability");
# endif
#endif

#if @GNULIB_PTHREAD_RWLOCK@
# if @REPLACE_PTHREAD_RWLOCK_TIMEDRDLOCK@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_rwlock_timedrdlock
#   define pthread_rwlock_timedrdlock rpl_pthread_rwlock_timedrdlock
#  endif
_GL_FUNCDECL_RPL (pthread_rwlock_timedrdlock, int,
                  (pthread_rwlock_t *lock, const struct timespec *abstime)
                  _GL_ARG_NONNULL ((1, 2)));
_GL_CXXALIAS_RPL (pthread_rwlock_timedrdlock, int,
                  (pthread_rwlock_t *lock, const struct timespec *abstime));
# else
#  if !@HAVE_PTHREAD_RWLOCK_TIMEDRDLOCK@
_GL_FUNCDECL_SYS (pthread_rwlock_timedrdlock, int,
                  (pthread_rwlock_t *lock, const struct timespec *abstime)
                  _GL_ARG_NONNULL ((1, 2)));
#  endif
_GL_CXXALIAS_SYS (pthread_rwlock_timedrdlock, int,
                  (pthread_rwlock_t *lock, const struct timespec *abstime));
# endif
_GL_CXXALIASWARN (pthread_rwlock_timedrdlock);
#elif defined GNULIB_POSIXCHECK
# undef pthread_rwlock_timedrdlock
# if HAVE_RAW_DECL_PTHREAD_RWLOCK_TIMEDRDLOCK
_GL_WARN_ON_USE (pthread_rwlock_timedrdlock, "pthread_rwlock_timedrdlock is not portable - "
                 "use gnulib module pthread-rwlock for portability");
# endif
#endif

#if @GNULIB_PTHREAD_RWLOCK@
# if @REPLACE_PTHREAD_RWLOCK_TIMEDWRLOCK@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_rwlock_timedwrlock
#   define pthread_rwlock_timedwrlock rpl_pthread_rwlock_timedwrlock
#  endif
_GL_FUNCDECL_RPL (pthread_rwlock_timedwrlock, int,
                  (pthread_rwlock_t *lock, const struct timespec *abstime)
                  _GL_ARG_NONNULL ((1, 2)));
_GL_CXXALIAS_RPL (pthread_rwlock_timedwrlock, int,
                  (pthread_rwlock_t *lock, const struct timespec *abstime));
# else
#  if !@HAVE_PTHREAD_RWLOCK_TIMEDWRLOCK@
_GL_FUNCDECL_SYS (pthread_rwlock_timedwrlock, int,
                  (pthread_rwlock_t *lock, const struct timespec *abstime)
                  _GL_ARG_NONNULL ((1, 2)));
#  endif
_GL_CXXALIAS_SYS (pthread_rwlock_timedwrlock, int,
                  (pthread_rwlock_t *lock, const struct timespec *abstime));
# endif
_GL_CXXALIASWARN (pthread_rwlock_timedwrlock);
#elif defined GNULIB_POSIXCHECK
# undef pthread_rwlock_timedwrlock
# if HAVE_RAW_DECL_PTHREAD_RWLOCK_TIMEDWRLOCK
_GL_WARN_ON_USE (pthread_rwlock_timedwrlock, "pthread_rwlock_timedwrlock is not portable - "
                 "use gnulib module pthread-rwlock for portability");
# endif
#endif

#if @GNULIB_PTHREAD_RWLOCK@
# if @REPLACE_PTHREAD_RWLOCK_UNLOCK@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_rwlock_unlock
#   define pthread_rwlock_unlock rpl_pthread_rwlock_unlock
#  endif
_GL_FUNCDECL_RPL (pthread_rwlock_unlock, int, (pthread_rwlock_t *lock)
                                              _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_rwlock_unlock, int, (pthread_rwlock_t *lock));
# else
#  if !@HAVE_PTHREAD_RWLOCK_UNLOCK@
_GL_FUNCDECL_SYS (pthread_rwlock_unlock, int, (pthread_rwlock_t *lock)
                                              _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_rwlock_unlock, int, (pthread_rwlock_t *lock));
# endif
_GL_CXXALIASWARN (pthread_rwlock_unlock);
#elif defined GNULIB_POSIXCHECK
# undef pthread_rwlock_unlock
# if HAVE_RAW_DECL_PTHREAD_RWLOCK_UNLOCK
_GL_WARN_ON_USE (pthread_rwlock_unlock, "pthread_rwlock_unlock is not portable - "
                 "use gnulib module pthread-rwlock for portability");
# endif
#endif

#if @GNULIB_PTHREAD_RWLOCK@
# if @REPLACE_PTHREAD_RWLOCK_DESTROY@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_rwlock_destroy
#   define pthread_rwlock_destroy rpl_pthread_rwlock_destroy
#  endif
_GL_FUNCDECL_RPL (pthread_rwlock_destroy, int, (pthread_rwlock_t *lock)
                                               _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_rwlock_destroy, int, (pthread_rwlock_t *lock));
# else
#  if !@HAVE_PTHREAD_RWLOCK_DESTROY@
_GL_FUNCDECL_SYS (pthread_rwlock_destroy, int, (pthread_rwlock_t *lock)
                                               _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_rwlock_destroy, int, (pthread_rwlock_t *lock));
# endif
_GL_CXXALIASWARN (pthread_rwlock_destroy);
#elif defined GNULIB_POSIXCHECK
# undef pthread_rwlock_destroy
# if HAVE_RAW_DECL_PTHREAD_RWLOCK_DESTROY
_GL_WARN_ON_USE (pthread_rwlock_destroy, "pthread_rwlock_destroy is not portable - "
                 "use gnulib module pthread-rwlock for portability");
# endif
#endif

/* =========== Condition variable functions =========== */

#if @GNULIB_PTHREAD_COND@
# if @REPLACE_PTHREAD_COND_INIT@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_cond_init
#   define pthread_cond_init rpl_pthread_cond_init
#  endif
_GL_FUNCDECL_RPL (pthread_cond_init, int,
                  (pthread_cond_t *cond, const pthread_condattr_t *attr)
                  _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_cond_init, int,
                  (pthread_cond_t *cond, const pthread_condattr_t *attr));
# else
#  if !@HAVE_PTHREAD_COND_INIT@
_GL_FUNCDECL_SYS (pthread_cond_init, int,
                  (pthread_cond_t *cond, const pthread_condattr_t *attr)
                  _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_cond_init, int,
                  (pthread_cond_t *cond, const pthread_condattr_t *attr));
# endif
_GL_CXXALIASWARN (pthread_cond_init);
#elif defined GNULIB_POSIXCHECK
# undef pthread_cond_init
# if HAVE_RAW_DECL_PTHREAD_COND_INIT
_GL_WARN_ON_USE (pthread_cond_init, "pthread_cond_init is not portable - "
                 "use gnulib module pthread-cond for portability");
# endif
#endif

#if @GNULIB_PTHREAD_COND@
# if @REPLACE_PTHREAD_CONDATTR_INIT@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_condattr_init
#   define pthread_condattr_init rpl_pthread_condattr_init
#  endif
_GL_FUNCDECL_RPL (pthread_condattr_init, int, (pthread_condattr_t *attr)
                                              _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_condattr_init, int, (pthread_condattr_t *attr));
# else
#  if !@HAVE_PTHREAD_CONDATTR_INIT@
_GL_FUNCDECL_SYS (pthread_condattr_init, int, (pthread_condattr_t *attr)
                                              _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_condattr_init, int, (pthread_condattr_t *attr));
# endif
_GL_CXXALIASWARN (pthread_condattr_init);
#elif defined GNULIB_POSIXCHECK
# undef pthread_condattr_init
# if HAVE_RAW_DECL_PTHREAD_CONDATTR_INIT
_GL_WARN_ON_USE (pthread_condattr_init, "pthread_condattr_init is not portable - "
                 "use gnulib module pthread-cond for portability");
# endif
#endif

#if @GNULIB_PTHREAD_COND@
# if @REPLACE_PTHREAD_CONDATTR_DESTROY@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_condattr_destroy
#   define pthread_condattr_destroy rpl_pthread_condattr_destroy
#  endif
_GL_FUNCDECL_RPL (pthread_condattr_destroy, int, (pthread_condattr_t *attr)
                                                 _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_condattr_destroy, int, (pthread_condattr_t *attr));
# else
#  if !@HAVE_PTHREAD_CONDATTR_DESTROY@
_GL_FUNCDECL_SYS (pthread_condattr_destroy, int, (pthread_condattr_t *attr)
                                                 _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_condattr_destroy, int, (pthread_condattr_t *attr));
# endif
_GL_CXXALIASWARN (pthread_condattr_destroy);
#elif defined GNULIB_POSIXCHECK
# undef pthread_condattr_destroy
# if HAVE_RAW_DECL_PTHREAD_CONDATTR_DESTROY
_GL_WARN_ON_USE (pthread_condattr_destroy, "pthread_condattr_destroy is not portable - "
                 "use gnulib module pthread-cond for portability");
# endif
#endif

#if @GNULIB_PTHREAD_COND@
# if @REPLACE_PTHREAD_COND_WAIT@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_cond_wait
#   define pthread_cond_wait rpl_pthread_cond_wait
#  endif
_GL_FUNCDECL_RPL (pthread_cond_wait, int,
                  (pthread_cond_t *cond, pthread_mutex_t *mutex)
                  _GL_ARG_NONNULL ((1, 2)));
_GL_CXXALIAS_RPL (pthread_cond_wait, int,
                  (pthread_cond_t *cond, pthread_mutex_t *mutex));
# else
#  if !@HAVE_PTHREAD_COND_WAIT@
_GL_FUNCDECL_SYS (pthread_cond_wait, int,
                  (pthread_cond_t *cond, pthread_mutex_t *mutex)
                  _GL_ARG_NONNULL ((1, 2)));
#  endif
_GL_CXXALIAS_SYS (pthread_cond_wait, int,
                  (pthread_cond_t *cond, pthread_mutex_t *mutex));
# endif
_GL_CXXALIASWARN (pthread_cond_wait);
#elif defined GNULIB_POSIXCHECK
# undef pthread_cond_wait
# if HAVE_RAW_DECL_PTHREAD_COND_WAIT
_GL_WARN_ON_USE (pthread_cond_wait, "pthread_cond_wait is not portable - "
                 "use gnulib module pthread-cond for portability");
# endif
#endif

#if @GNULIB_PTHREAD_COND@
# if @REPLACE_PTHREAD_COND_TIMEDWAIT@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_cond_timedwait
#   define pthread_cond_timedwait rpl_pthread_cond_timedwait
#  endif
_GL_FUNCDECL_RPL (pthread_cond_timedwait, int,
                  (pthread_cond_t *cond, pthread_mutex_t *mutex,
                   const struct timespec *abstime)
                  _GL_ARG_NONNULL ((1, 2, 3)));
_GL_CXXALIAS_RPL (pthread_cond_timedwait, int,
                  (pthread_cond_t *cond, pthread_mutex_t *mutex,
                   const struct timespec *abstime));
# else
#  if !@HAVE_PTHREAD_COND_TIMEDWAIT@
_GL_FUNCDECL_SYS (pthread_cond_timedwait, int,
                  (pthread_cond_t *cond, pthread_mutex_t *mutex,
                   const struct timespec *abstime)
                  _GL_ARG_NONNULL ((1, 2, 3)));
#  endif
_GL_CXXALIAS_SYS (pthread_cond_timedwait, int,
                  (pthread_cond_t *cond, pthread_mutex_t *mutex,
                   const struct timespec *abstime));
# endif
_GL_CXXALIASWARN (pthread_cond_timedwait);
#elif defined GNULIB_POSIXCHECK
# undef pthread_cond_timedwait
# if HAVE_RAW_DECL_PTHREAD_COND_TIMEDWAIT
_GL_WARN_ON_USE (pthread_cond_timedwait, "pthread_cond_timedwait is not portable - "
                 "use gnulib module pthread-cond for portability");
# endif
#endif

#if @GNULIB_PTHREAD_COND@
# if @REPLACE_PTHREAD_COND_SIGNAL@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_cond_signal
#   define pthread_cond_signal rpl_pthread_cond_signal
#  endif
_GL_FUNCDECL_RPL (pthread_cond_signal, int, (pthread_cond_t *cond)
                                            _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_cond_signal, int, (pthread_cond_t *cond));
# else
#  if !@HAVE_PTHREAD_COND_SIGNAL@
_GL_FUNCDECL_SYS (pthread_cond_signal, int, (pthread_cond_t *cond)
                                            _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_cond_signal, int, (pthread_cond_t *cond));
# endif
_GL_CXXALIASWARN (pthread_cond_signal);
#elif defined GNULIB_POSIXCHECK
# undef pthread_cond_signal
# if HAVE_RAW_DECL_PTHREAD_COND_SIGNAL
_GL_WARN_ON_USE (pthread_cond_signal, "pthread_cond_signal is not portable - "
                 "use gnulib module pthread-cond for portability");
# endif
#endif

#if @GNULIB_PTHREAD_COND@
# if @REPLACE_PTHREAD_COND_BROADCAST@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_cond_broadcast
#   define pthread_cond_broadcast rpl_pthread_cond_broadcast
#  endif
_GL_FUNCDECL_RPL (pthread_cond_broadcast, int, (pthread_cond_t *cond)
                                               _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_cond_broadcast, int, (pthread_cond_t *cond));
# else
#  if !@HAVE_PTHREAD_COND_BROADCAST@
_GL_FUNCDECL_SYS (pthread_cond_broadcast, int, (pthread_cond_t *cond)
                                               _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_cond_broadcast, int, (pthread_cond_t *cond));
# endif
_GL_CXXALIASWARN (pthread_cond_broadcast);
#elif defined GNULIB_POSIXCHECK
# undef pthread_cond_broadcast
# if HAVE_RAW_DECL_PTHREAD_COND_BROADCAST
_GL_WARN_ON_USE (pthread_cond_broadcast, "pthread_cond_broadcast is not portable - "
                 "use gnulib module pthread-cond for portability");
# endif
#endif

#if @GNULIB_PTHREAD_COND@
# if @REPLACE_PTHREAD_COND_DESTROY@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_cond_destroy
#   define pthread_cond_destroy rpl_pthread_cond_destroy
#  endif
_GL_FUNCDECL_RPL (pthread_cond_destroy, int, (pthread_cond_t *cond)
                                             _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_cond_destroy, int, (pthread_cond_t *cond));
# else
#  if !@HAVE_PTHREAD_COND_DESTROY@
_GL_FUNCDECL_SYS (pthread_cond_destroy, int, (pthread_cond_t *cond)
                                             _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_cond_destroy, int, (pthread_cond_t *cond));
# endif
_GL_CXXALIASWARN (pthread_cond_destroy);
#elif defined GNULIB_POSIXCHECK
# undef pthread_cond_destroy
# if HAVE_RAW_DECL_PTHREAD_COND_DESTROY
_GL_WARN_ON_USE (pthread_cond_destroy, "pthread_cond_destroy is not portable - "
                 "use gnulib module pthread-cond for portability");
# endif
#endif

/* =========== Thread-specific storage functions =========== */

#if @GNULIB_PTHREAD_TSS@
# if @REPLACE_PTHREAD_KEY_CREATE@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_key_create
#   define pthread_key_create rpl_pthread_key_create
#  endif
_GL_FUNCDECL_RPL (pthread_key_create, int,
                  (pthread_key_t *keyp, void (*destructor) (void *))
                  _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_key_create, int,
                  (pthread_key_t *keyp, void (*destructor) (void *)));
# else
#  if !@HAVE_PTHREAD_KEY_CREATE@
_GL_FUNCDECL_SYS (pthread_key_create, int,
                  (pthread_key_t *keyp, void (*destructor) (void *))
                  _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS_CAST (pthread_key_create, int,
                       (pthread_key_t *keyp, void (*destructor) (void *)));
# endif
_GL_CXXALIASWARN (pthread_key_create);
#elif defined GNULIB_POSIXCHECK
# undef pthread_key_create
# if HAVE_RAW_DECL_PTHREAD_KEY_CREATE
_GL_WARN_ON_USE (pthread_key_create, "pthread_key_create is not portable - "
                 "use gnulib module pthread-tss for portability");
# endif
#endif

#if @GNULIB_PTHREAD_TSS@
# if @REPLACE_PTHREAD_SETSPECIFIC@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_setspecific
#   define pthread_setspecific rpl_pthread_setspecific
#  endif
_GL_FUNCDECL_RPL (pthread_setspecific, int,
                  (pthread_key_t key, const void *value));
_GL_CXXALIAS_RPL (pthread_setspecific, int,
                  (pthread_key_t key, const void *value));
# else
#  if !@HAVE_PTHREAD_SETSPECIFIC@
_GL_FUNCDECL_SYS (pthread_setspecific, int,
                  (pthread_key_t key, const void *value));
#  endif
_GL_CXXALIAS_SYS (pthread_setspecific, int,
                  (pthread_key_t key, const void *value));
# endif
_GL_CXXALIASWARN (pthread_setspecific);
#elif defined GNULIB_POSIXCHECK
# undef pthread_setspecific
# if HAVE_RAW_DECL_PTHREAD_SETSPECIFIC
_GL_WARN_ON_USE (pthread_setspecific, "pthread_setspecific is not portable - "
                 "use gnulib module pthread-tss for portability");
# endif
#endif

#if @GNULIB_PTHREAD_TSS@
# if @REPLACE_PTHREAD_GETSPECIFIC@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_getspecific
#   define pthread_getspecific rpl_pthread_getspecific
#  endif
_GL_FUNCDECL_RPL (pthread_getspecific, void *, (pthread_key_t key));
_GL_CXXALIAS_RPL (pthread_getspecific, void *, (pthread_key_t key));
# else
#  if !@HAVE_PTHREAD_GETSPECIFIC@
_GL_FUNCDECL_SYS (pthread_getspecific, void *, (pthread_key_t key));
#  endif
_GL_CXXALIAS_SYS (pthread_getspecific, void *, (pthread_key_t key));
# endif
_GL_CXXALIASWARN (pthread_getspecific);
#elif defined GNULIB_POSIXCHECK
# undef pthread_getspecific
# if HAVE_RAW_DECL_PTHREAD_GETSPECIFIC
_GL_WARN_ON_USE (pthread_getspecific, "pthread_getspecific is not portable - "
                 "use gnulib module pthread-tss for portability");
# endif
#endif

#if @GNULIB_PTHREAD_TSS@
# if @REPLACE_PTHREAD_KEY_DELETE@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_key_delete
#   define pthread_key_delete rpl_pthread_key_delete
#  endif
_GL_FUNCDECL_RPL (pthread_key_delete, int, (pthread_key_t key));
_GL_CXXALIAS_RPL (pthread_key_delete, int, (pthread_key_t key));
# else
#  if !@HAVE_PTHREAD_KEY_DELETE@
_GL_FUNCDECL_SYS (pthread_key_delete, int, (pthread_key_t key));
#  endif
_GL_CXXALIAS_SYS (pthread_key_delete, int, (pthread_key_t key));
# endif
_GL_CXXALIASWARN (pthread_key_delete);
#elif defined GNULIB_POSIXCHECK
# undef pthread_key_delete
# if HAVE_RAW_DECL_PTHREAD_KEY_DELETE
_GL_WARN_ON_USE (pthread_key_delete, "pthread_key_delete is not portable - "
                 "use gnulib module pthread-tss for portability");
# endif
#endif

/* =========== Spinlock functions =========== */

#if @GNULIB_PTHREAD_SPIN@
# if @REPLACE_PTHREAD_SPIN_INIT@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_spin_init
#   define pthread_spin_init rpl_pthread_spin_init
#  endif
_GL_FUNCDECL_RPL (pthread_spin_init, int,
                  (pthread_spinlock_t *lock, int shared_across_processes)
                  _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_spin_init, int,
                  (pthread_spinlock_t *lock, int shared_across_processes));
# else
#  if !@HAVE_PTHREAD_SPIN_INIT@
_GL_FUNCDECL_SYS (pthread_spin_init, int,
                  (pthread_spinlock_t *lock, int shared_across_processes)
                  _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_spin_init, int,
                  (pthread_spinlock_t *lock, int shared_across_processes));
# endif
# if __GLIBC__ >= 2
_GL_CXXALIASWARN (pthread_spin_init);
# endif
#elif defined GNULIB_POSIXCHECK
# undef pthread_spin_init
# if HAVE_RAW_DECL_PTHREAD_SPIN_INIT
_GL_WARN_ON_USE (pthread_spin_init, "pthread_spin_init is not portable - "
                 "use gnulib module pthread-spin for portability");
# endif
#endif

#if @GNULIB_PTHREAD_SPIN@
# if @REPLACE_PTHREAD_SPIN_LOCK@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_spin_lock
#   define pthread_spin_lock rpl_pthread_spin_lock
#  endif
_GL_FUNCDECL_RPL (pthread_spin_lock, int, (pthread_spinlock_t *lock)
                                          _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_spin_lock, int, (pthread_spinlock_t *lock));
# else
#  if !@HAVE_PTHREAD_SPIN_LOCK@
_GL_FUNCDECL_SYS (pthread_spin_lock, int, (pthread_spinlock_t *lock)
                                          _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_spin_lock, int, (pthread_spinlock_t *lock));
# endif
# if __GLIBC__ >= 2
_GL_CXXALIASWARN (pthread_spin_lock);
# endif
#elif defined GNULIB_POSIXCHECK
# undef pthread_spin_lock
# if HAVE_RAW_DECL_PTHREAD_SPIN_LOCK
_GL_WARN_ON_USE (pthread_spin_lock, "pthread_spin_lock is not portable - "
                 "use gnulib module pthread-spin for portability");
# endif
#endif

#if @GNULIB_PTHREAD_SPIN@
# if @REPLACE_PTHREAD_SPIN_TRYLOCK@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_spin_trylock
#   define pthread_spin_trylock rpl_pthread_spin_trylock
#  endif
_GL_FUNCDECL_RPL (pthread_spin_trylock, int, (pthread_spinlock_t *lock)
                                             _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_spin_trylock, int, (pthread_spinlock_t *lock));
# else
#  if !@HAVE_PTHREAD_SPIN_TRYLOCK@
_GL_FUNCDECL_SYS (pthread_spin_trylock, int, (pthread_spinlock_t *lock)
                                             _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_spin_trylock, int, (pthread_spinlock_t *lock));
# endif
# if __GLIBC__ >= 2
_GL_CXXALIASWARN (pthread_spin_trylock);
# endif
#elif defined GNULIB_POSIXCHECK
# undef pthread_spin_trylock
# if HAVE_RAW_DECL_PTHREAD_SPIN_TRYLOCK
_GL_WARN_ON_USE (pthread_spin_trylock, "pthread_spin_trylock is not portable - "
                 "use gnulib module pthread-spin for portability");
# endif
#endif

#if @GNULIB_PTHREAD_SPIN@
# if @REPLACE_PTHREAD_SPIN_UNLOCK@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_spin_unlock
#   define pthread_spin_unlock rpl_pthread_spin_unlock
#  endif
_GL_FUNCDECL_RPL (pthread_spin_unlock, int, (pthread_spinlock_t *lock)
                                            _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_spin_unlock, int, (pthread_spinlock_t *lock));
# else
#  if !@HAVE_PTHREAD_SPIN_UNLOCK@
_GL_FUNCDECL_SYS (pthread_spin_unlock, int, (pthread_spinlock_t *lock)
                                            _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_spin_unlock, int, (pthread_spinlock_t *lock));
# endif
# if __GLIBC__ >= 2
_GL_CXXALIASWARN (pthread_spin_unlock);
# endif
#elif defined GNULIB_POSIXCHECK
# undef pthread_spin_unlock
# if HAVE_RAW_DECL_PTHREAD_SPIN_UNLOCK
_GL_WARN_ON_USE (pthread_spin_unlock, "pthread_spin_unlock is not portable - "
                 "use gnulib module pthread-spin for portability");
# endif
#endif

#if @GNULIB_PTHREAD_SPIN@
# if @REPLACE_PTHREAD_SPIN_DESTROY@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef pthread_spin_destroy
#   define pthread_spin_destroy rpl_pthread_spin_destroy
#  endif
_GL_FUNCDECL_RPL (pthread_spin_destroy, int, (pthread_spinlock_t *lock)
                                             _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (pthread_spin_destroy, int, (pthread_spinlock_t *lock));
# else
#  if !@HAVE_PTHREAD_SPIN_DESTROY@
_GL_FUNCDECL_SYS (pthread_spin_destroy, int, (pthread_spinlock_t *lock)
                                             _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (pthread_spin_destroy, int, (pthread_spinlock_t *lock));
# endif
# if __GLIBC__ >= 2
_GL_CXXALIASWARN (pthread_spin_destroy);
# endif
#elif defined GNULIB_POSIXCHECK
# undef pthread_spin_destroy
# if HAVE_RAW_DECL_PTHREAD_SPIN_DESTROY
_GL_WARN_ON_USE (pthread_spin_destroy, "pthread_spin_destroy is not portable - "
                 "use gnulib module pthread-spin for portability");
# endif
#endif


#endif /* _@GUARD_PREFIX@_PTHREAD_H_ */
#endif /* _@GUARD_PREFIX@_PTHREAD_H_ */
#endif
