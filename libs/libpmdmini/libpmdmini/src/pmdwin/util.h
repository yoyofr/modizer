//=============================================================================
//			Utility Functions
//				Programmed by C60
//=============================================================================

#ifndef UTIL_H
#define UTIL_H

#include "portability_fmpmd.h"

int32_t ismbblead(uint32_t c);
char *tab2spc(char *dest, const char *src, int32_t tabcolumn);
char *delesc(char *dest, const char *src);
char *zen2tohan(char *dest, const char *src);

#endif	// UTIL_H
