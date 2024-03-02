/*
* Poor man's emulation of a 8580 revision SID based filter.
*
* WebSid (c) 2019 Jürgen Wothke
* version 0.93
* 
* Terms of Use: This software is licensed under a CC BY-NC-SA 
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/
#ifndef WEBSID_FILTER8580_H
#define WEBSID_FILTER8580_H

#include "filter.h"

/**
* This class handles the filter of a 8580 revision chip.
*
* It is a construct exclusively used by the SID class and access is restricted accordingly.
*/
class Filter8580 : public Filter{
protected:
	Filter8580(class SID* sid);
	virtual ~Filter8580();

	virtual void resyncCache();
	
	virtual double doGetFilterOutput(double sum_filter_in, double* band_pass, double* low_pass, double* hi_pass);

	friend class SID;
private:	
	double _cutoff_ratio_8580;
	double _cutoff;
};


#endif
