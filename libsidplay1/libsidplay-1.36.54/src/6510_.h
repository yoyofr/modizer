//
// /home/ms/source/sidplay/libsidplay/emu/RCS/6510_.h,v
//

#ifndef SIDPLAY1_MPU_6510__H
#define SIDPLAY1_MPU_6510__H


#include "mytypes.h"


extern bool sidKeysOff[32];
extern bool sidKeysOn[32];
extern ubyte optr3readWave;
extern ubyte optr3readEnve;
extern ubyte* c64mem1;
extern ubyte* c64mem2;

extern bool c64memAlloc();
extern bool c64memFree();
extern void c64memClear();
extern void c64memReset(int clockSpeed, ubyte randomSeed);
extern ubyte c64memRamRom(uword address);

extern void initInterpreter(int memoryMode);
extern bool interpreter(uword pc, ubyte ramrom, ubyte a, ubyte x, ubyte y);


#endif  /* SIDPLAY1_MPU_6510__H */
