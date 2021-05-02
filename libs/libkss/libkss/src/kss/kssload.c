#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kss.h"

#ifdef KSS_ZIP_SUPPORT
/* Reference : memzip.h -- 2001 Mamiya (NEZplug) */
#include "../zlib/zlib.h" /* ZLIB1.1.4 required */

enum {
  ZIPMETHOD_STORED = 0,
  ZIPMETHOD_SHRUNK = 1,
  ZIPMETHOD_REDUCED1 = 2,
  ZIPMETHOD_REDUCED2 = 3,
  ZIPMETHOD_REDUCED3 = 4,
  ZIPMETHOD_REDUCED4 = 5,
  ZIPMETHOD_IMPLODED = 6,
  ZIPMETHOD_TOKENIZED = 7,
  ZIPMETHOD_DEFLATED = 8,
  ZIPMETHOD_ENHDEFLATED = 9,
  ZIPMETHOD_DCLIMPLODED = 10,
  ZIPMETHOD_MAX,
  ZIPMETHOD_UNKONOWN = -1
};

typedef struct {
  uint8_t *top;
  int size;
  uint8_t *cur;

  struct ZIP_LOCAL_HEADER {
    uint32_t version;
    uint32_t flag;
    uint32_t method;
    uint32_t time;
    uint32_t crc32;
    uint32_t size_def;
    uint32_t size_inf;
    uint32_t size_fn;
    uint32_t size_ext;
    uint8_t fname[260];
  } zlh;

} ZMFILE;

static uint32_t GetWordLE(uint8_t *p) { return (p[1] << 8) | p[0]; }
static uint32_t GetDwordLE(uint8_t *p) { return (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0]; }

static uint8_t *GetLocalHeader(struct ZIP_LOCAL_HEADER *pzlh, uint8_t *p) {
  /* ID */
  if (strncmp(p, "PK\x03\x04", 4))
    return NULL;

  pzlh->version = GetWordLE(p + 0x04);
  pzlh->flag = GetWordLE(p + 0x06);
  pzlh->method = GetWordLE(p + 0x08);
  pzlh->time = GetDwordLE(p + 0x0a);
  pzlh->crc32 = GetDwordLE(p + 0x0e);
  pzlh->size_def = GetDwordLE(p + 0x12);
  pzlh->size_inf = GetDwordLE(p + 0x16);
  pzlh->size_fn = GetWordLE(p + 0x1a);
  pzlh->size_ext = GetWordLE(p + 0x1c);
  p += 0x1e;
  memcpy(pzlh->fname, p, pzlh->size_fn);
  pzlh->fname[pzlh->size_fn] = '\0';
  return p + pzlh->size_fn + pzlh->size_ext;
}

static ZMFILE *zm_open(char *zip_buf, int size) {
  ZMFILE *zfp;

  zfp = malloc(sizeof(ZMFILE));
  if (zfp == NULL)
    return NULL;

  zfp->top = zip_buf;
  zfp->size = size;
  zfp->cur = zfp->top;
  if (GetLocalHeader(&zfp->zlh, zfp->cur) == NULL) {
    free(zfp);
    return NULL;
  }

  return zfp;
}

static void zm_topfile(ZMFILE *zfp) {
  zfp->cur = zfp->top;
  GetLocalHeader(&zfp->zlh, zfp->cur);
}

static int zm_nextfile(ZMFILE *zfp) {
  zfp->cur += 0x1e + zfp->zlh.size_fn + zfp->zlh.size_ext + zfp->zlh.size_def;
  if (zfp->cur >= zfp->top + zfp->size)
    return 1;
  if (!GetLocalHeader(&zfp->zlh, zfp->cur))
    return 1;
  return 0;
}

