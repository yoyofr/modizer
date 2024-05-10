/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - cart_rom.h                                              *
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

#ifndef M64P_PI_CART_ROM_H
#define M64P_PI_CART_ROM_H

#include <stddef.h>
#include <stdint.h>

#include "osal/preproc.h"

struct cart_rom
{
    uint8_t* rom;
    size_t rom_size;

    uint32_t last_write;
};

void connect_cart_rom(struct cart_rom* cart_rom,
                      uint8_t* rom, size_t rom_size);

void init_cart_rom(struct cart_rom* cart_rom);

uint32_t read_cart_rom(struct pi_controller* pi, uint32_t address);
void write_cart_rom(struct pi_controller* pi, uint32_t address, uint32_t value, uint32_t mask);

#endif
