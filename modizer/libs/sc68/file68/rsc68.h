/**
 * @ingroup   file68_rsc68_devel
 * @file      file68/rsc68.h
 * @author    Benjamin Gerard <ben@sashipa.com>
 * @date      1998/10/07
 * @brief     resources header.
 *
 * $Id: rsc68.h 503 2005-06-24 08:52:56Z loke $
 *
 */

/* Copyright (C) 1998-2003 Benjamin Gerard */

#ifndef _RSC68_H_
#define _RSC68_H_

#include "istream68.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup   file68_rsc68_devel  Resource files
 *  @ingroup    file68_devel
 *
 *    Provides resource access functions.
 *
 *  @{
 */

/** Resource type. */
typedef enum
{
  rsc68_replay,       /**< 68000 external replay.           */
  rsc68_config,       /**< Config file.                     */
  rsc68_sample,       /**< sc68 sample files.               */
  rsc68_dll,          /**< sc68 dynamic library.            */
  rsc68_music,        /**< sc68 music files.                */

  rsc68_last,         /**< last valid type.                 */
} rsc68_t;

/** Resource specific information. */
typedef struct {
  rsc68_t type; /**< Resource type. */
  /** Information depending on type. */
  union {
    /** rsc68_music resource information. */
    struct {
      int track; /**< Force this track (-1:n/a).              */
      int loop;  /**< Force track(s) default loop (-1:n/a).   */
      int time;  /**< Force track(s) time in second (-1:n/a). */
    } music;
  } data;

} rsc68_info_t;

/** Resource handle function type. */ 
typedef istream_t * (*rsc68_handler_t)(rsc68_t type,
				       const char * name,
				       int mode,
				       rsc68_info_t * info);

/** Set shared resource path.
 *
 *    The rsc68_set_share() function set the shared resource
 *    path. The path will be duplicate by SC68strdup(). If path is
 *    null the current path is freed.
 *
 *    @param  path  New shared resource path (0 for free).
 *
 *    @return new path (duplicated string).
 *    @retval 0 error (except for freeing)
 *
 */
const char * rsc68_set_share(const char *path);

/** Set user resource path.
 *
 *    The rsc68_set_user() function set the user resource path. The
 *    path will be duplicate by SC68strdup(). If path is null the
 *    current path is freed.
 *
 *    @param  path  New user resource path (0 for free).
 *
 *    @return new path (duplicated string).
 *    @retval 0 error (except for freeing)
 */
const char * rsc68_set_user(const char *path);

/** Set sc68 local music database path.
 *
 *    The rsc68_set_music() function set the local music database
 *    path. The path will be duplicate by SC68strdup(). If path is
 *    null the current path is freed.
 *
 *    @param  path  New local music database path (0 for free).
 *
 *    @return new path (duplicated string).
 *    @retval 0 error (except for freeing)
 */
const char * rsc68_set_music(const char *path);

/** Set sc68 remote music database path.
 *
 *    The rsc68_set_remote_music() function set the remote music
 *    database path. The path will be duplicate by SC68strdup(). If
 *    path is null the current path is freed.
 *
 *    @param  path  New remote music database path (0 for free).
 *
 *    @return new path (duplicated string).
 *    @retval 0 error (except for freeing)
 */
const char * rsc68_set_remote_music(const char *path);

/** Get resource pathes.
 *
 *  @param  share   Get the shared resource path (0 to ignore).
 *  @param  user    Get the user resource path (0 to ignore).
 *  @param  lmusic  Get the local music database path (0 to ignore).
 *  @param  rmusic  Get the remote music database path (0 to ignore).
 */
void rsc68_get_path(const char ** share,
		    const char ** user,
		    const char ** lmusic,
		    const char ** rmusic);

/** Set/Get resource handler.
 *
 *    The rsc68_set_handler() function set the current resource
 *    handler.  If 0 is given as fct parameter the function does not
 *    set the handler.  In all case the function returns the current
 *    handler. See below for more information about the default
 *    resource handler.
 *
 *    @par Resource handler
 *    The resource handler is a function called by the rsc68_open()
 *    function.  Some preliminary tests has already been performed. So
 *    the handler can assume that the name is not a NULL pointer and
 *    the mode is valid (either 1:reading or 2:writing). The resource
 *    handler @b must return an @b already opened istream_t or 0 in
 *    error case.
 *
 *    @par Default resource handler
 *    The Default handler use a the istream_file_create() function.
 *    - If open mode is 2 (write mode) the default handler use the user
 *    resource path.
 *    - If open mode is 1 (read mode) the default handler tries in this
 *    order the user resource path and the shared resource path.
 *
 *    @param  fct  New resource handler (0 for reading current value).
 *    @return previous value.
 */
rsc68_handler_t rsc68_set_handler(rsc68_handler_t fct);

/** Open a resource in given mode.
 *
 *    The function rsc68_open() function opens an istream_t to
 *    access a resource.
 *
 *   @param  type    Type of resource to open.
 *   @param  name    Name of resource.
 *   @param  mode    1:read-access, 2:write-access.
 *   @param  info    Get additionnal info (depends on type)
 *
 *   @return  already opened istream_t stream.
 *   @retval  0 error.
 *
 * @see rsc68_set_handler() for more info on resource handler.
 */
istream_t * rsc68_open(rsc68_t type,
		       const char *name,
		       int mode,
		       rsc68_info_t * info);

/** Open a resource URL in given mode.
 *
 *   @param  url     Any valid rsc68://type/ URL
 *   @param  mode    1:read-access, 2:write-access.
 *   @param  info    Get additionnal info (depends on type)
 *
 *   @return  already opened istream_t stream.
 *   @retval  0 error.
 *
 * @see rsc68_open()
 */
istream_t * rsc68_open_url(const char *url,
			   int mode,
			   rsc68_info_t * info);


/** Get music parameters from string.
 *
 *   The rsc68_get_music_params() function parses the str string and
 *   stores track loop and time in the info struct. If it successes the
 *   info::type is set to rsc68_music else it is set rsc68_last.
 *
 *   @param  info  info to fill (0:parse only)
 *   @param  str   URL part containing music parameter ":track:loop:time"
 *
 *   @return  Parsing stop position
 *   @retval  str   on invalid string
 *   @retval  >str  next '/' or end of string
 */
const char * rsc68_get_music_params(rsc68_info_t *info,
				    const char * str);

/**
 *@}
 */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _RSC68_H_ */
