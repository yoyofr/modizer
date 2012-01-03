#include "Spline.h"

#include <limits>
#include <stdio.h>

namespace reSIDfp
{

Spline::Spline(const float input[][2], const int inputLength) :
	paramsLength(inputLength-1),
	params(new float[paramsLength][6]) {

	for (int i = 0; i < paramsLength; i ++) {
		const float* p0 = i != 0 ? input[i-1] : NULL;
		const float* p1 = input[i];
		const float* p2 = input[i+1];
		const float* p3 = i != inputLength - 2 ? input[i+2] : NULL;

		float k1, k2;
		if (p0 == NULL) {
			k2 = (p3[1] - p1[1])/(p3[0] - p1[0]);
			k1 = (3.*(p2[1] - p1[1])/(p2[0] - p1[0]) - k2)/2.;
		} else if (p3 == NULL) {
			k1 = (p2[1] - p0[1])/(p2[0] - p0[0]);
			k2 = (3.*(p2[1] - p1[1])/(p2[0] - p1[0]) - k1)/2.;
		} else {
			k1 = (p2[1] - p0[1])/(p2[0] - p0[0]);
			k2 = (p3[1] - p1[1])/(p3[0] - p1[0]);
		}

		const float x1 = p1[0];
		const float y1 = p1[1];
		const float x2 = p2[0];
		const float y2 = p2[1];

		const float dx = x2 - x1;
		const float dy = y2 - y1;

		const float a = ((k1 + k2) - 2.*dy/dx)/(dx*dx);
		const float b = ((k2 - k1)/dx - 3.*(x1 + x2)*a)/2.;
		const float c = k1 - (3.*x1*a + 2.*b)*x1;
		const float d = y1 - ((x1*a + b)*x1 + c)*x1;

		params[i][0] = x1;
		params[i][1] = x2;
		params[i][2] = a;
		params[i][3] = b;
		params[i][4] = c;
		params[i][5] = d;
	}

	/* Fix the value ranges, because we interpolate outside original bounds if necessary. */
	params[0][0] = std::numeric_limits<float>::min();
	params[paramsLength - 1][1] = std::numeric_limits<float>::max();

	c = params[0];
}

void Spline::evaluate(const float x, float* out) {
	if (x < c[0] || x > c[1]) {
		for (int i = 0; i < paramsLength; i ++) {
			if (params[i][1] < x) {
				continue;
			}
			c = params[i];
			break;
		}
	}

	const float y = ((c[2]*x + c[3])*x + c[4])*x + c[5];
	const float yd = (3.*c[2]*x + 2.*c[3])*x + c[4];
	out[0] = y;
	out[1] = yd;
}

} // namespace reSIDfp
