/**
 * This file is based on mbm2kssx.c and mbm2kssx.h written by Mamiya.
 * The original source can be found at http://nezplug.sourceforge.net/
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <memory.h>
#include "kss.h"

#define KSS_HEADER_SIZE 0x20
#define HEADER_SIZE 0x200
static uint8_t kss_header[KSS_HEADER_SIZE + HEADER_SIZE] = {
    0x4B, 0x53, 0x53, 0x58, 0x00, 0x3E, 0x00, 0x00, 0x40, 0x3E, 0x9F, 0xFD, 0x00, 0x00, 0x10, 0x0D, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4D, 0x42, 0x4D, 0x32, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0xF3, 0x3E, 0xC3, 0x21, 0x1D, 0x3F, 0x32, 0x1C, 0x00, 0x22, 0x1D, 0x00, 0x21, 0x05, 0x3F, 0x32, 0x20, 0x00,
    0x22, 0x21, 0x00, 0x21, 0x1E, 0x3F, 0x32, 0x30, 0x00, 0x22, 0x31, 0x00, 0x21, 0x1D, 0x3F, 0x32, 0x80, 0x01, 0x22,
    0x81, 0x01, 0x21, 0x1C, 0x3F, 0x32, 0x83, 0x01, 0x22, 0x84, 0x01, 0x21, 0x0E, 0x3F, 0x32, 0x7D, 0xF3, 0x22, 0x7E,
    0xF3, 0x3E, 0xC9, 0x32, 0x9F, 0xFD, 0x32, 0xA0, 0xFD, 0x32, 0xA1, 0xFD, 0x32, 0xA2, 0xFD, 0x32, 0xA3, 0xFD, 0x21,
    0x00, 0xDA, 0x11, 0x01, 0xDA, 0x01, 0x1E, 0x00, 0x36, 0x00, 0xED, 0xB0, 0x3A, 0x03, 0x3E, 0xE6, 0x03, 0x32, 0x00,
    0xDA, 0x3E, 0x03, 0x32, 0x02, 0xDA, 0x21, 0x00, 0x80, 0x22, 0x03, 0xDA, 0x2A, 0x00, 0x40, 0x11, 0x00, 0x40, 0x19,
    0x11, 0x00, 0x80, 0x01, 0x38, 0x00, 0x3E, 0x03, 0xD3, 0xFE, 0xED, 0xB0, 0x08, 0x3E, 0x00, 0xB7, 0x08, 0x3E, 0x03,
    0x32, 0x26, 0x3F, 0xCD, 0x08, 0x40, 0xCD, 0xE0, 0x3E, 0x3A, 0x02, 0xDA, 0x32, 0x26, 0x3F, 0xCD, 0x0C, 0x40, 0x3A,
    0x02, 0xDA, 0xD3, 0xFE, 0xCD, 0x04, 0x40, 0xFB, 0xC9, 0x06, 0x04, 0x21, 0x00, 0x80, 0xC5, 0xE5, 0x11, 0x00, 0xC0,
    0x01, 0x00, 0x10, 0x3E, 0x06, 0xD3, 0xFE, 0xED, 0xB0, 0xD1, 0x21, 0x00, 0xC0, 0x01, 0x00, 0x10, 0xAF, 0xD3, 0xFE,
    0xED, 0xB0, 0xD5, 0xE1, 0xC1, 0x10, 0xE1, 0xC9, 0xE5, 0xD5, 0x37, 0x3F, 0xED, 0x52, 0xD1, 0xE1, 0xC9, 0x79, 0xFE,
    0x27, 0x20, 0x09, 0x3A, 0x26, 0x3F, 0xD3, 0xFE, 0x3C, 0x32, 0x26, 0x3F, 0xAF, 0xC9, 0xE3, 0xF5, 0x3E, 0xC3, 0x77,
    0xF1, 0xE3, 0xC9, 0x00,
};

static unsigned char *drv_top = 0;

#if !defined(EXCLUDE_DRIVER_ALL) && !defined(EXCLUDE_DRIVER_MBR143)
static uint8_t drv_code[] = {
#include "drivers/mbr143.h"
};
#else
static uint8_t drv_code[8192];
#endif

static int drv_size = sizeof(drv_code);
static int mbplay = 0;
static int mbkload = 0;
static int mbmload = 0;
static uint8_t mbkdata[0x8038];
static int mbksize = 0;
static int cnv_mode = 2;
static int vsync_ntsc = 1;

static char *memfind(char *buff, int bsize, char *str, int len) {
  char *p;

  if (buff) {
    for (p = buff; p + len < buff + bsize; p++)
      if (memcmp(str, p, len) == 0)
        return p;
  }
  return NULL;
}

static int read_address(uint8_t *buff) { return buff[0] + (buff[1] << 8); }

static int mbmdrv_init() {
  uint8_t *tmp;

  drv_top = (unsigned char *)memfind((char *)drv_code, drv_size, "AB\0", 4);
  if (!drv_top)
    return 1;
  drv_size -= (drv_top - drv_code);

  tmp = (uint8_t *)memfind((char *)drv_top, drv_size, "MBPLAY", 7);
  if (!tmp)
    return 1;
  mbplay = read_address(tmp + 7);

  tmp = (uint8_t *)memfind((char *)drv_top, drv_size, "MBMLOAD", 8);
  if (!tmp)
    return 1;
  mbmload = read_address(tmp + 8);

  tmp = (uint8_t *)memfind((char *)drv_top, drv_size, "MBKLOAD", 8);
  if (!tmp)
    return 1;
  mbkload = read_address(tmp + 8);

  drv_top[0x4000 - 0x4000] = (drv_size >> 0) & 255;
  drv_top[0x4001 - 0x4000] = (drv_size >> 8) & 255;
  drv_top[0x4002 - 0x4000] = '\0';
  drv_top[0x4003 - 0x4000] = '\0';
  drv_top[0x4004 - 0x4000] = '\xc3';
  drv_top[0x4005 - 0x4000] = (mbplay >> 0) & 255;
  drv_top[0x4006 - 0x4000] = (mbplay >> 8) & 255;
  drv_top[0x4007 - 0x4000] = '\0';
  drv_top[0x4008 - 0x4000] = '\xc3';
  drv_top[0x4009 - 0x4000] = (mbkload >> 0) & 255;
  drv_top[0x400a - 0x4000] = (mbkload >> 8) & 255;
  drv_top[0x400b - 0x4000] = '\0';
  drv_top[0x400c - 0x4000] = '\xc3';
  drv_top[0x400d - 0x4000] = (mbmload >> 0) & 255;
  drv_top[(0x400e) - 0x4000] = (mbmload >> 8) & 255;
  drv_top[0x400f - 0x4000] = '\0';

  return 0;
}

int KSS_load_mbmdrv(const char *filename) {
  FILE *fp;
  size_t size;

  if ((fp = fopen(filename, "rb")) == NULL)
    return 1;

  fseek(fp, 0, SEEK_END);
  size = ftell(fp);

  if (size > 0x8000) {
    fclose(fp);
    return 1;
  }

  fseek(fp, 0, SEEK_SET);
  drv_size = fread(drv_code, 1, size, fp);
  fclose(fp);

  return mbmdrv_init();
}

int KSS_set_mbmdrv(const uint8_t *mbmdrv, uint32_t size) {

  if (mbmdrv && 0x100 < drv_size && drv_size < 0x8000) {
    memcpy(drv_code, mbmdrv, size);
    drv_size = size;
  } else {
    drv_size = 0;
    return 1;
  }

  return mbmdrv_init();
}

static char mbkpath[4][512];

/*
 SEARCH ORDER
 1. .\MBMFILE.MBK
 2. .\MBKNAME.MBK (MBKNAME = MBK name specified in the MBM header.)
 3. EXTRA_PATH\MBKNAME.MBK
 4. .\DUMMY.MBK
*/

