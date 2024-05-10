/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - cart_rom.c                                              *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2014 Bobby Smiles                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "usf/usf.h"

#include "usf/usf_internal.h"

#include "cart_rom.h"
#include "pi_controller.h"

void connect_cart_rom(struct cart_rom* cart_rom,
                      uint8_t* rom, size_t rom_size)
{
    cart_rom->rom = rom;
    cart_rom->rom_size = rom_size;
}

void init_cart_rom(struct cart_rom* cart_rom)
{
    cart_rom->last_write = 0;
}

static osal_inline uint32_t rom_address(uint32_t address)
{
    return (address & 0x03fffffc);
}

uint32_t read_cart_rom(struct pi_controller* pi, uint32_t address)
{
    const uint32_t addr = rom_address(address);

    if (pi->cart_rom.last_write != 0)
    {
        const uint32_t res = pi->cart_rom.last_write;
        pi->cart_rom.last_write = 0;
        return res;
    }
    else
    {
        return *(uint32_t*)(pi->cart_rom.rom + addr);
    }
}

void write_cart_rom(struct pi_controller* pi, uint32_t address, uint32_t value, uint32_t mask)
{
    pi->cart_rom.last_write = value & mask;
}
