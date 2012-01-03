//
// /home/ms/source/sidplay/libsidplay/emu/RCS/envelope.h,v
//

#ifndef SIDPLAY1_ENVELOPE_H
#define SIDPLAY1_ENVELOPE_H


#include "mytypes.h"

static const ubyte ENVE_STARTATTACK = 0;
static const ubyte ENVE_STARTRELEASE = 2;

static const ubyte ENVE_ATTACK = 4;
static const ubyte ENVE_DECAY = 6;
static const ubyte ENVE_SUSTAIN = 8;
static const ubyte ENVE_RELEASE = 10;
static const ubyte ENVE_SUSTAINDECAY = 12;
static const ubyte ENVE_MUTE = 14;

static const ubyte ENVE_STARTSHORTATTACK = 16;
static const ubyte ENVE_SHORTATTACK = 16;

static const ubyte ENVE_ALTER = 32;


#endif  /* SIDPLAY1_ENVELOPE_H */
