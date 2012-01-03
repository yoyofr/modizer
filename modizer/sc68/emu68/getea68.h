/**
 * @ingroup   emu68_core_devel
 * @file      emu68/getea68.h
 * @author    Benjamin Gerard <ben@sashipa.com>
 * @date      1999/03/13
 * @brief     68k effective address calculation function table.
 *
 * $Id: getea68.h 503 2005-06-24 08:52:56Z loke $
 *
 */

/* Copyright (C) 1998-2003 Benjamin Gerard */

#ifndef _GETEA68_H_
#define _GETEA68_H_

#include "type68.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup  emu68_core_devel
 *
 * @{
 */

/** @name Effective address calculation tables.
 *
 *   The get_ab[bwl] tables are used by EMU68 to calculate operand
 *   effective address. Each of them is indexed by operand addressing
 *   mode. Each entry is a pointer to a function which do everything
 *   neccessary to update processor state (e.g. address register
 *   increment or decrement). Function parameter is register number
 *   coded in the three first bit (0 to 2) of 68k opcode. When the
 *   mode is 7, register parameter is used as an index in a second
 *   level function table for extended addressing mode.
 *
 * @{
 */

/** Byte operand effective address calculation function table. */
extern u32 (*get_eab[8])(u32 reg);

/** Word operand effective address calculation function table. */
extern u32 (*get_eaw[8])(u32 reg);

/** Long operand effective address calculation function table. */
extern u32 (*get_eal[8])(u32 reg);

/**@}*/

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _GETEA68_H_ */

