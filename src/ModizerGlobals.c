/*
 *  ModizerGlobals.c
 *  modizer
 *
 *  Created by yoyofr on 19/10/2025.
 *  Copyright 2025 __YoyoFR / Yohann Magnien__. All rights reserved.
 *
 */

#include <stdlib.h>
#include <string.h>

#include "ModizerConstants.h"
#include "ModizerTypes.h"

//#define R_BASE1 0xEC
//#define G_BASE1 0xAD
//#define B_BASE1 0xF0
//
//#define R_BASE2 0xB0
//#define G_BASE2 0x90
//#define B_BASE2 0xFF


t_pattern_colortheme modpat_colorStd={
    {100,50,150},  //volume bar color
    {230,76,153},  //highlight bar color
    {0xEC,0xAD,0xF0},  //frame color 1
    {0xB0,0x90,0xFF},  //frame color 2
    {0xF0,0xF0,0xF0},  //Line nb color 1
    {0xF0,0xF0,0x00},  //Line nb color 2
    {0xFF,0xFF,0xFF},  //Note color
    {0x80,0xE0,0xFF},  //Instrument color
    {0x80,0xFF,0x80},  //Volume color
    {0xFF,0x80,0xE0},  //Effect nb color
    {0xFF,0xE0,0x80}  //Effect value color
};

t_pattern_colortheme modpat_colorAlt={
    {0xFA,0xAA,0x20},  //volume bar color
    {0x9E,0xF7,0xAA},  //highlight bar color
    {0x0F,0x3F,0xF7},  //frame color 1
    {0x66,0x69,0xFA},  //frame color 2
    {0xFF,0xFF,0xFF},  //Line nb color 1
    {0xE5,0x98,0xF5},  //Line nb color 2
    {0xF0,0xF0,0xF0},  //Note color
    {0xF0,0xF0,0xF0},  //Instrument color
    {0xF0,0xF0,0xF0},  //Volume color
    {0xF0,0xF0,0xF0},  //Effect nb color
    {0xF0,0xF0,0xF0}  //Effect value color
};


t_pattern_colortheme *modpat_curTheme;

