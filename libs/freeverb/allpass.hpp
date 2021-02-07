// Allpass filter declaration
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain

#ifndef _allpass_
#define _allpass_
#include "tuning.h"
#include "denormals.h"

class allpass
{
public:
					allpass();
			void	setbuffer(int *buf, int size);
	inline  int	process(int inp);
			void	mute();
			void	setfeedback(float val);
			float	getfeedback();
// private:
	int	feedback;
	int	*buffer;
	int		bufsize;
	int		bufidx;
};


// Big to inline - but crucial for speed

inline int allpass::process(int input)
{
	int output;
	int bufout;
	
	bufout = buffer[bufidx];
	undenormalise(bufout);
	
	output = -input + bufout;
	buffer[bufidx] = input + FMUL(bufout, feedback);

	if(++bufidx>=bufsize) bufidx = 0;

	return output;
}

#endif//_allpass

//ends
