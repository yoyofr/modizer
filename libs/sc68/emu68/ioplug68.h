/**
 * @ingroup   emu68_ioplug_devel
 * @file      emu68/ioplug68.h
 * @date      1999/03/13
 * @brief     68k IO plugger header.
 * @author    Benjamin Gerard <ben@sashipa.com>
 *
 * $Id: ioplug68.h 503 2005-06-24 08:52:56Z loke $
 *
 */
 
/* Copyright (C) 1998-2003 Benjamin Gerard */

#ifndef _IOPLUG68_H_
#define _IOPLUG68_H_

#include "struct68.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup  emu68_ioplug_devel  68k IO plugger
 * @ingroup   emu68_devel
 * @brief     IO plugging and mapping facilities.
 *
 *   Provide functions for warm plugging/unplugging of IO
 *   chipset. Limitations are explained in 
 *   @ref emu68_mem68_devel "68k memory and IO manager"
 *   detailed description.
 *
 * @{
 */

/** Unplug all IO.
 *
 *    Process EMU68ioplug_unplug() function for all pluged IO.
 *
 */
void EMU68ioplug_unplug_all(void);

/**  Unplug an IO.
 *
 *     The EMU68ioplug_unplug() function removes an IO from pluged IO list
 *     and reset memory access handler for its area.
 *
 *  @param   io  Address of IO structure to unplug.
 *
 *  @return   error-code
 *  @retval   0   Success
 *  @retval   <0  Error (probably no matching IO)
 */
int EMU68ioplug_unplug(io68_t *io);

/**  Plug an IO.
 *
 *     The EMU68ioplug() function add an IO to pluged IO list and add
 *     suitable memory access handlers.
 *
 *  @param  io  Address of IO structure to plug.
 */
void EMU68ioplug(io68_t *io);

/**
 *@}
 */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _IOPLUG68_H_ */
