#include "Dac.h"

namespace reSIDfp
{

void Dac::kinkedDac(float* dac, const int dacLength, const float _2R_div_R, const bool term) {
	const float R_INFINITY = 1e6;

	// Calculate voltage contribution by each individual bit in the R-2R ladder.
	for (int set_bit = 0; set_bit < dacLength; set_bit++) {
		int bit;

		float Vn = 1.;          // Normalized bit voltage.
		float R = 1.;           // Normalized R
		const float _2R = _2R_div_R*R;  // 2R
		float Rn = term ?         // Rn = 2R for correct termination,
			_2R : R_INFINITY;         // INFINITY for missing termination.

		// Calculate DAC "tail" resistance by repeated parallel substitution.
		for (bit = 0; bit < set_bit; bit++) {
			Rn = (Rn == R_INFINITY) ?
				R + _2R :
				R + _2R*Rn/(_2R + Rn); // R + 2R || Rn
		}

		// Source transformation for bit voltage.
		if (Rn == R_INFINITY) {
			Rn = _2R;
		} else {
			Rn = _2R*Rn/(_2R + Rn);  // 2R || Rn
			Vn = Vn*Rn/_2R;
		}

		// Calculate DAC output voltage by repeated source transformation from
		// the "tail".

		for (++bit; bit < dacLength; bit++) {
			Rn += R;
			const float I = Vn/Rn;
			Rn = _2R*Rn/(_2R + Rn);  // 2R || Rn
			Vn = Rn*I;
		}

		dac[set_bit] = Vn;
	}

	/* Normalize to integerish behavior */
	float Vsum = 0.;
	for (int i = 0; i < dacLength; i ++) {
		Vsum += dac[i];
	}
	Vsum /= 1 << dacLength;
	for (int i = 0; i < dacLength; i ++) {
		dac[i] /= Vsum;
	}
}

} // namespace reSIDfp
