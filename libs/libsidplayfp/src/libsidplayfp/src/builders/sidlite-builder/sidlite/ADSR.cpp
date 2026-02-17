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

#include "ADSR.h"

#include "sl_defs.h"
#include "sl_constants.h"

namespace SIDLite
{

enum ADSRstateBits
{
    GATE_BITVAL=0x01,
    ATTACK_BITVAL=0x80,
    DECAYSUSTAIN_BITVAL=0x40,
    HOLDZEROn_BITVAL=0x10
};

static const short ADSRprescalePeriods[16] =
{
    9, 32, 63, 95, 149, 220, 267, 313, 392, 977, 1954, 3126, 3907, 11720, 19532, 31251
};

static const unsigned char ADSRexponentPeriods[256] =
{
    1, 30, 30, 30, 30, 30, 30, 16, 16, 16, 16, 16, 16, 16, 16,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 4, 4, 4, 4, 4, //pos0:1  pos6:30  pos14:16  pos26:8
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, //pos54:4 //pos93:2
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

void ADSR::clock(char cycles)
{
    for (int Channel=0; Channel<SID_CHANNEL_COUNT; Channel++)
    {
        unsigned char *ChannelPtr = &regs[Channel*7];
        unsigned char AD = ChannelPtr[5];
        unsigned char SR = ChannelPtr[6];
        unsigned char *ADSRstatePtr = &(ADSRstate[Channel]);
        unsigned short *RateCounterPtr = &(RateCounter[Channel]);
        unsigned char *EnvelopeCounterPtr = &(EnvelopeCounter[Channel]);
        unsigned char *ExponentCounterPtr = &(ExponentCounter[Channel]);

        unsigned char PrevGate = (*ADSRstatePtr & GATE_BITVAL);
        if (UNLIKELY(PrevGate != (ChannelPtr[4] & GATE_BITVAL)))
        { //gatebit-change?
            if (PrevGate)
                *ADSRstatePtr &= ~(GATE_BITVAL | ATTACK_BITVAL | DECAYSUSTAIN_BITVAL); //falling edge
            else
                *ADSRstatePtr = (GATE_BITVAL | ATTACK_BITVAL | DECAYSUSTAIN_BITVAL | HOLDZEROn_BITVAL); //rising edge
        }

        unsigned short PrescalePeriod;
        if (*ADSRstatePtr & ATTACK_BITVAL)
            PrescalePeriod = ADSRprescalePeriods[AD >> 4];
        else if (*ADSRstatePtr & DECAYSUSTAIN_BITVAL)
            PrescalePeriod = ADSRprescalePeriods[AD & 0x0F];
        else
            PrescalePeriod = ADSRprescalePeriods[SR & 0x0F];

        *RateCounterPtr += cycles;
        if (UNLIKELY(*RateCounterPtr >= 0x8000))
            *RateCounterPtr -= 0x8000; //*RateCounterPtr &= 0x7FFF; // can wrap around (ADSR delay-bug: short 1st frame)

        if (UNLIKELY(PrescalePeriod <= *RateCounterPtr && *RateCounterPtr < PrescalePeriod+cycles))
        {
            // ratecounter shot (matches rateperiod) (in genuine SID ratecounter is LFSR)
            *RateCounterPtr -= PrescalePeriod; // reset rate-counter on period-match
            if ((*ADSRstatePtr & ATTACK_BITVAL) || ++(*ExponentCounterPtr) == ADSRexponentPeriods[*EnvelopeCounterPtr])
            {
                *ExponentCounterPtr = 0;
                if (*ADSRstatePtr & HOLDZEROn_BITVAL)
                {
                    if (*ADSRstatePtr & ATTACK_BITVAL)
                    {
                        ++(*EnvelopeCounterPtr);
                        if (*EnvelopeCounterPtr == 0xFF)
                            *ADSRstatePtr &= ~ATTACK_BITVAL;
                    }
                    else if (!(*ADSRstatePtr & DECAYSUSTAIN_BITVAL) || *EnvelopeCounterPtr != (SR&0xF0)+(SR>>4))
                    {
                        --(*EnvelopeCounterPtr); // resid adds 1 cycle delay, we omit that mechanism here
                        if (*EnvelopeCounterPtr == 0)
                            *ADSRstatePtr &= ~HOLDZEROn_BITVAL;
                    }
                }
            }
        }
    }
}

ADSR::ADSR(unsigned char *regs) :
    regs(regs)
{
    reset();
}

void ADSR::reset()
{
    for (int Channel=0; Channel<SID_CHANNEL_COUNT; Channel++)
    {
        ADSRstate[Channel] = 0;
        RateCounter[Channel] = 0;
        EnvelopeCounter[Channel] = 0;
        ExponentCounter[Channel] = 0;
    }
}

}
