/*
* Utility used to access "historic" SID register state.
*
* WebSid (c) 2023 Jürgen Wothke
* version 0.96
*
* Terms of Use: This software is licensed under a CC BY-NC-SA
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/

#include <cstdlib>
#include <cmath>

extern "C" {
#include "memory.h"
}

#include "sid.h"
extern "C" uint8_t sidReadVoiceLevel(uint8_t sid_idx, uint8_t voice_idx);
extern "C" uint8_t sidReadMem(uint16_t addr);

#include "SidSnapshot.h"


using websid::SidSnapshot;


// keep the private stuff private..

#define REGS2RECORD (25 + 3)	// only the first 25 regs (trailing paddle regs (etc) are ignored) - but adding "envelope levels" of all three voices

static uint8_t** _reg_snapshots = 0;
static uint32_t _snapshot_smpl_count = 0;
static uint32_t _snapshot_toggle = 0;

static uint16_t _snapshot_alloc = 0;
static uint32_t _snapshot_pos = 0;
static uint16_t _snapshot_max = 0;

static uint16_t _chunk_size = 0;
static uint32_t _proc_buf_size = 0;



uint16_t getSIDRegister(uint8_t sid_idx, uint16_t reg) {
       return  reg >= 0x1B ? sidReadMem(SID::getSIDBaseAddr(sid_idx) + reg) : memReadIO(SID::getSIDBaseAddr(sid_idx) + reg);
}

void SidSnapshot::init(uint16_t chunk_size, uint32_t proc_buf_size)
{
	_chunk_size = chunk_size;
	_proc_buf_size = proc_buf_size;

	_snapshot_smpl_count = 0;

	if (!_reg_snapshots)
	{
		_reg_snapshots = (uint8_t**) calloc(MAX_SIDS, sizeof(uint8_t*));
	}

	uint16_t n = (uint16_t)ceil((float)_proc_buf_size / _chunk_size);	// interval different from UI's "ticks" based calcs

	if (_snapshot_alloc < n)
	{
		for (uint8_t i = 0; i < MAX_SIDS; i++) {
			if (_reg_snapshots[i]) free(_reg_snapshots[i]);

			_reg_snapshots[i] = (uint8_t*)calloc(REGS2RECORD, n * 2);	// double buffer the duration of WebAudio buffer
		}

		_snapshot_alloc = n;
	}
	else { }	// just leave excess size unused

	_snapshot_max = n;
	_snapshot_pos = 0;
	_snapshot_toggle = 0;
}

void SidSnapshot::record()
{
	uint32_t offset = _snapshot_pos * REGS2RECORD;

	for (uint8_t i = 0; i < SID::getNumberUsedChips(); i++) {
		uint8_t* sid_buf = &(_reg_snapshots[i][offset]);

		uint16_t j;
		for (j = 0; j < REGS2RECORD-3; j++) {
			sid_buf[j] = getSIDRegister(i, j);
		}
		// save "envelope" levels of all three voices
		sid_buf[j++] = sidReadVoiceLevel(i, 0);
		sid_buf[j++] = sidReadVoiceLevel(i, 1);
		sid_buf[j] = sidReadVoiceLevel(i, 2);
	}

	_snapshot_smpl_count += _chunk_size;

	// setup next target buffer location
	if (_snapshot_smpl_count >= _proc_buf_size)
	{
		if (!_snapshot_toggle)
		{
			_snapshot_toggle = 1;	// switch to 2nd buffer
			_snapshot_pos = _snapshot_max;
		}
		else
		{
			_snapshot_toggle = 0;	// switch to 1st buffer
			_snapshot_pos = 0;
		}
		_snapshot_smpl_count -= _proc_buf_size; // track overflow

	}
	else
	{
		_snapshot_pos++;
	}
}

uint16_t SidSnapshot::getRegister(uint8_t sid_idx, uint16_t reg, uint8_t buf_idx, uint32_t tick)
{

	if ((reg < (REGS2RECORD-3))&&(buf_idx<0xFF))
	{
		// cached snapshots are spaced "1 frame" apart while WebAudio-side measures time in 256-sample ticks..
		// map the respective input to the corresponding cache block (the imprecision should not be relevant
		// for the actual use cases.. see "piano view" in DeepSid)

		uint8_t* sid_buf = &(_reg_snapshots[sid_idx][buf_idx ? _snapshot_max * REGS2RECORD : 0]);
		uint32_t idx = (tick << 8) / _chunk_size;

		sid_buf += idx * REGS2RECORD;
		return sid_buf[reg];
	}
	else
	{
		// fallback to latest state of emulator
		return getSIDRegister(sid_idx, reg);
	}
}

uint16_t SidSnapshot::readVoiceLevel(uint8_t sid_idx, uint8_t voice_idx, uint8_t buf_idx, uint32_t tick)
{
	uint8_t* sid_buf = &(_reg_snapshots[sid_idx][buf_idx ? _snapshot_max*REGS2RECORD : 0]);
	uint32_t idx = (tick << 8) / _chunk_size;
	sid_buf += idx * REGS2RECORD;

	return 	sid_buf[REGS2RECORD -3 + voice_idx];
}

