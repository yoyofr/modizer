// Comb filter class declaration
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain

#ifndef _comb_
#define _comb_

#include "tuning.h"
#include "denormals.h"
#include <stdio.h>

class comb
{
public:
					comb();
			void	setbuffer(int *buf, int size);
		inline  int	process(int inp);
			void	mute();
			void	setdamp(float val);
			float	getdamp();
			void	setfeedback(float val);
			float	getfeedback();
private:
	int	feedback;
	int	filterstore;
	int	damp1;
	int	damp2;
	int	*buffer;
	int		bufsize;
	int		bufidx;
};


// Big to inline - but crucial for speed

inline int comb::process(int input)
{
	int output;

	output = buffer[bufidx];
	undenormalise(output);

	filterstore = FMUL(output, damp2) + FMUL(filterstore, damp1);
	undenormalise(filterstore);

	buffer[bufidx] = input + FMUL(filterstore, feedback);
	if(++bufidx>=bufsize) bufidx = 0;

	return output;
}

#endif //_comb_

//ends
