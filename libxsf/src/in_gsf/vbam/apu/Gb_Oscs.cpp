// Gb_Snd_Emu 0.2.0. http://www.slack.net/~ant/

#include "Gb_Apu.h"

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

static const bool cgb_02 = false; // enables bug in early CGB units that causes problems in some games
static const bool cgb_05 = false; // enables CGB-05 zombie behavior

static const int trigger_mask = 0x80;
static const int length_enabled = 0x40;

void Gb_Osc::reset()
{
	this->output = nullptr;
	this->last_amp = this->delay = 0;
	this->phase = 0;
	this->enabled = false;
}

void Gb_Osc::update_amp(blip_time_t time, int new_amp)
{
	this->output->set_modified();
	int delta = new_amp - this->last_amp;
	if (delta)
	{
		this->last_amp = new_amp;
		this->med_synth->offset(time, delta, this->output);
	}
}

// Units

void Gb_Osc::clock_length()
{
	if ((this->regs[4] & length_enabled) && this->length_ctr)
	{
		if (--this->length_ctr <= 0)
			this->enabled = false;
	}
}

int Gb_Env::reload_env_timer()
{
	int raw = this->regs[2] & 7;
	this->env_delay = raw ? raw : 8;
	return raw;
}

void Gb_Env::clock_envelope()
{
	if (this->env_enabled && --this->env_delay <= 0 && this->reload_env_timer())
	{
		int v = this->volume + (this->regs[2] & 0x08 ? 1 : -1);
		if (0 <= v && v <= 15)
			this->volume = v;
		else
			this->env_enabled = false;
	}
}

void Gb_Sweep_Square::reload_sweep_timer()
{
	this->sweep_delay = (this->regs[0] & period_mask) >> 4;
	if (!this->sweep_delay)
		this->sweep_delay = 8;
}

void Gb_Sweep_Square::calc_sweep(bool update)
{
	int shift = this->regs[0] & shift_mask;
	int delta = this->sweep_freq >> shift;
	this->sweep_neg = !!(this->regs[0] & 0x08);
	int freq = this->sweep_freq + (this->sweep_neg ? -delta : delta);

	if (freq > 0x7FF)
		this->enabled = false;
	else if (shift && update)
	{
		this->sweep_freq = freq;

		this->regs[3] = freq & 0xFF;
		this->regs[4] = (this->regs[4] & ~0x07) | (freq >> 8 & 0x07);
	}
}

void Gb_Sweep_Square::clock_sweep()
{
	if (--this->sweep_delay <= 0)
	{
		this->reload_sweep_timer();
		if (this->sweep_enabled && (this->regs[0] & period_mask))
		{
			this->calc_sweep(true);
			this->calc_sweep(false);
		}
	}
}

int Gb_Wave::access(unsigned addr) const
{
	if (this->enabled && this->mode != Gb_Apu::mode_agb)
	{
		addr = this->phase & (bank_size - 1);
		if (this->mode == Gb_Apu::mode_dmg)
		{
			++addr;
			if (this->delay > clk_mul)
				return -1; // can only access within narrow time window while playing
		}
		addr >>= 1;
	}
	return addr & 0x0F;
}

// write_register

int Gb_Osc::write_trig(int frame_phase, int max_len, int old_data)
{
	int data = this->regs[4];

	if ((frame_phase & 1) && !(old_data & length_enabled) && this->length_ctr)
	{
		if ((data & length_enabled) || cgb_02)
			--this->length_ctr;
	}

	if (data & trigger_mask)
	{
		this->enabled = true;
		if (!this->length_ctr)
		{
			this->length_ctr = max_len;
			if ((frame_phase & 1) && (data & length_enabled))
				--this->length_ctr;
		}
	}

	if (!this->length_ctr)
		this->enabled = false;

	return data & trigger_mask;
}

void Gb_Env::zombie_volume(int old, int data)
{
	int v = this->volume;
	if (this->mode == Gb_Apu::mode_agb || cgb_05)
	{
		// CGB-05 behavior, very close to AGB behavior as well
		if ((old ^ data) & 8)
		{
			if (!(old & 8))
			{
				++v;
				if (old & 7)
					++v;
			}

			v = 16 - v;
		}
		else if ((old & 0x0F) == 8)
			++v;
	}
	else
	{
		// CGB-04&02 behavior, very close to MGB behavior as well
		if (!(old & 7) && this->env_enabled)
			++v;
		else if (!(old & 8))
			v += 2;

		if ((old ^ data) & 8)
			v = 16 - v;
	}
	this->volume = v & 0x0F;
}

