/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - ri_controller.c                                         *
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

#include "ri_controller.h"

#include "memory/memory_tools.h"

#include <string.h>

void init_ri(struct ri_controller* ri)
{
    memset(ri->regs, 0, RI_REGS_COUNT*sizeof(uint32_t));
}

static osal_inline uint32_t ri_reg(uint32_t address)
{
    return (address & 0xffff) >> 2;
}

uint32_t read_ri_regs(struct ri_controller* ri, uint32_t address)
{
    const uint32_t reg = ri_reg(address);

    return ri->regs[reg];
}

void write_ri_regs(struct ri_controller* ri, uint32_t address, uint32_t value, uint32_t mask)
{
    const uint32_t reg = ri_reg(address);

    masked_write(&ri->regs[reg], value, mask);
}

