/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - si_controller.c                                         *
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

#include "si_controller.h"

#include "api/m64p_types.h"
#include "api/callbacks.h"
#include "main/main.h"
#include "memory/memory_tools.h"
#include "r4300/cp0.h"
#include "r4300/interupt.h"
#include "r4300/r4300.h"
#include "r4300/r4300_core.h"
#include "ri/rdram.h"

#include <string.h>


static void dma_si_write(struct si_controller* si)
{
    int i;

    if (si->regs[SI_PIF_ADDR_WR64B_REG] != 0x1FC007C0)
    {
        DebugMessage(si->r4300->state, M64MSG_ERROR, "dma_si_write(): unknown SI use");
        si->r4300->state->stop=1;
    }

    for (i = 0; i < PIF_RAM_SIZE; i += 4)
    {
        *((uint32_t*)(&si->pif.ram[i])) = sl(si->rdram->dram[(si->regs[SI_DRAM_ADDR_REG]+i)/4]);
    }
    
    update_pif_write(si);
    update_count(si->r4300->state);

    if (si->r4300->state->g_delay_si) {
        add_interupt_event(si->r4300->state, SI_INT, /*0x100*/0x900);
    } else {
        si->regs[SI_STATUS_REG] |= 0x1000; // INTERRUPT
        signal_rcp_interrupt(si->r4300, MI_INTR_SI);
    }
}

static void dma_si_read(struct si_controller* si)
{
    int i;

    if (si->regs[SI_PIF_ADDR_RD64B_REG] != 0x1FC007C0)
    {
        DebugMessage(si->r4300->state, M64MSG_ERROR, "dma_si_read(): unknown SI use");
        si->r4300->state->stop=1;
    }

    update_pif_read(si);

    for (i = 0; i < PIF_RAM_SIZE; i += 4)
    {
        si->rdram->dram[(si->regs[SI_DRAM_ADDR_REG]+i)/4] = sl(*(uint32_t*)(&si->pif.ram[i]));
    }

    update_count(si->r4300->state);

    if (si->r4300->state->g_delay_si) {
        add_interupt_event(si->r4300->state, SI_INT, /*0x100*/0x900);
    } else {
        si->regs[SI_STATUS_REG] |= 0x1000; // INTERRUPT
        signal_rcp_interrupt(si->r4300, MI_INTR_SI);
    }
}


void connect_si(struct si_controller* si,
                struct r4300_core* r4300,
                struct rdram* rdram)
{
    si->r4300 = r4300;
    si->rdram = rdram;
}

void init_si(struct si_controller* si)
{
    memset(si->regs, 0, SI_REGS_COUNT*sizeof(uint32_t));

    init_pif(&si->pif);
}

static osal_inline uint32_t si_reg(uint32_t address)
{
    return (address & 0xffff) >> 2;
}

uint32_t read_si_regs(struct si_controller* si, uint32_t address)
{
    const uint32_t reg = si_reg(address);

    return si->regs[reg];
}

void write_si_regs(struct si_controller* si, uint32_t address, uint32_t value, uint32_t mask)
{
    const uint32_t reg = si_reg(address);

    switch (reg)
    {
    case SI_DRAM_ADDR_REG:
        masked_write(&si->regs[SI_DRAM_ADDR_REG], value, mask);
        break;

    case SI_PIF_ADDR_RD64B_REG:
        masked_write(&si->regs[SI_PIF_ADDR_RD64B_REG], value, mask);
        dma_si_read(si);
        break;

    case SI_PIF_ADDR_WR64B_REG:
        masked_write(&si->regs[SI_PIF_ADDR_WR64B_REG], value, mask);
        dma_si_write(si);
        break;

    case SI_STATUS_REG:
        si->regs[SI_STATUS_REG] &= ~0x1000;
        clear_rcp_interrupt(si->r4300, MI_INTR_SI);
        break;
    }
}

void si_end_of_dma_event(struct si_controller* si)
{
    si->pif.ram[0x3f] = 0x0;

    /* trigger SI interrupt */
    si->regs[SI_STATUS_REG] |= 0x1000;
    raise_rcp_interrupt(si->r4300, MI_INTR_SI);
}

