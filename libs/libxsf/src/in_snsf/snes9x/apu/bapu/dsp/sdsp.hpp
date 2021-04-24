#pragma once

#include "../snes/snes.hpp"
#include "SPC_DSP.h"

namespace SNES
{
	class DSP : public Processor
	{
	public:
		uint8_t read(uint8_t addr)
		{
			this->synchronize();
			return this->spc_dsp.read(addr);
		}

		void synchronize()
		{
			if (this->clock)
			{
				this->spc_dsp.run(this->clock);
				this->clock = 0;
			}
		}

		void write(uint8_t addr, uint8_t data)
		{
			this->synchronize();
			this->spc_dsp.write(addr, data);
		}

		void power();
		void reset();

		DSP();

		SPC_DSP spc_dsp;
	};

	extern DSP dsp;
}
