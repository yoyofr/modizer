/**
 * This file is part of reSID, a MOS6581 SID emulator engine.
 * Copyright (C) 2004  Dag Lem <resid@nimrod.no>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * @author Ken Händel
 *
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "SincResampler.h"

#include <math.h>
#include <iostream>
#include <sstream>
#include <assert.h>
#include <string.h>

#ifdef HAVE_MMINTRIN_H
#  include <mmintrin.h>
#endif

namespace reSIDfp
{

std::map<std::string, array<short> > SincResampler::FIR_CACHE;

const float SincResampler::I0E = 1e-6;

float SincResampler::I0(const float x) {
	float sum = 1., u = 1., n = 1.;
	const float halfx = x / 2.;

	do {
		const float temp = halfx / n;
		u *= temp * temp;
		sum += u;
		n += 1.;
	} while (u >= I0E * sum);

	return sum;
}

int SincResampler::convolve(const short* a, const short* b, const int bLength) {
#ifdef HAVE_MMINTRIN_H
	__m64 acc = _mm_setzero_si64();

	const int n = bLength/4;
	for (int i=0; i < n; i++) {
		const __m64 tmp = _mm_madd_pi16(*(__m64*)a, *(__m64*)b);
		acc = _mm_add_pi16(acc, tmp);
		a+=4;
		b+=4;
	}
	int out = _mm_cvtsi64_si32(acc)+_mm_cvtsi64_si32(_mm_srli_si64(acc, 32));
	_mm_empty();

	const int m = bLength & 3;
	for (int i=0; i < m; i++) {
		out += *a++ * *b++;
	}
#else
	int out = 0;
	for (int i = 0; i < bLength; i++) {
		out += *a++ * *b++;
	}
#endif
	return (out + (1 << 14)) >> 15;
}

int SincResampler::fir(const int subcycle) {
	/* find the first of the nearest fir tables close to the phase */
	int firTableFirst = (subcycle * firRES >> 10);
	const int firTableOffset = (subcycle * firRES) & 0x3ff;

	/*
	* find firN most recent samples, plus one extra in case the FIR wraps.
	*/
	int sampleStart = sampleIndex - firN + RINGSIZE - 1;

	const int v1 = convolve(sample+sampleStart, (*firTable)[firTableFirst], firN);
	// Use next FIR table, wrap around to first FIR table using
	// previous sample.
	if (++firTableFirst == firRES) {
		firTableFirst = 0;
		++sampleStart;
	}

	const int v2 = convolve(sample+sampleStart, (*firTable)[firTableFirst], firN);

	// Linear interpolation between the sinc tables yields good
	// approximation for the exact value.
	return v1 + (firTableOffset * (v2 - v1) >> 10);
}

