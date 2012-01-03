#define FILTER8580_CPP

#include "Filter8580.h"

namespace reSIDfp
{

void Filter8580::updatedCenterFrequency() {
	w0 = (float) (2.*M_PI*highFreq*fc/2047/1e6);
}

void Filter8580::updatedResonance() {
	_1_div_Q = 1.f / (0.707f + res/15.f);
}

void Filter8580::input(const int input) {
	ve = input << 4;
}

void Filter8580::setFilterCurve(const float curvePosition) {
	highFreq = curvePosition;
}

} // namespace reSIDfp
