/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2025 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include "simpleMixer.h"

#include <cassert>
#include <climits>
#include <cstring>
#include <iostream>
#ifdef __cpp_lib_unreachable
#  include <utility>
#endif

#include "sidemu.h"


namespace libsidplayfp
{

unsigned int SimpleMixer::doMix(short *buffer, unsigned int samples)
{
    unsigned int j = 0;
    for (unsigned int i=0; i<samples; i++)
    {
        for (size_t k=0; k<m_buffers.size(); k++)
        {
            const short *buf = m_buffers[k];
            m_iSamples[k] = buf[i];
        }

        for (auto mix: m_mix)
        {
            const int_least32_t tmp = (this->*(mix))();
            assert((tmp >= SHRT_MIN) && (tmp <= SHRT_MAX));
            buffer[j++] = static_cast<short>(tmp);
        }
    }

    return j;
}

SimpleMixer::SimpleMixer(bool stereo, short** buffers, int chips)
{
    switch (chips)
    {
    case 1:
        m_mix.push_back(stereo ? &SimpleMixer::stereo_OneChip : &SimpleMixer::template mono<1>);
        if (stereo) m_mix.push_back(&SimpleMixer::stereo_OneChip);
        break;
    case 2:
        m_mix.push_back(stereo ? &SimpleMixer::stereo_ch1_TwoChips : &SimpleMixer::template mono<2>);
        if (stereo) m_mix.push_back(&SimpleMixer::stereo_ch2_TwoChips);
        break;
    case 3:
        m_mix.push_back(stereo ? &SimpleMixer::stereo_ch1_ThreeChips : &SimpleMixer::template mono<3>);
        if (stereo) m_mix.push_back(&SimpleMixer::stereo_ch2_ThreeChips);
        break;
#ifdef __cpp_lib_unreachable
    default:
        std::unreachable();
#endif
    }

    m_iSamples.resize(chips);
    for (int i=0; i<chips; i++)
        m_buffers.push_back(buffers[i]);
}

}
