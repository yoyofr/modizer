// Blip_Buffer 0.4.1. http://www.slack.net/~ant/

#include <limits>
#include <numeric>
#include <cmath>
#include <cstring>
#include "Blip_Buffer.h"
#include "XSFCommon.h"

/* Copyright (C) 2003-2007 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

// TODO: use scoped for variables in treble_eq()

Blip_Buffer::Blip_Buffer()
{
	this->factor_ = static_cast<uint32_t>(std::numeric_limits<long>::max());
	this->buffer_.clear();
	this->buffer_size_ = 0;
	this->sample_rate_ = 0;
	this->bass_shift_ = 0;
	this->clock_rate_ = 0;
	this->bass_freq_ = 16;
	this->length_ = 0;

	// assumptions code makes about implementation-defined features
#ifndef NDEBUG
	// right shift of negative value preserves sign
	buf_t_ i = -0x7FFFFFFE;
	assert((i >> 1) == -0x3FFFFFFF);

	// casting to short truncates to 16 bits and sign-extends
	i = 0x18000;
	assert(static_cast<short>(i) == -0x8000);
#endif

	this->clear();
}

Blip_Buffer::~Blip_Buffer()
{
}

void Blip_Buffer::clear(int entire_buffer)
{
	this->offset_ = 0;
	this->reader_accum_ = 0;
	this->modified_ = nullptr;
	if (!this->buffer_.empty())
	{
		long count = entire_buffer ? this->buffer_size_ : this->samples_avail();
		memset(&this->buffer_[0], 0, (count + blip_buffer_extra_) * sizeof(buf_t_));
	}
}

void Blip_Buffer::set_sample_rate(long new_rate, long msec)
{
	// start with maximum length that resampled time can represent
	long new_size = (std::numeric_limits<unsigned long>::max() >> BLIP_BUFFER_ACCURACY) - blip_buffer_extra_ - 64;
	if (msec != blip_max_length)
	{
		long s = (new_rate * (msec + 1) + 999) / 1000;
		if (s < new_size)
			new_size = s;
		else
			assert(false); // fails if requested buffer length exceeds limit
	}

	if (this->buffer_size_ != new_size)
		this->buffer_.resize(new_size + blip_buffer_extra_);

	this->buffer_size_ = new_size;

	// update things based on the sample rate
	this->sample_rate_ = new_rate;
	this->length_ = new_size * 1000 / new_rate - 1;
	if (msec)
		assert(this->length_ == msec); // ensure length is same as that passed in

	// update these since they depend on sample rate
	if (this->clock_rate_)
		this->clock_rate(this->clock_rate_);
	this->bass_freq(this->bass_freq_);

	this->clear();
}

blip_resampled_time_t Blip_Buffer::clock_rate_factor(long rate) const
{
	double ratio = static_cast<double>(this->sample_rate_) / rate;
	int32_t factor = static_cast<int32_t>(std::floor(ratio * (1L << BLIP_BUFFER_ACCURACY) + 0.5));
	assert(factor > 0 || !this->sample_rate_); // fails if clock/output ratio is too large
	return static_cast<blip_resampled_time_t>(factor);
}

void Blip_Buffer::bass_freq(int freq)
{
	this->bass_freq_ = freq;
	int shift = 31;
	if (freq > 0)
	{
		shift = 13;
		long f = (freq << 16) / this->sample_rate_;
		while ((f >>= 1) && --shift);
	}
	this->bass_shift_ = shift;
}

void Blip_Buffer::end_frame(blip_time_t t)
{
	this->offset_ += t * this->factor_;
	assert(this->samples_avail() <= static_cast<long>(this->buffer_size_)); // fails if time is past end of buffer
}

long Blip_Buffer::count_samples(blip_time_t t) const
{
	auto last_sample = this->resampled_time(t) >> BLIP_BUFFER_ACCURACY;
	auto first_sample = this->offset_ >> BLIP_BUFFER_ACCURACY;
	return static_cast<long>(last_sample - first_sample);
}

blip_time_t Blip_Buffer::count_clocks(long count) const
{
	if (!this->factor_)
	{
		assert(false); // sample rate and clock rates must be set first
		return 0;
	}

	if (count > this->buffer_size_)
		count = this->buffer_size_;
	auto time = static_cast<blip_resampled_time_t>(count) << BLIP_BUFFER_ACCURACY;
	return static_cast<blip_time_t>((time - this->offset_ + this->factor_ - 1) / this->factor_);
}

void Blip_Buffer::remove_samples(long count)
{
	if (count)
	{
		this->remove_silence(count);

		// copy remaining samples to beginning and clear old samples
		long remain = this->samples_avail() + blip_buffer_extra_;
		memmove(&this->buffer_[0], &this->buffer_[count], remain * sizeof(buf_t_));
		memset(&this->buffer_[remain], 0, count * sizeof(buf_t_));
	}
}

// Blip_Synth_

Blip_Synth_Fast_::Blip_Synth_Fast_()
{
	this->buf = nullptr;
	this->last_amp = this->delta_factor = 0;
}

void Blip_Synth_Fast_::volume_unit(double new_unit)
{
	this->delta_factor = static_cast<int>(new_unit * (1L << blip_sample_bits) + 0.5);
}

#ifndef BLIP_BUFFER_FAST
Blip_Synth_::Blip_Synth_(short *p, int w) : impulses(p), width(w)
{
	this->volume_unit_ = 0.0;
	this->kernel_unit = 0;
	this->buf = nullptr;
	this->last_amp = this->delta_factor = 0;
}

#ifndef M_PI
static const double M_PI = 3.1415926535897932384626433832795029;
#endif

static void gen_sinc(float *out, int count, double oversample, double treble, double cutoff)
{
	if (cutoff >= 0.999)
		cutoff = 0.999;

	if (treble < -300.0)
		treble = -300.0;
	if (treble > 5.0)
		treble = 5.0;

	static const double maxh = 4096.0;
	double rolloff = std::pow(10.0, 1.0 / (maxh * 20.0) * treble / (1.0 - cutoff));
	double pow_a_n = std::pow(rolloff, maxh - maxh * cutoff);
	double to_angle = M_PI / 2 / maxh / oversample;
	for (int i = 0; i < count; ++i)
	{
		double angle = ((i - count) * 2 + 1) * to_angle;
		double c = rolloff * std::cos((maxh - 1.0) * angle) - std::cos(maxh * angle);
		double cos_nc_angle = std::cos(maxh * cutoff * angle);
		double cos_nc1_angle = std::cos((maxh * cutoff - 1.0) * angle);
		double cos_angle = std::cos(angle);

		c = c * pow_a_n - rolloff * cos_nc1_angle + cos_nc_angle;
		double d = 1.0 + rolloff * (rolloff - cos_angle - cos_angle);
		double b = 2.0 - cos_angle - cos_angle;
		double a = 1.0 - cos_angle - cos_nc_angle + cos_nc1_angle;

		out[i] = static_cast<float>((a * d + c * b) / (b * d)); // a / b + c / d
	}
}

void blip_eq_t::generate(float *out, int count) const
{
	// lower cutoff freq for narrow kernels with their wider transition band
	// (8 points->1.49, 16 points->1.15)
	double oversample = blip_res * 2.25 / count + 0.85;
	double half_rate = this->sample_rate * 0.5;
	if (this->cutoff_freq)
		oversample = half_rate / this->cutoff_freq;
	double cutoff = this->rolloff_freq * oversample / half_rate;

	gen_sinc(out, count, blip_res * oversample, this->treble, cutoff);

	// apply (half of) hamming window
	double to_fraction = M_PI / (count - 1);
	for (int i = count; i--; )
		out[i] *= 0.54f - 0.46f * static_cast<float>(std::cos(i * to_fraction));
}

void Blip_Synth_::adjust_impulse()
{
	// sum pairs for each phase and add error correction to end of first half
	int size = this->impulses_size();
	for (int p = blip_res; p-- >= blip_res / 2; )
	{
		int p2 = blip_res - 2 - p;
		long error = this->kernel_unit;
		for (int i = 1; i < size; i += blip_res)
			error -= this->impulses[i + p] + this->impulses[i + p2];
		if (p == p2)
			error /= 2; // phase = 0.5 impulse uses same half for both sides
		this->impulses[size - blip_res + p] += static_cast<short>(error);
		//printf( "error: %ld\n", error );
	}

	//for (int i = blip_res; i--; printf("\n"))
		//for (int j = 0; j < this->width / 2; ++j)
			//printf("%5ld,", this->impulses[j * blip_res + i + 1]);
}

void Blip_Synth_::treble_eq(const blip_eq_t &eq)
{
	float fimpulse[blip_res / 2 * (blip_widest_impulse_ - 1) + blip_res * 2];

	int half_size = blip_res / 2 * (this->width - 1);
	eq.generate(&fimpulse[blip_res], half_size);

	int i;

	// need mirror slightly past center for calculation
	for (i = blip_res; i--; )
		fimpulse[blip_res + half_size + i] = fimpulse[blip_res + half_size - 1 - i];

	// starts at 0
	std::fill_n(&fimpulse[0], static_cast<unsigned>(blip_res), 0.0f);

	// find rescale factor
	double total = std::accumulate(&fimpulse[blip_res], &fimpulse[blip_res + half_size], 0.0);

	//static const double base_unit = 44800.0 - 128 * 18; // allows treble up to +0 dB
	//static const double base_unit = 37888.0; // allows treble to +5 dB
	static const double base_unit = 32768.0; // necessary for blip_unscaled to work
	double rescale = base_unit / 2 / total;
	this->kernel_unit = static_cast<long>(base_unit);

	// integrate, first difference, rescale, convert to int
	double sum = 0.0;
	double next = 0.0;
	int size = this->impulses_size();
	for (i = 0; i < size; ++i)
	{
		this->impulses[i] = static_cast<short>(std::floor((next - sum) * rescale + 0.5));
		sum += fimpulse[i];
		next += fimpulse[i + blip_res];
	}
	this->adjust_impulse();

	// volume might require rescaling
	double vol = this->volume_unit_;
	if (!fEqual(vol, 0.0))
	{
		this->volume_unit_ = 0.0;
		this->volume_unit(vol);
	}
}

void Blip_Synth_::volume_unit(double new_unit)
{
	if (!fEqual(new_unit, this->volume_unit_))
	{
		// use default eq if it hasn't been set yet
		if (!this->kernel_unit)
			this->treble_eq(-8.0);

		this->volume_unit_ = new_unit;
		double factor = new_unit * (1L << blip_sample_bits) / this->kernel_unit;

		if (factor > 0.0)
		{
			int shift = 0;

			// if unit is really small, might need to attenuate kernel
			while (factor < 2.0)
			{
				++shift;
				factor *= 2.0;
			}

			if (shift)
			{
				this->kernel_unit >>= shift;
				assert(this->kernel_unit > 0); // fails if volume unit is too low

				// keep values positive to avoid round-towards-zero of sign-preserving
				// right shift for negative values
				long offset = 0x8000 + (1 << (shift - 1));
				long offset2 = 0x8000 >> shift;
				for (int i = this->impulses_size(); i--; )
					this->impulses[i] = static_cast<short>(((this->impulses[i] + offset) >> shift) - offset2);
				this->adjust_impulse();
			}
		}
		this->delta_factor = static_cast<int>(std::floor(factor + 0.5));
		//printf( "delta_factor: %d, kernel_unit: %d\n", delta_factor, kernel_unit );
	}
}
#endif

long Blip_Buffer::read_samples(blip_sample_t *out_, long max_samples, bool stereo)
{
	long count = this->samples_avail();
	if (count > max_samples)
		count = max_samples;

	if (count)
	{
		int bass = BLIP_READER_BASS(this);
		BLIP_READER_BEGIN(reader, *this);
		BLIP_READER_ADJ_(reader, count);
		auto out = &out_[count];
		int32_t offset = static_cast<int32_t>(-count);

		do
		{
			int32_t s = BLIP_READER_READ(reader);
			BLIP_READER_NEXT_IDX_(reader, bass, offset);
			BLIP_CLAMP(s, s);
			out[offset * (stereo ? 2 : 1)] = static_cast<blip_sample_t>(s);
		} while (++offset);

		BLIP_READER_END(reader, *this);

		this->remove_samples(count);
	}
	return count;
}

void Blip_Buffer::mix_samples(const blip_sample_t *in, long count)
{
	auto out = &this->buffer_[(this->offset_ >> BLIP_BUFFER_ACCURACY) + blip_widest_impulse_ / 2];

	static const int sample_shift = blip_sample_bits - 16;
	int32_t prev = 0;
	while (count--)
	{
		int32_t s = static_cast<int32_t>(*in++) << sample_shift;
		*out += s - prev;
		prev = s;
		++out;
	}
	*out -= prev;
}
