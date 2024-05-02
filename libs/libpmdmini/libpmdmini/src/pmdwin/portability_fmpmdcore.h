#ifndef PORTABILITY_FMPMDCORE_H
#define PORTABILITY_FMPMDCORE_H

#include "portability_fmpmd.h"
#include "opna.h"



#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif


#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif



typedef struct stereosampletag
{
	FM::Sample left;
	FM::Sample right;
} StereoSample;

#if defined _WIN32

#pragma pack( push, enter_include1 )
#pragma pack(2)

typedef struct stereo16bittag
{
	int16_t left;
	int16_t right;
} Stereo16bit;

#pragma pack( pop, enter_include1 )

#else

typedef struct stereo16bittag
{
	int16_t left __attribute__((aligned(2)));
	int16_t right __attribute__((aligned(2)));
} Stereo16bit __attribute__((aligned(2)));

#endif




#endif	// PORTABILITY_FMPMDCORE_H
