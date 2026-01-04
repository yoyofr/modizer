#ifndef NESTYPES_H__
#define NESTYPES_H__

#if defined(_MSC_VER)
#define NEVER_REACH __assume(0);
#elif defined(__BORLANDC__)
#define __fastcall __msfastcall
#elif defined(__GNUC__)
#ifndef EMSCRIPTEN
#define __inline		__inline__
#endif
#undef __fastcall
#define __fastcall
#endif
#ifndef NEVER_REACH
#define NEVER_REACH
#endif


#ifdef USE_SDL
typedef int				Int;
typedef unsigned int	Uint;
typedef signed int		Int32;
typedef signed short	Int16;
typedef signed char		Int8;
typedef char			Char;
#else
typedef int				Int;
typedef unsigned int	Uint;
typedef signed int		Int32;
typedef unsigned int	Uint32;
typedef signed long		Int64;
typedef unsigned long	Uint64;
typedef signed short	Int16;
typedef unsigned short	Uint16;
typedef signed char		Int8;
typedef unsigned char	Uint8;
typedef char			Char;
#endif


#include <stdlib.h>
#ifndef _WIN32
#include <string.h>
#else
#include <malloc.h>
#endif
#include <memory.h>

// #ifdef _WIN32
//#define XSLEEP(t)		_sleep(t)
//#else
#define XSLEEP(t)
//#endif
#define XMALLOC(s)		malloc(s)
#define XREALLOC(p,s)	realloc(p,s)
#define XFREE(p)		free(p)
#define XMEMCPY(d,s,n)	memcpy(d,s,n)
#define XMEMSET(d,c,n)	memset(d,c,n)

#endif /* NESTYPES_H__ */
