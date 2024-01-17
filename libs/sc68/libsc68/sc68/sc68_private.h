/**
 * @ingroup  lib_sc68
 * @file     sc68_private.h
 * @author   Benjamin Gerard
 * @date     2016/08/30
 * @brief    sc68 library private header.
 *
 *   Put here anything needed to compile this project. It's possible
 *   short circuit or add new things by defining PRIVATE_H a la
 *   config.h.
 *
 */

/* Copyright (c) 1998-2016 Benjamin Gerard */

#ifdef SC68_SC68_H
# error sc68_private.h must be include before sc68.h
#endif

#ifdef SC68_PRIVATE_H
# error sc68_private.h must be include once
#endif
#define SC68_PRIVATE_H 1

#ifdef HAVE_PRIVATE_H
# include "private.h"
#endif

/* ---------------------------------------------------------------- */

#if defined(DEBUG) && !defined(_DEBUG)
# define _DEBUG DEBUG
#elif !defined(DEBUG) && defined(_DEBUG)
# define DEBUG _DEBUG
#endif

#ifndef SC68_WIN32
# if defined(WIN32) || defined(_WIN32) || defined(__SYMBIAN32__)
#  define SC68_WIN32 1
# endif
#endif

#if defined(DLL_EXPORT) && !defined(sc68_lib_EXPORTS)
# define sc68_lib_EXPORTS DLL_EXPORT
#endif

#ifndef SC68_API
# if defined(SC68_WIN32) && defined(sc68_lib_EXPORTS)
#  define SC68_API __declspec(dllexport)
# endif
#endif

#define BUILD_SC68 1
