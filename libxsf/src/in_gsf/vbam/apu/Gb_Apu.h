// Nintendo Game Boy sound hardware emulator with save state support

// Gb_Snd_Emu 0.2.0
#pragma once

#include "Gb_Oscs.h"

class Gb_Apu
{
public:
	// Basics

	// Clock rate that sound hardware runs at.
	enum { clock_rate = 4194304 * GB_APU_OVERCLOCK };

	// Sets buffer(s) to generate sound into. If left and right are NULL, output is mono.
	// If all are NULL, no output is generated but other emulation still runs.
	// If chan is specified, only that channel's output is changed, otherwise all are.
	enum { osc_count = 4 }; // 0: Square 1, 1: Square 2, 2: Wave, 3: Noise
	void set_output(Blip_Buffer *center, Blip_Buffer *left = nullptr, Blip_Buffer *right = nullptr, int chan = osc_count);

	// Resets hardware to initial power on state BEFORE boot ROM runs. Mode selects
	// sound hardware. Additional AGB wave features are enabled separately.
	enum mode_t
	{
		mode_dmg, // Game Boy monochrome
		mode_cgb, // Game Boy Color
		mode_agb // Game Boy Advance
	};
	void reset(mode_t mode = mode_cgb, bool agb_wave = false);

	// Reads and writes must be within the start_addr to end_addr range, inclusive.
	// Addresses outside this range are not mapped to the sound hardware.
	enum { start_addr = 0xFF10 };
	enum { end_addr = 0xFF3F };
	enum { register_count = end_addr - start_addr + 1 };

	// Times are specified as the number of clocks since the beginning of the
	// current time frame.

	// Emulates CPU write of data to addr at specified time.
	void write_register(blip_time_t time, unsigned addr, int data);

	// Emulates CPU read from addr at specified time.
	int read_register(blip_time_t time, unsigned addr);

	// Emulates sound hardware up to specified time, ends current time frame, then
	// starts a new frame at time 0.
	void end_frame(blip_time_t frame_length);

	// Sound adjustments

	// Sets overall volume, where 1.0 is normal.
	void volume(double);

	// If true, reduces clicking by disabling DAC biasing. Note that this reduces
	// emulation accuracy, since the clicks are authentic.
	void reduce_clicks(bool reduce = true);

	// Sets treble equalization.
	void treble_eq(const blip_eq_t &);

	// Treble and bass values for various hardware.
	enum
	{
		speaker_treble = -47, // speaker on system
		speaker_bass = 2000,
		dmg_treble = 0, // headphones on each system
		dmg_bass = 30,
		cgb_treble = 0,
		cgb_bass = 300, // CGB has much less bass
		agb_treble = 0,
		agb_bass = 30
	};

	// Sets frame sequencer rate, where 1.0 is normal. Meant for adjusting the
	// tempo in a game music player.
	void set_tempo(double);

	Gb_Apu();

private:
	// noncopyable
	Gb_Apu(const Gb_Apu &);
	Gb_Apu &operator=(const Gb_Apu &);

	Gb_Osc *oscs[osc_count];
	blip_time_t last_time; // time sound emulator has been run to
	blip_time_t frame_period; // clocks between each frame sequencer step
	double volume_;
	bool reduce_clicks_;

	Gb_Sweep_Square square1;
	Gb_Square square2;
	Gb_Wave wave;
	Gb_Noise noise;
	blip_time_t frame_time; // time of next frame sequencer action
	int frame_phase; // phase of next frame sequencer step
	enum { regs_size = register_count + 0x10 };
	uint8_t regs[regs_size];// last values written to registers

	// large objects after everything else
	Gb_Osc::Good_Synth good_synth;
	Gb_Osc::Med_Synth med_synth[2];

	void reset_lengths();
	void reset_regs();
	int calc_output(int osc) const;
	void apply_stereo();
	void apply_volume();
	void synth_volume(int);
	void run_until_(blip_time_t);
	void run_until(blip_time_t);
	void silence_osc(Gb_Osc &);
	void write_osc(int index, int reg, int old_data, int data);

public:
	int read_status();
};
