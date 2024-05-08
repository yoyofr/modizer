
#define _POSIX_C_SOURCE 200809l
#include "common/fmplayer_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <strings.h>
#include <iconv.h>
#ifdef __APPLE__
#include <xlocale.h>
#else
#include <locale.h>
#endif
#include <langinfo.h>

static void *fileread(const char *path, size_t maxsize, size_t *filesize, enum fmplayer_file_error *error) {
  FILE *f = 0;
  void *buf = 0;

  f = fopen(path, "rb");
  if (!f) {
    if (error) *error = FMPLAYER_FILE_ERR_NOTFOUND;
    goto err;
  }
  if (fseek(f, 0, SEEK_END)) {
    *error = FMPLAYER_FILE_ERR_FILEIO;
    goto err;
  }
  size_t fsize;
  {
    long ssize = ftell(f);
    if (ssize < 0) {
      *error = FMPLAYER_FILE_ERR_FILEIO;
      goto err;
    }
    if (maxsize && ((size_t)ssize > maxsize)) {
      *error = FMPLAYER_FILE_ERR_BADFILE_SIZE;
      goto err;
    }
    fsize = ssize;
  }
  if (fseek(f, 0, SEEK_SET)) {
    *error = FMPLAYER_FILE_ERR_FILEIO;
    goto err;
  }
  buf = malloc(fsize);
  if (!buf) {
    if (error) *error = FMPLAYER_FILE_ERR_NOMEM;
    goto err;
  }
  if (fread(buf, 1, fsize, f) != fsize) {
    *error = FMPLAYER_FILE_ERR_FILEIO;
    goto err;
  }
  fclose(f);
  *filesize = fsize;
  return buf;
err:
  free(buf);
  if (f) fclose(f);
  return 0;
}

void *fmplayer_fileread(const void *pathptr, const char *pcmname, const char *extension,
                        size_t maxsize, size_t *filesize, enum fmplayer_file_error *error) {
  const char *path = pathptr;
  if (!pcmname) return fileread(path, maxsize, filesize, error);

  char *namebuf = 0;
  char *dirbuf = 0;
  DIR *d = 0;
  
  if (extension) {
    size_t namebuflen = strlen(pcmname) + strlen(extension) + 1;
    namebuf = malloc(namebuflen);
    if (!namebuf) {
      if (error) *error = FMPLAYER_FILE_ERR_NOMEM;
      goto err;
    }
    strcpy(namebuf, pcmname);
    strcat(namebuf, extension);
    pcmname = namebuf;
  }

  const char *slash = strrchr(path, '/');
  const char *dirpath = 0;
  if (slash) {
    dirbuf = strdup(path);
    if (!dirbuf) {
      if (error) *error = FMPLAYER_FILE_ERR_NOMEM;
      goto err;
    }
    *strrchr(dirbuf, '/') = 0;
    dirpath = dirbuf;
  } else {
    dirpath = ".";
  }
  d = opendir(dirpath);
  if (!d) {
    *error = FMPLAYER_FILE_ERR_FILEIO;
    goto err;
  }
  const struct dirent *de;
  while ((de = readdir(d))) {
    if (!strcasecmp(de->d_name, pcmname)) {
      size_t pathlen = strlen(dirpath) + 1 + strlen(de->d_name) + 1;
      char *pcmpath = malloc(pathlen);
      if (!pcmpath) {
        if (error) *error = FMPLAYER_FILE_ERR_NOMEM;
        goto err;
      }
      strcpy(pcmpath, dirpath);
      strcat(pcmpath, "/");
      strcat(pcmpath, de->d_name);
      void *buf = fileread(pcmpath, maxsize, filesize, error);
      free(pcmpath);
      closedir(d);
      free(dirbuf);
      free(namebuf);
      return buf;
    }
  }
  if (error) *error = FMPLAYER_FILE_ERR_NOTFOUND;
err:
  if (d) closedir(d);
  free(dirbuf);
  free(namebuf);
  return 0;
}

void *fmplayer_path_dup(const void *path) {
  return strdup(path);
}

char *fmplayer_path_filename_sjis(const void *pathptr) {
  char *path = (char *)pathptr;
  locale_t loc = 0;
  iconv_t ic = (iconv_t)-1;
  char *pathbuf = 0;
  char *fname = strrchr(path, '/');
  if (fname) {
    if (*fname) fname++;
  } else {
    fname = path;
  }
  // use locale from environment values (to guess filesystem path encoding)
  loc = newlocale(LC_CTYPE_MASK, "", 0);
  if (!loc) {
    //perror("newlocale");
    goto err;
  }
  const int pathbuflen = 160;
  pathbuf = malloc(pathbuflen);
  if (!pathbuf) goto err;
  ic = iconv_open("SHIFT_JIS", nl_langinfo_l(CODESET, loc));
  if (ic == (iconv_t)-1) {
    //perror("iconv_open");
    goto err;
  }
  char *src = fname, *dst = pathbuf;
  size_t srcleft = strlen(fname);
  size_t dstleft = pathbuflen;
  if (iconv(ic, &src, &srcleft, &dst, &dstleft) == (size_t)-1) {
    //perror("iconv");
    goto err;
  }
  iconv_close(ic);
  *dst = 0;
  freelocale(loc);
  return pathbuf;
err:
  free(pathbuf);
  if (ic != (iconv_t)-1) iconv_close(ic);
  if (loc) freelocale(loc);
  return strdup(fname);
}
