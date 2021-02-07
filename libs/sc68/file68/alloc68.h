/**
 * @ingroup file68_alloc68_devel
 * @file    file68/alloc68.h
 * @author  Benjamin Gerard
 * @date    2003/04/11
 * @brief   dynamic memory header.
 *
 * $Id: alloc68.h 503 2005-06-24 08:52:56Z loke $
 *
 */

/* Copyright (C) 1998-2003 Benjamin Gerard */


#ifndef _ALLOC68_H_
#define _ALLOC68_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup   file68_alloc68_devel  Dynamic memory buffers
 *  @ingroup    file68_devel
 *
 *    Provides dynamic memory management functions.
 *
 *  @{
 */

/** Alloc function (malloc). */
typedef void * (*sc68_alloc_t)(unsigned int);

/** Free function (free). */
typedef void (*sc68_free_t)(void *);

/** Allocate dynamic memory.
 *
 *   The SC68alloc() function calls user defined dynamic memory
 *   allocation handler. 
 *
 * @param  n  Size of buffer to allocate.
 *
 * @return pointer to allocated memory buffer.
 * @retval 0 error
 *
 * @see SC68set_alloc()
 * @see SC68calloc()
 * @see SC68free()
 */
void * SC68alloc(unsigned int n);

/** Allocate and clean dynamic memory.
 *
 *   The SC68calloc() function calls user defined dynamic memory
 *   allocation handler and fills memory buffer with 0. 
 *
 * @param  n  Size of buffer to allocate.
 *
 * @return pointer to allocated memory buffer.
 * @retval 0 error
 *
 * @see SC68set_alloc()
 * @see SC68alloc()
 * @see SC68free()
 */
void * SC68calloc(unsigned int n);


/** Free dynamic memory.
 *
 *   The SC68free() function calls user defined dynamic memory
 *   free handler. 
 *
 * @param  data  Previously allocated memory buffer.
 *
 * @return pointer to allocated memory
 * @retval 0 Failure.
 *
 * @see SC68set_free()
 * @see SC68alloc()
 */
void SC68free(void * data);

/** Set/get dynamic memory allocation handler.
 *
 * @param  alloc  Set new alloc handler (0:get old value).
 *
 * @return previous alloc handler.
 *
 * @see SC68alloc()
 */
sc68_alloc_t SC68set_alloc(sc68_alloc_t alloc);

/** Set/get dynamic memory free handler.
 *
 * @param  free  Set new free handler (0:get old value).
 * @return previous free handler.
 *
 * @see SC68free()
 */
sc68_free_t SC68set_free(sc68_free_t free);

/**
 *@}
 */

#ifdef __cplusplus
}
#endif

#endif /* #define _ALLOC68_H_ */
