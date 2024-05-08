#ifndef MYON_FMPLAYER_FONTROM_H_INCLUDED
#define MYON_FMPLAYER_FONTROM_H_INCLUDED

#include <stdbool.h>

struct fmdsp_font;

// true if font rom available, false if using alternatives
bool fmplayer_font_rom_load(struct fmdsp_font *font);

#endif // MYON_FMPLAYER_FONTROM_H_INCLUDED
