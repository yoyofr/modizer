// Blip_Buffer 0.4.1. http://www.slack.net/~ant/

#include <algorithm>
#include "Multi_Buffer.h"

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

Multi_Buffer::Multi_Buffer(int spf) : samples_per_frame_(spf)
{
	this->length_ = 0;
	this->sample_rate_ = 0;
	this->channels_changed_count_ = 1;
	this->channel_types_ = nullptr;
	this->channel_count_ = 0;
	this->immediate_removal_ = true;
}

Multi_Buffer::channel_t Multi_Buffer::channel(int /*index*/)
{
	static const channel_t ch = { 0, 0, 0 };
	return ch;
}

// Tracked_Blip_Buffer

Tracked_Blip_Buffer::Tracked_Blip_Buffer()
{
	this->last_non_silence = 0;
}

void Tracked_Blip_Buffer::clear()
{
	this->last_non_silence = 0;
	Blip_Buffer::clear();
}

void Tracked_Blip_Buffer::end_frame(blip_time_t t)
{
	Blip_Buffer::end_frame(t);
	if (this->clear_modified())
		this->last_non_silence = this->samples_avail() + blip_buffer_extra_;
}

uint32_t Tracked_Blip_Buffer::non_silent() const
{
	return this->last_non_silence | this->unsettled();
}

void Tracked_Blip_Buffer::remove_(long n)
{
	if ((this->last_non_silence -= n) < 0)
		this->last_non_silence = 0;
}

void Tracked_Blip_Buffer::remove_silence(long n)
{
	this->remove_(n);
	Blip_Buffer::remove_silence(n);
}

void Tracked_Blip_Buffer::remove_samples(long n)
{
	this->remove_(n);
	Blip_Buffer::remove_samples(n);
}

long Tracked_Blip_Buffer::read_samples(blip_sample_t *out, long count)
{
	count = Blip_Buffer::read_samples(out, count);
	this->remove_(count);
	return count;
}

// Stereo_Buffer

static const int stereo = 2;

Stereo_Buffer::Stereo_Buffer() : Multi_Buffer(2)
{
	this->chan.center = this->mixer.bufs[2] = &this->bufs[2];
	this->chan.left = this->mixer.bufs[0] = &this->bufs[0];
	this->chan.right = this->mixer.bufs[1] = &this->bufs[1];
	this->mixer.samples_read = 0;
}

Stereo_Buffer::~Stereo_Buffer() { }

void Stereo_Buffer::set_sample_rate(long rate, long msec)
{
	this->mixer.samples_read = 0;
	for (int i = bufs_size; --i >= 0; )
		this->bufs[i].set_sample_rate(rate, msec);
	Multi_Buffer::set_sample_rate(this->bufs[0].sample_rate(), this->bufs[0].length());
}

void Stereo_Buffer::clock_rate(long rate)
{
	for (int i = bufs_size; --i >= 0; )
		this->bufs[i].clock_rate(rate);
}

void Stereo_Buffer::bass_freq(int bass)
{
	for (int i = bufs_size; --i >= 0; )
		this->bufs[i].bass_freq(bass);
}

void Stereo_Buffer::clear()
{
	this->mixer.samples_read = 0;
	for (int i = bufs_size; --i >= 0; )
		this->bufs[i].clear();
}

void Stereo_Buffer::end_frame(blip_time_t time)
{
	for (int i = bufs_size; --i >= 0; )
		this->bufs[i].end_frame(time);
}

long Stereo_Buffer::read_samples(blip_sample_t *out, long out_size)
{
	assert(!(out_size & 1)); // must read an even number of samples
	out_size = std::min(out_size, this->samples_avail());

	int pair_count = static_cast<int>(out_size >> 1);
	if (pair_count)
	{
		this->mixer.read_pairs(out, pair_count);

		if (this->samples_avail() <= 0 || this->immediate_removal())
		{
			for (int i = bufs_size; --i >= 0; )
			{
				auto &b = this->bufs[i];
				// TODO: might miss non-silence settling since it checks END of last read
				if (!b.non_silent())
					b.remove_silence(this->mixer.samples_read);
				else
					b.remove_samples(this->mixer.samples_read);
			}
			this->mixer.samples_read = 0;
		}
	}
	return out_size;
}

// Stereo_Mixer

// mixers use a single index value to improve performance on register-challenged processors
// offset goes from negative to zero

void Stereo_Mixer::read_pairs(blip_sample_t *out, int count)
{
	// TODO: if caller never marks buffers as modified, uses mono
	// except that buffer isn't cleared, so caller can encounter
	// subtle problems and not realize the cause.
	this->samples_read += count;
	if (this->bufs[0]->non_silent() | this->bufs[1]->non_silent())
		this->mix_stereo(out, count);
	else
		this->mix_mono(out, count);
}

void Stereo_Mixer::mix_mono(blip_sample_t *out_, int count)
{
	int bass = BLIP_READER_BASS(this->bufs[2]);
	BLIP_READER_BEGIN(center, *this->bufs[2]);
	BLIP_READER_ADJ_(center, this->samples_read);

	typedef blip_sample_t stereo_blip_sample_t[stereo];
	auto out = reinterpret_cast<stereo_blip_sample_t *>(out_) + count;
	int offset = -count;
	do
	{
		int32_t s = BLIP_READER_READ(center);
		BLIP_READER_NEXT_IDX_(center, bass, offset);
		BLIP_CLAMP(s, s);

		out[offset][0] = out[offset][1] = static_cast<blip_sample_t>(s);
	} while (++offset);

	BLIP_READER_END(center, *this->bufs[2]);
}

void Stereo_Mixer::mix_stereo(blip_sample_t *out_, int count)
{
	auto out = &out_[count * stereo];

	// do left + center and right + center separately to reduce register load
	auto buf = &this->bufs[2];
	while (true) // loop runs twice
	{
		--buf;
		--out;

		int bass = BLIP_READER_BASS(this->bufs[2]);
		BLIP_READER_BEGIN(side, **buf);
		BLIP_READER_BEGIN(center, *this->bufs[2]);

		BLIP_READER_ADJ_(side, this->samples_read);
		BLIP_READER_ADJ_(center, this->samples_read);

		int offset = -count;
		do
		{
			int32_t s = BLIP_READER_READ_RAW(center) + BLIP_READER_READ_RAW(side);
			s >>= blip_sample_bits - 16;
			BLIP_READER_NEXT_IDX_(side, bass, offset);
			BLIP_READER_NEXT_IDX_(center, bass, offset);
			BLIP_CLAMP(s, s);

			++offset; // before write since out is decremented to slightly before end
			out[offset * stereo] = static_cast<blip_sample_t>(s);
		} while (offset);

		BLIP_READER_END(side, **buf);

		if (buf != this->bufs)
			continue;

		// only end center once
		BLIP_READER_END(center, *this->bufs[2]);
		break;
	}
}
