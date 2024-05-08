#include "fmplayer_fontrom.h"
#include "fmdsp/font.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "fmdsp/fontrom_shinonome.inc"

#define DATADIR "/.local/share/98fmplayer/"

static struct {
  uint8_t fontrombuf[FONT_ROM_FILESIZE];
} g;

bool fmplayer_font_rom_load(struct fmdsp_font *font) {
  const char *path = "font.rom";
  const char *home = getenv("HOME");
  char *dpath = 0;
  FILE *f = 0;
  fmdsp_font_from_font_rom(font, g.fontrombuf);
  if (home) {
    const char *datadir = DATADIR;
    dpath = malloc(strlen(home)+strlen(datadir)+strlen(path)+1);
    if (dpath) {
      strcpy(dpath, home);
      strcat(dpath, datadir);
      strcat(dpath, path);
      path = dpath;
    }
  }
  f = fopen(path, "r");
  free(dpath);
  if (!f) goto err;
  if (fseek(f, 0, SEEK_END) != 0) goto err;
  long size = ftell(f);
  if (size != FONT_ROM_FILESIZE) goto err;
  if (fseek(f, 0, SEEK_SET) != 0) goto err;
  if (fread(g.fontrombuf, 1, FONT_ROM_FILESIZE, f) != FONT_ROM_FILESIZE) {
    goto err;
  }
  fclose(f);
  return true;

err:
  if (f) fclose(f);
  fmdsp_font_from_font_rom(font, fmdsp_shinonome_font_rom);
  return false;
}
