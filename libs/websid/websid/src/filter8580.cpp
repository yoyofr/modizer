/*
* Poor man's emulation of a 8580 revision SID based filter.
*
* The impl used here is derived from Hermit's cSID-light.
*
* WebSid (c) 2019 Jürgen Wothke
* version 0.93
*
* Terms of Use: This software is licensed under a CC BY-NC-SA
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/

#include "filter8580.h"

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>


Filter8580::Filter8580(SID* sid) : Filter(sid) {
}

Filter8580::~Filter8580() {
}

void Filter8580::resyncCache() {
	// since this only depends on the sid regs, it is sufficient to update this after reg updates
#ifdef USE_FILTER
   _cutoff_ratio_8580 = ((double) -2.0) * 3.1415926535897932385 * (12500.0 / 2048) / _sample_rate;

	// NOTE: +1 is meant to model that even a 0 cutoff will still let through some signal..
	_cutoff = ((double)_reg_cutoff_lo) +  _reg_cutoff_hi * 8 + 1;

	// slightly arched curve that rises from 0 to ca 0.8 (rises progressively slower)
	// http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIxLjAtZXhwKHgqLTcuOTg5NDgzMjcwMjM3NzE0NzM4MTE1MDM5NTI4NjAzOWUtNCkiLCJjb2xvciI6IiMwMDAwMDAifSx7InR5cGUiOjEwMDAsIndpbmRvdyI6WyIxIiwiMjA0OCIsIjAiLCIxLjEiXX1d
	_cutoff = 1.0 - exp(_cutoff * _cutoff_ratio_8580);

	// seems to be similar to what old resid is using but resulting in lower end-point
	_resonance = pow(2.0, ((4.0 - (_reg_res_flt >> 4)) / 8));	// i.e. 1.41 to 0.39
#endif
}

double Filter8580::doGetFilterOutput(double sum_filter_in, double* band_pass, double* low_pass, double* hi_pass) {

	(*hi_pass) = sum_filter_in + (*band_pass) * _resonance + (*low_pass);
	(*band_pass) = (*band_pass) - (*hi_pass) * _cutoff;
	(*low_pass) = (*low_pass) + (*band_pass) * _cutoff;

	double filter_out = 0;

	// note: filter impl seems to invert the data (see 2012_High-Score_Power_Ballad 
	// where two voices playing the same notes cancelled each other out..)
	
	// verified with my 8580R5
	if (_hipass_ena)	{ filter_out -= (*hi_pass); }	
	if (_bandpass_ena)	{ filter_out -= (*band_pass); }
	if (_lowpass_ena)	{ filter_out += (*low_pass); }
	
	return filter_out;
}
