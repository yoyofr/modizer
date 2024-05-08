#ifndef MYON_FMPLAYER_COMMON_H_INCLUDED
#define MYON_FMPLAYER_COMMON_H_INCLUDED

#include <stddef.h>
#include <stdbool.h>

void *fmplayer_load_data(const char *name, size_t size);

struct fmdriver_work;
struct ppz8;
struct opna;
struct opna_timer;
void fmplayer_init_work_opna(
  struct fmdriver_work *work,
  struct ppz8 *ppz8,
  struct opna *opna,
  struct opna_timer *timer,
  void *adpcm_ram
);

#endif // MYON_FMPLAYER_COMMON_H_INCLUDED
