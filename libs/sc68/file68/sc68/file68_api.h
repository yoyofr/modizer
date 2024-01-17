/**
 * @ingroup  lib_file68
 * @file     sc68/file68_api.h
 * @author   Benjamin Gerard
 * @date     2007-02-25
 * @brief    Symbol exportation header.
 */

/* Copyright (c) 1998-2016 Benjamin Gerard */

#ifndef FILE68_API_H
#define FILE68_API_H

#ifndef FILE68_EXTERN
# ifdef __cplusplus
#   define FILE68_EXTERN extern "C"
# else
#   define FILE68_EXTERN extern
# endif
#endif

/**
 * @defgroup lib_file68 file68 library
 * @ingroup  sc68_dev_lib
 *
 *   file68 is a library to manipulate sc68 files and access sc68
 *   resources and much more.
 *
 * @{
 */

/**
 * Decorate symbols exported for public.
 */
#ifndef FILE68_API
# define FILE68_API FILE68_EXTERN
#endif

/**
 * @}
 */

#endif
