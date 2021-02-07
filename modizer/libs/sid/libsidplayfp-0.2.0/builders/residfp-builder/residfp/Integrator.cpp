#define INTEGRATOR_CPP

#include "Integrator.h"

namespace reSIDfp
{

void Integrator::setVw(const int Vw) {
	Vddt_Vw_2 = (Vddt - Vw) * (Vddt - Vw) >> 1;
}

} // namespace reSIDfp
