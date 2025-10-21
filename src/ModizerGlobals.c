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
    {230,76,153},  //highlight bar color
    {255,255,255},  //Header color
    {82,90,145},  //Header BG color
    {82,90,145},  //frame color 1
    {29,40,61},  //frame color 2
    {0xF0,0xF0,0xF0},  //Line nb color 1
    {0xF0,0xF0,0xF0},  //Line nb color 1 H
    {0xF0,0xF0,0x00},  //Line nb color 2
    {0xF0,0xF0,0x00},  //Line nb color 2 H
    {0xFF,0xFF,0xFF},  //Note color
    {0xFF,0xFF,0xFF},  //Note color H
    {0x80,0xE0,0xFF},  //Instrument color
    {0x80,0xE0,0xFF},  //Instrument color H
    {0x80,0xFF,0x80},  //Volume color
    {0x80,0xFF,0x80},  //Volume color H
    {0xFF,0x80,0xE0},  //Effect nb color
    {0xFF,0x80,0xE0},  //Effect nb color H
    {0xFF,0xE0,0x80},  //Effect value color
    {0xFF,0xE0,0x80},  //Effect value color H
    {100,50,150},  //volume bar low color
    {200,12,150},  //volume bar high color
    MDZ_THEMEFLAG_VolDep|MDZ_THEMEFLAG_BordersLR,              //volume bar mode
    "Std",
};

t_pattern_colortheme modpat_colorAlt={
    {0x9E,0xF7,0xAA},  //highlight bar color
    {255,255,255},  //Header color
    {0x0F/2,0x3F/2,0xF7/2},  //Header BG color
    {0x0F/2,0x3F/2,0xF7/2},  //frame color 1
    {0x66/2,0x69/2,0xFA/2},  //frame color 2
    {0xFF,0xFF,0xFF},  //Line nb color 1
    {0xFF,0xFF,0xFF},  //Line nb color 1 H
    {0xE5,0x98,0xF5},  //Line nb color 2
    {0xE5,0x98,0xF5},  //Line nb color 2 H
    {0xF0,0xF0,0xF0},  //Note color
    {0xF0,0xF0,0xF0},  //Note color H
    {0xF0,0xF0,0xF0},  //Instrument color
    {0xF0,0xF0,0xF0},  //Instrument color H
    {0xF0,0xF0,0xF0},  //Volume color
    {0xF0,0xF0,0xF0},  //Volume color H
    {0xF0,0xF0,0xF0},  //Effect nb color
    {0xF0,0xF0,0xF0},  //Effect nb color H
    {0xF0,0xF0,0xF0},  //Effect value color
    {0xF0,0xF0,0xF0},  //Effect value color H
    {150,50,100},  //volume bar low color
    {150,12,200},  //volume bar high color
    MDZ_THEMEFLAG_VolDep|MDZ_THEMEFLAG_BordersLR|MDZ_THEMEFLAG_BordersTop,              //volume bar mode
    "Alt",
};

t_pattern_colortheme modpat_colorScreamTracker={
    {72*2,65*2,61*2},  //highlight bar color
    {250,224,146},  //Header color
    {79,70,44},  //Header BG color
    {163,147,93},  //frame color 1
    {163,147,93},  //frame color 2
    {79,70,44},  //Line nb color 1
    {79,70,44},  //Line nb color 1 H
    {79,70,44},  //Line nb color 2
    {79,70,44},  //Line nb color 2 H
    {154,150,147},  //Note color
    {154,150,147},  //Note color H
    {154,150,147},  //Instrument color
    {154,150,147},  //Instrument color H
    {154,150,147},  //Volume color
    {154,150,147},  //Volume color H
    {154,150,147},  //Effect nb color
    {154,150,147},  //Effect nb color H
    {154,150,147},  //Effect value color
    {154,150,147},  //Effect value color H
    {0,40,0},  //volume bar low color
    {80,176,50},  //volume bar high color
    MDZ_THEMEFLAG_VolDep,              //volume bar mode
    "ST3",
};

t_pattern_colortheme modpat_colorProTracker={
    {123,123,123},  //highlight bar color
    {0,0,0},  //Header color
    {123,123,123},  //Header BG color
    {123,123,123},  //frame color 1
    {123,123,123},  //frame color 2
    {52,66,224},  //Line nb color 1
    {0,0,0},  //Line nb color 1 H
    {52,66,224},  //Line nb color 2
    {0,0,0},  //Line nb color 2 H
    {52,66,224},  //Note color
    {0,0,0},  //Note color H
    {52,66,224},  //Instrument color
    {0,0,0},  //Instrument color H
    {52,66,224},  //Volume color
    {0,0,0},  //Volume color H
    {52,66,224},  //Effect nb color
    {0,0,0},  //Effect nb color H
    {52,66,224},  //Effect value color
    {0,0,0},  //Effect value color H
    {64,255,64},  //volume bar low color
    {255,64,32},  //volume bar high color
    MDZ_THEMEFLAG_VolDep|MDZ_THEMEFLAG_BordersLR|MDZ_THEMEFLAG_NoFillLineNb|MDZ_THEMEFLAG_HighlightZoom,              //volume bar mode
    "Pro",
};

t_pattern_colortheme modpat_colorImpulseTracker={
    {86*2,66*2,61*2},  //highlight bar color
    {00,00,00},  //Header color
    {120,91,72},  //Header BG color
    {177,151,125},  //frame color 1
    {177,151,125},  //frame color 2
    {0,0,0},  //Line nb color 1
    {0,0,0},  //Line nb color 1 H
    {0,0,0},  //Line nb color 2
    {0,0,0},  //Line nb color 2 H
    {92,152,83},  //Note color
    {92,152,83},  //Note color H
    {70,127,97},  //Instrument color
    {70,127,97},  //Instrument color H
    {137,137,137},  //Volume color
    {137,137,137},  //Volume color H
    {177,151,125},  //Effect nb color
    {177,151,125},  //Effect nb color H
    {74,133,103},  //Effect value color
    {74,133,103},  //Effect value color H
    {51,51,129},  //volume bar low color
    {251,251,96},  //volume bar high color
    MDZ_THEMEFLAG_VolDep,              //volume bar mode
    "IT",
};


t_pattern_colortheme *modpat_curTheme;
t_pattern_colortheme *modpat_themesList[]={
    &modpat_colorStd,
    &modpat_colorAlt,
    &modpat_colorScreamTracker,
    &modpat_colorProTracker,
    &modpat_colorImpulseTracker
};
int modpat_themesNb=sizeof(modpat_themesList)/sizeof(modpat_themesList[0]);