SincResampler::SincResampler(const float clockFrequency, const float samplingFrequency, const float highestAccurateFrequency) :
	sampleIndex(0),
	cyclesPerSample((int) (clockFrequency / samplingFrequency * 1024.)),
	sampleOffset(0),
	outputValue(0) {

	// 16 bits -> -96dB stopband attenuation.
	const float A = -20. * log10(1.0 / (1 << BITS));
	// A fraction of the bandwidth is allocated to the transition band, which we float
	// because we design the filter to transition halfway at nyquist.
	const float dw = (1. - 2.*highestAccurateFrequency / samplingFrequency) * M_PI * 2.;

	// For calculation of beta and N see the reference for the kaiserord
	// function in the MATLAB Signal Processing Toolbox:
	// http://www.mathworks.com/access/helpdesk/help/toolbox/signal/kaiserord.html
	const float beta = 0.1102 * (A - 8.7);
	const float I0beta = I0(beta);
	const float cyclesPerSampleD = clockFrequency / samplingFrequency;

	{
		// The filter order will maximally be 124 with the current constraints.
		// N >= (96.33 - 7.95)/(2 * pi * 2.285 * (maxfreq - passbandfreq) >= 123
		// The filter order is equal to the number of zero crossings, i.e.
		// it should be an even number (sinc is symmetric about x = 0).
		int N = (int) ((A - 7.95) / (2.285 * dw) + 0.5);
		N += N & 1;

		// The filter length is equal to the filter order + 1.
		// The filter length must be an odd number (sinc is symmetric about
		// x = 0).
		firN = (int) (N * cyclesPerSampleD) + 1;
		firN |= 1;

		// Check whether the sample ring buffer would overflow.
		/*if (firN > RINGSIZE - 1) {
			throw RuntimeException(String.format("Resampling quality exceeds the available buffer: %d required, but only has %d", firN, RINGSIZE));
		}*/
		assert(firN < RINGSIZE);

		/* Error is bounded by err < 1.234 / L^2, so L = sqrt(1.234 / (2^-16)) = sqrt(1.234 * 2^16). */
		firRES = (int) ceil(sqrt(1.234 * (1 << BITS)) / cyclesPerSampleD);

		/* firN*firRES represent the total resolution of the sinc sampling. JOS
		* recommends a length of 2^BITS, but we don't quite use that good a filter.
		* The filter test program indicates that the filter performs well, though. */
	}

	std::ostringstream o(std::ostringstream::out);
	o << firN << "," << firRES << "," << cyclesPerSampleD;
	const std::string firKey = o.str();
	std::map<std::string, array<short> >::iterator lb = FIR_CACHE.lower_bound(firKey);

	/* The FIR computation is expensive and we set sampling parameters often, but
	* from a very small set of choices. Thus, caching is used to speed initialization.
	*/
	if (lb != FIR_CACHE.end() && !(FIR_CACHE.key_comp()(firKey, lb->first))) {
		firTable = &(lb->second);
	} else {
		// Allocate memory for FIR tables.
		array<short> tempTable(firRES, firN);
		firTable = &(FIR_CACHE.insert(lb, std::map<std::string, array<short> >::value_type(firKey, tempTable))->second);

		// The cutoff frequency is midway through the transition band, in effect the same as nyquist.
		const float wc = M_PI;

		/* Calculate the sinc tables. */
		const float scale = 32768.0 * wc / cyclesPerSampleD / M_PI;

		for (int i = 0; i < firRES; i ++) {
			const float jPhase = (float) i / firRES + firN/2;
			for (int j = 0; j < firN; j ++) {
				const float x = j - jPhase;

				const float xt = x / (firN/2);
				const float kaiserXt = fabs(xt) < 1. ? I0(beta * sqrt(1. - xt * xt)) / I0beta : 0.;

				const float wt = wc * x / cyclesPerSampleD;
				const float sincWt = fabs(wt) >= 1e-8 ? sin(wt) / wt : 1.;

				(*firTable)[i][j] = (short) (scale * sincWt * kaiserXt);
			}
		}
	}
}

const bool SincResampler::input(const int input) {
	bool ready = false;

	sample[sampleIndex] = sample[sampleIndex + RINGSIZE] = input;
	sampleIndex = (sampleIndex + 1) & (RINGSIZE - 1);

	if (sampleOffset < 1024) {
		outputValue = fir(sampleOffset);
		ready = true;
		sampleOffset += cyclesPerSample;
	}
	sampleOffset -= 1024;

	return ready;
}

void SincResampler::reset() {
	memset(sample, 0, RINGSIZE * 2 * sizeof(sample[0]));
	sampleOffset = 0;
}
#if 0
void SincResampler::main(String[] args) {
	float RATE = 985248.4;
	Resampler r = new TwoPassSincResampler(RATE, 48000.0, 20000.0);

	Map<float, float> results = new TreeMap<float, float>();
	long start = System.currentTimeMillis();
	for (float freq = 1000; freq < RATE/2; freq *= 1.01) {
		/* prefill resampler buffer */
		int k = 0;
		float omega = 2 * Math.PI * freq / RATE;
		for (int j = 0; j < RINGSIZE; j ++) {
			int signal = (int) (32768.0 * Math.sin(k++ * omega) * Math.sqrt(2));
			r.input(signal);
		}

		int n = 0;
		float pwr = 0;
		/* Now, during measurement stage, put 100 cycles of waveform through filter. */
		for (int j = 0; j < 100000; j ++) {
			int signal = (int) (32768.0 * Math.sin(k++ * omega) * Math.sqrt(2));
			if (r.input(signal)) {
				float out = r.output();
				pwr += out * out;
				n += 1;
			}
		}

		results.put(freq, 10 * Math.log10(pwr/n));
	}
	long end = System.currentTimeMillis();

	for (Entry<float, float> freq : results.entrySet()) {
		System.out.println(String.format("%6.0f Hz %f dB", freq.getKey(), freq.getValue()));
	}
	System.out.println("Filtering time " + (end - start) + " ms");
}

#endif

} // namespace reSIDfp
