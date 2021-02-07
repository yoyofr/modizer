/**
 * @ingroup file68_istream68_devel
 * @file    file68/istream68_null.h
 * @author  Benjamin Gerard
 * @date    2003/10/10
 * @brief   Null stream header.
 *
 * $Id: istream68_null.h 503 2005-06-24 08:52:56Z loke $
 *
 */

/* Copyright (C) 1998-2003 Benjamin Gerard */

#ifndef _ISTREAM68_NULL_H_
#define _ISTREAM68_NULL_H_

#include "istream68.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @name      Null stream
 *  @ingroup   file68_istream68_devel
 *
 *    Implements a null istream_t.
 *
 *    Null stream does nothing but checking some trivial errors (like
 *    access without opening) and dealing with a virtual stream position.
 *    The null stream length is the position the highest byte that
 *    has been either read or write. The length is resetted at open.
 *    It implies that stream length can be retrieved by the istream_length()
 *    function after istream_close() call.
 *
 *  @{
 */

/** Creates a null stream.
 *
 *  @param  name   Optionnal name
 *  @return stream
 *  @retval 0 on error
 *
 *  @note   filename is prefixed by "null://".
 */
istream_t * istream_null_create(const char * name);

/**@}*/

#ifdef __cplusplus
}
#endif


#endif /* #define _ISTREAM68_NULL_H_ */
