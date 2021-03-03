/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - vi_controller.c                                         *
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

#include "vi_controller.h"

#include "main/main.h"
#include "memory/memory.h"
#include "r4300/r4300_core.h"
#include "r4300/cp0.h"
#include "r4300/interupt.h"

#include <string.h>

void connect_vi(struct vi_controller* vi,
                struct r4300_core* r4300)
{
    vi->r4300 = r4300;
}

void init_vi(struct vi_controller* vi)
{
    memset(vi->regs, 0, VI_REGS_COUNT*sizeof(uint32_t));
    vi->field = 0;

    vi->delay = vi->next_vi = 5000;
}


int read_vi_regs(void* opaque, uint32_t address, uint32_t* value)
{
    struct vi_controller* vi = (struct vi_controller*)opaque;
    uint32_t reg = vi_reg(address);

    if (reg == VI_CURRENT_REG)
    {
        update_count(vi->r4300->state);
        vi->regs[VI_CURRENT_REG] = (vi->delay - (vi->next_vi - vi->r4300->state->g_cp0_regs[CP0_COUNT_REG]))/1500;
        vi->regs[VI_CURRENT_REG] = (vi->regs[VI_CURRENT_REG] & (~1)) | vi->field;
    }

    *value = vi->regs[reg];

    return 0;
}

int write_vi_regs(void* opaque, uint32_t address, uint32_t value, uint32_t mask)
{
    struct vi_controller* vi = (struct vi_controller*)opaque;
    usf_state_t * state = vi->r4300->state;
    uint32_t reg = vi_reg(address);
	int32_t count_per_scanline;

    switch(reg)
    {
    case VI_STATUS_REG:
        if ((vi->regs[VI_STATUS_REG] & mask) != (value & mask))
        {
            masked_write(&vi->regs[VI_STATUS_REG], value, mask);
        }
        return 0;

    case VI_WIDTH_REG:
        if ((vi->regs[VI_WIDTH_REG] & mask) != (value & mask))
        {
            masked_write(&vi->regs[VI_WIDTH_REG], value, mask);
        }
        return 0;

    case VI_CURRENT_REG:
        clear_rcp_interrupt(vi->r4300, MI_INTR_VI);
        return 0;

    case VI_V_SYNC_REG:
        masked_write(&vi->regs[reg], value, mask);
        count_per_scanline = ((float)state->ROM_PARAMS.aidacrate / (float)state->ROM_PARAMS.vilimit) / (vi->regs[VI_V_SYNC_REG] + 1);
        vi->delay = (vi->regs[VI_V_SYNC_REG] + 1) * count_per_scanline;
        if (vi->regs[VI_V_SYNC_REG] != 0 && vi->next_vi == 0)
        {
            update_count(vi->r4300->state);
            vi->next_vi = state->g_cp0_regs[CP0_COUNT_REG] + vi->delay;
            add_interupt_event_count(state, VI_INT, vi->next_vi);
        }
        return 0;
    }

    masked_write(&vi->regs[reg], value, mask);

    return 0;
}

void vi_vertical_interrupt_event(struct vi_controller* vi)
{
    usf_state_t * state = vi->r4300->state;
    
    /* toggle vi field if in interlaced mode */
    update_count(vi->r4300->state);
    vi->field ^= (vi->regs[VI_STATUS_REG] >> 6) & 0x1;

    vi->next_vi = state->g_cp0_regs[CP0_COUNT_REG] + vi->delay;
    add_interupt_event_count(state, VI_INT, vi->next_vi);

    /* trigger interrupt */
    raise_rcp_interrupt(vi->r4300, MI_INTR_VI);
}

