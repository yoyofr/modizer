#ifndef FILTERMODELCONFIG_H
#define FILTERMODELCONFIG_H

#define OPAMP_SIZE 22
#define DAC_SIZE 11

namespace reSIDfp
{

class Integrator;

class FilterModelConfig {

private:
	static const float opamp_voltage[OPAMP_SIZE][2];

	const float voice_voltage_range;
	const float voice_DC_voltage;

	// Capacitor value.
	const float C;

	// Transistor parameters.
	const float Vdd;
	const float Vth;			// Threshold voltage
	const float uCox_vcr;			// 1/2*u*Cox
	const float WL_vcr;			// W/L for VCR
	const float uCox_snake;		// 1/2*u*Cox
	const float WL_snake;			// W/L for "snake"

	// DAC parameters.
	const float dac_zero;
	const float dac_scale;
	const float dac_2R_div_R;
	const bool dac_term;

	/* Derived stuff */
	const float vmin, norm;
	float opamp_working_point;
	float dac[DAC_SIZE];
	unsigned short* vcr_Vg;
	unsigned short* vcr_n_Ids_term;
	int* opamp_rev;
	unsigned short* mixer[8];
	unsigned short* summer[7];
	unsigned short* gain[16];

	float evaluateTransistor(const float Vw, const float vi, const float vx);

	FilterModelConfig();
	~FilterModelConfig();

public:
	static FilterModelConfig* getInstance();

	const float getDacZero(const float adjustment);

	const int getVO_T16();

	const int getVoiceScaleS14();

	const int getVoiceDC();

	unsigned short** getGain() { return gain; }

	unsigned short** getSummer() { return summer; }

	unsigned short** getMixer() { return mixer; }

	/**
	 * Make DAC
	 * must be deleted
	 */
	const unsigned int* getDAC(const float dac_zero);

	Integrator* buildIntegrator();

	/**
	 * Estimate the center frequency corresponding to some FC setting.
	 *
	 * FIXME: this function is extremely sensitive to prevailing voltage offsets.
	 * They got to be right within about 0.1V, or the results will be simply wrong.
	 * This casts doubt on the feasibility of this approach. Perhaps the offsets
	 * at the integrators would need to be statically solved first for 1-voice null
	 * input.
	 *
	 * @param fc
	 * @return frequency in Hz
	 */
	const float estimateFrequency(const float dac_zero, const int fc);
};

} // namespace reSIDfp

#endif