bool Gb_Env::write_register(int frame_phase, int reg, int old, int data)
{
	static const int max_len = 64;

	switch (reg)
	{
		case 1:
			this->length_ctr = max_len - (data & (max_len - 1));
			break;

		case 2:
			if (!this->dac_enabled())
				this->enabled = false;

			this->zombie_volume(old, data);

			if ((data & 7) && this->env_delay == 8)
			{
				this->env_delay = 1;
				this->clock_envelope(); // TODO: really happens at next length clock
			}
			break;

		case 4:
			if (this->write_trig(frame_phase, max_len, old))
			{
				this->volume = this->regs[2] >> 4;
				this->reload_env_timer();
				this->env_enabled = true;
				if (frame_phase == 7)
					++this->env_delay;
				if (!this->dac_enabled())
					this->enabled = false;
				return true;
			}
	}
	return false;
}

bool Gb_Square::write_register(int frame_phase, int reg, int old_data, int data)
{
	bool result = Gb_Env::write_register(frame_phase, reg, old_data, data);
	if (result)
		this->delay = (this->delay & (4 * clk_mul - 1)) + this->period();
	return result;
}

void Gb_Noise::write_register(int frame_phase, int reg, int old_data, int data)
{
	if (Gb_Env::write_register(frame_phase, reg, old_data, data))
	{
		this->phase = 0x7FFF;
		this->delay += 8 * clk_mul;
	}
}

void Gb_Sweep_Square::write_register(int frame_phase, int reg, int old_data, int data)
{
	if (!reg && this->sweep_enabled && this->sweep_neg && !(data & 0x08))
		this->enabled = false; // sweep negate disabled after used

	if (Gb_Square::write_register(frame_phase, reg, old_data, data))
	{
		this->sweep_freq = this->frequency();
		this->sweep_neg = false;
		this->reload_sweep_timer();
		this->sweep_enabled = !!(this->regs[0] & (period_mask | shift_mask));
		if (this->regs[0] & shift_mask)
			this->calc_sweep(false);
	}
}

void Gb_Wave::corrupt_wave()
{
	int pos = ((this->phase + 1) & (bank_size - 1)) >> 1;
	if (pos < 4)
		this->wave_ram[0] = this->wave_ram[pos];
	else
		for (int i = 4; --i >= 0; )
			this->wave_ram[i] = this->wave_ram[(pos & ~3) + i];
}

void Gb_Wave::write_register(int frame_phase, int reg, int old_data, int data)
{
	static const int max_len = 256;

	switch (reg)
	{
		case 0:
			if (!this->dac_enabled())
				this->enabled = false;
			break;

		case 1:
			this->length_ctr = max_len - data;
			break;

		case 4:
			bool was_enabled = this->enabled;
			if (this->write_trig(frame_phase, max_len, old_data))
			{
				if (!this->dac_enabled())
					this->enabled = false;
				else if (this->mode == Gb_Apu::mode_dmg && was_enabled && static_cast<unsigned>(this->delay - 2 * clk_mul) < 2 * clk_mul)
					this->corrupt_wave();

				this->phase = 0;
				this->delay = this->period() + 6 * clk_mul;
			}
	}
}

void Gb_Apu::write_osc(int index, int reg, int old_data, int data)
{
	reg -= index * 5;
	switch (index)
	{
		case 0:
			this->square1.write_register(this->frame_phase, reg, old_data, data);
			break;
		case 1:
			this->square2.write_register(this->frame_phase, reg, old_data, data);
			break;
		case 2:
			this->wave.write_register(this->frame_phase, reg, old_data, data);
			break;
		case 3:
			this->noise.write_register(this->frame_phase, reg, old_data, data);
	}
}

// Synthesis

