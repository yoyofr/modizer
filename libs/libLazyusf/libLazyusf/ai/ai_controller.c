/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - ai_controller.c                                         *
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

#include "ai_controller.h"

#include "main/rom.h"
#include "memory/memory_tools.h"
#include "r4300/cp0.h"
#include "r4300/r4300_core.h"
#include "r4300/interupt.h"
#include "ri/rdram.h"
#include "vi/vi_controller.h"

#include <string.h>

/* Clang complains that 0x80000000 is too large for enumerators, which it says must be 'int' */
static const uint32_t AI_STATUS_BUSY = 0x40000000;
static const uint32_t AI_STATUS_FULL = 0x80000000;


static uint32_t get_remaining_dma_length(struct ai_controller* ai)
{
    unsigned int next_ai_event;
    unsigned int remaining_dma_duration;

    if (ai->fifo[0].duration == 0)
        return 0;

    update_count(ai->r4300->state);
    next_ai_event = get_event(ai->r4300->state, AI_INT);
    if (next_ai_event == 0)
        return 0;

    remaining_dma_duration = next_ai_event - ai->r4300->state->g_cp0_regs[CP0_COUNT_REG];

    if (remaining_dma_duration >= 0x80000000)
        return 0;

    return (uint32_t)((uint64_t)remaining_dma_duration * ai->fifo[0].length / ai->fifo[0].duration);
}

static unsigned int get_dma_duration(struct ai_controller* ai)
{
    unsigned int samples_per_sec = ai->r4300->state->ROM_PARAMS.aidacrate / (1 + ai->regs[AI_DACRATE_REG]);

    return (uint32_t)(((uint64_t)ai->regs[AI_LEN_REG]*ai->vi->delay*ai->r4300->state->ROM_PARAMS.vilimit)
        / (4 * samples_per_sec));
}


static void do_dma(struct ai_controller* ai, const struct ai_dma* dma)
{
#ifdef DEBUG_INFO
    if (ai->r4300->state->debug_log)
      fprintf(ai->r4300->state->debug_log, "Audio DMA push: %d %d\n", dma->address, dma->length);
#endif

    /* lazy initialization of sample format */
    if (ai->samples_format_changed)
    {
        unsigned int frequency = (ai->regs[AI_DACRATE_REG] == 0)
            ? 44100
            : ai->r4300->state->ROM_PARAMS.aidacrate / (1 + ai->regs[AI_DACRATE_REG]);

        unsigned int bits = (ai->regs[AI_BITRATE_REG] == 0)
            ? 16
            : 1 + ai->regs[AI_BITRATE_REG];

        set_audio_format(ai, frequency, bits);

        ai->samples_format_changed = 0;
    }

    /* push audio samples to external sink */
    push_audio_samples(ai, &ai->rdram->dram[dma->address/4], dma->length);

    /* schedule end of dma event */
    update_count(ai->r4300->state);
    if (!(ai->regs[AI_STATUS_REG] & AI_STATUS_FULL))
    {
        remove_event(ai->r4300->state, AI_INT);
        add_interupt_event(ai->r4300->state, AI_INT, ai->r4300->state->g_delay_ai ? dma->duration : 0);
    }
}

void ai_fifo_queue_int(struct ai_controller* ai)
{
    add_interupt_event(ai->r4300->state, AI_INT, ai->r4300->state->g_delay_ai ? get_dma_duration(ai) : 0);
}

static void fifo_push(struct ai_controller* ai)
{
    unsigned int duration = get_dma_duration(ai);

    if (ai->regs[AI_STATUS_REG] & AI_STATUS_BUSY)
    {
        ai->fifo[1].address = ai->regs[AI_DRAM_ADDR_REG];
        ai->fifo[1].length = ai->regs[AI_LEN_REG];
        ai->fifo[1].duration = duration;

        if (ai->r4300->state->enableFIFOfull)
            ai->regs[AI_STATUS_REG] |= AI_STATUS_FULL;
        else
            do_dma(ai, &ai->fifo[1]);
    }
    else
    {
        ai->fifo[0].address = ai->regs[AI_DRAM_ADDR_REG];
        ai->fifo[0].length = ai->regs[AI_LEN_REG];
        ai->fifo[0].duration = duration;
        ai->regs[AI_STATUS_REG] |= AI_STATUS_BUSY;

        do_dma(ai, &ai->fifo[0]);
    }
}

static void fifo_pop(struct ai_controller* ai)
{
    if (ai->regs[AI_STATUS_REG] & AI_STATUS_FULL)
    {
        ai->fifo[0].address = ai->fifo[1].address;
        ai->fifo[0].length = ai->fifo[1].length;
        ai->fifo[0].duration = ai->fifo[1].duration;
        ai->regs[AI_STATUS_REG] &= ~AI_STATUS_FULL;

        do_dma(ai, &ai->fifo[0]);
    }
    else
    {
        ai->regs[AI_STATUS_REG] &= ~AI_STATUS_BUSY;
    }
}


void set_audio_format(struct ai_controller* ai, unsigned int frequency, unsigned int bits)
{
    ai->set_audio_format(ai->user_data, frequency, bits);
}

void push_audio_samples(struct ai_controller* ai, const void* buffer, size_t size)
{
    ai->push_audio_samples(ai->user_data, buffer, size);
}


void connect_ai(struct ai_controller* ai,
                struct r4300_core* r4300,
                struct rdram* rdram,
                struct vi_controller* vi)
{
    ai->r4300 = r4300;
    ai->rdram = rdram;
    ai->vi = vi;
}

void init_ai(struct ai_controller* ai)
{
    memset(ai->regs, 0, AI_REGS_COUNT*sizeof(uint32_t));
    memset(ai->fifo, 0, 2*sizeof(struct ai_dma));
    ai->samples_format_changed = 0;
}

static osal_inline uint32_t ai_reg(uint32_t address)
{
    return (address & 0xffff) >> 2;
}

uint32_t read_ai_regs(struct ai_controller* ai, uint32_t address)
{
    const uint32_t reg = ai_reg(address);

    if (reg == AI_LEN_REG)
    {
        return get_remaining_dma_length(ai);
    }
    else
    {
        return ai->regs[reg];
    }
}

void write_ai_regs(struct ai_controller* ai, uint32_t address, uint32_t value, uint32_t mask)
{
    const uint32_t reg = ai_reg(address);

    switch (reg)
    {
    case AI_LEN_REG:
        masked_write(&ai->regs[AI_LEN_REG], value, mask);
        fifo_push(ai);
        break;

    case AI_STATUS_REG:
        clear_rcp_interrupt(ai->r4300, MI_INTR_AI);
        ai->r4300->mi.AudioIntrReg &= ~MI_INTR_AI;
        break;

    case AI_BITRATE_REG:
    case AI_DACRATE_REG:
        /* lazy audio format setting */
        if ((ai->regs[reg]) != (value & mask))
            ai->samples_format_changed = 1;

        masked_write(&ai->regs[reg], value, mask);
        break;
        
    default:
        masked_write(&ai->regs[reg], value, mask);
        break;
    }
}

void ai_end_of_dma_event(struct ai_controller* ai)
{
    fifo_pop(ai);
    ai->r4300->mi.AudioIntrReg |= MI_INTR_AI;
    raise_rcp_interrupt(ai->r4300, MI_INTR_AI);
}
