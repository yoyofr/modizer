/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#include <memory>
#include "apu.h"
#include "resampler.h"

#include "bapu/snes/snes.hpp"
#include "bapu/dsp/sdsp.hpp"
#include "bapu/smp/smp.hpp"

static constexpr int APU_DEFAULT_INPUT_RATE = 31950; // ~59.94Hz
static constexpr int APU_SAMPLE_BLOCK = 48;
static constexpr int APU_NUMERATOR_NTSC = 15664;
static constexpr int APU_DENOMINATOR_NTSC = 328125;
static constexpr int APU_NUMERATOR_PAL = 34176;
static constexpr int APU_DENOMINATOR_PAL = 709379;
// Max number of samples we'll ever generate before call to port API and
// moving the samples to the resampler.
// This is 535 sample frames, which corresponds to 1 video frame + some leeway
// for use with SoundSync, multiplied by 2, for left and right samples.
static constexpr int MINIMUM_BUFFER_SIZE = 550 * 2;

namespace SNES
{
	CPU cpu;
}

namespace spc
{
	static bool sound_in_sync = true;
	static bool sound_enabled = false;

	static std::unique_ptr<Resampler> resampler;

	static int32_t reference_time;
	static uint32_t remainder;

	static constexpr int timing_hack_numerator = 256;
	static int timing_hack_denominator = 256;
	/* Set these to NTSC for now. Will change to PAL in S9xAPUTimingSetSpeedup
	   if necessary on game load. */
	static uint32_t ratio_numerator = APU_NUMERATOR_NTSC;
	static uint32_t ratio_denominator = APU_DENOMINATOR_NTSC;
}

void S9xClearSamples()
{
	spc::resampler->clear();
}

bool S9xMixSamples(uint8_t *dest, int sample_count)
{
	int16_t *out = reinterpret_cast<int16_t *>(dest);

	if (Settings.Mute)
	{
		std::fill_n(&out[0], sample_count, 0);
		S9xClearSamples();
	}
	else
	{
		if (spc::resampler->avail() >= sample_count)
			spc::resampler->read(reinterpret_cast<short *>(out), sample_count);
		else
		{
			std::fill_n(&out[0], sample_count, 0);
			return false;
		}
	}

	spc::sound_in_sync = spc::resampler->space_empty() >= 535 * 2 || !Settings.SoundSync || Settings.TurboMode || Settings.Mute;

	return true;
}

int S9xGetSampleCount()
{
	return spc::resampler->avail();
}

void S9xLandSamples()
{
	spc::sound_in_sync = spc::resampler->space_empty() >= 535 * 2 || !Settings.SoundSync || Settings.TurboMode || Settings.Mute;
}

bool S9xSyncSound()
{
	if (!Settings.SoundSync || spc::sound_in_sync)
		return true;

	S9xLandSamples();

	return spc::sound_in_sync;
}

static void UpdatePlaybackRate()
{
	if (!Settings.SoundInputRate)
		Settings.SoundInputRate = APU_DEFAULT_INPUT_RATE;

	double time_ratio = static_cast<double>(Settings.SoundInputRate) * spc::timing_hack_numerator / (Settings.SoundPlaybackRate * spc::timing_hack_denominator);
	spc::resampler->time_ratio(time_ratio);
}

bool S9xInitSound(int buffer_ms)
{
	// The resampler and spc unit use samples (16-bit short) as arguments.
	int buffer_size_samples = MINIMUM_BUFFER_SIZE;
	int requested_buffer_size_samples = Settings.SoundPlaybackRate * buffer_ms * 2 / 1000;

	if (requested_buffer_size_samples > buffer_size_samples)
		buffer_size_samples = requested_buffer_size_samples;

	spc::resampler.reset(new Resampler(buffer_size_samples));
	if (!spc::resampler)
		return false;

	SNES::dsp.spc_dsp.set_output(spc::resampler.get());

	UpdatePlaybackRate();

	spc::sound_enabled = S9xOpenSoundDevice();

	return spc::sound_enabled;
}

void S9xSetSoundControl(uint8_t voice_switch)
{
	SNES::dsp.spc_dsp.set_stereo_switch((voice_switch << 8) | voice_switch);
}

void S9xSetSoundMute(bool mute)
{
	Settings.Mute = mute;
	if (!spc::sound_enabled)
		Settings.Mute = true;
}

bool S9xInitAPU()
{
	spc::resampler.reset();

	return true;
}

void S9xDeinitAPU()
{
	spc::resampler.reset();
}

static inline int S9xAPUGetClock(int32_t cpucycles)
{
	return (spc::ratio_numerator * (cpucycles - spc::reference_time) + spc::remainder) / spc::ratio_denominator;
}

static inline int S9xAPUGetClockRemainder(int32_t cpucycles)
{
	return (spc::ratio_numerator * (cpucycles - spc::reference_time) + spc::remainder) % spc::ratio_denominator;
}

void S9xAPUExecute()
{
	SNES::smp.clock -= S9xAPUGetClock(CPU.Cycles);
	SNES::smp.enter();

	spc::remainder = S9xAPUGetClockRemainder(CPU.Cycles);

	S9xAPUSetReferenceTime(CPU.Cycles);
}

uint8_t S9xAPUReadPort(int port)
{
	S9xAPUExecute();
	return static_cast<uint8_t>(SNES::smp.port_read(port & 3));
}

void S9xAPUWritePort(int port, uint8_t byte)
{
	S9xAPUExecute();
	SNES::cpu.port_write(port & 3, byte);
}

void S9xAPUSetReferenceTime(int32_t cpucycles)
{
	spc::reference_time = cpucycles;
}

void S9xAPUEndScanline()
{
	S9xAPUExecute();
	SNES::dsp.synchronize();

	if (spc::resampler->space_filled() >= APU_SAMPLE_BLOCK || !spc::sound_in_sync)
		S9xLandSamples();
}

void S9xAPUTimingSetSpeedup(int ticks)
{
	spc::timing_hack_denominator = 256 - ticks;

	spc::ratio_numerator = Settings.PAL ? APU_NUMERATOR_PAL : APU_NUMERATOR_NTSC;
	spc::ratio_denominator = (Settings.PAL ? APU_DENOMINATOR_PAL : APU_DENOMINATOR_NTSC) * spc::timing_hack_denominator / spc::timing_hack_numerator;

	UpdatePlaybackRate();
}

void S9xResetAPU()
{
	spc::reference_time = 0;
	spc::remainder = 0;

	SNES::cpu.reset();
	SNES::smp.power();
	SNES::dsp.power();

	S9xClearSamples();
}
