	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - main.c                                                  *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2012 CasualJames                                        *
 *   Copyright (C) 2008-2009 Richard Goedeken                              *
 *   Copyright (C) 2008 Ebenblues Nmn Okaygo Tillin9                       *
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

/* This is MUPEN64's main entry point. It contains code that is common
 * to both the gui and non-gui versions of mupen64. See
 * gui subdirectories for the gui-specific code.
 * if you want to implement an interface, you should look here
 */

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "usf/usf.h"

#include "usf/usf_internal.h"

#define M64P_CORE_PROTOTYPES 1
#include "api/m64p_types.h"
#include "api/callbacks.h"

#include "main.h"
#include "rom.h"
#include "savestates.h"
#include "util.h"

#include "ai/ai_controller.h"
#include "memory/memory.h"
#include "pi/pi_controller.h"
#include "r4300/r4300.h"
#include "r4300/r4300_core.h"
#include "r4300/interupt.h"
#include "r4300/reset.h"
#include "rdp/rdp_core.h"
#include "ri/ri_controller.h"
#include "ri/rdram.h"
#include "rsp/rsp_core.h"
#include "si/si_controller.h"
#include "vi/vi_controller.h"

/* version number for Core config section */
#define CONFIG_PARAM_VERSION 1.01

/*********************************************************************************************************
* helper functions
*/

void main_message(usf_state_t * state, m64p_msg_level level, unsigned int corner, const char *format, ...)
{
    va_list ap;
    char buffer[2049];
    va_start(ap, format);
    vsnprintf(buffer, 2047, format, ap);
    buffer[2048]='\0';
    va_end(ap);

    /* send message to front-end */
    DebugMessage(state, level, "%s", buffer);
}

/*********************************************************************************************************
* global functions, for adjusting the core emulator behavior
*/

m64p_error main_reset(usf_state_t * state, int do_hard_reset)
{
    if (do_hard_reset)
        state->reset_hard_job |= 1;
    else
        reset_soft(state);
    return M64ERR_SUCCESS;
}

/*********************************************************************************************************
* global functions, callbacks from the r4300 core or from other plugins
*/

/* called on vertical interrupt.
 * Allow the core to perform various things */
static void connect_all(
        usf_state_t* state,
        struct r4300_core* r4300,
        struct rdp_core* dp,
        struct rsp_core* sp,
        struct ai_controller* ai,
        struct pi_controller* pi,
        struct rdram* rdram,
        struct si_controller* si,
        struct vi_controller* vi,
        uint8_t* rom,
        size_t rom_size)
{
    connect_r4300(r4300, state);
    connect_rdp(dp, r4300, sp);
    connect_rsp(sp, r4300, dp, rdram);
    connect_ai(ai, r4300, rdram, vi);
    connect_pi(pi, r4300, rdram, rom, rom_size);
    connect_si(si, r4300, rdram);
    connect_vi(vi, r4300);
}

/*********************************************************************************************************
* emulation thread - runs the core
*/
m64p_error main_start(usf_state_t * state)
{
    unsigned int RDRAMSize;

    memcpy(&RDRAMSize, state->save_state + 4, 4);
    to_little_endian_buffer(&RDRAMSize, 4, 1);

    /* take the r4300 emulator mode from the config file at this point and cache it in a global variable */
#ifdef DEBUG_INFO
    state->r4300emu = CORE_PURE_INTERPRETER;
#else
    state->r4300emu = CORE_INTERPRETER;
#endif

    /* set some other core parameters based on the config file values */
    state->no_compiled_jump = 0;
    //state->g_delay_si = 1;
    state->g_delay_sp = 1;
    state->g_disable_tlb_write_exception = 1;

    connect_all(state, &state->g_r4300, &state->g_dp, &state->g_sp,
                &state->g_ai, &state->g_pi, &state->g_rdram, &state->g_si, &state->g_vi,
                state->g_rom, state->g_rom_size);

    init_memory(state, RDRAMSize);

    /* connect external audio sink to AI component */
    state->g_ai.user_data = state;
    state->g_ai.set_audio_format = usf_set_audio_format;
    state->g_ai.push_audio_samples = usf_push_audio_samples;

    /* call r4300 CPU core and run the game */
    r4300_reset_hard(state);
    r4300_reset_soft(state);
    r4300_begin(state);

    if (!savestates_load(state, state->save_state, state->save_state_size, 0))
        return M64ERR_INVALID_STATE;

    if (state->enableFIFOfull)
    {
        state->g_delay_ai = 1;
        ai_fifo_queue_int(&state->g_ai);
        state->g_ai.regs[AI_STATUS_REG] |= 0x40000000;
    }

    return M64ERR_SUCCESS;
}

void main_run(usf_state_t * state)
{
    r4300_execute(state);
}
