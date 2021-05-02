/*
 *
 * vm.c - Virtual Machine using kmz80.c for KSSPLAY written by Mitsutaka Okazaki 2001.
 *
 * 2001-06-15 : Version 0.00
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "mmap.h"
#include "vm.h"

/* Callback functions from KMZ80 */
static uint32_t memread(VM *vm, uint32_t a) { return MMAP_read_memory(vm->mmap, a); }

static void memwrite(VM *vm, uint32_t a, uint32_t d) {
  uint32_t page = a >> 13;

  if (vm->memwrite_handler) {
    vm->memwrite_handler(vm->memwrite_handler_context, a, d);
  }

  if (!vm->scc_disable) {
    if (vm->scc_type == VM_SCC_AUTO && (a == 0x9000 || a == 0xB000 || a == 0xBFFE)) {
      if (a == 0x9000 && d != 0x00)
        SCC_write(vm->scc, a, 0x3f);
      else
        SCC_write(vm->scc, a, d);
    } else if ((0x9800 <= a && a <= 0x98FF) || (0xB800 <= a && a <= 0xB8FF)) {
      SCC_write(vm->scc, a, d);
      LPDETECT_write(vm->lpde, a, d);
    }
  }

  if ((vm->DA8_enable) && (0x5000 <= a && a <= 0x5FFF)) {
    vm->DA8 >>= 1;
    vm->DA8 += ((int32_t)(d & 0xff) - 0x80) << 3;
    LPDETECT_write(vm->lpde, a, d);
  }

  if ((vm->bank_mode == BANK_8K) && ((a == 0x9000) || (a == 0xB000))) {
    MMAP_select_bank(vm->mmap, page, VM_BANK_SLOT, d);
  }

  MMAP_write_memory(vm->mmap, a, d);

}

static uint32_t ioread(VM *vm, uint32_t a) {
  a &= 0xff;

  if ((vm->psg) && ((a & 0xff) == 0xA2))
    return PSG_readIO(vm->psg);
  else if (a == 0xC1)
    return OPL_readIO(vm->opl);
  else if (a == 0xC0)
    return OPL_status(vm->opl);

  return 0xff;
}

static void iowrite(VM *vm, uint32_t a, uint32_t d) {
  a &= 0xff;

  if (vm->iowrite_handler) {
    vm->iowrite_handler(vm->iowrite_handler_context, a, d);
  }

  if (((a == STOPIO) || (a == LOOPIO) || (a == ADRLIO) || (a == ADRHIO)) && (vm->IO[EXTIO] != 0x7f)) {
    return;
  }

  vm->IO[a] = (uint8_t)(d & 0xff);

  if (vm->WIOPROC[a])
    vm->WIOPROC[a](vm, a, d);

  switch (a) {
  case 0x06:
    if (vm->sng) {
      SNG_writeGGIO(vm->sng, d);
      LPDETECT_write(vm->lpde, a, d);
    }

    break;

  case 0xA0:
  case 0xA1:
    if (vm->psg) {
      PSG_writeIO(vm->psg, a, d);
      LPDETECT_write(vm->lpde, a, d);
    }
    break;

  case 0xAA:
    vm->DA1 >>= 1;
    vm->DA1 += d & 0x80 ? 256 << 3 : 0;
    LPDETECT_write(vm->lpde, a, d);
    break;

  case 0xAB:
    vm->DA1 >>= 1;
    vm->DA1 += d & 0x01 ? 256 << 3 : 0;
    LPDETECT_write(vm->lpde, a, d);
    break;

  case 0x7E:
  case 0x7F:
    if (vm->sng) {
      SNG_writeIO(vm->sng, d);
      LPDETECT_write(vm->lpde, a, d);
    }
    break;

  case 0x7C:
  case 0x7D:
  case 0xF0:
  case 0xF1:
    if (vm->opll) {
      OPLL_writeIO(vm->opll, a, d);
      LPDETECT_write(vm->lpde, a, d);
    }
    break;

  case 0xC0:
  case 0xC1:
    if (vm->opl) {
      OPL_writeIO(vm->opl, a, d);
      LPDETECT_write(vm->lpde, a, d);
    }
    break;

  default:
    break;
  }

  if ((vm->bank_mode == BANK_16K) && (a == 0xfe)) {
    if ((vm->bank_min <= d) && (d < vm->bank_max))
      MMAP_select_bank(vm->mmap, 4, VM_BANK_SLOT, d);
    else
      MMAP_select_bank(vm->mmap, 4, VM_MAIN_SLOT, 2);
  }
}

static uint32_t busread(VM *vm, uint32_t mode) { return 0x38; }

