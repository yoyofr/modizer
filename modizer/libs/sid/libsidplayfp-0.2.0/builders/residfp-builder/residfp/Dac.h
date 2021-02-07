#ifndef DAC_H
#define DAC_H

namespace reSIDfp
{

/**
* Estimate DAC nonlinearity. The SID contains R-2R ladder, and some likely errors
* in the resistor lengths which result in errors depending on the bits chosen.
*
* This model was derived by Dag Lem, and is port of the upcoming reSID version.
* In average, it shows a value higher than the target by a value that depends
* on the _2R_div_R parameter. It differs from the version written by Antti Lankila
* chiefly in the emulation of the lacking termination of the 2R ladder, which
* destroys the output with respect to the low bits of the DAC.
*
* @param input digital value to convert to analog
* @param _2R_div_R nonlinearity parameter, 1.0 for perfect linearity.
* @param bits highest bit that may be set in input.
* @param termi is the dac terminated by a 2R resistor? (6581 DACs are not)
*
* @return the analog value as modeled from the R-2R network.
*/
class Dac {

public:
	static void kinkedDac(float* dac, const int dacLength, const float _2R_div_R, const bool term);
};

} // namespace reSIDfp

#endif
