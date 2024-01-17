/**
 * @ingroup   lib_emu68
 * @file      emu68/type68.h
 * @brief     Type definitions header.
 * @date      1999/03/13
 * @author    Benjamin Gerard
 */

/* Copyright (c) 1998-2016 Benjamin Gerard */

#ifndef EMU68_TYPE68_H
#define EMU68_TYPE68_H

#ifdef HAVE_STDINT_H
# include <stdint.h>
#else
typedef  int32_t  int_fast32_t;         /**< @nodoc */
typedef uint32_t uint_fast32_t;         /**< @nodoc */
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

/**
 * @defgroup  lib_emu68_types  Type definitions
 * @ingroup   lib_emu68
 *
 *   Definition of types used by EMU68 and SC68 related projects.
 *
 * @{
 */

/**
 * @name Fixed size integer types.
 * @{
 */

typedef  uint8_t  u8;      /**< Must be an unsigned 8 bit integer.  */
typedef   int8_t  s8;      /**< Must be a    signed 8 bit integer.  */

typedef uint16_t u16;      /**< Must be an unsigned 16 bit integer. */
typedef  int16_t s16;      /**< Must be a    signed 16 bit integer. */

typedef uint32_t u32;      /**< Must be an unsigned 32 bit integer. */
typedef  int32_t s32;      /**< Must be a    signed 32 bit integer. */

typedef uint64_t u64;      /**< Must be an unsigned 64 bit integer. */
typedef  int64_t s64;      /**< Must be a    signed 64 bit integer. */

/**
 * @}
 */

#ifndef INT68_BITS

/* Commonly used int (should be fastest at least 32 bit type). */
typedef  int_fast32_t int68_t;     /**< fast   signed integer type. */
typedef       int68_t sint68_t;    /**< fast   signed integer type. */
typedef uint_fast32_t uint68_t;    /**< fast unsigned integer type. */

#elif  INT68_BITS == 32

typedef           s32 int68_t;     /**< 32bit   signed integer type. */
typedef           s32 sint68_t;    /**< 32bit   signed integer type. */
typedef           u32 uint68_t;    /**< 32bit unsigned integer type. */

#elif  INT68_BITS == 64

typedef           s64 int68_t;     /**< 64bit   signed integer type. */
typedef           s64 sint68_t;    /**< 64bit   signed integer type. */
typedef           u64 uint68_t;    /**< 64bit unsigned integer type. */

#else
# error "invalid define INT68_BITS"
#endif

typedef   unsigned int    uint_t; /**< natural unsigned int.           */
typedef       uint68_t cycle68_t; /**< Type for cycle counters.        */
typedef        int68_t  addr68_t; /**< Type for 68k memory addressing. */
typedef struct  io68_s    io68_t; /**< IO chip instance type.          */
typedef struct emu68_s   emu68_t; /**< 68k emulator instance type.     */

/** 68k memory access function. */
typedef void (*memfunc68_t)(emu68_t * const);

/**
 * @}
 */

#endif
