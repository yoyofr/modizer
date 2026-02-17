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

#ifndef SIDLITE_WAVGEN_H
#define SIDLITE_WAVGEN_H

#include <utility>

namespace SIDLite
{

class ADSR;
class settings;

using wg_output_t = std::pair<int, int>;

class WavGen
{
public:
    WavGen(settings *s, unsigned char *regs);
    void reset();
    wg_output_t clock(ADSR *adsr);

    inline unsigned char getOsc3() const { return oscReg; }
    inline unsigned char getEnv3() const { return envReg; }
    int  ChannelOutput[3]; //YOYOFR
    int  ChannelFreq[3]; //YOYOFR
    int  ChannelEnv[3]; //YOYOFR
private:
    unsigned char *regs;
    settings      *s;

    int           PhaseAccu[3];       // 28bit precision instead of 24bit
    int           PrevPhaseAccu[3];   // (integerized ClockRatio fractionals, WebSID has similar solution)
    unsigned int  NoiseLFSR[3];
    unsigned int  PrevWavGenOut[3];
    unsigned char PrevWavData[3];
    signed char   PrevSounDemonDigiWF[3];
    unsigned int  RingSourceMSB;
    unsigned char SyncSourceMSBrise;

    unsigned char oscReg;
    unsigned char envReg;
};

}

#endif // SIDLITE_WAVGEN_H
