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

#ifndef SIDLITE_ADSR_H
#define SIDLITE_ADSR_H

namespace SIDLite
{

class ADSR
{
public:
    ADSR(unsigned char *regs);
    void reset();
    void clock(char cycles);

    inline unsigned char counter(int channel) const { return EnvelopeCounter[channel]; }

private:
    unsigned char *regs;

    unsigned short RateCounter[3];
    unsigned char  ADSRstate[3];
    unsigned char  EnvelopeCounter[3];
    unsigned char  ExponentCounter[3];
};

}

#endif // SIDLITE_ADSR_H
