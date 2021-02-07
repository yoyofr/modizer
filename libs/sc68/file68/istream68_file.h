/**
 * @ingroup file68_istream68_devel
 * @file    file68/istream68_file.h
 * @author  benjamin gerard
 * @date    2003/08/08
 * @brief   FILE stream header.
 *
 * $Id: istream68_file.h 503 2005-06-24 08:52:56Z loke $
 *
 */

/* Copyright (C) 1998-2003 Benjamin Gerard */

#ifndef _ISTREAM68_FILE_H_
#define _ISTREAM68_FILE_H_

#include "istream68.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @name      FILE stream
 *  @ingroup   file68_istream68_devel
 *
 *    Implements istream_t for standard "C" FILE.
 *
 *  @{
 */

/** Creates a stream for "C" FILE.
 *
 *  @param  fname  path of file.
 *  @param  mode   bit 0 : read access, bit 1 : write access.
 *
 *  @return stream
 *  @retval 0 on error
 *
 *  @note   filename is internally copied.
 */
istream_t * istream_file_create(const char * fname, int mode);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* #define _ISTREAM68_FILE_H_ */
