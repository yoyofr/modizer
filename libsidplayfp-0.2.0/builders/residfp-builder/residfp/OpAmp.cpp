#include "OpAmp.h"

#include <math.h>

namespace reSIDfp
{

const float OpAmp::EPSILON = 1e-8;

const float OpAmp::solve(const float n, const float vi) {
	// Start off with an estimate of x and a root bracket [ak, bk].
	// f is decreasing, so that f(ak) > 0 and f(bk) < 0.
	float ak = vmin;
	float bk = vmax;

	const float a = n + 1.;
	const float b = Vddt;
	const float b_vi = (b - vi);
	const float c = n * (b_vi * b_vi);

	while (true) {
		float xk = x;

		// Calculate f and df.
		opamp->evaluate(x, out);
		const float vo = out[0];
		const float dvo = out[1];

		const float b_vx = b - x;
		const float b_vo = b - vo;

		const float f = a * (b_vx * b_vx) - c - (b_vo * b_vo);
		const float df = 2. * (b_vo * dvo - a * b_vx);

		x -= f / df;
		if (fabs(x - xk) < EPSILON) {
			opamp->evaluate(x, out);
			return out[0];
		}

		// Narrow down root bracket.
		if (f < 0.) {
			// f(xk) < 0
			bk = xk;
		}
		else {
			// f(xk) > 0
			ak = xk;
		}

		if (x <= ak || x >= bk) {
			// Bisection step (ala Dekker's method).
			x = (ak + bk) * 0.5;
		}
	}
}

} // namespace reSIDfp
