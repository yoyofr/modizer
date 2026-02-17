/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2025 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2001 Simon White
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

#include "sidlite-emu.h"

#include <cmath>

//#include "sidlite/siddefs.h"
#include "sidplayfp/siddefs.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

namespace libsidplayfp
{

const char* SIDLiteEmu::getCredits()
{
    return
        "SIDLiteEmu V" VERSION " Engine:\n"
        "\t(C) 2025 Leandro Nini\n"
        "MOS6581/CSG8580 (SID) Emulation:\n"
        "\t(C) 2025 Leandro Nini\n"
        "\tBased on cRSID by Hermit (Mihaly Horvath)\n";
}

SIDLiteEmu::SIDLiteEmu(sidbuilder *builder) :
    sidemu(builder),
    m_sid(*(new SIDLite::SID))
{
    reset(0);
}

SIDLiteEmu::~SIDLiteEmu()
{
    delete &m_sid;
    delete[] m_buffer;
}

// Standard component options
void SIDLiteEmu::reset(uint8_t volume)
{
    m_accessClk = 0;
    m_sid.reset();
    m_sid.write(0x18, volume);
}

uint8_t SIDLiteEmu::read(uint_least8_t addr)
{
    clock();
    return m_sid.read(addr);
}

void SIDLiteEmu::write(uint_least8_t addr, uint8_t data)
{
    clock();
    m_sid.write(addr, data);
}

void SIDLiteEmu::clock()
{
    const event_clock_t cycles = eventScheduler->getTime(EVENT_CLOCK_PHI1) - m_accessClk;
    m_accessClk += cycles;
    m_bufferpos += m_sid.clock(cycles, m_buffer+m_bufferpos);
}
/*
void SIDLiteEmu::filter(bool enable)
{
      m_sid.enableFilter(enable);
}
*/
void SIDLiteEmu::sampling(float systemclock, float freq,
        SidConfig::sampling_method_t /*method*/)
{/*
    SIDLite::SamplingMethod sampleMethod;
    switch (method)
    {
    case SidConfig::INTERPOLATE:
        sampleMethod = SIDLite::DECIMATE;
        break;
    case SidConfig::RESAMPLE_INTERPOLATE:
        sampleMethod = SIDLite::RESAMPLE;
        break;
    default:
        m_status = false;
        m_error = ERR_INVALID_SAMPLING;
        return;
    }
*/
    //try
    {
        m_sid.setSamplingParameters(systemclock, freq);
    }
    //catch (SIDLite::SIDError const &)
    //{
    //    m_status = false;
    //    m_error = ERR_UNSUPPORTED_FREQ;
    //    return;
    //}

    if (m_buffer)
        delete[] m_buffer;

    // 20ms buffer
    const int buffersize = std::ceil((freq / 1000.f) * 20.f);
    m_buffer = new short[buffersize];
    m_status = true;
}

// Set the emulated SID model
void SIDLiteEmu::model(SidConfig::sid_model_t model, bool /*digiboost*/)
{
    unsigned short chipModel;
    switch (model)
    {
        case SidConfig::MOS6581:
            chipModel = 6581;
            //m_sid.input(0);
            break;
        case SidConfig::MOS8580:
            chipModel = 8580;
            //m_sid.input(digiboost ? -32768 : 0);
            break;
        default:
            m_status = false;
            m_error = ERR_INVALID_CHIP;
            return;
    }

    m_sid.setChipModel(chipModel);
    m_status = true;
}

}
