#ifndef TWOPASSSINCRESAMPLER_H
#define TWOPASSSINCRESAMPLER_H

#include "Resampler.h"
#include "SincResampler.h"

#include <math.h>

namespace reSIDfp
{

/**
 * Compose a more efficient SINC from chaining two other SINCs.
 *
 * @author Antti Lankila
 */
class TwoPassSincResampler : public Resampler {
private:
	SincResampler* s1;
	SincResampler* s2;

public:
	TwoPassSincResampler(const float clockFrequency, const float samplingFrequency,
		const float highestAccurateFrequency) {
		/* Calculation according to Laurent Ganier. It evaluates to about 120 kHz at typical settings.
		 * Some testing around the chosen value seems to confirm that this does work. */
		const float intermediateFrequency = 2. * highestAccurateFrequency
			+ sqrt(2. * highestAccurateFrequency * clockFrequency
			* (samplingFrequency - 2. * highestAccurateFrequency) / samplingFrequency);
		s1 = new SincResampler(clockFrequency, intermediateFrequency, highestAccurateFrequency);
		s2 = new SincResampler(intermediateFrequency, samplingFrequency, highestAccurateFrequency);
	}

	~TwoPassSincResampler() {
		delete s1;
		delete s2;
	}

	const bool input(const int sample) {
		return s1->input(sample) && s2->input(s1->output());
	}

	const int output() {
		return s2->output();
	}

	void reset() {
		s1->reset();
		s2->reset();
	}
};

} // namespace reSIDfp

#endif
