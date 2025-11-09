/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2021 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include "mmu.h"

#include "Banks/Bank.h"
#include "Banks/IOBank.h"

namespace libsidplayfp
{
// Use static type to allow compiler to optimize virtual function call
template<class BankType, BankType MMU::* Bank>
uint8_t readBank(MMU &self, uint_least16_t addr)
{
    return (self.*Bank).peek(addr);
}

uint8_t readIO(MMU &self, uint_least16_t addr)
{
    return self.ioBank->peek(addr);
}

class Bank;

MMU::MMU(EventScheduler &scheduler, IOBank* ioBank) :
    eventScheduler(scheduler),
    ioBank(ioBank),
    zeroRAMBank(*this, ramBank)
{
    cpuReadMap[0] = &readBank<ZeroRAMBank, &MMU::zeroRAMBank>;
    cpuWriteMap[0] = &zeroRAMBank;

    for (int i = 1; i < 16; i++)
    {
        cpuReadMap[i] = &readBank<SystemRAMBank, &MMU::ramBank>;
        cpuWriteMap[i] = &ramBank;
    }
}

void MMU::setCpuPort(uint8_t state)
{
    loram = (state & 1) != 0;
    hiram = (state & 2) != 0;
    charen = (state & 4) != 0;

    updateMappingPHI2();
}

void MMU::updateMappingPHI2()
{
    ReadFunc readRAM = &readBank<SystemRAMBank, &MMU::ramBank>;
    cpuReadMap[0xe] = cpuReadMap[0xf] = hiram ? &readBank<KernalRomBank, &MMU::kernalRomBank> : readRAM;
    cpuReadMap[0xa] = cpuReadMap[0xb] = (loram && hiram) ? &readBank<BasicRomBank, &MMU::basicRomBank> : readRAM;

    if (charen && (loram || hiram))
    {
        cpuReadMap[0xd] = &readIO;
        cpuWriteMap[0xd] = ioBank;
    }
    else
    {
        cpuReadMap[0xd] = (!charen && (loram || hiram)) ? &readBank<CharacterRomBank, &MMU::characterRomBank> : readRAM;
        cpuWriteMap[0xd] = &ramBank;
    }
}

void MMU::reset()
{
    ramBank.reset();
    zeroRAMBank.reset();

    // Reset the ROMs to undo the hacks applied
    kernalRomBank.reset();
    basicRomBank.reset();

    loram = false;
    hiram = false;
    charen = false;

    updateMappingPHI2();
}

// LCG
unsigned int random(unsigned int val)
{
    return val * 1664525 + 1013904223;
}

/*
 * This should actually return last byte read from VIC
 * but since the VIC emulation currently does not fetch
 * any value from memory we return a pseudo random value.
 */
uint8_t MMU::getLastReadByte() const
{
    seed = random(seed);
    return seed;
}

}