static void exec_setup(VM *vm, uint32_t pc) {
  uint32_t sp = 0xf380, rp;

  MMAP_write_memory(vm->mmap, --sp, 0);
  MMAP_write_memory(vm->mmap, --sp, 0xfe);
  MMAP_write_memory(vm->mmap, --sp, 0x18); /* JR +0 */
  MMAP_write_memory(vm->mmap, --sp, 0x76); /* HALT */
  rp = sp;
  MMAP_write_memory(vm->mmap, --sp, rp >> 8);
  MMAP_write_memory(vm->mmap, --sp, rp & 0xff);

  vm->context.sp = sp;
  vm->context.pc = pc;
  vm->context.regs8[REGID_HALTED] = 0;
}

/* Handler for KMEVENT */
static void vsync(KMEVENT *event, KMEVENT_ITEM_ID curid, VM *vm) {
  kmevent_settimer(&vm->kme, vm->vsync_id, vm->vsync_cycles);
  if (vm->context.regs8[REGID_HALTED])
    exec_setup(vm, vm->vsync_adr);
}

/* Interfaces for class VM  */
VM *VM_new(int rate) {
  VM *vm;

  if (!(vm = malloc(sizeof(VM))))
    return NULL;

  memset(vm, 0, sizeof(VM));

  vm->sng = SNG_new(MSX_CLK, rate);
  vm->psg = PSG_new(MSX_CLK, rate);
  vm->scc = SCC_new(MSX_CLK, rate);
  vm->opll = OPLL_new(MSX_CLK, rate);
  vm->opl = OPL_new(MSX_CLK, rate);
  vm->mmap = MMAP_new();
  vm->lpde = LPDETECT_new();
  vm->ram_mode = 0;
  vm->DA8_enable = 0;
  vm->bank_mode = BANK_16K;
  vm->psg_type = VM_PSG_AUTO;
  vm->scc_type = VM_SCC_AUTO;
  vm->opll_type = VM_OPLL_2413;
  assert(vm->mmap);

  return vm;
}

/* Delete Object */
void VM_delete(VM *vm) {
  MMAP_delete(vm->mmap);
  LPDETECT_delete(vm->lpde);
  SNG_delete(vm->sng);
  PSG_delete(vm->psg);
  SCC_delete(vm->scc);
  OPLL_delete(vm->opll);
  OPL_delete(vm->opl);
  kmevent_free(&vm->kme, vm->vsync_id);
  free(vm);
}

void VM_exec(VM *vm, uint32_t cycles) { kmz80_exec(&vm->context, cycles); }

void VM_exec_func(VM *vm, uint32_t func_adr) { exec_setup(vm, func_adr); }

void VM_set_wioproc(VM *vm, uint32_t a, VM_WIOPROC p) { vm->WIOPROC[a] = p; }

void VM_set_clock(VM *vm, uint32_t clock, double vsync_freq) {
  vm->clock = clock;
  vm->vsync_cycles = clock / vsync_freq;
  kmevent_settimer(&vm->kme, vm->vsync_id, vm->vsync_cycles);
}

void VM_reset_device(VM *vm) {
  PSG_reset(vm->psg);
  VM_set_PSG_type(vm, vm->psg_type);
  SCC_reset(vm->scc);
  VM_set_SCC_type(vm, vm->scc_type);
  OPLL_reset(vm->opll);
  VM_set_OPLL_type(vm, vm->opll_type);
  OPL_reset(vm->opl);
  VM_set_OPL_type(vm, vm->opl_type);
  SNG_reset(vm->sng);
}

void VM_set_PSG_type(VM *vm, uint32_t psg_type) {
  if (!vm->psg)
    return;

  if (psg_type == VM_PSG_AUTO)
    PSG_setVolumeMode(vm->psg, EMU2149_VOL_DEFAULT);
  else if (psg_type == VM_PSG_AY)
    PSG_setVolumeMode(vm->psg, EMU2149_VOL_AY_3_8910);
  else if (psg_type == VM_PSG_YM)
    PSG_setVolumeMode(vm->psg, EMU2149_VOL_YM2149);
  vm->psg_type = psg_type;
}

void VM_set_SCC_type(VM *vm, uint32_t scc_type) {
  if (!vm->scc)
    return;

  if (scc_type == VM_SCC_AUTO) {
    SCC_set_type(vm->scc, SCC_ENHANCED);
    SCC_write(vm->scc, 0x9000, 0x3f);
  } else if (scc_type == VM_SCC_STANDARD) {
    SCC_set_type(vm->scc, SCC_STANDARD);
    SCC_write(vm->scc, 0x9000, 0x3f);
  } else if (scc_type == VM_SCC_ENHANCED) {
    SCC_set_type(vm->scc, SCC_ENHANCED);
    SCC_write(vm->scc, 0xBFFE, 0x20);
    SCC_write(vm->scc, 0xB000, 0x80);
  }
  vm->scc_type = scc_type;
}

