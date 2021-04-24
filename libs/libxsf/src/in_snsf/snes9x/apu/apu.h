/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#pragma once

#include "../snes9x.h"

bool S9xInitAPU();
void S9xDeinitAPU();
void S9xResetAPU();
uint8_t S9xAPUReadPort(int);
void S9xAPUWritePort(int, uint8_t);
void S9xAPUEndScanline();
void S9xAPUSetReferenceTime(int32_t);
void S9xAPUTimingSetSpeedup(int);

bool S9xInitSound(int);
bool S9xOpenSoundDevice();

bool S9xSyncSound();
int S9xGetSampleCount();
void S9xSetSoundControl(uint8_t);
void S9xSetSoundMute(bool);
bool S9xMixSamples(uint8_t *, int);
