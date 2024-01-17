// EMSCRIPTEN.. original impl doesn't guard against multiple includes
#ifndef LIBSC68_EMU68_PRIVATE_H
#define LIBSC68_EMU68_PRIVATE_H

/**
 * @ingroup  lib_emu68
 * @file     emu68_private.h
 * @author   Benjamin Gerard
 * @date     2016/08/30
 * @brief    emu68 library private header.
 *
 *   Put anything here needed to compile this project. It's possible
 *   short circuit or add new things by defining PRIVATE_H a la
 *   config.h.
 *
 */

/* Copyright (c) 1998-2016 Benjamin Gerard */

#define BUILD_EMU68 1

#ifdef EMU68_EMU68_API_H
# error emu68_private.h must be include before emu68_api.h
#endif

#ifdef EMU68_PRIVATE_H
# error emu68_private.h must be include once
#endif
#define EMU68_PRIVATE_H 1

#ifdef HAVE_PRIVATE_H
# include "private.h"
#endif

/* ---------------------------------------------------------------- */

#if defined(DEBUG) && !defined(_DEBUG)
# define _DEBUG DEBUG
#elif !defined(DEBUG) && defined(_DEBUG)
# define DEBUG _DEBUG
#endif

#ifndef EMU68_WIN32
# if defined(WIN32) || defined(_WIN32) || defined(__SYMBIAN32__)
#  define EMU68_WIN32 1
# endif
#endif

#if defined(DLL_EXPORT) && !defined(emu68_lib_EXPORTS)
# define emu68_lib_EXPORTS DLL_EXPORT
#endif

#ifndef EMU68_API
# if defined(EMU68_WIN32) && defined(emu68_lib_EXPORTS)
#  define EMU68_API __declspec(dllexport)
# elif defined(SC68_EXTERN)
#  define EMU68_API SC68_EXTERN
# endif
#endif

/* ---------------------------------------------------------------- */

/* Should be only needed when compiling emu68 */

#define BASE_FIX (sizeof(int68_t)<<3) /* number of bit of int68_t type. */
#define BYTE_FIX (BASE_FIX-8)  /* shift to normalize byte operands. */
#define WORD_FIX (BASE_FIX-16) /* shift to normalize word operands. */
#define LONG_FIX (BASE_FIX-32) /* shift to normalize long operands. */
#define SIGN_FIX (BASE_FIX-1)  /* sign bit raw. */
#define SIGN_BIT SIGN_FIX      /* sign bit raw. */
#define SIGN_MSK ( (int68_t) ( (uint68_t) 1 << SIGN_FIX ) ) /* sign mask. */

#define BYTE_MSK ( (int68_t) 0xFFFFFF00 )
#define WORD_MSK ( (int68_t) 0xFFFF0000 )
#define LONG_MSK ( (int68_t) 0x00000000 )

#define NRM_BYTE_MSK ( (int68_t) ( (uint68_t) 0x000000FF << BYTE_FIX ) )
#define NRM_WORD_MSK ( (int68_t) ( (uint68_t) 0x0000FFFF << WORD_FIX ) )
#define NRM_LONG_MSK ( (int68_t) ( (uint68_t) 0xFFFFFFFF << LONG_FIX ) )
/* L={7,15,31} */
#define NRM_MSK(L)   ( (int68_t) ( (int68_t) ((uint68_t) 1 << SIGN_BIT) >> (L)))

#define NRM_BYTE_ONE ( (int68_t) ( (uint68_t) 0x00000001 << BYTE_FIX ) )
#define NRM_WORD_ONE ( (int68_t) ( (uint68_t) 0x00000001 << WORD_FIX ) )
#define NRM_LONG_ONE ( (int68_t) ( (uint68_t) 0x00000001 << LONG_FIX ) )


#endif