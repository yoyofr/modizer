/*
* Poor man's emulation of a 6581 revision SID based filter.
*
* In the real 6581 SID HW the respective filter circuits rely on a particular implementation of
* "fake op-amps" that is very dependent on specific component behavior under borderline
* conditions (e.g. transistors running in subthreshold, triode, and saturation modes while
* serving as a resistor). This seems to be the root cause why actual 6581 chips are very sensitive
* to the slightest variations of used raw materials and why different SID production batches
* rarely sound the same.
*
* A correct simulation gets rather messy/complex and the resid team seems to have done a great job
* accurately modeling the actual physical design (see residfp-2.2). While fascinating from a technical
* perspective, it seems questionable if a respective correct low-level modeling is worth the trouble
* for practical purposes, e.g. my 4 6581 SIDs (various R3 and R4 models) all sound completely
* different and while a correct model could be parameterized to correctly emulate all the dozens
* (if not hundreds) existing chip variations, an emulation perfectly tuned to mimic the wrong SID chip
* specimen may be less desirable than a flawed approximation which more closely matches the user's
* expectations.
*
* Antti Lankila's analysis (https://bel.fi/alankila/c64-sw/fc-curves/curves.png ) confirms the
* variability between different SID chip instances. It shows the types of cutoff curves that can be
* expected and shows that there is a specific distortion effect (kinking) - which the resid team traced
* back to the D/A conversion. It seems to be strongest at the 1/2 mark and then get successively
* weaker at higher subdivisions 1/pow(2,x). After the 1/32 steps the distortions seem to get so small
* that they are barely distinguishable from the regular noise in the measurements..
* note: The above graph uses a logarithmic y-scale, i.e. one linear scale the curves would just be
* steeper.
*
* The previous implementation had also been based on Hermit's respective jsSid code. Inspired by Antti
* Lankila "type 3" design (see "procedure" explained here: https://bel.fi/alankila/c64-sw/index-cpp.html )
* I decided to upgrade the implementation with respective distortion features. -> "high voltage favors 
* distortion"
*
* As a base a simple S-curve is used (e.g.
* http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIwLjk3LygxLjArZXhwKC0oKHgqMi0xNTAwKS8xNDUpKSkrMC4wMjkiLCJjb2xvciI6IiMwMDAwMDAifSx7InR5cGUiOjAsImVxIjoiMC45Ny8oMS4wK2V4cCgtKCh4KjIpLzE0NSkpKSswLjAyOSIsImNvbG9yIjoiI0YwMTExMSJ9LHsidHlwZSI6MTAwMCwid2luZG93IjpbIi0xMDAwIiwiMTAyNCIsIi0wIiwiMSJdfV0- )
* and for different distortion levels that same curve is provided in differently offset variants.
* See https://www.wothke.ch/websid_filter/ for online UI.
*
* This implementation is not based on a physical modeling of actual SID HW components (resistors, caps, etc).
* Looking at current resid research, respective "correct" modeling quickly seems to get fairly complex
* and I just do not have the respective electronics background to even try a proper modeling
* approach. But as was explained above there are "infinite" variations of how actual SID chips sound and
* even if one had the perfect model, the generated audio output will most likely be different from what
* the users expects. The approach taken here is therefore to take some shortcuts and hope that it sounds
* close to what most users expect.
*
* Known limitations:
*
* - The distortions in real 6581 chips are so bad, that the resulting audio output is often 
*   plagued by massive "random clicks/beeps" and/or "noise" (see my recordings of real SID output) - which 
*   makes for a poor listening experience for the majority of songs that were not specifically designed 
*   to depend on a specific flaw or one specific chip specimen. This emulation will sound much "too clean"
*   and not be able to reproduce those distortions.
*
* WebSid (c) 2019 Jürgen Wothke
* version 0.93
*
* Terms of Use: This software is licensed under a CC BY-NC-SA
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/

#include "filter6581.h"

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>


#define MAX_AMP 80000	// just a wild guess
#define DIST_IDX_SCALER (((double)DIST_LEVELS)/(CUTOFF_SIZE*2))

double calcDistRescale(double distort_offset, double distort_scale) {	
	double w= ((double)MAX_AMP+distort_offset)/distort_scale;	
	
	// if this is larger than 2048 then it means that the input
	// voltage is given more weight than the _reg_cutoff - and the 
	// "scaled range" must be adjusted accordingly
	
	return w < (CUTOFF_SIZE*2) ? DIST_IDX_SCALER : DIST_IDX_SCALER *(w/(CUTOFF_SIZE*2));
}

// XXX fixme; defaults tuned using 48kHz samplerate.. adjust to the actually used sample rate!

// The below settings were hand-tuned using a MOS 6581 R4AR.
// params are global and equally affect all SIDs configured to emulate a 6581 model

	// base curve
double Filter6581::_base = 0.036;				// curve's min
double Filter6581::_max = 0.892;				// curve's max

double Filter6581::_steepness = 144.856;		// steepness of the S-curve's center section (lower is steeper)
double Filter6581::_x_offset =  1473.75; 		// amount of "right moving" the S-curve (0 puts the rising
												// inflection point at 0)
	// kinking effect
double Filter6581::_kink = 320;

	// "voltage based" distortion
double Filter6581::_distort = 9.76;				// "step size" between successive distortion levels
double Filter6581::_distort_offset = 87200;		// filter-output/cutoff register "voltage" scaling
double Filter6581::_distort_scale =	99.7875;	// filter-output/cutoff register "voltage" scaling
double Filter6581::_distort_threshold = 1134;	// filter-output/cutoff register "voltage" threshold above which
												// distortion kicks in
															
double Filter6581::_distort_1_div_scale = 1.0 / Filter6581::_distort_scale;
double Filter6581::_distort_rescale = calcDistRescale(Filter6581::_distort_offset, Filter6581::_distort_scale);

												
// copy of above params used to interface with JavaScript side
double Filter6581::_shadow_config_6581[9];


bool Filter6581::_distortion_cache_ready = false;

// "kink"-distorted "cutoff register"
//double Filter6581::_kinked[CUTOFF_SIZE];

// precalculated filter cutoffs for different levels of distortion
double Filter6581::_distortion_tbls_by_cutoff[CUTOFF_SIZE][DIST_LEVELS];

// currently selected row from the above table: precalculated
// distortion levels for the currently selected filter cutoff
double* Filter6581::_distortion_tbl = 0;

// copy of cutoff information of a specific distortion level
// used to interface with JavaScript side
double Filter6581::_tmp_cutoff_tbl[CUTOFF_SIZE];

Filter6581::Filter6581(SID* sid) : Filter(sid) {
	Filter6581::init();
}

Filter6581::~Filter6581() {
}

void Filter6581::init() {
	if (!_distortion_cache_ready)
		Filter6581::setFilterConfig6581(_base, _max, _steepness, _x_offset, _distort, _distort_offset, _distort_scale, _distort_threshold, _kink);
}

void Filter6581::resyncCache() {
#ifdef USE_FILTER
	int reg_cutoff = _reg_cutoff_lo + _reg_cutoff_hi * 8;
	_reg_cutoff = (double)reg_cutoff;

	_distortion_tbl = _distortion_tbls_by_cutoff[reg_cutoff >> 1];

	// see http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiI4LjAveCIsImNvbG9yIjoiIzAwMDAwMCJ9LHsidHlwZSI6MCwiZXEiOiIxLygwLjcwNyt4LzE1KSIsImNvbG9yIjoiI0VCMEMwQyJ9LHsidHlwZSI6MCwiZXEiOiIxLjQxIiwiY29sb3IiOiIjMDAwMDAwIn0seyJ0eXBlIjowLCJlcSI6IjAuOS8oMStleHAoLSgoLXgqMTYuNykvNDApLTMuNCkpKzAuNTQiLCJjb2xvciI6IiMxODA2N0QifSx7InR5cGUiOjEwMDAsIndpbmRvdyI6WyIwIiwiMTUiLCIwIiwiMiJdfV0-

	// Correct resonance handling seems to be much more complex than what is done
	// here (see respective "gain" implementation in recent resid) and this here is bound to
	// lead to produce somewhat flawed results.

	// interestingly Hermit had added a fixed output in the low resonances (see back curves in above graph)
	// making the higher range fall steeper (whatever he tried to achieve might no longer be necessary
	// after I added distortion?). todo: find a testcase where this "feature" had any benefit:

//	_resonance = ((_reg_res_flt > 0x5F) ? 8.0 / (_reg_res_flt >> 4) : 1.41);	// Hermit's

	// whereas some older resid here used a continuously falling curve that covered about the
	// same result range (see red curve):

	_resonance = 1.0/(0.707 + (_reg_res_flt >> 4)/0x0f);
#endif
}

double Filter6581::cutoffMultiplier(double filter_out) {

	filter_out+= _distort_offset;		// sim "all" positive voltage levels, e.g. 0..160000 range (plus overflows at both ends)
	filter_out*= _distort_1_div_scale;	//  scale to "same" 0..2047 range as cutoff register	

	// "The FC and source mix together.." - todo: the used proportions might need some fine-tuning
//	filter_out=((filter_out+_kinked[(int)_reg_cutoff])*0.5) - _distort_threshold;	// testcase where this makes any difference?
	filter_out=((filter_out+_reg_cutoff)*0.5) - _distort_threshold;		// XXX averaging could be avoided by compensating in the other params
	
	// XXX FIXME: the offset used on the input that is fed to filter causes a strong "flipping" effect 
	// that on those filters that do flip.. this may well mess up the matching used here..
	
	int i= 0;	
	if (filter_out > 0) {		
		int index = (int) (filter_out * _distort_rescale);
		i = index < DIST_LEVELS ? index : DIST_LEVELS-1;
	}
	return _distortion_tbl[i];
}

double* Filter6581::getFilterConfig6581() {
	_shadow_config_6581[0] = _base;
	_shadow_config_6581[1] = _max;
	_shadow_config_6581[2] = _steepness;
	_shadow_config_6581[3] = _x_offset;
	_shadow_config_6581[4] = _distort;
	_shadow_config_6581[5] = _distort_offset;
	_shadow_config_6581[6] = _distort_scale;
	_shadow_config_6581[7] = _distort_threshold;
	_shadow_config_6581[8] = _kink;

	return _shadow_config_6581;
}

int Filter6581::setFilterConfig6581(double base, double max, double steepness, double x_offset, double distort,
								double distort_offset, double distort_scale, double distort_threshold, double kink) {
	_base= base;
	_max= max;
	_steepness= steepness;
	_x_offset= x_offset;
	_distort= distort;
	_distort_offset= distort_offset;
	_distort_scale= distort_scale;	
	_distort_threshold= distort_threshold;
	_kink= kink;
	
	_distort_1_div_scale= 1.0 / _distort_scale;
	_distort_rescale = calcDistRescale(distort_offset, distort_scale);


	for (int cutoff_level = 0; cutoff_level < CUTOFF_SIZE; cutoff_level++) {

		// add "kink" distortions
		double k= 0;
		for (int i= 1; i<=5; i++) {	// higher sub-divisions probably make no sense (see graph)
			int m= CUTOFF_SIZE/pow(2, i);
			k+= ((double)(cutoff_level%m))/m*(0.05/i);
		}
//		_kinked[cutoff_level] = k;

		for (int slice = 0; slice < DIST_LEVELS; slice++) {
			double co= max * (max-base) /
							(1.0 + exp(-((((cutoff_level<<1) - x_offset + (_kink*k)) + distort*slice)/steepness)))
							+ base;

			_distortion_tbls_by_cutoff[cutoff_level][slice] = co;
		}
	}
	_distortion_cache_ready= true;

	// todo/room for improvement: reset filter output of all SIDs when settings are
	// changed... to allow recovery after bad settings..

	return 0;
}

double* Filter6581::getCutoff6581(int distort_level) {
	Filter6581::init();	// allow access before emulator has been properly initialized

	if ((distort_level >= 0) && (distort_level < DIST_LEVELS)) {
		for (int cutoff_level = 0; cutoff_level < CUTOFF_SIZE; cutoff_level++) {
			_tmp_cutoff_tbl[cutoff_level]= _distortion_tbls_by_cutoff[cutoff_level][distort_level];
		}
	}
	return _tmp_cutoff_tbl;
}

#define DAMPEN 0.7	// needed to avoid wild oscillations in wf_02_BP_6581.sid test song

double Filter6581::doGetFilterOutput(double sum_filter_in, double* band_pass, double* low_pass, double* hi_pass) {

	(*hi_pass) = (sum_filter_in + (*band_pass) * _resonance + (*low_pass)) * DAMPEN;
	(*band_pass) = (*band_pass) - (*hi_pass) * cutoffMultiplier(-(*hi_pass));
	(*low_pass) = (*low_pass) + (*band_pass) * cutoffMultiplier(-(*band_pass));

	double filter_out = 0;

	// verified with my different 6581 SIDs
	if (_hipass_ena)	{ filter_out -= (*hi_pass); }
	if (_bandpass_ena)	{ filter_out -= (*band_pass); }
	if (_lowpass_ena)	{ filter_out += (*low_pass); }		
			
	return filter_out;
}
