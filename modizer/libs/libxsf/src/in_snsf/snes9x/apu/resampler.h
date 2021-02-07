/* Simple resampler based on bsnes's ruby audio library */

#pragma once

#include "ring_buffer.h"

class Resampler : public ring_buffer
{
public:
	virtual void clear() = 0;
	virtual void time_ratio(double) = 0;
	virtual void read(short *, int) = 0;
	virtual int avail() = 0;

	Resampler(int num_samples) : ring_buffer(num_samples << 1)
	{
	}

	virtual ~Resampler()
	{
	}

	bool push(short *src, int num_samples)
	{
		if (this->max_write() < num_samples)
			return false;

		!num_samples || ring_buffer::push(reinterpret_cast<unsigned char *>(src), num_samples << 1);

		return true;
	}

	int max_write()
	{
		return this->space_empty () >> 1;
	}

	void resize(int num_samples)
	{
		ring_buffer::resize(num_samples << 1);
	}
};
