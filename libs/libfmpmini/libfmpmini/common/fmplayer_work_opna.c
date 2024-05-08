#include "fmplayer_common.h"
#include "fmplayer_drumrom.h"
#include "fmdriver/fmdriver.h"
#include "fmdriver/ppz8.h"
#include "libopna/opna.h"
#include "libopna/opnatimer.h"
#include <string.h>

enum {
  SRATE = 55467,
  PPZ8MIX = 0xa000,
};

static void opna_writereg_libopna(struct fmdriver_work *work, unsigned addr, unsigned data) {
  struct opna_timer *timer = (struct opna_timer *)work->opna;
  opna_timer_writereg(timer, addr, data);
}

static unsigned opna_readreg_libopna(struct fmdriver_work *work, unsigned addr) {
  struct opna_timer *timer = (struct opna_timer *)work->opna;
  return opna_readreg(timer->opna, addr);
}

static uint8_t opna_status_libopna(struct fmdriver_work *work, bool a1) {
  struct opna_timer *timer = (struct opna_timer *)work->opna;
  uint8_t status = opna_timer_status(timer);
  if (!a1) {
    status &= 0x83;
  }
  return status;
}

static void opna_int_cb(void *userptr) {
  struct fmdriver_work *work = (struct fmdriver_work *)userptr;
  work->driver_opna_interrupt(work);
}

static void opna_mix_cb(void *userptr, int16_t *buf, unsigned samples) {
  struct ppz8 *ppz8 = (struct ppz8 *)userptr;
  ppz8_mix(ppz8, buf, samples);
}

void fmplayer_init_work_opna(
  struct fmdriver_work *work,
  struct ppz8 *ppz8,
  struct opna *opna,
  struct opna_timer *timer,
  void *adpcm_ram
) {
  opna_reset(opna);
  fmplayer_drum_rom_load(&opna->drum);
  opna_adpcm_set_ram_256k(&opna->adpcm, adpcm_ram);
  opna_timer_reset(timer, opna);
  ppz8_init(ppz8, SRATE, PPZ8MIX);
  memset(work, 0, sizeof(*work));
  work->opna_writereg = opna_writereg_libopna;
  work->opna_readreg = opna_readreg_libopna;
  work->opna_status = opna_status_libopna;
  work->opna = timer;
  work->ppz8 = ppz8;
  work->ppz8_functbl = &ppz8_functbl;
  opna_timer_set_int_callback(timer, opna_int_cb, work);
  opna_timer_set_mix_callback(timer, opna_mix_cb, ppz8);
}
