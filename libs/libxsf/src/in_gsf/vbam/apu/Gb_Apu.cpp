// Gb_Snd_Emu 0.2.0. http://www.slack.net/~ant/

#include <algorithm>
#include "Gb_Apu.h"
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

static const unsigned vol_reg = 0xFF24;
static const unsigned stereo_reg = 0xFF25;
static const unsigned status_reg = 0xFF26;
static const unsigned wave_ram = 0xFF30;

static const int power_mask = 0x80;

void Gb_Apu::treble_eq(const blip_eq_t &eq)
{
	this->good_synth.treble_eq(eq);
	this->med_synth[0].treble_eq(eq);
	this->med_synth[1].treble_eq(eq);
}

int Gb_Apu::calc_output(int osc) const
{
	int bits = this->regs[stereo_reg - start_addr] >> osc;
	return (bits >> 3 & 2) | (bits & 1);
}

void Gb_Apu::set_output(Blip_Buffer *center, Blip_Buffer *left, Blip_Buffer *right, int osc)
{
	// Must be silent (all NULL), mono (left and right NULL), or stereo (none NULL)
	assert(!center || (center && !left && !right) || (center && left && right));
	assert(static_cast<unsigned>(osc) <= osc_count); // fails if you pass invalid osc index

	if (!center || !left || !right)
		left = right = center;

	int i = static_cast<unsigned>(osc) % osc_count;
	do
	{
		auto &o = *this->oscs[i];
		o.outputs[1] = right;
		o.outputs[2] = left;
		o.outputs[3] = center;
		o.output = o.outputs[this->calc_output(i)];
	} while (++i < osc);
}

void Gb_Apu::synth_volume(int iv)
{
	double v = this->volume_ * 0.60 / osc_count / 15 /*steps*/ / 8 /*master vol range*/ * iv;
	this->good_synth.volume(v);
	this->med_synth[0].volume(v);
	this->med_synth[1].volume(v * 1.4);
}

void Gb_Apu::apply_volume()
{
	// TODO: Doesn't handle differing left and right volumes (panning).
	// Not worth the complexity.
	int data = this->regs[vol_reg - start_addr];
	int left = data >> 4 & 7;
	int right = data & 7;
	//if ( data & 0x88 ) dprintf( "Vin: %02X\n", data & 0x88 );
	//if ( left != right ) dprintf( "l: %d r: %d\n", left, right );
	this->synth_volume(std::max(left, right) + 1);
}

void Gb_Apu::volume(double v)
{
	if (!fEqual(this->volume_, v))
	{
		this->volume_ = v;
		this->apply_volume();
	}
}

void Gb_Apu::reset_regs()
{
	std::fill_n(&this->regs[0], 0x20, 0);

	this->square1.reset();
	this->square2.reset();
	this->wave.reset();
	this->noise.reset();

	this->apply_volume();
}

void Gb_Apu::reset_lengths()
{
	this->square1.length_ctr = 64;
	this->square2.length_ctr = 64;
	this->wave.length_ctr = 256;
	this->noise.length_ctr = 64;
}

void Gb_Apu::reduce_clicks(bool reduce)
{
	this->reduce_clicks_ = reduce;

	// Click reduction makes DAC off generate same output as volume 0
	int dac_off_amp = 0;
	if (reduce && this->wave.mode != mode_agb) // AGB already eliminates clicks
		dac_off_amp = -Gb_Osc::dac_bias;

	for (int i = 0; i < osc_count; ++i)
		this->oscs[i]->dac_off_amp = dac_off_amp;

	// AGB always eliminates clicks on wave channel using same method
	if (this->wave.mode == mode_agb)
		this->wave.dac_off_amp = -Gb_Osc::dac_bias;
}

void Gb_Apu::reset(mode_t mode, bool agb_wave)
{
	// Hardware mode
	if (agb_wave)
		mode = mode_agb; // using AGB wave features implies AGB hardware
	this->wave.agb_mask = agb_wave ? 0xFF : 0;
	for (int i = 0; i < osc_count; ++i)
		this->oscs[i]->mode = mode;
	this->reduce_clicks(this->reduce_clicks_);

	// Reset state
	this->frame_time = 0;
	this->last_time = 0;
	this->frame_phase = 0;

	this->reset_regs();
	this->reset_lengths();

	// Load initial wave RAM
	static const uint8_t initial_wave[][16] =
	{
		{ 0x84, 0x40, 0x43, 0xAA, 0x2D, 0x78, 0x92, 0x3C, 0x60, 0x59, 0x59, 0xB0, 0x34, 0xB8, 0x2E, 0xDA },
		{ 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF }
	};
	for (int b = 2; --b >= 0; )
	{
		// Init both banks (does nothing if not in AGB mode)
		// TODO: verify that this works
		this->write_register(0, 0xFF1A, b * 0x40);
		for (unsigned i = 0; i < sizeof(initial_wave[0]); ++i)
			this->write_register(0, i + wave_ram, initial_wave[mode != mode_dmg][i]);
	}
}

void Gb_Apu::set_tempo(double t)
{
	this->frame_period = 4194304 / 512; // 512 Hz
	if (!fEqual(t, 1.0))
		this->frame_period = static_cast<blip_time_t>(this->frame_period / t);
}

Gb_Apu::Gb_Apu()
{
	this->wave.wave_ram = &this->regs[wave_ram - start_addr];

	this->oscs[0] = &this->square1;
	this->oscs[1] = &this->square2;
	this->oscs[2] = &this->wave;
	this->oscs[3] = &this->noise;

	for (int i = osc_count; --i >= 0; )
	{
		auto &o = *this->oscs[i];
		o.regs = &this->regs[i * 5];
		o.output = o.outputs[0] = o.outputs[1] = o.outputs[2] = o.outputs[3] = nullptr;
		o.good_synth = &this->good_synth;
		o.med_synth = &this->med_synth[i == 3 ? 1 : 0];
	}

	this->reduce_clicks_ = false;
	this->set_tempo(1.0);
	this->volume_ = 1.0;
	this->reset();
}

