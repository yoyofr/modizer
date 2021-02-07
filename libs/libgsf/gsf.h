#include "psftag.h"

#ifndef BYTE
typedef unsigned char BYTE;
#endif


extern void gsfDisplayError (char *, ...);

extern BOOL IsTagPresent (BYTE *);
extern BOOL IsValidGSF (BYTE *);
extern void setupSound(void);
extern int GSFRun(char *);
extern void GSFClose(void) ;
extern BOOL EmulationLoop(void);

