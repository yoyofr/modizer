/**
 * @ingroup   io68_mfp_devel
 * @file      io68/mfp_io.h
 * @author    Benjamin Gerard <ben@sashipa.com>
 * @date      1999/03/20
 * @brief     MFP-68901 IO plugin header.
 *
 * $Id: mfp_io.h 503 2005-06-24 08:52:56Z loke $
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#ifndef _MFP_IO_H_
#define _MFP_IO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "struct68.h"

/** @addtogroup  io68_mfp_devel
 *  @{
 */

/** @name MFP-68901 (Atari-ST timers) IO plugin
 *  @{
 */

/** MFP-68901 IO plugin instance. */

extern io68_t mfp_io;

/**@}*/

/**
 *@}
 */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _MFP_IO_H_ */
