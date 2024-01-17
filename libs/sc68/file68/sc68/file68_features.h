/**
 * @ingroup  file68_lib
 * @file     sc68/file68_features.h
 * @brief    Features and options for this build suports.
 * @author   Benjamin Gerard
 * @date     2011-09-09
 */

/* Copyright (c) 1998-2016 Benjamin Gerard */

#ifndef FILE68_FEATURES_H
#define FILE68_FEATURES_H

#define FILE68_UNICE68
#define FILE68_Z
#undef FILE68_CURL
#undef FILE68_AO

enum {
  FILE68_SPR_MIN = 6250,
  FILE68_SPR_DEF = 48000,
  FILE68_SPR_MAX = 125166
};

#endif
