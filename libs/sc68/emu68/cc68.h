/**
 * @ingroup   emu68_core_devel
 * @file      emu68/cc68.h
 * @author    Benjamin Gerard <ben@sashipa.com>
 * @date      1999/03/13
 * @brief     Condition code function table header.
 *
 * $Id: cc68.h 503 2005-06-24 08:52:56Z loke $
 *
 */

/* Copyright (C) 1998-2003 Benjamin Gerard */

#ifndef _CC68_H_
#define _CC68_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup   emu68_core_devel
 *  @{
 */

/** Code condition testing function table.
 *
 *    Condition code function table is used by EMU68 for conditional
 *    instructions emulation including Bcc, Scc and DCcc. The 4 condition
 *    bits of instruction is used as index to call corresponding test
 *    function. Each test function performs suitable SR bits operations and
 *    return 0 if condition is false and other if condition is satisfied.
 */
extern int (*is_cc[8])(void);

/**
 *@}
 */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _CC68_H_ */