void VM_set_OPLL_type(VM *vm, uint32_t opll_type) {
  if (!vm->opll)
    return;

  if (opll_type == VM_OPLL_2413)
    OPLL_reset_patch(vm->opll, OPLL_2413_TONE);
  else if (opll_type == VM_OPLL_VRC7)
    OPLL_reset_patch(vm->opll, OPLL_VRC7_TONE);
  else if (opll_type == VM_OPLL_281B)
    OPLL_reset_patch(vm->opll, OPLL_281B_TONE);
  else
    ;
  vm->opll_type = opll_type;
}

void VM_set_OPL_type(VM *vm, uint32_t opl_type) { vm->opl_type = opl_type; }

void VM_reset(VM *vm, uint32_t clock, uint32_t init_adr, uint32_t vsync_adr, double vsync_freq, uint32_t song,
              uint32_t DA8) {
  /* Reset KMZ80 */
  kmz80_reset(&vm->context);
  LPDETECT_reset(vm->lpde);

  memset(vm->IO, 0, sizeof(vm->IO));
  memset(vm->WIOPROC, 0, sizeof(vm->IO));
  vm->DA1 = 0;
  vm->DA8_enable = DA8;

  vm->context.user = vm;
  vm->context.memread = (void *)memread;
  vm->context.memwrite = (void *)memwrite;
  vm->context.ioread = (void *)ioread;
  vm->context.iowrite = (void *)iowrite;
  vm->context.busread = (void *)busread;

  vm->context.regs8[REGID_M1CYCLE] = 2;
  vm->context.regs8[REGID_A] = (uint8_t)(song & 0xff);
  vm->context.regs8[REGID_HALTED] = 0;
  vm->context.exflag = 3;
  vm->context.regs8[REGID_IFF1] = 0;
  vm->context.regs8[REGID_IFF2] = 0;
  vm->context.regs8[REGID_INTREQ] = 0;
  vm->context.regs8[REGID_IMODE] = 1;

  /* Execute init code : Wait until return the init (max 1 sec). */
  vm->clock = clock;
  VM_exec_func(vm, init_adr);

  /* VSYNC SETTINGS */
  vm->vsync_adr = vsync_adr;
  kmevent_init(&vm->kme);
  vm->vsync_id = kmevent_alloc(&vm->kme);
  kmevent_setevent(&vm->kme, vm->vsync_id, vsync, vm);
  vm->context.kmevent = &vm->kme;
  VM_set_clock(vm, clock, vsync_freq);
}

void VM_init_memory(VM *vm, uint32_t ram_mode, uint32_t offset, uint32_t size, uint8_t *data) {
  int i;
  uint8_t *main_memory;

  assert(vm);
  assert(vm->mmap);

  vm->ram_mode = ram_mode;

  if (!(main_memory = malloc(0x10000))) {
    assert(0);
    return;
  }

  memset(main_memory, 0xC9, 0x10000);
  memcpy(main_memory + 0x93, "\xC3\x01\x00\xC3\x09\x00", 6);
  memcpy(main_memory + 0x01, "\xD3\xA0\xF5\x7B\xD3\xA1\xF1\xC9", 8);
  memcpy(main_memory + 0x09, "\xD3\xA0\xDB\xA2\xC9", 5);
  memset(main_memory + 0x4000, 0, 0xC000);
  if ((offset + size) > 0x10000)
    size = 0x10000 - offset;
  memcpy(main_memory + offset, data, size);

  for (i = 0; i < 4; i++) {
    MMAP_set_bank_data(vm->mmap, VM_MAIN_SLOT, i, BANK_16K, main_memory + 0x4000 * i);
    MMAP_set_bank_attr(vm->mmap, VM_MAIN_SLOT, i, BANK_READABLE | BANK_WRITEABLE);
    MMAP_select_bank(vm->mmap, i << 1, VM_MAIN_SLOT, i);
  }

  if (!ram_mode)
    MMAP_set_bank_attr(vm->mmap, VM_MAIN_SLOT, 2, BANK_READABLE);

  free(main_memory);
}

void VM_init_bank(VM *vm, uint32_t mode, uint32_t num, uint32_t offset, uint8_t *data) {
  uint32_t size, i;

  assert(vm);
  assert(vm->mmap);

  vm->bank_mode = mode;
  vm->bank_min = offset;
  vm->bank_max = offset + num;

  if (mode == BANK_16K) {
    size = 0x4000;
    for (i = 0; i < num; i++)
      MMAP_set_bank_data(vm->mmap, VM_BANK_SLOT, i + offset, BANK_16K, data + i * size);
  } else if (mode == BANK_8K) {
    size = 0x2000;
    for (i = 0; i < num; i++)
      MMAP_set_bank_data(vm->mmap, VM_BANK_SLOT, i + offset, BANK_8K, data + i * size);
  } else {
    assert(0);
  }
}

void VM_write_memory(VM *vm, uint32_t a, uint32_t d) {
  memwrite(vm, a, d);
}

void VM_write_io(VM *vm, uint32_t a, uint32_t d) {
  iowrite(vm, a, d);
}