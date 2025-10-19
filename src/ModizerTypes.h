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
    uint8_t volume_bar[3];
    uint8_t highlight_bar[3];
    uint8_t frame_base1[3];
    uint8_t frame_base2[3];
    uint8_t lineNb_col1[3];
    uint8_t lineNb_col2[3];
    uint8_t note_col[3];
    uint8_t instrument_col[3];
    uint8_t volume_col[3];
    uint8_t effect_col[3];
    uint8_t param_col[3];
} t_pattern_colortheme;

#ifdef __cplusplus
#define MDZ_EXTERN extern "C"
#else
#define MDZ_EXTERN extern
#endif


MDZ_EXTERN t_pattern_colortheme modpat_colorStd,modpat_colorAlt;
MDZ_EXTERN t_pattern_colortheme *modpat_curTheme;

#endif

