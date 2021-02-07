/**
 * @ingroup   file68_devel
 * @file      file68/init68.h
 * @author    Benjamin Gerard <ben@sashipa.com>
 * @date      2003/09/26
 * @brief     file68 initialization
 *
 * $Id: init68.h 503 2005-06-24 08:52:56Z loke $
 *
 */

/* Copyright (C) 1998-2003 Benjamin Gerard */

#ifndef _INIT68_H_
#define _INIT68_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup   file68_devel
 *  @{
 */

/** Initialize file68 library.
 *
 *  @return new argc
 */
int file68_init(char **argv, int argc);

/** Shutdown file68 library.
 *
 */
void file68_shutdown(void);

/**
 *@}
 */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _INIT68_H_ */