int KSS_autoload_mbk(const char *mbmfile, const char *extra_path, const char *dummy_file) {
  FILE *fp;
  char buf[0x148];
  char mbkname[13];
  char *p;
  int i;

#ifdef _WIN32
    const char PATH_SEPARATOR = '\\';
    const char *PATH_SEPARATOR_STR = "\\";
#else
    const char PATH_SEPARATOR = '/';
    const char *PATH_SEPARATOR_STR = "/";
#endif

  fp = fopen(mbmfile, "rb");
  if (!fp)
    return 1;
  if (fread(buf, 1, 0x148, fp) != 0x148)
    return 1;
  fclose(fp);

  /* extract mbkname */
  for (i = 0; i < 8 && buf[0x140 + i] != ' '; i++)
    mbkname[i] = buf[0x140 + i];
  mbkname[i] = '\0';
  strcat(mbkname, ".MBK");

  strncpy(mbkpath[0], mbmfile, 499);
  p = strrchr(mbkpath[0], '.');
  if (p)
    *(p + 1) = '\0';
  strcat(mbkpath[0], "MBK");

  strncpy(mbkpath[1], mbmfile, 499);
  mbkpath[1][499] = '\0';
  p = strrchr(mbkpath[1], PATH_SEPARATOR);
  if (p)
    *(p + 1) = '\0';
  strcat(mbkpath[1], mbkname);

  strncpy(mbkpath[2], extra_path, 499);
  mbkpath[2][499] = '\0';
  if (mbkpath[2][strlen(mbkpath[2]) - 1] != PATH_SEPARATOR)
    strcat(mbkpath[2], PATH_SEPARATOR_STR);
  strcat(mbkpath[2], mbkname);

  if (dummy_file && dummy_file[0]) {
    strncpy(mbkpath[3], mbmfile, 499);
    p = strrchr(mbkpath[3], PATH_SEPARATOR);
    if (p)
      *(p + 1) = '\0';
    strcat(mbkpath[3], dummy_file);
  } else
    mbkpath[3][0] = '\0';

  return 0;
}

