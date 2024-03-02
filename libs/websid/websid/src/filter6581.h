/*
* Poor man's emulation of a 6581 revision SID based filter.
*
* WebSid (c) 2019 Jürgen Wothke
* version 0.93
* 
* Terms of Use: This software is licensed under a CC BY-NC-SA 
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/
#ifndef WEBSID_FILTER6581_H
#define WEBSID_FILTER6581_H

#include "filter.h"


#define CUTOFF_SIZE 1024
#define DIST_LEVELS 256	// number of different distortion levels

/**
* This class handles the filter of a 6581 revision chip.
*
* Except for the accessors to the global configuration, it is a construct exclusively 
* used by the SID class and access is restricted accordingly.
*/
class Filter6581 : public Filter {
public:
	// "emscripten friendly" array containing the used filter params (same order as in below setter)
	static double* getFilterConfig6581();

	static int setFilterConfig6581(double base, double max, double steepness, double x_offset, double distort, 
								double distort_offset, double distort_scale, double distort_threshold, double kink);
	/*
	* not reentrant! result array is shared by all calls to this method
	* @param distort_level is in range 0..DIST_LEVELS-1 .. invalid value results in no-op
	* @return array with CUTOFF_SIZE entries
	*/ 
	static double* getCutoff6581(int distort_level);

protected:
	Filter6581(class SID* sid);
	virtual ~Filter6581();

	static void init();

	virtual void resyncCache();

	virtual double doGetFilterOutput(double sum_filter_in, double* band_pass, double* low_pass, double* hi_pass);

	double cutoffMultiplier(double filter_out);
	
	friend class SID;		
private:
		// base curve
	static double _base;				// curve's min
	static double _max;					// curve's max
	static double _steepness;			// steepness of the S-curve's center section (lower is steeper)
	static double _x_offset;			// amount of "right moving" the S-curve (0 puts the rising inflection point
										// at 0)
		// "voltage based" distortion
	static double _distort;				// difference between successive distortion levels
	static double _distort_offset;		// filter-output/cutoff register "voltage" offset to create positive range
	static double _distort_scale;		// filter-output/cutoff register "voltage" scaling used to map distortion
	static double _distort_threshold;	// filter-output/cutoff register "voltage" threshold above which distortion kicks in

	static double _distort_1_div_scale;	// optimization based on _distort_scale
	static double _distort_rescale;		// optimization based on _distort_scale
		// kinking effect
	static double _kink;

	// copy of above params used to interface with JavaScript side
	static double _shadow_config_6581[9];

	static bool _distortion_cache_ready;

	// "kink"-distorted "cutoff register"
	//static double _kinked[CUTOFF_SIZE];

	// precalculated filter cutoffs for different levels of distortion
	static double _distortion_tbls_by_cutoff[CUTOFF_SIZE][DIST_LEVELS];

	// currently selected row from the above table: precalculated
	// distortion levels for the currently selected filter cutoff
	static double* _distortion_tbl;

	// copy of cutoff information of a specific distortion level 
	// used to interface with JavaScript side
	static double _tmp_cutoff_tbl[CUTOFF_SIZE];
	
	// combined content of "11-bit filter cutoff" register
	double _reg_cutoff;	
};

#endif
