// Reverb model declaration
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain

#ifndef _revmodel_
#define _revmodel_

#include "comb.hpp"
#include "allpass.hpp"
#include "tuning.h"

const int predelaysize = 48000*2;

class revmodel
{
public:
					revmodel();
			void	mute();
			void	processreplace(unsigned char *inout, long buflen);
			void	setroomsize(float value);
			float	getroomsize();
			void	setdamp(float value);
			float	getdamp();
			void	setwet(float value);
			float	getwet();
			void	setdry(float value);
			float	getdry();
			void	setwidth(float value);
			float	getwidth();
			void	setmode(float value);
			float	getmode();
			void	setsrate(int in_srate);
			void	setpredelay(float in_sec);
private:
			void	update();
private:
	int	gain;
	float	roomsize,roomsize1;
	float	damp,damp1;
	float	wet;
	int	wet1,wet2;;
	int 	dry;
	float	width;
	float	mode;

	// The following are all declared inline 
	// to remove the need for dynamic allocation
	// with its subsequent error-checking messiness

	// Comb filters
	comb	combL[numcombs];
	comb	combR[numcombs];

	// Allpass filters
	allpass	allpassL[numallpasses];
	allpass	allpassR[numallpasses];

	// Buffers for the combs
	int	bufcombL1[combtuningL1];
	int	bufcombR1[combtuningR1];
	int	bufcombL2[combtuningL2];
	int	bufcombR2[combtuningR2];
	int	bufcombL3[combtuningL3];
	int	bufcombR3[combtuningR3];
	int	bufcombL4[combtuningL4];
	int	bufcombR4[combtuningR4];
	int	bufcombL5[combtuningL5];
	int	bufcombR5[combtuningR5];
	int	bufcombL6[combtuningL6];
	int	bufcombR6[combtuningR6];
	int	bufcombL7[combtuningL7];
	int	bufcombR7[combtuningR7];
	int	bufcombL8[combtuningL8];
	int	bufcombR8[combtuningR8];

	// Buffers for the allpasses
	int	bufallpassL1[allpasstuningL1];
	int	bufallpassR1[allpasstuningR1];
	int	bufallpassL2[allpasstuningL2];
	int	bufallpassR2[allpasstuningR2];
	int	bufallpassL3[allpasstuningL3];
	int	bufallpassR3[allpasstuningR3];
	int	bufallpassL4[allpasstuningL4];
	int	bufallpassR4[allpasstuningR4];

	int	predelayL[predelaysize];
	int	predelayR[predelaysize];

	int	srate;
	int	predelay_rdidx;
	int	predelay_wridx;
};

#endif//_revmodel_

//ends
