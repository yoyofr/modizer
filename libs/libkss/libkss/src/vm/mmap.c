/*
 *
 * mmap.c - Memory Mapper for Virtual Machine (like MSX) written by Mitsutaka Okazaki 2001.
 *
 * 2001-06-16 : version 0.00
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include "mmap.h"

#if defined(_MSC_VER)
#define INLINE __forceinline
#elif defined(__GNUC__)
#define INLINE __inline__
#else
#define INLINE static
#endif

/* for write only and read only memory */
static uint8_t dummy_read_map[0x2000], dummy_write_map[0x2000];

#define FAILED(x) ((x) == NULL)

static BANK *BANK_new(uint32_t type, uint8_t *data) {
  BANK *bank;
  uint32_t size;

  if (type == BANK_16K)
    size = 0x4000;
  else if (type == BANK_8K)
    size = 0x2000;
  else
    return NULL;

  if (FAILED(bank = malloc(sizeof(BANK))))
    return NULL;
  if (FAILED(bank->body = malloc(size))) {
    free(bank);
    return NULL;
  }

  bank->type = type;
  bank->refc = 0;
  bank->attr = BANK_READABLE;
  if (data)
    memcpy(bank->body, data, size);

  return bank;
}

static void BANK_delete(BANK *bank) {
  if (bank) {
    free(bank->body);
    free(bank);
  }
}

void MMAP_set_page_attr(MMAP *mmap, uint32_t page, uint32_t attr) {
  MMAP_set_bank_attr(mmap, mmap->current_slot[page], mmap->current_bank[page], attr);
}

void MMAP_set_bank_attr(MMAP *mmap, uint32_t slot, uint32_t bank, uint32_t attr) {
  int i;
  assert(slot < 0x10);
  assert(bank < 0x100);

  if (mmap->bank[slot][bank])
    mmap->bank[slot][bank]->attr = attr;

  for (i = 0; i < 8; i++) {
    if ((mmap->current_slot[i] == slot) && (mmap->current_bank[i] == bank))
      MMAP_select_bank(mmap, i, slot, bank);
  }
}

void MMAP_set_bank_data(MMAP *mmap, uint32_t slot, uint32_t bank, uint32_t type, uint8_t *data) {
  assert(slot < 0x10);
  assert(bank < 0x100);

  if (mmap->bank[slot][bank])
    MMAP_unset_bank(mmap, slot, bank);
  if (FAILED(mmap->bank[slot][bank] = BANK_new(type, data))) {
    assert(0); /* Out of Memory */
    return;
  }

  {
    int i;
    for (i = 0; i < 8; i++)
      MMAP_select_bank(mmap, i, mmap->current_slot[i], mmap->current_bank[i]);
  }
}

void MMAP_unset_bank(MMAP *mmap, uint32_t slot, uint32_t bank) {
  assert(slot < 0x10);
  assert(bank < 0x100);

  if (mmap->bank[slot][bank]) {
    if (mmap->bank[slot][bank]->refc)
      mmap->bank[slot][bank]->refc--;
    else
      BANK_delete(mmap->bank[slot][bank]);
    mmap->bank[slot][bank] = NULL;
  }

  {
    int i;
    for (i = 0; i < 8; i++)
      MMAP_select_bank(mmap, i, mmap->current_slot[i], mmap->current_bank[i]);
  }
}

void MMAP_mirror_bank(MMAP *mmap, uint32_t src_slot, uint32_t src_bank, uint32_t dst_slot, uint32_t dst_bank) {
  assert(src_slot < 0x10);
  assert(src_bank < 0x100);
  assert(dst_slot < 0x10);
  assert(dst_bank < 0x100);

  MMAP_unset_bank(mmap, dst_slot, dst_bank);

  mmap->bank[dst_slot][dst_bank] = mmap->bank[src_slot][dst_slot];
  if (mmap->bank[dst_slot][dst_bank])
    mmap->bank[dst_slot][dst_bank]->refc++;

  {
    int i;
    for (i = 0; i < 8; i++)
      MMAP_select_bank(mmap, i, mmap->current_slot[i], mmap->current_bank[i]);
  }
}

void MMAP_select_bank(MMAP *mmap, uint32_t page, uint32_t slot, uint32_t bank) {
  assert(slot < 0x10);
  assert(bank < 0x100);

  /*
  if(mmap->bank[slog][bank])
  printf("BANK SELECT: PAGE%d - SLOT%d is BANK%d (%d) \n", page, slot, bank, mmap->bank[slot][bank]->attr) ;
  */
  if (!mmap->bank[slot][bank]) {
    mmap->readmap[page] = dummy_read_map;
    mmap->writemap[page] = dummy_write_map;
    mmap->current_slot[page] = slot;
    mmap->current_bank[page] = bank;
  } else if (mmap->bank[slot][bank]->type == BANK_16K) {
    page &= 6;

    if (mmap->bank[slot][bank]->attr & BANK_READABLE) {
      mmap->readmap[page] = mmap->bank[slot][bank]->body;
      mmap->readmap[page + 1] = mmap->bank[slot][bank]->body + 0x2000;
    } else {
      mmap->readmap[page] = dummy_read_map;
      mmap->readmap[page + 1] = dummy_read_map;
    }

    if (mmap->bank[slot][bank]->attr & BANK_WRITEABLE) {
      mmap->writemap[page] = mmap->bank[slot][bank]->body;
      mmap->writemap[page + 1] = mmap->bank[slot][bank]->body + 0x2000;
    } else {
      mmap->writemap[page] = dummy_write_map;
      mmap->writemap[page + 1] = dummy_write_map;
    }

    mmap->current_slot[page] = mmap->current_slot[page + 1] = slot;
    mmap->current_bank[page] = mmap->current_bank[page + 1] = bank;
  } else if (mmap->bank[slot][bank]->type == BANK_8K) {
    if (mmap->bank[slot][bank]->attr & BANK_READABLE)
      mmap->readmap[page] = mmap->bank[slot][bank]->body;
    else
      mmap->readmap[page] = dummy_read_map;
    if (mmap->bank[slot][bank]->attr & BANK_WRITEABLE)
      mmap->writemap[page] = mmap->bank[slot][bank]->body;
    else
      mmap->writemap[page] = dummy_write_map;

    mmap->current_slot[page] = slot;
    mmap->current_bank[page] = bank;
  } else {
    assert(0);
  }
}

uint32_t MMAP_read_memory(MMAP *mmap, uint32_t adr) {
  assert(adr < 0x10000);
  return mmap->readmap[adr >> 13][adr & 0x1fff];
}

void MMAP_write_memory(MMAP *mmap, uint32_t adr, uint32_t val) {
  assert(adr < 0x10000);
  mmap->writemap[adr >> 13][adr & 0x1fff] = (uint8_t)(val & 0xff);
}

MMAP *MMAP_new() {
  MMAP *mmap;
  int i;

  if (FAILED(mmap = malloc(sizeof(MMAP))))
    return NULL;
  memset(mmap, 0, sizeof(MMAP));

  for (i = 0; i < 8; i++)
    MMAP_select_bank(mmap, i, 0, i);

  return mmap;
}

void MMAP_delete(MMAP *mmap) {
  int slot, bank;
  for (slot = 0; slot < 0x10; slot++)
    for (bank = 0; bank < 0x100; bank++)
      MMAP_unset_bank(mmap, slot, bank);

  free(mmap);
}
