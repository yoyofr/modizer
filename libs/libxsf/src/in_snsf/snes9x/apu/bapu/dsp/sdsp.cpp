#include "../snes/snes.hpp"
#include "sdsp.hpp"
#include "SPC_DSP.h"
#include "../smp/smp.hpp"

namespace SNES
{
	DSP dsp;

	void DSP::power()
	{
		this->spc_dsp.init(smp.apuram.get());
		this->spc_dsp.reset();
		this->clock = 0;
	}

	void DSP::reset()
	{
		this->spc_dsp.soft_reset();
		this->clock = 0;
	}

	DSP::DSP()
	{
		this->clock = 0;
	}
}
