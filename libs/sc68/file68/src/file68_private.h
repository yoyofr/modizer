/**
 * @ingroup  lib_file68
 * @file     file68_private.h
 * @author   Benjamin Gerard
 * @date     2016/08/22
 * @brief    file68 library private header.
 */

/* Copyright (c) 1998-2016 Benjamin Gerard */

#ifdef FILE68_API_H
# error file68_private.h must be include before file68_api.h
#endif

#ifdef FILE68_PRIVATE_H
# error file68_private.h must be include once
#endif
#define FILE68_PRIVATE_H 1

#ifdef HAVE_PRIVATE_H
# include "private.h"
#endif

/* ---------------------------------------------------------------- */

#if defined(DEBUG) && !defined(_DEBUG)
# define _DEBUG DEBUG
#elif !defined(DEBUG) && defined(_DEBUG)
# define DEBUG _DEBUG
#endif

#ifndef FILE68_WIN32
# if defined(WIN32) || defined(_WIN32) || defined(__SYMBIAN32__)
#  define FILE68_WIN32 1
# endif
#endif

#if defined(DLL_EXPORT) && !defined(file68_lib_EXPORTS)
# define file68_lib_EXPORTS DLL_EXPORT
#endif

#ifndef FILE68_API
# if defined(FILE68_WIN32) && defined(file68_lib_EXPORTS)
#  define FILE68_API __declspec(dllexport)
# endif
#endif
