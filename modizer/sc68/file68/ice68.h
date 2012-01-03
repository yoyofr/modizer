/**
 * @ingroup file68_ice68_devel
 * @file    file68/ice68.h
 * @author  benjamin gerard
 * @date    2003/09/06
 * @brief   ICE loader header.
 *
 * $Id: ice68.h 503 2005-06-24 08:52:56Z loke $
 */

#ifndef _ICE68_H_
#define _ICE68_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "istream68.h"

/** @defgroup  file68_ice68_devel  ICE loader support.
 *  @ingroup   file68_devel
 *
 *    Provides functions for loading ICE stream.
 *
 *  @{
 */

/** Test ice file header magic header.
 *
 *  @param  buffer  Buffer containing at least 12 bytes from ice header.
 *
 *  @retval  1  buffer seems to be iceped..
 *  @retval  0  buffer is not iceped.
 */
int ice_is_magic(const void * buffer);

/** Load an iced stream.
 *
 *    The ice_load() function loads and depack an ice packed file from a
 *    stream and returns a allocate buffer with unpacked data.
 *
 * @param  is     Stream to load (must be opened in read mode).
 * @param  ulen   Pointer to save uncompressed size.
 *
 * @return Pointer to the unpressed data buffer.
 * @retval 0 Error
 */
void *ice_load(istream_t *is, int *ulen);

/** Load an iced file.
 *
 * @param  fname  File to load.
 * @param  ulen   Pointer to save uncompressed size.
 *
 * @return Pointer to the unpressed data buffer.
 * @retval 0 Error
 *
 * @see ice_load()
 */
void *ice_load_file(const char *fname, int *ulen);

/**
 *@}
 */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _ICE68_H_ */

