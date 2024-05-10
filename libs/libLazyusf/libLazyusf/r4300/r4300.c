/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - r4300.c                                                 *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2002 Hacktarux                                          *
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

#include <stdlib.h>
#include <string.h>

#include "usf/usf.h"

#include "usf/usf_internal.h"

#include "ai/ai_controller.h"
#include "api/m64p_types.h"
#include "api/callbacks.h"
#include "memory/memory.h"
#include "memory/memory_mmu.h"
#include "main/main.h"
#include "main/rom.h"
#include "pi/pi_controller.h"
#include "rsp/rsp_core.h"
#include "si/si_controller.h"
#include "vi/vi_controller.h"

#include "r4300.h"
#include "r4300_core.h"
#include "cached_interp.h"
#include "cp0.h"
#include "cp1.h"
#include "ops.h"
#include "interupt.h"
#include "pure_interp.h"
#include "recomp.h"
#include "tlb.h"

#ifdef DBG
#include "debugger/dbg_types.h"
#include "debugger/debugger.h"
#endif

#if defined(COUNT_INSTR)
#include "instr_counters.h"
#endif

void generic_jump_to(usf_state_t * state, unsigned int address)
{
#ifdef DEBUG_INFO
   if (state->r4300emu == CORE_PURE_INTERPRETER)
      state->PC->addr = address;
   else
#endif
   {
      jump_to(state, address);
   }
}

/* this hard reset function simulates the boot-up state of the R4300 CPU */
void r4300_reset_hard(usf_state_t * state)
{
    unsigned int i;

    // clear r4300 registers and TLB entries
    for (i = 0; i < 32; i++)
    {
        state->reg[i]=0;
        state->g_cp0_regs[i]=0;
        state->reg_cop1_fgr_64[i]=0;

        // --------------tlb------------------------
        state->tlb_e[i].mask=0;
        state->tlb_e[i].vpn2=0;
        state->tlb_e[i].g=0;
        state->tlb_e[i].asid=0;
        state->tlb_e[i].pfn_even=0;
        state->tlb_e[i].c_even=0;
        state->tlb_e[i].d_even=0;
        state->tlb_e[i].v_even=0;
        state->tlb_e[i].pfn_odd=0;
        state->tlb_e[i].c_odd=0;
        state->tlb_e[i].d_odd=0;
        state->tlb_e[i].v_odd=0;
        state->tlb_e[i].r=0;
        //tlb_e[i].check_parity_mask=0x1000;

        state->tlb_e[i].start_even=0;
        state->tlb_e[i].end_even=0;
        state->tlb_e[i].phys_even=0;
        state->tlb_e[i].start_odd=0;
        state->tlb_e[i].end_odd=0;
        state->tlb_e[i].phys_odd=0;
    }
    memory_reset_tlb(&state->io);
    state->llbit=0;
    state->hi=0;
    state->lo=0;
    state->FCR0=0x511;
    state->FCR31=0;

    // set COP0 registers
    state->g_cp0_regs[CP0_RANDOM_REG] = 31;
    state->g_cp0_regs[CP0_STATUS_REG]= 0x34000000;
    set_fpr_pointers(state, state->g_cp0_regs[CP0_STATUS_REG]);
    state->g_cp0_regs[CP0_CONFIG_REG]= 0x6e463;
    state->g_cp0_regs[CP0_PREVID_REG] = 0xb00;
    state->g_cp0_regs[CP0_COUNT_REG] = 0x5000;
    state->g_cp0_regs[CP0_CAUSE_REG] = 0x5C;
    state->g_cp0_regs[CP0_CONTEXT_REG] = 0x7FFFF0;
    state->g_cp0_regs[CP0_EPC_REG] = 0xFFFFFFFF;
    state->g_cp0_regs[CP0_BADVADDR_REG] = 0xFFFFFFFF;
    state->g_cp0_regs[CP0_ERROREPC_REG] = 0xFFFFFFFF;
   
    state->rounding_mode = 0x33F;
}


static unsigned int get_tv_type(usf_state_t * state)
{
    switch(state->ROM_PARAMS.systemtype)
    {
    default:
    case SYSTEM_NTSC: return 1;
    case SYSTEM_PAL: return 0;
    case SYSTEM_MPAL: return 2;
    }
}

