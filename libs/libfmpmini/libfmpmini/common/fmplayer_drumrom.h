#ifndef MYON_FMPLAYER_DRUMROM_H_INCLUDED
#define MYON_FMPLAYER_DRUMROM_H_INCLUDED

#include <stdbool.h>

struct opna_drum;
bool fmplayer_drum_rom_load(struct opna_drum *drum);
bool fmplayer_drum_loaded(void);

#endif // MYON_FMPLAYER_DRUMROM_H_INCLUDED
