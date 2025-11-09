/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2024 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000 Simon White
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "mixer.h"

#include <cassert>
#include <cstring>

#include "sidemu.h"


namespace libsidplayfp
{

void Mixer::clockChips()
{
    for (sidemu* chip: m_chips)
        chip->clock();
}

void Mixer::resetBufs()
{
    for (sidemu* chip: m_chips)
        chip->bufferpos(0);
}

//YOYOFR
int Mixer::samplesReady() {
    return m_chips.front()->bufferpos();
}
//YOYOFR

void Mixer::doMix()
{
    short *outputBuffer = m_sampleBuffer + m_sampleIndex;

    // extract buffer info now that the SID is updated.
    // clock() may update bufferpos.
    // NB: if more than one chip exists, their bufferpos is identical to first chip's.
    const int sampleCount = m_chips.front()->bufferpos();

    int i = 0;
    while (
        (i < sampleCount) &&
        (m_sampleIndex < m_sampleCount) &&      // Handle whatever output the sid has generated so far
        (i + m_fastForwardFactor < sampleCount) // Are there enough samples to generate the next one?
    )
    {
        // This is a crude boxcar low-pass filter to
        // reduce aliasing during fast forward.
        for (size_t k = 0; k < m_chips.size(); k++)
        {
            int_least32_t sample = 0;
            const short *buffer = m_chips[k]->buffer() + i;
            for (int j = 0; j < m_fastForwardFactor; j++)
            {
                sample += buffer[j];
            }

            m_iSamples[k] = sample / m_fastForwardFactor;
        }

        // increment i to mark we ate some samples, finish the boxcar thing.
        i += m_fastForwardFactor;

        const unsigned int channels = m_stereo ? 2 : 1;
        for (unsigned int ch = 0; ch < channels; ch++)
        {
            const int_least32_t tmp = (this->*(m_scale[ch]))(ch);
            assert(tmp >= -32768 && tmp <= 32767);
            *outputBuffer++ = static_cast<short>(tmp);
            m_sampleIndex++;
        }
    }

    // move the unhandled data to start of buffer, if any.
    const int samplesLeft = sampleCount - i;
    assert(samplesLeft >= 0);

    for (sidemu* chip: m_chips)
    {
        short* buffer = chip->buffer();
        std::memmove(buffer, buffer+i, samplesLeft*2);
        chip->bufferpos(samplesLeft);
    }

    m_wait = static_cast<uint_least32_t>(samplesLeft) > m_sampleCount;
}

void Mixer::begin(short *buffer, uint_least32_t count)
{
    // sanity checks

    // don't allow odd counts for stereo playback
    if (m_stereo && (count & 1))
        throw badBufferSize();

    m_sampleIndex  = 0;
    m_sampleCount  = count;
    m_sampleBuffer = buffer;

    m_wait = false;
}

void Mixer::updateParams()
{
    switch (m_chips.size())
    {
    case 1:
        m_mix[0] = m_stereo ? &Mixer::stereo_OneChip : &Mixer::template mono<1>;
        if (m_stereo) m_mix[1] = &Mixer::stereo_OneChip;
        break;
    case 2:
        m_mix[0] = m_stereo ? &Mixer::stereo_ch1_TwoChips : &Mixer::template mono<2>;
        if (m_stereo) m_mix[1] = &Mixer::stereo_ch2_TwoChips;
        break;
    case 3:
        m_mix[0] = m_stereo ? &Mixer::stereo_ch1_ThreeChips : &Mixer::template mono<3>;
        if (m_stereo) m_mix[1] = &Mixer::stereo_ch2_ThreeChips;
        break;
     }
}

void Mixer::addSid(sidemu *chip)
{
    if (chip != nullptr)
    {
        m_chips.push_back(chip);

        m_iSamples.resize(m_chips.size());

        if (m_mix.size() > 0)
            updateParams();
    }
}

void Mixer::setStereo(bool stereo)
{
    if (m_stereo != stereo)
    {
        m_stereo = stereo;

        m_mix.resize(m_stereo ? 2 : 1);

        updateParams();
    }
}

bool Mixer::setFastForward(int ff)
{
    if (ff < 1 || ff > 32)
        return false;

    m_fastForwardFactor = ff;
    return true;
}

void Mixer::setVolume(int_least32_t left, int_least32_t right)
{
    m_volume.clear();
    m_volume.push_back(left);
    m_volume.push_back(right);

    m_scale.clear();
    m_scale.push_back(left  == VOLUME_MAX ? &Mixer::noScale : &Mixer::scale);
    m_scale.push_back(right == VOLUME_MAX ? &Mixer::noScale : &Mixer::scale);
}

}