/* Simulates end result of PIFBootROM execution */
void r4300_reset_soft(usf_state_t * state)
{
    unsigned int rom_type = 0;              /* 0:Cart, 1:DD */
    unsigned int reset_type = 0;            /* 0:ColdReset, 1:NMI */
    unsigned int s7 = 0;                    /* ??? */
    unsigned int tv_type = get_tv_type(state);   /* 0:PAL, 1:NTSC, 2:MPAL */
    uint32_t bsd_dom1_config = (state->g_rom && state->g_rom_size >= 4) ? *(uint32_t*)state->g_rom : 0;

    state->g_cp0_regs[CP0_STATUS_REG] = 0x34000000;
    state->g_cp0_regs[CP0_CONFIG_REG] = 0x0006e463;

    state->g_sp.regs[SP_STATUS_REG] = 1;
    state->g_sp.regs2[SP_PC_REG] = 0;

    state->g_pi.regs[PI_BSD_DOM1_LAT_REG] = (bsd_dom1_config      ) & 0xff;
    state->g_pi.regs[PI_BSD_DOM1_PWD_REG] = (bsd_dom1_config >>  8) & 0xff;
    state->g_pi.regs[PI_BSD_DOM1_PGS_REG] = (bsd_dom1_config >> 16) & 0x0f;
    state->g_pi.regs[PI_BSD_DOM1_RLS_REG] = (bsd_dom1_config >> 20) & 0x03;
    state->g_pi.regs[PI_STATUS_REG] = 0;

    state->g_ai.regs[AI_DRAM_ADDR_REG] = 0;
    state->g_ai.regs[AI_LEN_REG] = 0;

    state->g_vi.regs[VI_V_INTR_REG] = 1023;
    state->g_vi.regs[VI_CURRENT_REG] = 0;
    state->g_vi.regs[VI_H_START_REG] = 0;

    state->g_r4300.mi.regs[MI_INTR_REG] &= ~(MI_INTR_PI | MI_INTR_VI | MI_INTR_AI | MI_INTR_SP);

    if (state->g_rom && state->g_rom_size >= 0xfc0)
        memcpy((unsigned char*)state->g_sp.mem+0x40, state->g_rom+0x40, 0xfc0);

    state->reg[19] = rom_type;     /* s3 */
    state->reg[20] = tv_type;      /* s4 */
    state->reg[21] = reset_type;   /* s5 */
    state->reg[22] = state->g_si.pif.cic.seed;/* s6 */
    state->reg[23] = s7;           /* s7 */

    /* required by CIC x105 */
    state->g_sp.mem[0x1000/4] = 0x3c0dbfc0;
    state->g_sp.mem[0x1004/4] = 0x8da807fc;
    state->g_sp.mem[0x1008/4] = 0x25ad07c0;
    state->g_sp.mem[0x100c/4] = 0x31080080;
    state->g_sp.mem[0x1010/4] = 0x5500fffc;
    state->g_sp.mem[0x1014/4] = 0x3c0dbfc0;
    state->g_sp.mem[0x1018/4] = 0x8da80024;
    state->g_sp.mem[0x101c/4] = 0x3c0bb000;

    /* required by CIC x105 */
    state->reg[11] = 0xffffffffa4000040ULL; /* t3 */
    state->reg[29] = 0xffffffffa4001ff0ULL; /* sp */
    state->reg[31] = 0xffffffffa4001550ULL; /* ra */

    /* ready to execute IPL3 */
}

void r4300_begin(usf_state_t * state)
{
    state->current_instruction_table = cached_interpreter_table;
    
    state->delay_slot=0;
    state->stop = 0;
    state->rompause = 0;
    
    state->next_interupt = 624999;
    init_interupt(state);

#ifdef DEBUG_INFO
    if (state->r4300emu == CORE_PURE_INTERPRETER)
    {
        DebugMessage(state, M64MSG_INFO, "Starting R4300 emulator: Pure Interpreter");
        state->r4300emu = CORE_PURE_INTERPRETER;
        state->PC = (precomp_instr*) calloc(sizeof(precomp_instr), 1);
    }
    else /* if (r4300emu == CORE_INTERPRETER) */
#endif
    {
        DebugMessage(state, M64MSG_INFO, "Starting R4300 emulator: Cached Interpreter");
        state->r4300emu = CORE_INTERPRETER;
        init_blocks(state);
    }
}

void r4300_execute(usf_state_t * state)
{
#ifdef DEBUG_INFO
    if (state->r4300emu == CORE_PURE_INTERPRETER)
    {
        pure_interpreter(state);
    }
    else /* if (r4300emu == CORE_INTERPRETER) */
#endif
    {
        /* Prevent segfault on failed jump_to */
        if (!state->actual->block)
            return;

        while (!state->stop)
            state->PC->ops(state);
    }
}

void r4300_end(usf_state_t * state)
{
#ifdef DEBUG_INFO
    if (state->r4300emu == CORE_PURE_INTERPRETER)
    {
        if (state->PC)
        {
          free(state->PC);
          state->PC = 0;
        }
    }
    else /* if (r4300emu == CORE_INTERPRETER) */
#endif
    {
        free_blocks(state);
    }
    
    DebugMessage(state, M64MSG_INFO, "R4300 emulator finished.");
}