static int load_mbk() {
  int i;

  for (i = 0; i < 4; i++) {
    if (KSS_load_mbk(mbkpath[i]) == 0)
      return 0;
  }
  return 1;
}

int KSS_load_mbk(const char *mbkpath) {
  FILE *fp;

  mbksize = 0;

  fp = fopen(mbkpath, "rb");
  if (!fp)
    return 1;
  fread(mbkdata, 1, 0x8038, fp);
  fclose(fp);

  mbksize = 0x8038;
  return 0;
}

int KSS_set_mbk(const uint8_t *mbkp) {
  if (mbkp) {
    memcpy(mbkdata, mbkp, 0x8038);
    mbksize = 0x8038;
  } else {
    mbksize = 0;
  }

  return 0;
}

static KSS *mbm2kss(const uint8_t *mbmdata, size_t mbmsize, int devtype, int isntsc) {
  char *kssbuf;
  KSS *kss = 0;
  int tmp = 0;

  drv_top[0x4008 - 0x4000] = mbksize ? '\xc3' : '\xc9';

  tmp = drv_size + HEADER_SIZE + (mbksize ? 0x38 : 0);

  kss_header[0x6] = (tmp >> 0) & 255;
  kss_header[0x7] = (tmp >> 8) & 255;
  kss_header[0xc] = mbksize ? 4 : 6;
  kss_header[0xd] = mbksize ? 3 : 1;

  switch (devtype) {
  default:
  case 0: /* msx-audio */
    kss_header[0xf] = 4 + 8;
    kss_header[KSS_HEADER_SIZE + 0x03] = '0';
    break;
  case 1: /* msx-music */
    kss_header[0xf] = 4 + 1;
    kss_header[KSS_HEADER_SIZE + 0x03] = '1';
    break;
  case 2: /* both */
    kss_header[0xf] = 4 + 8 + 1;
    kss_header[KSS_HEADER_SIZE + 0x03] = '2';
    break;
  case 3: /* stereo */
    kss_header[0xf] = 4 + 8 + 1 + 16;
    kss_header[KSS_HEADER_SIZE + 0x03] = '2';
    break;
  }

  if (!isntsc)
    kss_header[0xf] |= 0x40;
  memset(&kss_header[KSS_HEADER_SIZE + 0x10], 0, 0x30);
  memcpy(&kss_header[KSS_HEADER_SIZE + 0x10], mbmdata + 0xCF, 0x28);

  kssbuf = malloc(KSS_HEADER_SIZE + HEADER_SIZE + drv_size + mbksize + mbmsize);

  if (kssbuf != NULL) {
    tmp = 0;
    memcpy(kssbuf + tmp, kss_header, KSS_HEADER_SIZE + HEADER_SIZE);
    tmp += KSS_HEADER_SIZE + HEADER_SIZE;
    memcpy(kssbuf + tmp, drv_top, drv_size);
    tmp += drv_size;
    memcpy(kssbuf + tmp, mbkdata, mbksize);
    tmp += mbksize;
    memcpy(kssbuf + tmp, mbmdata, mbmsize);
    tmp += mbmsize;
    kss = KSS_new((uint8_t *)kssbuf, tmp);
    free(kssbuf);
  }

  return kss;
}

int KSS_isMBMdata(uint8_t *data, uint32_t size) {
  return 0; // undecidable.
}

KSS *KSS_mbm2kss(const uint8_t *data, uint32_t size) {

  if (0<drv_size && !drv_top) {
    mbmdrv_init();
  }

  if (drv_top) {
    load_mbk();
    return mbm2kss(data, size, cnv_mode, vsync_ntsc);
  } else {
    return NULL;
  }
}

void KSS_set_mbmparam(int mode, int stereo, int ntsc) {
  cnv_mode = mode < 2 ? mode : 2;
  if (cnv_mode == 2 && stereo)
    cnv_mode = 3;
  vsync_ntsc = ntsc;
}

void KSS_get_info_mbmdata(KSS *kss, uint8_t *data, uint32_t size) {
  uint32_t i = 0, offset = 0;

  if (size < 0x200)
    return;

  strcpy((char *)kss->idstr, "MBM");

  for (i = 0; i < 0x28; i++)
    kss->title[i] = data[0x0CF + i];
  kss->title[i] = '\0';

  if (kss->extra == NULL) {
    kss->extra = malloc(64);
    offset = sprintf((char *)kss->extra, "MBKFILE: ");
    for (i = 0; i < 0x8; i++)
      kss->extra[offset + i] = data[0x140 + i];
    kss->extra[offset + i] = '\0';
    if (mbksize == 0)
      strcat((char *)kss->extra, "\r\n(MBK is not Found)");
  }

  kss->loop_detectable = 0;
  kss->stop_detectable = 0;
}
