#ifndef LIBOPNA_OPNADRUM_H_INCLUDED
#define LIBOPNA_OPNADRUM_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#ifdef LIBOPNA_ENABLE_LEVELDATA
#include "leveldata/leveldata.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define OPNA_ROM_BD_START  0x0000
#define OPNA_ROM_SD_START  0x01c0
#define OPNA_ROM_TOP_START 0x0440
#define OPNA_ROM_HH_START  0x1b80
#define OPNA_ROM_TOM_START 0x1d00
#define OPNA_ROM_RIM_START 0x1f80
#define OPNA_ROM_SIZE      0x2000

#define OPNA_ROM_BD_SIZE   ((OPNA_ROM_SD_START-OPNA_ROM_BD_START)*2*3)
#define OPNA_ROM_SD_SIZE   ((OPNA_ROM_TOP_START-OPNA_ROM_SD_START)*2*3)
#define OPNA_ROM_TOP_SIZE   ((OPNA_ROM_HH_START-OPNA_ROM_TOP_START)*2*3)
#define OPNA_ROM_HH_SIZE   ((OPNA_ROM_TOM_START-OPNA_ROM_HH_START)*2*3)
#define OPNA_ROM_TOM_SIZE   ((OPNA_ROM_RIM_START-OPNA_ROM_TOM_START)*2*6)
#define OPNA_ROM_RIM_SIZE   ((OPNA_ROM_SIZE-OPNA_ROM_RIM_START)*2*6)

struct opna_drum {
  struct {
    int16_t *data;
    bool playing;
    unsigned index;
    unsigned len;
    unsigned level;
    bool left;
    bool right;
#ifdef LIBOPNA_ENABLE_LEVELDATA
    struct leveldata leveldata;
#endif
  } drums[6];
  unsigned total_level;
  int16_t rom_bd[OPNA_ROM_BD_SIZE];
  int16_t rom_sd[OPNA_ROM_SD_SIZE];
  int16_t rom_top[OPNA_ROM_TOP_SIZE];
  int16_t rom_hh[OPNA_ROM_HH_SIZE];
  int16_t rom_tom[OPNA_ROM_TOM_SIZE];
  int16_t rom_rim[OPNA_ROM_RIM_SIZE];
  unsigned mask;
};

void opna_drum_reset(struct opna_drum *drum);
// set rom data, size: 0x2000 (8192) bytes
void opna_drum_set_rom(struct opna_drum *drum, void *rom);

void opna_drum_mix(struct opna_drum *drum, int16_t *buf, int samples);

void opna_drum_writereg(struct opna_drum *drum, unsigned reg, unsigned val);

#ifdef __cplusplus
}
#endif

#endif // LIBOPNA_OPNADRUM_H_INCLUDED
