#include "common/fmplayer_file.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>
#include <stdlib.h>
#include <wchar.h>

#if !defined(FMPLAYER_FILE_WIN_UTF16) && !defined(FMPLAYER_FILE_WIN_UTF8)
#error "specify path string format"
#endif

static void *fileread(const wchar_t *path,
                      size_t maxsize, size_t *filesize,
                      enum fmplayer_file_error *error) {
  HANDLE file = INVALID_HANDLE_VALUE;
  void *buf = 0;
  file = CreateFileW(path, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (file == INVALID_HANDLE_VALUE) {
    if (error) *error = FMPLAYER_FILE_ERR_NOTFOUND;
    goto err;
  }
  LARGE_INTEGER li;
  if (!GetFileSizeEx(file, &li)) {
    if (error) *error = FMPLAYER_FILE_ERR_FILEIO;
    goto err;
  }
  if (li.HighPart || (maxsize && (li.LowPart > maxsize))) {
    if (error) *error = FMPLAYER_FILE_ERR_BADFILE_SIZE;
    goto err;
  }
  buf = malloc(li.LowPart);
  if (!buf) {
    if (error) *error = FMPLAYER_FILE_ERR_NOMEM;
    goto err;
  }
  DWORD readlen;
  if (!ReadFile(file, buf, li.LowPart, &readlen, 0) || (readlen != li.LowPart)) {
    if (error) *error = FMPLAYER_FILE_ERR_FILEIO;
    goto err;
  }
  *filesize = li.QuadPart;
  CloseHandle(file);
  return buf;
err:
  free(buf);
  if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
  return 0;
}

static inline wchar_t *u8tou16(const char *s) {
  wchar_t *ws = 0;
  int slen = MultiByteToWideChar(CP_UTF8, 0, s, -1, 0, 0);
  if (!slen) goto err;
  ws = malloc(slen * sizeof(wchar_t));
  if (!ws) goto err;
  if (!MultiByteToWideChar(CP_UTF8, 0, s, -1, ws, slen)) goto err;
  return ws;
err:
  free(ws);
  return 0;
}

void *fmplayer_fileread(const void *pathptr, const char *pcmname, const char *extension,
                        size_t maxsize, size_t *filesize, enum fmplayer_file_error *error) {
  const wchar_t *path = 0;
#if defined(FMPLAYER_FILE_WIN_UTF16)
  path = wcsdup(pathptr);
#elif defined(FMPLAYER_FILE_WIN_UTF8)
  path = u8tou16(pathptr);
#endif
  wchar_t *wpcmpath = 0, *wpcmname = 0, *wpcmextname = 0;
  if (!pcmname) return fileread(path, maxsize, filesize, error);
  int wpcmnamelen = MultiByteToWideChar(932, 0, pcmname, -1, 0, 0);
  if (!wpcmnamelen) goto err;
  if (extension) {
    int wextensionlen = MultiByteToWideChar(932, 0, extension, -1, 0, 0);
    if (!wextensionlen) goto err;
    wpcmnamelen += wextensionlen;
    wpcmnamelen -= 1;
    wpcmextname = malloc(wextensionlen * sizeof(wchar_t));
    if (!wpcmextname) goto err;
    if (!MultiByteToWideChar(932, 0, extension, -1, wpcmextname, wextensionlen)) goto err;
  }
  wpcmname = malloc(wpcmnamelen * sizeof(wchar_t));
  if (!wpcmname) goto err;
  if (!MultiByteToWideChar(932, 0, pcmname, -1, wpcmname, wpcmnamelen)) goto err;
  if (wpcmextname) wcscat(wpcmname, wpcmextname);
  wpcmpath = malloc((wcslen(path) + 1 + wcslen(wpcmname) + 1) * sizeof(wchar_t));
  if (!wpcmpath) goto err;
  wcscpy(wpcmpath, path);
  PathRemoveFileSpecW(wpcmpath);
  wcscat(wpcmpath, L"\\");
  wcscat(wpcmpath, wpcmname);
  void *buf = fileread(wpcmpath, maxsize, filesize, error);
  free(wpcmextname);
  free(wpcmname);
  free(wpcmpath);
  free((void *)path);
  return buf;
err:
  free(wpcmextname);
  free(wpcmname);
  free(wpcmpath);
  free((void *)path);
  return 0;
}

void *fmplayer_path_dup(const void *pathptr) {
#if defined(FMPLAYER_FILE_WIN_UTF16)
  const wchar_t *path = pathptr;
  return wcsdup(path);
#elif defined(FMPLAYER_FILE_WIN_UTF8)
  const char *path = pathptr;
  return strdup(path);
#endif
}

char *fmplayer_path_filename_sjis(const void *pathptr) {
  wchar_t *u16_path = 0;
  char *filename_sjis = 0;
#if defined(FMPLAYER_FILE_WIN_UTF16)
  const wchar_t *path = pathptr;
  u16_path = wcsdup(path);
#elif defined(FMPLAYER_FILE_WIN_UTF8)
  const char *path = pathptr;
  u16_path = u8tou16(path);
#endif
  if (!u16_path) goto err;
  PathStripPathW(u16_path);
  int bufsize = WideCharToMultiByte(932, 0, u16_path, -1, 0, 0, 0, 0);
  if (bufsize <= 0) goto err;
  filename_sjis = malloc(bufsize);
  if (!filename_sjis) goto err;
  bufsize = WideCharToMultiByte(
      932, 0, u16_path, -1, filename_sjis, bufsize, 0, 0);
  if (!bufsize) goto err;
  free(u16_path);
  return filename_sjis;

err:
  free(filename_sjis);
  free(u16_path);
  return 0;
}
