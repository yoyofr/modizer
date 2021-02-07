/**
 * @ingroup   emu68_core_devel
 * @file      emu68/inst68.h
 * @author    Benjamin Gerard <ben@sashipa.com>
 * @date      1999/03/13
 * @brief     68k arithmetic and logical instruction header.
 *
 * $Id: inst68.h 503 2005-06-24 08:52:56Z loke $
 *
 */

/* Copyright (C) 1998-2003 Benjamin Gerard */

#ifndef _INST68_H_
#define _INST68_H_

#include "type68.h"

#ifdef __cplusplus
extern "C" {
#endif


/** @addtogroup   emu68_core_devel
 *
 *   68k arithmetic and logical instruction emulation.
 *   Logical and arithmetical instructions are emulated with functions
 *   instead of macros to prevent from excessive code size generation that
 *   hurt processor cache. By the way these functions could easily be
 *   written in assembler and improve emulator execution time. All these
 *   functions work with 32 bit values. To perform other size instructions,
 *   operands must be left shifted in order to locate operands most
 *   signifiant bit at the 31st bit.
 *
 *  @{
 */

/** @name Arithmetic instruction functions
 *  @{
 */

/** Addition @return a+b+c */
s32 add68(s32 a, s32 b, s32 c);

/** Subtraction @return a-b-c */
s32 sub68(s32 a, s32 b, s32 c);

/** Signed multiplication @return (a>>16)*(b>>16) */
s32 muls68(s32 a, s32 b);

/** Unsigned multiplication @return (a>>16)*(b>>16) */
s32 mulu68(u32 a, u32 b);

/** Signed divide @return MSW:a%(b>>16) LSW:a/(b>>16) */
s32 divs68(s32 a, s32 b);

/** Unsigned divide @return MSW:a%(b>>16) LSW:a/(b>>16) */
s32 divu68(u32 a, u32 b);

/**@}*/


/** @name Logical instruction functions
 *  @{
 */

/** Bitwise AND @return a&b */
s32 and68(u32 a, u32 b);

/** Bitwise OR @return a|b */
s32 orr68(u32 a, u32 b);

/** Bitwise exclusif OR @return a^b */
s32 eor68(u32 a, u32 b);

/** First complement @return ~s */
s32 not68(s32 s);

/**@}*/

/**
 *@}
 */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _INST68_H_ */
