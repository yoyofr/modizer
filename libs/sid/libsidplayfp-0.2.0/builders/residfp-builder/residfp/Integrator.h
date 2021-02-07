#ifndef INTEGRATOR_H
#define INTEGRATOR_H

#include "siddefs-fp.h"

namespace reSIDfp
{

/**
 * Java port of the reSID 1.0 filter VCR+opamp+capacitor element (integrator) by Dag Lem.
 *
 * Java port and subthreshold current simulation added by Antti S. Lankila.
 *
 * @author Antti S. Lankila
 * @author Dag Lem
 * @author Leandro Nini
 */
class Integrator {

private:
	unsigned int Vddt_Vw_2;
	int Vddt, n_snake, x;
	int vc;
	const unsigned short* vcr_Vg;
	const unsigned short* vcr_n_Ids_term;
	const int* opamp_rev;

public:
	Integrator(const unsigned short* vcr_Vg, const unsigned short* vcr_n_Ids_term,
		const int* opamp_rev, const int Vddt, const int n_snake) :
		Vddt_Vw_2(0),
		Vddt(Vddt),
		n_snake(n_snake),
		x(0),
		vc(0),
		vcr_Vg(vcr_Vg),
		vcr_n_Ids_term(vcr_n_Ids_term),
		opamp_rev(opamp_rev) {}

	void setVw(const int Vw);

	const int solve(const int vi);
};

} // namespace reSIDfp

#if RESID_INLINING || defined(INTEGRATOR_CPP)

namespace reSIDfp
{

RESID_INLINE
const int Integrator::solve(const int vi) {
	// "Snake" voltages for triode mode calculation.
	const int Vgst = Vddt - x;
	const int Vgdt = Vddt - vi;
	const unsigned int Vgst_2 = Vgst*Vgst;
	const unsigned int Vgdt_2 = Vgdt*Vgdt;

	// "Snake" current, scaled by (1/m)*2^13*m*2^16*m*2^16*2^-15 = m*2^30
	const int n_I_snake = n_snake*((Vgst_2 >> 15) - (Vgdt_2 >> 15));

	// VCR gate voltage.       // Scaled by m*2^16
	// Vg = Vddt - sqrt(((Vddt - Vw)^2 + Vgdt^2)/2)
	const int Vg = (int)vcr_Vg[(Vddt_Vw_2 >> 16) + (Vgdt_2 >> 17)];

	// VCR voltages for EKV model table lookup.
	const int Vgs = Vg > x ? Vg - x : 0;
	const int Vgd = Vg > vi ? Vg - vi : 0;

	// VCR current, scaled by m*2^15*2^15 = m*2^30
	const int n_I_vcr = (int)(vcr_n_Ids_term[Vgs & 0xffff] - vcr_n_Ids_term[Vgd & 0xffff]) << 15;

	// Change in capacitor charge.
	vc += n_I_snake + n_I_vcr;

	// vx = g(vc)
	x = opamp_rev[((vc >> 15) + (1 << 15)) & 0xffff];

	// Return vo.
	return x - (vc >> 14);
}

} // namespace reSIDfp

#endif

#endif
