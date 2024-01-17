/**
 * @ingroup  lib_io68
 * @file     io68_private.h
 * @author   Benjamin Gerard
 * @date     2016/09/03
 * @brief    io68 library private header.
 *
 *   Put anything here needed to compile this project. It's possible
 *   short circuit or add new things by defining PRIVATE_H a la
 *   config.h.
 *
 */

/* Copyright (c) 1998-2016 Benjamin Gerard */

#define BUILD_IO68 1

#ifdef IO68_IO68_API_H
# error io68_private.h must be include before io68_api.h
#endif

#ifdef IO68_PRIVATE_H
# error io68_private.h must be include once
#endif
#define IO68_PRIVATE_H 1

#ifdef HAVE_PRIVATE_H
# include "private.h"
#endif

/* ---------------------------------------------------------------- */

#if defined(DEBUG) && !defined(_DEBUG)
# define _DEBUG DEBUG
#elif !defined(DEBUG) && defined(_DEBUG)
# define DEBUG _DEBUG
#endif

#ifndef IO68_WIN32
# if defined(WIN32) || defined(_WIN32) || defined(__SYMBIAN32__)
#  define IO68_WIN32 1
# endif
#endif

#if defined(DLL_EXPORT) && !defined(io68_lib_EXPORTS)
# define io68_lib_EXPORTS DLL_EXPORT
#endif

#ifndef IO68_API
# if defined(IO68_WIN32) && defined(io68_lib_EXPORTS)
#  define IO68_API __declspec(dllexport)
# elif defined(SC68_EXTERN)
#  define IO68_API SC68_EXTERN
# endif
#endif
