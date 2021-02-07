#pragma once

// Sound emulation setup/options and GBA sound emulation

#include <cstdint>

//// Setup/options (these affect GBA and GB sound)

// Initializes sound and returns true if successful. Sets sound quality to
// current value in soundQuality global.
bool soundInit();

// Manages muting bitmask. The bits control the following channels:
// 0x001 Pulse 1
// 0x002 Pulse 2
// 0x004 Wave
// 0x008 Noise
// 0x100 PCM 1
// 0x200 PCM 2
void soundSetEnable(int mask);

// Pauses/resumes system sound output
void soundPause();
void soundResume();

// Cleans up sound. Afterwards, soundInit() can be called again.
void soundShutdown();

//// GBA sound options

void soundSetSampleRate(long sampleRate);

// Sound settings
extern bool soundInterpolation; // 1 if PCM should have low-pass filtering

//// GBA sound emulation

// GBA sound registers
enum
{
	SGCNT0_H = 0x82,
	FIFOA_L = 0xa0,
	FIFOA_H = 0xa2,
	FIFOB_L = 0xa4,
	FIFOB_H = 0xa6
};

// Resets emulated sound hardware
void soundReset();

// Emulates write to sound hardware
void soundEvent(uint32_t addr, uint8_t data);
void soundEvent(uint32_t addr, uint16_t data); // TODO: error-prone to overload like this

// Notifies emulator that a timer has overflowed
void soundTimerOverflow(int which);

// Notifies emulator that SOUND_CLOCK_TICKS clocks have passed
void psoundTickfn();
extern int SOUND_CLOCK_TICKS; // Number of 16.8 MHz clocks between calls to soundTick()
extern int soundTicks; // Number of 16.8 MHz clocks until soundTick() will be called

class Multi_Buffer;

void flush_samples(Multi_Buffer *buffer);
