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

#ifndef SIDLITE_FILTER_H
#define SIDLITE_FILTER_H

namespace SIDLite
{

class settings;

class Filter
{
public:
    Filter(settings *s, unsigned char *regs);
    void reset();
    int clock(int FilterInput, int NonFiltered);

    inline int getLevel() const { return Level; }

    void rebuildCutoffTables(unsigned short samplerate);

private:
    unsigned char *regs;
    settings      *s;

    unsigned short *CutoffMul8580;
    unsigned short *CutoffMul6581;

    signed int     PrevVolume; // lowpass-filtered version of Volume-band register
    int            PrevLowPass;
    int            PrevBandPass;
    int            Level;      // filtered version, good for VU-meter display
    unsigned char  VUmeterUpdateCounter;
};

}

#endif // SIDLITE_FILTER_H

