/*
 * This file is part of sidplayfp, a SID player engine.
 *
 * Copyright 2011-2025 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright (C) 2000 Simon White
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "mixer.h"

#include <cassert>
#include <cstring>

Mixer::Mixer() :
    m_rand(257254)
{
    setVolume(VOLUME_MAX);
}

void Mixer::initialize(unsigned int chips, bool stereo)
{
    assert((chips >= 1) && (chips <= 3));
    m_channels = stereo ? 2 : 1;
    m_mix.resize(m_channels);
    m_chips = chips;
    m_iSamples.resize(chips);
    switch (chips)
    {
    case 1:
        m_mix[0] = stereo ? &Mixer::stereo_OneChip : &Mixer::template mono<1>;
        if (stereo) m_mix[1] = &Mixer::stereo_OneChip;
        break;
    case 2:
        m_mix[0] = stereo ? &Mixer::stereo_ch1_TwoChips : &Mixer::template mono<2>;
        if (stereo) m_mix[1] = &Mixer::stereo_ch2_TwoChips;
        break;
    case 3:
        m_mix[0] = stereo ? &Mixer::stereo_ch1_ThreeChips : &Mixer::template mono<3>;
        if (stereo) m_mix[1] = &Mixer::stereo_ch2_ThreeChips;
        break;
     }
}

void Mixer::begin(short *buffer, uint_least32_t length)
{
    m_dest = buffer;
    m_dest_size = length;

    m_pos = m_buffer.size();
    if (m_pos) LIKELY
        std::memcpy(m_dest, m_buffer.data(), m_pos*sizeof(short));
}

uint_least32_t Mixer::mix(short** buffers, uint_least32_t start, uint_least32_t length, short* dest)
{
    uint_least32_t j = 0;
    for (uint_least32_t i=0; i<length;)
    {
        if (m_fastForwardFactor == 1) LIKELY
        {
            for (unsigned int c=0; c<m_chips; c++)
            {
                m_iSamples[c] = buffers[c][start+i];
            }

            i++;
        }
        else
        {
            for (unsigned int c=0; c<m_chips; c++)
            {
                // Apply boxcar filter
                int_least32_t sample = 0;
                const short *buffer = &buffers[c][start+i];
                for (unsigned int k = 0; k < m_fastForwardFactor; k++)
                {
                    sample += buffer[k];
                }

                m_iSamples[c] = sample / m_fastForwardFactor;
            }

            // increment i to mark we ate some samples.
            i += m_fastForwardFactor;
        }

        for (unsigned int c=0; c<m_channels; c++)
        {
            const int_least32_t tmp = (this->*(m_scale))(c);
            assert(tmp >= -32768 && tmp <= 32767);
            dest[j++] = static_cast<short>(tmp);
        }
    }
    return j;
}

void Mixer::doMix(short** buffers, uint_least32_t samples)
{
    uint_least32_t const cnt = std::min(samples, (m_dest_size-m_pos)/m_channels);
    uint_least32_t const res = mix(buffers, 0, cnt, m_dest+m_pos);
    m_pos += res;

    // save remaining samples, if any
    uint_least32_t const rem = samples - cnt;
    if (rem)
    {
        m_buffer.resize(static_cast<std::size_t>(rem)*m_channels);
        mix(buffers, cnt, rem, m_buffer.data());
    }
}

void Mixer::setVolume(unsigned int vol)
{
    assert(vol <= VOLUME_MAX);
    m_volume = vol;

    m_scale = (vol == VOLUME_MAX) ? &Mixer::noScale : &Mixer::scale;
}

bool Mixer::setFastForward(unsigned int ff)
{
    if (ff < 1 || ff > 32)
        return false;

    m_fastForwardFactor = ff;
    return true;
}
