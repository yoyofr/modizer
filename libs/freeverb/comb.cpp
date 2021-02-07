// Comb filter implementation
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain

#include "tuning.h"
#include "comb.hpp"

comb::comb()
{
	filterstore = 0;
	bufidx = 0;
}

void comb::setbuffer(int *buf, int size) 
{
	buffer = buf; 
	bufsize = size;
}

void comb::mute()
{
	for (int i=0; i<bufsize; i++)
		buffer[i]=0;
}

void comb::setdamp(float val) 
{
	damp1 = INTTIZE(val); 
	damp2 = INTTIZE(1-val);
}

float comb::getdamp() 
{
	return damp1;
}

void comb::setfeedback(float val) 
{
	feedback = INTTIZE(val);
}

float comb::getfeedback() 
{
	return feedback;
}

// ends
