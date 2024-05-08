#include "fmplayer_fontrom.h"
#include "fmdsp/font.h"
#include <stdint.h>
#include <stdbool.h>
#include <wchar.h>
#include "common/winfont.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>

static struct {
  uint8_t fontrombuf[FONT_ROM_FILESIZE];
  bool font_rom_loaded;
} g;

bool fmplayer_font_rom_load(struct fmdsp_font *font) {
  const wchar_t *path = L"font.rom";
  wchar_t exepath[MAX_PATH];
  if (GetModuleFileNameW(0, exepath, MAX_PATH)) {
    PathRemoveFileSpecW(exepath);
    if ((wcslen(exepath) + wcslen(path) + 1 + 1) < MAX_PATH) {
      wcscat(exepath, L"\\");
      wcscat(exepath, path);
      path = exepath;
    }
  }
  HANDLE file = CreateFileW(path, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (file == INVALID_HANDLE_VALUE) goto err;
  DWORD filesize = GetFileSize(file, 0);
  if (filesize != FONT_ROM_FILESIZE) goto err;
  DWORD readbytes;
  if (!ReadFile(file, g.fontrombuf, FONT_ROM_FILESIZE, &readbytes, 0) || readbytes != FONT_ROM_FILESIZE) goto err;
  CloseHandle(file);
  fmdsp_font_from_font_rom(font, g.fontrombuf);
  g.font_rom_loaded = true;
  return true;
err:
  if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
  fmdsp_font_win(font);
  return false;
}
