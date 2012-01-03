/**
 * @ingroup   emu68_devel
 * @file      cc68.c
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      1999/03/13
 * @brief     Code condition function table.
 * @version   $Id: cc68.c 503 2005-06-24 08:52:56Z loke $
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */
 
#include "struct68.h"
#include "srdef68.h"
#include "cc68.h"

extern reg68_t reg68;

/***************************
* Condition code functions *
***************************/

static int is_f( void ) { return 0; }
static int is_ls( void ) { return !!IS_LS(reg68.sr); }
static int is_cs( void ) { return !!IS_CS(reg68.sr); }
static int is_eq( void ) { return !!IS_EQ(reg68.sr); }
static int is_vs( void ) { return !!IS_VS(reg68.sr); }
static int is_mi( void ) { return !!IS_MI(reg68.sr); }
static int is_lt( void ) { return !!IS_LT(reg68.sr); }
static int is_le( void ) { return !!IS_LE(reg68.sr); }

int (*is_cc[8])(void ) =
{
  is_f, is_ls, is_cs, is_eq,
  is_vs, is_mi, is_lt, is_le,
};
