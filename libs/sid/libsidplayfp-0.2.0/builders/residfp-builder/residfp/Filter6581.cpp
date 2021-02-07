#define FILTER6581_CPP

#include "Filter6581.h"

namespace reSIDfp
{

Filter6581::~Filter6581() {

	delete hpIntegrator;
	delete bpIntegrator;
	delete [] f0_dac;
}

void Filter6581::input(const int sample) {
	ve = (sample * voiceScaleS14 * 3 >> 10) + mixer[0][0];
}

void Filter6581::updatedCenterFrequency() {
	const int Vw = f0_dac[fc];
	hpIntegrator->setVw(Vw);
	bpIntegrator->setVw(Vw);
}

void Filter6581::updatedResonance() {
	currentResonance = gain[~res & 0xf];
}

void Filter6581::updatedMixing() {
	currentGain = gain[vol];

	int ni = 0;
	int no = 0;

	if (filt1) {
		ni++;
	} else {
		no++;
	}
	if (filt2) {
		ni++;
	} else {
		no++;
	}
	if (filt3) {
		ni++;
	} else {
		no++;
	}
	if (filtE) {
		ni++;
	} else {
		no++;
	}
	currentSummer = summer[ni];

	if (lp) {
		no++;
	}
	if (bp) {
		no++;
	}
	if (hp) {
		no++;
	}
	currentMixer = mixer[no];
}

void Filter6581::setFilterCurve(const float curvePosition) {
	delete [] f0_dac;
	f0_dac = FilterModelConfig::getInstance()->getDAC(FilterModelConfig::getInstance()->getDacZero(curvePosition));
	updatedCenterFrequency();
}

} // namespace reSIDfp
