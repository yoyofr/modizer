#ifndef OPAMP_H
#define OPAMP_H

#include "Spline.h"

namespace reSIDfp
{

/**
 * This class solves the opamp equation when loaded by different sets of resistors.
 * Equations and first implementation were written by Dag Lem.
 * This class is a rewrite without use of fixed point integer mathematics, and
 * uses the actual voltages instead of the normalized values.
 *
 * @author alankila
 */
class OpAmp {

private:
	static const float EPSILON;

	/** Current root position (cached as guess to speed up next iteration) */
	float x;

	const float Vddt, vmin, vmax;

	Spline* opamp;

	float out[2];

public:
	/**
	 * Opamp input -> output voltage conversion
	 *
	 * @param opamp opamp mapping table as pairs of points (in -> out)
	 * @param Vddt transistor dt parameter (in volts)
	 */
	OpAmp(const float opamp[][2], const int opamplength, const float Vddt) :
		x(0.),
		Vddt(Vddt),
		vmin(opamp[0][0]),
		vmax(opamp[opamplength - 1][0]),
		opamp(new Spline(opamp, opamplength)) {}

	~OpAmp() { delete opamp; }

	void reset() {
		x = vmin;
	}

	/**
	 * Solve the opamp equation for input vi in loading context n
	 * 
	 * @param n the ratio of input/output loading
	 * @param vi input
	 * @return vo
	 */
	const float solve(const float n, const float vi);
};

} // namespace reSIDfp

#endif
