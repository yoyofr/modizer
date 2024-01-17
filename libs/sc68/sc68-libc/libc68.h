/**
 * @ingroup  libc68_lib
 * @file     libc68.h
 * @author   Benjamin Gerard
 * @date     2013-08-02
 * @brief    Libc missing or faulty functions replacements.
 */

/* Copyright (c) 1998-2016 Benjamin Gerard */

#ifndef LIBC68_H
#define LIBC68_H

#ifndef SC68_EXTERN
# ifdef __cplusplus
#  define SC68_EXTERN extern "C"
# else
#  define SC68_EXTERN
# endif
#endif

#ifndef HAVE_STRTOUL
SC68_EXTERN
unsigned long int strtoul(const char *s, char **end, int base);
#endif

#ifndef HAVE_BASENAME
SC68_EXTERN
char *basename(char *path);
#endif

#endif
