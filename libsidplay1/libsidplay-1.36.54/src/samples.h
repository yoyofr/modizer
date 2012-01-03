//
// /home/ms/source/sidplay/libsidplay/emu/RCS/samples.h,v
//

#ifndef SIDPLAY1_SAMPLES_H
#define SIDPLAY1_SAMPLES_H


#include "mytypes.h"
#include "myendian.h"


extern void sampleEmuCheckForInit();
extern void sampleEmuInit();          // precalculate tables + reset
extern void sampleEmuReset();         // reset some important variables

extern sbyte (*sampleEmuRout)();


#endif  /* SIDPLAY1_SAMPLES_H */
