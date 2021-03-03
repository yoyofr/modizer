/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - recomp.h                                                *
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

#ifndef M64P_R4300_RECOMP_H
#define M64P_R4300_RECOMP_H

#include <stddef.h>

#include "osal/preproc.h"

#ifndef PRECOMP_STRUCTS
#define PRECOMP_STRUCTS

#if defined(__x86_64__)
#include "x86_64/assemble_struct.h"
#else
#include "x86/assemble_struct.h"
#endif

typedef struct _precomp_instr
{
   void (osal_fastcall *ops)(usf_state_t * state);
   union
     {
    struct
      {
         long long int *rs;
         long long int *rt;
         short immediate;
      } i;
    struct
      {
         unsigned int inst_index;
      } j;
    struct
      {
         long long int *rs;
         long long int *rt;
         long long int *rd;
         unsigned char sa;
         unsigned char nrd;
      } r;
    struct
      {
         unsigned char base;
         unsigned char ft;
         short offset;
      } lf;
    struct
      {
         unsigned char ft;
         unsigned char fs;
         unsigned char fd;
      } cf;
     } f;
   unsigned int addr; /* word-aligned instruction address in r4300 address space */
   unsigned int local_addr; /* byte offset to start of corresponding x86_64 instructions, from start of code block */
   reg_cache_struct reg_cache_infos;
} precomp_instr;

typedef struct _precomp_block
{
   precomp_instr *block;
   unsigned int start;
   unsigned int end;
   unsigned char *code;
   unsigned int code_length;
   unsigned int max_code_length;
   void *jumps_table;
   int jumps_number;
   void *riprel_table;
   int riprel_number;
   //unsigned char md5[16];
   unsigned int adler32;
} precomp_block;
#endif

void recompile_block(usf_state_t *, int *source, precomp_block *block, unsigned int func);
void init_block(usf_state_t *, precomp_block *block);
void free_block(usf_state_t *, precomp_block *block);
void recompile_opcode(usf_state_t *);
void dyna_jump(usf_state_t *);
void dyna_start(usf_state_t *, void *code);
void dyna_stop(usf_state_t *);
void *realloc_exec(usf_state_t *, void *ptr, size_t oldsize, size_t newsize);

#if defined(__x86_64__)
  #include "x86_64/assemble.h"
  #include "x86_64/regcache.h"
#else
  #include "x86/assemble.h"
  #include "x86/regcache.h"
#endif

#endif /* M64P_R4300_RECOMP_H */

