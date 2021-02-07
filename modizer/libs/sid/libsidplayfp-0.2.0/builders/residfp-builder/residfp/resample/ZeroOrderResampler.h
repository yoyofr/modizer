#ifndef ZEROORDER_RESAMPLER_H
#define ZEROORDER_RESAMPLER_H

#include "Resampler.h"

namespace reSIDfp
{

/**
 * Return sample with linear interpolation.
 *
 * @author Antti Lankila
 */
class ZeroOrderResampler : public Resampler {

private:
	int cachedSample;

	const int cyclesPerSample;
	int sampleOffset;
	int outputValue;

public:
	ZeroOrderResampler(const float clockFrequency, const float samplingFrequency) :
		cachedSample(0),
		cyclesPerSample((int) (clockFrequency / samplingFrequency * 1024.f)),
		sampleOffset(0),
		outputValue(0) {}

	const bool input(const int sample) {
		bool ready = false;

		if (sampleOffset < 1024) {
			outputValue = cachedSample + (sampleOffset * (sample - cachedSample) >> 10);
			ready = true;
			sampleOffset += cyclesPerSample;
		}
		sampleOffset -= 1024;

		cachedSample = sample;

		return ready;
	}

	const int output() { return outputValue; }

	void reset() {
		sampleOffset = 0;
		cachedSample = 0;
	}
};

} // namespace reSIDfp

#endif
