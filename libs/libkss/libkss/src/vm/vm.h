#ifndef _VM_H_
#define _VM_H_

#include "emu2149/emu2149.h"
#include "emu2212/emu2212.h"
#include "emu2413/emu2413.h"
#include "emu8950/emu8950.h"
#include "emu76489/emu76489.h"
#include "kmz80/kmz80.h"

#include "mmap.h"
#include "detect.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EXTIO (0x40)
#define STOPIO (0x41)
#define LOOPIO (0x42)
#define ADRLIO (0x43)
#define ADRHIO (0x44)

#define VM_INVALID_ADDRESS (0x10000)

typedef struct tagVM VM;
typedef int (*VM_WIOPROC)(VM *, uint32_t, uint32_t);

struct tagVM {
  KMZ80_CONTEXT context;
  KMEVENT kme;
  KMEVENT_ITEM_ID vsync_id;
  uint32_t vsync_adr;
  MMAP *mmap;
  LPDETECT *lpde;

  uint8_t IO[0x100];
  VM_WIOPROC WIOPROC[0x100];

  uint32_t clock; /* CPU clock */
  uint32_t vsync_cycles;

  uint32_t bank_mode;
  uint32_t bank_min;
  uint32_t bank_max;
  uint32_t ram_mode;
  uint32_t scc_disable;
  uint32_t psg_type;
  uint32_t scc_type;
  uint32_t opll_type;
  uint32_t opl_type;
  uint32_t DA8_enable;

  int32_t DA1; /* 1bit D/A (I/O mapped 0xAA) */
  int32_t DA8; /* 8bit D/A (memory mapped 0x5000 - 0x5FFF) */

  PSG *psg;
  SCC *scc;
  OPLL *opll;
  OPL *opl;
  SNG *sng;

  void *fp;

  void *iowrite_handler_context;
  void (*iowrite_handler)(void *context, uint32_t a, uint32_t d);
  void *memwrite_handler_context;
  void (*memwrite_handler)(void *context, uint32_t a, uint32_t d);
};

#define MSX_CLK (3579545)

enum { VM_MAIN_SLOT = 0, VM_BANK_SLOT = 1 };

enum { VM_PSG_AUTO = 0, VM_PSG_AY, VM_PSG_YM };
enum { VM_SCC_AUTO = 0, VM_SCC_STANDARD, VM_SCC_ENHANCED };
enum { VM_OPLL_2413 = 0, VM_OPLL_VRC7, VM_OPLL_281B };
enum { VM_OPL_PANA = 0, VM_OPL_TOSH, VM_OPL_PHIL };

VM *VM_new();
void VM_delete(VM *vm);
void VM_reset_device(VM *vm);
void VM_reset(VM *vm, uint32_t cpu_clk, uint32_t pc, uint32_t play_adr, double vsync_freq, uint32_t song,
              uint32_t DA8);
void VM_init_memory(VM *vm, uint32_t ram_mode, uint32_t offset, uint32_t num, uint8_t *data);
void VM_init_bank(VM *vm, uint32_t mode, uint32_t num, uint32_t offset, uint8_t *data);
void VM_exec(VM *vm, uint32_t cycles);
void VM_exec_func(VM *vm, uint32_t init_adr);
void VM_set_clock(VM *vm, uint32_t clock, double vsync_freq);
void VM_set_wioproc(VM *vm, uint32_t a, VM_WIOPROC p);

void VM_set_PSG_type(VM *vm, uint32_t psg_type);
void VM_set_SCC_type(VM *vm, uint32_t scc_type);
void VM_set_OPLL_type(VM *vm, uint32_t opll_type);
void VM_set_OPL_type(VM *vm, uint32_t opl_type);

void VM_write_memory(VM *vm, uint32_t a, uint32_t d);
void VM_write_io(VM *vm, uint32_t a, uint32_t d);

#ifdef __cplusplus
}
#endif

#endif
