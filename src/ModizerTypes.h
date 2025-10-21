/*
 *  ModizerTypes.h
 *  modizer
 *
 *  Created by yoyofr on 19/10/2025.
 *  Copyright 2025 __YoyoFR / Yohann Magnien__. All rights reserved.
 *
 */

//#define LOAD_PROFILE
#ifndef MODIZER_TYPES_H
#define MODIZER_TYPES_H

typedef struct {
    uint8_t highlight_bar[3];
    uint8_t header_col[3];
    uint8_t headerBG_col[3];  //only draw header BG is col is different from frame_base 1
    uint8_t frame_base1[3];
    uint8_t frame_base2[3];
    uint8_t lineNb_col1[3];
    uint8_t lineNb_col1H[3];
    uint8_t lineNb_col2[3];
    uint8_t lineNb_col2H[3];
    uint8_t note_col[3];
    uint8_t note_colH[3];
    uint8_t instrument_col[3];
    uint8_t instrument_colH[3];
    uint8_t volume_col[3];
    uint8_t volume_colH[3];
    uint8_t effect_col[3];
    uint8_t effect_colH[3];
    uint8_t param_col[3];
    uint8_t param_colH[3];
    uint8_t volume_barL[3];
    uint8_t volume_barH[3];
    uint8_t theme_flag;  //
    const char *themeName;
} t_pattern_colortheme;

enum t_pattern_theme_flag {
  MDZ_THEMEFLAG_VolDep=1<<0,
  MDZ_THEMEFLAG_BordersLR=1<<1,
  MDZ_THEMEFLAG_BordersTop=1<<2,
  MDZ_THEMEFLAG_NoFillLineNb=1<<3,
  MDZ_THEMEFLAG_HighlightZoom=1<<4,
};

#ifdef __cplusplus
#define MDZ_EXTERN extern "C"
#else
#define MDZ_EXTERN extern
#endif


MDZ_EXTERN t_pattern_colortheme modpat_colorStd,modpat_colorAlt;
MDZ_EXTERN t_pattern_colortheme *modpat_curTheme;
MDZ_EXTERN t_pattern_colortheme *modpat_curTheme;
MDZ_EXTERN t_pattern_colortheme *modpat_themesList[];
MDZ_EXTERN int modpat_themesNb;

#endif

