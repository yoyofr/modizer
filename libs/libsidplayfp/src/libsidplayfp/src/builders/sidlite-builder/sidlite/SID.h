/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright (C) 2025-2026 Leandro Nini
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

// Based on cRSID lightweight RealSID by Hermit (Mihaly Horvath)

#ifndef SIDLITE_SID_H
#define SIDLITE_SID_H

#include "ADSR.h"
#include "Filter.h"
#include "WavGen.h"
#include "sl_settings.h"

#include <array>

namespace SIDLite
{

class SID
{
public:
    SID();
    void reset();
    void write(int addr, int value);
    int read(int addr);
    int clock(unsigned int cycles, short* buf);

    void setChipModel(int model);
    void setRealSIDmode(bool mode);
    void setSamplingParameters(unsigned int clockFrequency, unsigned short samplingFrequency);

    int getLevel() const { return filter.getLevel(); }

private:
    unsigned char regs[0x20] = {0};

    ADSR              adsr;
    Filter            filter;
    WavGen            wavgen;
    settings          s;

    short             SampleCycleCnt;

private:
    inline signed short generateSample(unsigned int &cycles);
    inline int emulateC64(unsigned int &cycles);
};

}

#endif // SIDLITE_SID_H
