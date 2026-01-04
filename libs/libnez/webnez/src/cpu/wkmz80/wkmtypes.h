/*
  KMxxx common header
  by Mamiya
*/

#ifndef KMTYPES_H_
#define KMTYPES_H_
#ifdef __cplusplus
extern "C" {
#endif


#ifndef Uint32
#define Uint32 unsigned int
#endif
#ifndef Uint8
#define Uint8 unsigned char
#endif

#if defined(EMSCRIPTEN)
#ifdef __inline
#undef __inline
#endif
#define __inline
#elif defined(_MSC_VER)
#elif defined(__BORLANDC__)
#elif defined(__GNUC__)
#ifndef __inline
#define __inline __inline__
#endif
#else
#ifndef __inline
#define __inline static
#endif
#endif

#ifdef __cplusplus
}
#endif
#endif
