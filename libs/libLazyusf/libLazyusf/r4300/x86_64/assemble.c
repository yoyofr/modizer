/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - assemble.c                                              *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2007 Richard Goedeken (Richard42)                       *
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
#include <stdio.h>

#include "usf/usf.h"

#include "usf/usf_internal.h"

#include "assemble.h"

#include "api/m64p_types.h"
#include "api/callbacks.h"
#include "osal/preproc.h"
#include "r4300/recomph.h"
#include "r4300/recomp.h"
#include "r4300/r4300.h"

/* Placeholder for RIP-relative offsets is maxmimum 32-bit signed value.
 * So, if recompiled code is run without running passe2() first, it will
 * cause an exception.
*/
#define REL_PLACEHOLDER 0x7fffffff

/* Static Functions */

void add_jump(usf_state_t * state, unsigned int pc_addr, unsigned int mi_addr, unsigned int absolute64)
{
  if (state->jumps_number == state->max_jumps_number)
  {
    state->max_jumps_number += 512;
    state->jumps_table = realloc(state->jumps_table, state->max_jumps_number*sizeof(jump_table));
  }
  state->jumps_table[state->jumps_number].pc_addr = pc_addr;
  state->jumps_table[state->jumps_number].mi_addr = mi_addr;
  state->jumps_table[state->jumps_number].absolute64 = absolute64;
  state->jumps_number++;
}

/* Global Functions */

void init_assembler(usf_state_t * state, void *block_jumps_table, int block_jumps_number, void *block_riprel_table, int block_riprel_number)
{
  if (block_jumps_table)
  {
    state->jumps_table = block_jumps_table;
    state->jumps_number = block_jumps_number;
    if (state->jumps_number <= 512)
      state->max_jumps_number = 512;
    else
      state->max_jumps_number = (state->jumps_number + 511) & 0xfffffe00;
  }
  else
  {
    state->jumps_table = malloc(512*sizeof(jump_table));
    state->jumps_number = 0;
    state->max_jumps_number = 512;
  }

  if (block_riprel_table)
  {
    state->riprel_table = block_riprel_table;
    state->riprel_number = block_riprel_number;
    if (state->riprel_number <= 512)
      state->max_riprel_number = 512;
    else
      state->max_riprel_number = (state->riprel_number + 511) & 0xfffffe00;
  }
  else
  {
    state->riprel_table = malloc(512 * sizeof(riprelative_table));
    state->riprel_number = 0;
    state->max_riprel_number = 512;
  }
}

void free_assembler(usf_state_t * state, void **block_jumps_table, int *block_jumps_number, void **block_riprel_table, int *block_riprel_number)
{
  *block_jumps_table = state->jumps_table;
  *block_jumps_number = state->jumps_number;
  *block_riprel_table = state->riprel_table;
  *block_riprel_number = state->riprel_number;
}

void passe2(usf_state_t * state, precomp_instr *dest, int start, int end, precomp_block *block)
{
  unsigned int i;

  build_wrappers(state, dest, start, end, block);

  /* First, fix up all the jumps.  This involves a table lookup to find the offset into the block of x86_64 code for
   * for start of a recompiled r4300i instruction corresponding to the given jump destination address in the N64
   * address space.  Next, the relative offset between this destination and the location of the jump instruction is
   * computed and stored in memory, so that the jump will branch to the right place in the recompiled code.
   */
  for (i = 0; i < state->jumps_number; i++)
  {
    precomp_instr *jump_instr = dest + ((state->jumps_table[i].mi_addr - dest[0].addr) / 4);
    unsigned int   jmp_offset_loc = state->jumps_table[i].pc_addr;
    unsigned char *addr_dest = NULL;
    /* calculate the destination address to jump to */
    if (jump_instr->reg_cache_infos.need_map)
    {
      addr_dest = jump_instr->reg_cache_infos.jump_wrapper;
    }
    else
    {
      addr_dest = block->code + jump_instr->local_addr;
    }
    /* write either a 32-bit IP-relative offset or a 64-bit absolute address */
    if (state->jumps_table[i].absolute64)
    {
      *((unsigned long long *) (block->code + jmp_offset_loc)) = (unsigned long long) addr_dest;
    }
    else
    {
      long jump_rel_offset = (long) (addr_dest - (block->code + jmp_offset_loc + 4));
      *((int *) (block->code + jmp_offset_loc)) = (int) jump_rel_offset;
      if (jump_rel_offset >= 0x7fffffffLL || jump_rel_offset < -0x80000000LL)
      {
        DebugMessage(state, M64MSG_ERROR, "assembler pass2 error: offset too big for relative jump from %p to %p",
                     (block->code + jmp_offset_loc + 4), addr_dest);
        OSAL_BREAKPOINT_INTERRUPT
      }
    }
  }

  /* Next, fix up all of the RIP-relative memory accesses.  This is unique to the x86_64 architecture, because
   * the 32-bit absolute displacement addressing mode is not available (and there's no 64-bit absolute displacement
   * mode either).
   */
  for (i = 0; i < state->riprel_number; i++)
  {
    unsigned char *rel_offset_ptr = block->code + state->riprel_table[i].pc_addr;
    long rip_rel_offset = (long) (state->riprel_table[i].global_dst - (rel_offset_ptr + 4 + state->riprel_table[i].extra_bytes));
    if (rip_rel_offset >= 0x7fffffffLL || rip_rel_offset < -0x80000000LL)
    {
      DebugMessage(state, M64MSG_ERROR, "assembler pass2 error: offset too big between mem target: %p and code position: %p",
                   state->riprel_table[i].global_dst, rel_offset_ptr);
      OSAL_BREAKPOINT_INTERRUPT
    }
    *((int *) rel_offset_ptr) = (int) rip_rel_offset;
  }

}

void jump_start_rel8(usf_state_t * state)
{
  state->g_jump_start8 = state->code_length;
}

void jump_start_rel32(usf_state_t * state)
{
  state->g_jump_start32 = state->code_length;
}

void jump_end_rel8(usf_state_t * state)
{
  unsigned int jump_end = state->code_length;
  int jump_vec = jump_end - state->g_jump_start8;

  if (jump_vec > 127 || jump_vec < -128)
  {
    DebugMessage(state, M64MSG_ERROR, "Error: 8-bit relative jump too long! From %x to %x", state->g_jump_start8, jump_end);
    OSAL_BREAKPOINT_INTERRUPT
  }

  state->code_length = state->g_jump_start8 - 1;
  put8(state, jump_vec);
  state->code_length = jump_end;
}

void jump_end_rel32(usf_state_t * state)
{
  unsigned int jump_end = state->code_length;
  int jump_vec = jump_end - state->g_jump_start32;

  state->code_length = state->g_jump_start32 - 4;
  put32(state, jump_vec);
  state->code_length = jump_end;
}
