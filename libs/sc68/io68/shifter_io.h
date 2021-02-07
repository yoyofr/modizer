/**
 * @ingroup   io68_shifter_devel
 * @file      io68/shifter_io.h
 * @author    Benjamin Gerard <ben@sashipa.com>
 * @date      1999/06/10
 * @brief     shifter IO plugin header.
 *
 * $Id: shifter_io.h 503 2005-06-24 08:52:56Z loke $
 */

/* Copyright (C) 1998-2003 Ben(jamin) Gerard */

#ifndef _SHIFTER_IO_H_
#define _SHIFTER_IO_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup  io68_shifter_devel  Atari-ST shifter emulator
 *  @ingroup   io68_devel
 *
 *    Atari-ST shifter emulator is limited to 50/60Hz detection.
 *    It is used by some player to adapt the replay speed.
 *    By default this shifter always claims to be in 50hz.
 *
 *  @todo Include a write to this register in sc68 when replay speed
 *        is 60Hz. 
 *  @{
 */

#include "struct68.h"

/** @name Atari-ST shifter IO plugin
 *  @{
 */

/** Shifter IO plugin instance. */
extern io68_t shifter_io;

/**@}*/

/**
 *@}
 */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _SHIFTER_IO_H_ */
