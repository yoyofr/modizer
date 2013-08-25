/*
 * This file is part of reSID, a MOS6581 SID emulator engine.
 * Copyright (C) 2004  Dag Lem <resid@nimrod.no>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * @author Ken Händel
 *
 */

#define SID_CPP

#include "SID.h"

#include <limits>

#include "Filter6581.h"
#include "Filter8580.h"
#include "Potentiometer.h"
#include "WaveformCalculator.h"
#include "resample/TwoPassSincResampler.h"
#include "resample/ZeroOrderResampler.h"

namespace reSIDfp
{

const int SID::BUS_TTL = 0x9000;

SID::SID() :
	filter6581(new Filter6581()),
	filter8580(new Filter8580()),
	externalFilter(new ExternalFilter()),
	potX(new Potentiometer()),
	potY(new Potentiometer()),
	resampler(0) {

	voice[0] = new Voice();
	voice[1] = new Voice();
	voice[2] = new Voice();

	muted[0] = muted[1] = muted[2] = false;

	reset();
	setChipModel(MOS8580);
}

SID::~SID() {
	delete filter6581;
	delete filter8580;
	delete externalFilter;
	delete potX;
	delete potY;
	delete voice[0];
	delete voice[1];
	delete voice[2];
	delete resampler;
}

void SID::writeImmediate(const int offset, const unsigned char value) {
	switch (offset) {
	case 0x00:
		voice[0]->wave->writeFREQ_LO(value);
		break;
	case 0x01:
		voice[0]->wave->writeFREQ_HI(value);
		break;
	case 0x02:
		voice[0]->wave->writePW_LO(value);
		break;
	case 0x03:
		voice[0]->wave->writePW_HI(value);
		break;
	case 0x04:
		voice[0]->writeCONTROL_REG(muted[0] ? 0 : value);
		break;
	case 0x05:
		voice[0]->envelope->writeATTACK_DECAY(value);
		break;
	case 0x06:
		voice[0]->envelope->writeSUSTAIN_RELEASE(value);
		break;
	case 0x07:
		voice[1]->wave->writeFREQ_LO(value);
		break;
	case 0x08:
		voice[1]->wave->writeFREQ_HI(value);
		break;
	case 0x09:
		voice[1]->wave->writePW_LO(value);
		break;
	case 0x0a:
		voice[1]->wave->writePW_HI(value);
		break;
	case 0x0b:
		voice[1]->writeCONTROL_REG(muted[1] ? 0 : value);
		break;
	case 0x0c:
		voice[1]->envelope->writeATTACK_DECAY(value);
		break;
	case 0x0d:
		voice[1]->envelope->writeSUSTAIN_RELEASE(value);
		break;
	case 0x0e:
		voice[2]->wave->writeFREQ_LO(value);
		break;
	case 0x0f:
		voice[2]->wave->writeFREQ_HI(value);
		break;
	case 0x10:
		voice[2]->wave->writePW_LO(value);
		break;
	case 0x11:
		voice[2]->wave->writePW_HI(value);
		break;
	case 0x12:
		voice[2]->writeCONTROL_REG(muted[2] ? 0 : value);
		break;
	case 0x13:
		voice[2]->envelope->writeATTACK_DECAY(value);
		break;
	case 0x14:
		voice[2]->envelope->writeSUSTAIN_RELEASE(value);
		break;
	case 0x15:
		filter6581->writeFC_LO(value);
		filter8580->writeFC_LO(value);
		break;
	case 0x16:
		filter6581->writeFC_HI(value);
		filter8580->writeFC_HI(value);
		break;
	case 0x17:
		filter6581->writeRES_FILT(value);
		filter8580->writeRES_FILT(value);
		break;
	case 0x18:
		filter6581->writeMODE_VOL(value);
		filter8580->writeMODE_VOL(value);
		break;
	default:
		break;
	}

	/* Update voicesync just in case. */
	voiceSync(false);
}

void SID::ageBusValue(const int n) {
	if (busValueTtl != 0) {
		busValueTtl -= n;
		if (busValueTtl <= 0) {
			busValue = 0;
			busValueTtl = 0;
		}
	}
}

void SID::voiceSync(const bool sync) {
	if (sync) {
		/* Synchronize the 3 waveform generators. */
		for (int i = 0; i < 3 ; i ++) {
			voice[i]->wave->synchronize(voice[(i+1) % 3]->wave, voice[(i+2) % 3]->wave);
		}
	}

	/* Calculate the time to next voice sync */
	nextVoiceSync = std::numeric_limits<int>::max();
	for (int i = 0; i < 3; i ++) {
		const int accumulator = voice[i]->wave->readAccumulator();
		const int freq = voice[i]->wave->readFreq();

		if (voice[i]->wave->readTest() || freq == 0 || !voice[(i+1) % 3]->wave->readSync()) {
			continue;
		}

		const int thisVoiceSync = ((0x7fffff - accumulator) & 0xffffff) / freq + 1;
		if (thisVoiceSync < nextVoiceSync) {
			nextVoiceSync = thisVoiceSync;
		}
	}
}

void SID::setChipModel(const ChipModel model) {
	this->model = model;

	if (model == MOS6581) {
		filter = filter6581;
	} else if (model == MOS8580) {
		filter = filter8580;
	} else {
        char msg[64];
        sprintf(msg,"Don't know how to handle chip type %d",(int)model);
		throw SIDError(msg);
	}

	/* calculate waveform-related tables, feed them to the generator */
	array<short>* tables = WaveformCalculator::getInstance()->buildTable(model);

	/* update voice offsets */
	for (int i = 0; i < 3; i++) {
		voice[i]->envelope->setChipModel(model);
		voice[i]->wave->setChipModel(model);
		voice[i]->wave->setWaveformModels(tables);
	}
}

void SID::reset() {
	for (int i = 0; i < 3; i++) {
		voice[i]->reset();
	}

	filter6581->reset();
	filter8580->reset();
	externalFilter->reset();

	if (resampler) {
		resampler->reset();
	}

	busValue = 0;
	busValueTtl = 0;
	delayedOffset = -1;
	voiceSync(false);
}

void SID::input(const int value) {
	filter6581->input(value);
	filter8580->input(value);
}

unsigned char SID::read(const int offset) {
	unsigned char value;

	switch (offset) {
	case 0x19:
		value = potX->readPOT();
		busValueTtl = BUS_TTL;
		break;
	case 0x1a:
		value = potY->readPOT();
		busValueTtl = BUS_TTL;
		break;
	case 0x1b:
		value = voice[2]->wave->readOSC();
		break;
	case 0x1c:
		value = voice[2]->envelope->readENV();
		busValueTtl = BUS_TTL;
		break;
	default:
		value = busValue;
		busValueTtl /= 2;
		break;
	}

	busValue = value;
	return value;
}

void SID::write(const int offset, const unsigned char value) {
	busValue = value;
	busValueTtl = BUS_TTL;

	if (model == MOS8580) {
		delayedOffset = offset;
		delayedValue = value;
	} else {
		writeImmediate(offset, value);
	}
}

void SID::setSamplingParameters(const float clockFrequency, const SamplingMethod method, const float samplingFrequency, const float highestAccurateFrequency) {
	filter6581->setClockFrequency(clockFrequency);
	filter8580->setClockFrequency(clockFrequency);
	externalFilter->setClockFrequency(clockFrequency);

	delete resampler;

	switch (method) {
	case DECIMATE:
		resampler = new ZeroOrderResampler(clockFrequency, samplingFrequency);
		break;
	case RESAMPLE:
		resampler = new TwoPassSincResampler(clockFrequency, samplingFrequency, highestAccurateFrequency);
		break;
	default:
            char msg[64];
            sprintf(msg,"Unknown samplingmethod: %d",(int)method);
            throw SIDError(msg);
		throw SIDError(msg);
	}
}

void SID::clockSilent(int cycles) {
	ageBusValue(cycles);

	while (cycles != 0) {
		int delta_t = std::min(nextVoiceSync, cycles);
		if (delta_t > 0) {
			if (delayedOffset != -1) {
				delta_t = 1;
			}

			for (int i = 0; i < delta_t; i ++) {
				/* clock waveform generators (can affect OSC3) */
				voice[0]->wave->clock();
				voice[1]->wave->clock();
				voice[2]->wave->clock();

				/* clock ENV3 only */
				voice[2]->envelope->clock();
			}

			if (delayedOffset != -1) {
				writeImmediate(delayedOffset, delayedValue);
				delayedOffset = -1;
			}

			cycles -= delta_t;
			nextVoiceSync -= delta_t;
		}

		if (nextVoiceSync == 0) {
			voiceSync(true);
		}
	}
}

} // namespace reSIDfp