void Gb_Square::run(blip_time_t time, blip_time_t end_time)
{
	// Calc duty and phase
	static const uint8_t duty_offsets[] = { 1, 1, 3, 7 };
	static const uint8_t duties[] = { 1, 2, 4, 6 };
	int duty_code = this->regs[1] >> 6;
	int duty_offset = duty_offsets[duty_code];
	int duty = duties[duty_code];
	if (this->mode == Gb_Apu::mode_agb)
	{
		// AGB uses inverted duty
		duty_offset -= duty;
		duty = 8 - duty;
	}
	int ph = (this->phase + duty_offset) & 7;

	// Determine what will be generated
	int vol = 0;
	auto out = this->output;
	if (out)
	{
		int amp = this->dac_off_amp;
		if (this->dac_enabled())
		{
			if (this->enabled)
				vol = this->volume;

			amp = -dac_bias;
			if (this->mode == Gb_Apu::mode_agb)
				amp = -(vol >> 1);

			// Play inaudible frequencies as constant amplitude
			if (this->frequency() >= 0x7FA && this->delay < 32 * clk_mul)
			{
				amp += (vol * duty) >> 3;
				vol = 0;
			}

			if (ph < duty)
			{
				amp += vol;
				vol = -vol;
			}
		}
		this->update_amp(time, amp);
	}

	// Generate wave
	time += this->delay;
	if (time < end_time)
	{
		int per = this->period();
		if (!vol)
		{
			// Maintain phase when not playing
			int32_t count = (end_time - time + per - 1) / per;
			ph += count; // will be masked below
			time += static_cast<blip_time_t>(count) * per;
		}
		else
		{
			// Output amplitude transitions
			int delta = vol;
			do
			{
				ph = (ph + 1) & 7;
				if (!ph || ph == duty)
				{
					this->good_synth->offset_inline(time, delta, out);
					delta = -delta;
				}
				time += per;
			} while (time < end_time);

			if (delta != vol)
				this->last_amp -= delta;
		}
		this->phase = (ph - duty_offset) & 7;
	}
	this->delay = time - end_time;
}

// Quickly runs LFSR for a large number of clocks. For use when noise is generating
// no sound.
static unsigned run_lfsr(unsigned s, unsigned mask, int32_t count)
{
	static const bool optimized = true; // set to false to use only unoptimized loop in middle

	// optimization used in several places:
	// ((s & (1 << b)) << n) ^ ((s & (1 << b)) << (n + 1)) = (s & (1 << b)) * (3 << n)

	if (mask == 0x4000 && optimized)
	{
		if (count >= 32767)
			count %= 32767;

		// Convert from Fibonacci to Galois configuration,
		// shifted left 1 bit
		s ^= (s & 1) * 0x8000;

		// Each iteration is equivalent to clocking LFSR 255 times
		while ((count -= 255) > 0)
			s ^= ((s & 0xE) << 12) ^ ((s & 0xE) << 11) ^ (s >> 3);
		count += 255;

		// Each iteration is equivalent to clocking LFSR 15 times
		// (interesting similarity to single clocking below)
		while ((count -= 15) > 0)
			s ^= ((s & 2) * (3 << 13)) ^ (s >> 1);
		count += 15;

		// Remaining singles
		while (--count >= 0)
			s = ((s & 2) * (3 << 13)) ^ (s >> 1);

		// Convert back to Fibonacci configuration
		s &= 0x7FFF;
	}
	else if (count < 8 || !optimized)
	{
		// won't fully replace upper 8 bits, so have to do the unoptimized way
		while (--count >= 0)
			s = (s >> 1 | mask) ^ (mask & (0 - ((s - 1) & 2)));
	}
	else
	{
		if (count > 127)
		{
			count %= 127;
			if (!count)
				count = 127; // must run at least once
		}

		// Need to keep one extra bit of history
		s = s << 1 & 0xFF;

		// Convert from Fibonacci to Galois configuration,
		// shifted left 2 bits
		s ^= (s & 2) * 0x80;

		// Each iteration is equivalent to clocking LFSR 7 times
		// (interesting similarity to single clocking below)
		while ((count -= 7) > 0)
			s ^= ((s & 4) * (3 << 5)) ^ (s >> 1);
		count += 7;

		// Remaining singles
		while (--count >= 0)
			s = ((s & 4) * (3 << 5)) ^ (s >> 1);

		// Convert back to Fibonacci configuration and
		// repeat last 8 bits above significant 7
		s = (s << 7 & 0x7F80) | (s >> 1 & 0x7F);
	}

	return s;
}