uint32_t zm_extract(ZMFILE *zfp, void *pbuf) {
  uint8_t *p = zfp->cur + 0x1e + zfp->zlh.size_fn + zfp->zlh.size_ext;
  z_stream str;
  int ret;

  if (zfp->zlh.method == ZIPMETHOD_STORED) {
    memcpy(pbuf, p, zfp->zlh.size_inf);
    return zfp->zlh.size_inf;
  }
  if (zfp->zlh.method != ZIPMETHOD_DEFLATED)
    return 0;

  str.total_out = 0;
  str.zalloc = (alloc_func)NULL;
  str.zfree = (free_func)NULL;
  str.opaque = (voidpf)0;

  if (inflateInit2(&str, -MAX_WBITS) < 0)
    return 0;
  str.next_in = p;
  str.avail_in = zfp->zlh.size_def;
  str.next_out = pbuf;
  str.avail_out = zfp->zlh.size_inf;
  ret = inflate(&str, Z_SYNC_FLUSH);
  inflateEnd(&str);

  if (ret < 0)
    return 0;
  return zfp->zlh.size_inf;
}

static void zm_close(ZMFILE *zfp) { free(zfp); }

static int ext_chk(const char *file, const char *ext) {
  int i = strlen(file);
  int j = strlen(ext);

  if (j >= i)
    return 0;

  while (0 <= j) {
    if (tolower(file[i]) != tolower(ext[j]))
      return 0;
    i--;
    j--;
  }

  if (file[i] == '.')
    return 1;

  return 0;
}

/* search & extract the target file from zip_buf */
static char *zip_extract(const char *ext, char *zip_buf, int size, int *size_inf) {
  ZMFILE *zfp;
  char *ext_buf = NULL;

  if ((zfp = zm_open(zip_buf, size)) == NULL)
    return NULL;
  while (1) {
    if (ext_chk(zfp->zlh.fname, ext))
      break;
    if (zm_nextfile(zfp)) {
      zm_close(zfp);
      return NULL;
    }
  }

  if ((ext_buf = malloc(zfp->zlh.size_inf)) == NULL) {
    zm_close(zfp);
    return NULL;
  }
  if ((*size_inf = zm_extract(zfp, ext_buf)) == 0) {
    zm_close(zfp);
    free(ext_buf);
    return NULL;
  }

  zm_close(zfp);
  return ext_buf;
}
#endif /* KSS_ZIP_SUPPORT */

KSS *KSS_load_file(char *fn) {
  FILE *fp;
  KSS *kss = NULL;
  char *read_buf = NULL;
  int file_length, length = 0;

#ifdef KSS_ZIP_SUPPORT
  char *inf_buf;
  int size_inf;
#endif

  if ((fp = fopen(fn, "rb")) == NULL) {
    char *p;
    int i = 0;

    if ((p = malloc(strlen(fn) + 6)) == NULL)
      return NULL;
    strcpy(p, fn);
    i = strlen(p);
    while (i > 0 && p[i] != '.')
      i--;
    if (p[i] == '.')
      p[i] = '\0';
    strcat(p, ".KSS");
    fp = fopen(p, "rb");
    free(p);
    if (fp == NULL)
      return NULL;
  }

  fseek(fp, 0L, SEEK_END);
  file_length = ftell(fp);
  fseek(fp, 0L, SEEK_SET);

  if ((read_buf = (char *)malloc(file_length)) == NULL) {
    fclose(fp);
    return NULL;
  }

  length = fread(read_buf, 1, file_length, fp);
  fclose(fp);

#ifdef KSS_ZIP_SUPPORT
  if (length >= 0x1e && strncmp(read_buf, "PK\x03\x04", 4) == 0) {
    if ((inf_buf = zip_extract("KSS", read_buf, length, &size_inf)) == NULL) {
      free(read_buf);
      return NULL;
    }
    free(read_buf);
    read_buf = inf_buf;
    length = size_inf;
  }
#endif /* KSS_ZIP_SUPPORT */

  if ((kss = KSS_bin2kss((uint8_t *)read_buf, length, fn)) == NULL) {
    free(read_buf);
    return NULL;
  }

  free(read_buf);
  return kss;
}