void Gb_Apu::run_until_(blip_time_t end_time)
{
	while (true)
	{
		// run oscillators
		blip_time_t time = end_time;
		if (time > this->frame_time)
			time = this->frame_time;

		this->square1.run(this->last_time, time);
		this->square2.run(this->last_time, time);
		this->wave.run(this->last_time, time);
		this->noise.run(this->last_time, time);
		this->last_time = time;

		if (time == end_time)
			break;

		// run frame sequencer
		this->frame_time += this->frame_period * Gb_Osc::clk_mul;
		switch (this->frame_phase++)
		{
			case 2:
			case 6:
				// 128 Hz
				this->square1.clock_sweep();
			case 0:
			case 4:
				// 256 Hz
				this->square1.clock_length();
				this->square2.clock_length();
				this->wave.clock_length();
				this->noise.clock_length();
				break;
			case 7:
				// 64 Hz
				this->frame_phase = 0;
				this->square1.clock_envelope();
				this->square2.clock_envelope();
				this->noise.clock_envelope();
		}
	}
}

void Gb_Apu::run_until(blip_time_t time)
{
	assert(time >= this->last_time); // end_time must not be before previous time
	if (time > this->last_time)
		this->run_until_(time);
}

void Gb_Apu::end_frame(blip_time_t end_time)
{
	if (end_time > this->last_time)
		this->run_until(end_time);

	this->frame_time -= end_time;
	assert(this->frame_time >= 0);

	this->last_time -= end_time;
	assert(this->last_time >= 0);
}

void Gb_Apu::silence_osc(Gb_Osc &o)
{
	int delta = -o.last_amp;
	if (delta)
	{
		o.last_amp = 0;
		if (o.output)
		{
			o.output->set_modified();
			this->med_synth[0].offset(this->last_time, delta, o.output);
		}
	}
}

void Gb_Apu::apply_stereo()
{
	for (int i = osc_count; --i >= 0; )
	{
		auto &o = *this->oscs[i];
		auto out = o.outputs[this->calc_output(i)];
		if (o.output != out)
		{
			this->silence_osc(o);
			o.output = out;
		}
	}
}

void Gb_Apu::write_register(blip_time_t time, unsigned addr, int data)
{
	assert(static_cast<unsigned>(data) < 0x100);

	int reg = addr - start_addr;
	if (static_cast<unsigned>(reg) >= register_count)
	{
		assert(false);
		return;
	}

	if (addr < status_reg && !(this->regs[status_reg - start_addr] & power_mask))
	{
		// Power is off

		// length counters can only be written in DMG mode
		if (this->wave.mode != mode_dmg || (reg != 1 && reg != 6 && reg != 11 && reg != 16))
			return;

		if (reg < 10)
			data &= 0x3F; // clear square duty
	}

	this->run_until(time);

	if (addr >= wave_ram)
		this->wave.write(addr, data);
	else
	{
		int old_data = this->regs[reg];
		this->regs[reg] = data;

		if (addr < vol_reg)
			// Oscillator
			this->write_osc(reg / 5, reg, old_data, data);
		else if (addr == vol_reg && data != old_data)
		{
			// Master volume
			for (int i = osc_count; --i >= 0; )
				this->silence_osc(*this->oscs[i]);

			this->apply_volume();
		}
		else if (addr == stereo_reg)
			// Stereo panning
			this->apply_stereo();
		else if (addr == status_reg && ((data ^ old_data) & power_mask))
		{
			// Power control
			this->frame_phase = 0;
			for (int i = osc_count; --i >= 0; )
				this->silence_osc(*this->oscs[i]);

			this->reset_regs();
			if (this->wave.mode != mode_dmg)
				this->reset_lengths();

			this->regs[status_reg - start_addr] = data;
		}
	}
}

int Gb_Apu::read_register(blip_time_t time, unsigned addr)
{
	this->run_until(time);

	int reg = addr - start_addr;
	if (static_cast<unsigned>(reg) >= register_count)
	{
		assert(false);
		return 0;
	}

	if (addr >= wave_ram)
		return this->wave.read(addr);

	// Value read back has some bits always set
	static const uint8_t masks[] =
	{
		0x80, 0x3F, 0x00, 0xFF, 0xBF,
		0xFF, 0x3F, 0x00, 0xFF, 0xBF,
		0x7F, 0xFF, 0x9F, 0xFF, 0xBF,
		0xFF, 0xFF, 0x00, 0x00, 0xBF,
		0x00, 0x00, 0x70,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	};
	int mask = masks[reg];
	if (this->wave.agb_mask && (reg == 10 || reg == 12))
		mask = 0x1F; // extra implemented bits in wave regs on AGB
	int data = this->regs[reg] | mask;

	// Status register
	if (addr == status_reg)
	{
		data &= 0xF0;
		data |= static_cast<int>(this->square1.enabled);
		data |= static_cast<int>(this->square2.enabled) << 1;
		data |= static_cast<int>(this->wave.enabled) << 2;
		data |= static_cast<int>(this->noise.enabled) << 3;
	}

	return data;
}

int Gb_Apu::read_status()
{
	int data = static_cast<int>(this->square1.enabled);
	data |= static_cast<int>(this->square2.enabled) << 1;
	data |= static_cast<int>(this->wave.enabled) << 2;
	data |= static_cast<int>(this->noise.enabled) << 3;
	return data;
}