void Gb_Noise::run(blip_time_t time, blip_time_t end_time)
{
	// Determine what will be generated
	int vol = 0;
	auto out = this->output;
	if (out)
	{
		int amp = this->dac_off_amp;
		if (this->dac_enabled())
		{
			if (this->enabled)
				vol = this->volume;

			amp = -dac_bias;
			if (this->mode == Gb_Apu::mode_agb)
				amp = -(vol >> 1);

			if (!(this->phase & 1))
			{
				amp += vol;
				vol = -vol;
			}
		}

		// AGB negates final output
		if (this->mode == Gb_Apu::mode_agb)
		{
			vol = -vol;
			amp = -amp;
		}

		this->update_amp(time, amp);
	}

	// Run timer and calculate time of next LFSR clock
	static const uint8_t period1s[] = { 1, 2, 4, 6, 8, 10, 12, 14 };
	int period1 = period1s[this->regs[3] & 7] * clk_mul;
	int32_t extra = (end_time - time) - this->delay;
	int per2 = this->period2();
	time += this->delay + ((this->divider ^ (per2 >> 1)) & (per2 - 1)) * period1;

	int32_t count = extra < 0 ? 0 : (extra + period1 - 1) / period1;
	this->divider = (this->divider - count) & period2_mask;
	this->delay = count * period1 - extra;

	// Generate wave
	if (time < end_time)
	{
		unsigned mask = this->lfsr_mask();
		unsigned bits = this->phase;

		int per = this->period2(period1 * 8);
		if (this->period2_index() >= 0xE)
			time = end_time;
		else if (!vol)
		{
			// Maintain phase when not playing
			count = (end_time - time + per - 1) / per;
			time += static_cast<blip_time_t>(count) * per;
			bits = run_lfsr(bits, ~mask, count);
		}
		else
		{
			// Output amplitude transitions
			int delta = -vol;
			do
			{
				unsigned changed = bits + 1;
				bits = bits >> 1 & mask;
				if (changed & 2)
				{
					bits |= ~mask;
					delta = -delta;
					this->med_synth->offset_inline(time, delta, out);
				}
				time += per;
			} while (time < end_time);

			if (delta == vol)
				this->last_amp += delta;
		}
		this->phase = bits;
	}
}

void Gb_Wave::run(blip_time_t time, blip_time_t end_time)
{
	// Calc volume
	static const uint8_t volumes[] = { 0, 4, 2, 1, 3, 3, 3, 3 };
	static const int volume_shift = 2;
	int volume_idx = this->regs[2] >> 5 & (this->agb_mask | 3); // 2 bits on DMG/CGB, 3 on AGB
	int volume_mul = volumes[volume_idx];

	// Determine what will be generated
	int playing = 0;
	auto out = this->output;
	if (out)
	{
		int amp = this->dac_off_amp;
		if (this->dac_enabled())
		{
			// Play inaudible frequencies as constant amplitude
			amp = 8 << 4; // really depends on average of all samples in wave

			// if delay is larger, constant amplitude won't start yet
			if (this->frequency() <= 0x7FB || this->delay > 15 * clk_mul)
			{
				if (volume_mul)
					playing = static_cast<int>(this->enabled);

				amp = (this->sample_buf << (this->phase << 2 & 4) & 0xF0) * playing;
			}

			amp = ((amp * volume_mul) >> (volume_shift + 4)) - dac_bias;
		}
		this->update_amp(time, amp);
	}

	// Generate wave
	time += this->delay;
	if (time < end_time)
	{
		auto wave = this->wave_ram;

		// wave size and bank
		static const int size20_mask = 0x20;
		int flags = this->regs[0] & this->agb_mask;
		int wave_mask = (flags & size20_mask) | 0x1F;
		int swap_banks = 0;
		if (flags & bank40_mask)
		{
			swap_banks = flags & size20_mask;
			wave += bank_size / 2 - (swap_banks >> 1);
		}

		int ph = this->phase ^ swap_banks;
		ph = (ph + 1) & wave_mask; // pre-advance

		int per = this->period();
		if (!playing)
		{
			// Maintain phase when not playing
			int32_t count = (end_time - time + per - 1) / per;
			ph += count; // will be masked below
			time += static_cast<blip_time_t>(count) * per;
		}
		else
		{
			// Output amplitude transitions
			int lamp = this->last_amp + dac_bias;
			do
			{
				// Extract nybble
				int nybble = wave[ph >> 1] << (ph << 2 & 4) & 0xF0;
				ph = (ph + 1) & wave_mask;

				// Scale by volume
				int amp = (nybble * volume_mul) >> (volume_shift + 4);

				int delta = amp - lamp;
				if (delta)
				{
					lamp = amp;
					this->med_synth->offset_inline(time, delta, out);
				}
				time += per;
			} while (time < end_time);
			this->last_amp = lamp - dac_bias;
		}
		ph = (ph - 1) & wave_mask; // undo pre-advance and mask position

		// Keep track of last byte read
		if (this->enabled)
			this->sample_buf = wave[ph >> 1];

		this->phase = ph ^ swap_banks; // undo swapped banks
	}
	this->delay = time - end_time;
}
