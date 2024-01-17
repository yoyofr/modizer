/**
 * @ingroup   lib_emu68
 * @file      emu68/emu68_api.h
 * @brief     emu68 library export header.
 * @author    Benjamin Gerard
 * @date      2009/02/10
 */

/* Copyright (c) 1998-2016 Benjamin Gerard */

#ifndef EMU68_EMU68_API_H
#define EMU68_EMU68_API_H

#ifndef EMU68_EXTERN
# include "../sc68/sc68.h"
# define EMU68_EXTERN SC68_EXTERN
#endif

#ifndef EMU68_API
# define EMU68_API EMU68_EXTERN
#endif

#endif
