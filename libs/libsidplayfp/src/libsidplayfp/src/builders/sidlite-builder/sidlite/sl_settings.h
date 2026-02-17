/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright (C) 2026 Leandro Nini
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

#ifndef SIDLITE_SETTINGS_H
#define SIDLITE_SETTINGS_H

namespace SIDLite
{

class SID;

class settings
{
    friend class SID;

public:
    inline bool is8580() const { return ChipModel == 8580; }
    inline unsigned short getSampleClockRatio() const { return SampleClockRatio; }
    inline bool getRealSIDmode() const { return RealSIDmode; }

private:
    unsigned short SampleClockRatio;    // ratio of CPU-clock and samplerate
    unsigned short ChipModel   = 8580;  // values: 8580 / 6581
    bool           RealSIDmode = true;
};

}

#endif
