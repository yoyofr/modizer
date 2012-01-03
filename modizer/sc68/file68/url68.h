/**
 * @ingroup   file68_url68_devel
 * @file      file68/url68.h
 * @author    Benjamin Gerard <ben@sashipa.com>
 * @date      2003/10/28
 * @brief     sc68 URL header.
 *
 * $Id: url68.h 503 2005-06-24 08:52:56Z loke $
 */

/* Copyright (C) 1998-2003 Benjamin Gerard */

#ifndef _URL68_H_
#define _URL68_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "istream68.h"

/** @defgroup  file68_url68_devel sc68 URL
 *  @ingroup   file68_devel
 *
 *     Provides functions for sc68 URL manipulation.
 *
 *  @{
 */


/** Get protocol from URL string.
 *
 *  @param  protocol  buffer to store URL
 *  @param  size      protocol buffer size
 *  @param  url       UR string
 *
 *  @return error code
 *  @retval 0     success
 *  @retval -1    failure
 */
int url68_get_protocol(char * protocol, int size, const char *url);

/** Test if a protocol is local.
 *
 *  @note Currently the url68_local_protocol() function tests if
 *        protocol is local and seekable.
 *    
 *  @param  protocol  protocol to test
 *
 *  @return  1  protocol is local (0,"","FILE","LOCAL","NULL")
 *  @return  0  protocol may be remote
 */
int url68_local_protocol(const char * protocol);

/** Create a stream for an URL.
 *
 *  Here is a list of protocols currently supported:
 *  - file:// local file
 *  - local:// alias for file://
 *  - stdin:// standard input
 *  - stdout:// standard output
 *  - stderr:// standard error
 *  - null:// null (zero) file
 *  - protocol supported by curl (HTTP, HTTPS, FTP, GOPHER,
 *       DICT,  TELNET,  LDAP  or FILE)
 *
 *  @param  url   URL or file
 *  @param  mode  open mode (1:read, 2:write).
 *
 *  @return stream
 *  @retval 0 error
 */
istream_t * url68_stream_create(const char * url, int mode);

/**
 *@}
 */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _URL68_H_ */
