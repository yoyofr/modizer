/**
 * @ingroup file68_error68_devel
 * @file    file68/error68.h
 * @author  benjamin gerard
 * @date    2003/08/08
 * @brief   error message header.
 *
 * $Id: error68.h 503 2005-06-24 08:52:56Z loke $
 *
 *
 */

/* Copyright (C) 1998-2003 Benjamin Gerard */

#ifndef _ERROR68_H_
#define _ERROR68_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup   file68_error68_devel  Error message
 *  @ingroup    file68_devel
 *
 *    Provides error message push and pop functions.
 *    Error messages consist on a fixed size stack of fixed
 *    length strings.
 *
 *  @{
 */

/** Push a formatted error message.
 *
 *    The SC68error_add() function format error string into stack buffer. If
 *    stack is full, the oldest error message is removed.
 *
 *  @param  format  printf() like format string
 *
 *  @return error-code
 *  @retval -1
 */
int SC68error_add(const char *format, ... );

/** Get last error message.
 *
 *    The SC68error_get() function retrieves last error message and removes
 *    it from error message stack.
 *
 *  @return  Static string to last error message
 *  @retval  0  No stacked error message
 */
const char * SC68error_get(void);

/**
 *@}
 */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _ERROR68_H_ */
