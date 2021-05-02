#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kss.h"

static void get_information(KSS *kss, uint8_t *data, uint32_t size) {
  int i;

  if (!kss)
    return;

  kss->type = KSS_check_type(data, size, NULL);

  switch (kss->type) {
  case MGSDATA:
    KSS_get_info_mgsdata(kss, data, size);
    break;

  case MPK103DATA:
  case MPK106DATA:
    KSS_get_info_mpkdata(kss, data, size);
    break;

  case BGMDATA:
    KSS_get_info_bgmdata(kss, data, size);
    break;

  case OPXDATA:
    KSS_get_info_opxdata(kss, data, size);
    break;

  default:
    if (size > 0x100 && strncmp((const char *)data, "MBM", 3) == 0) /* for MBM2KSS */
    {
      for (i = 0; i < 0x28; i++)
        kss->title[i] = data[0x10 + i];
      kss->title[i] = '\0';
    } else
      kss->title[0] = '\0';
    break;
  }

  return;
}

void KSS_get_info_kssdata(KSS *kss, uint8_t *data, uint32_t size) {
  int i;
  uint16_t header_size;

  if (size < 16)
    return;

  header_size = 16 + (unsigned char)data[0x0e];
  get_information(kss, data + header_size, size - header_size);

  if ((kss->title[0] == '\0') && (size > 0x2009))
    get_information(kss, data + 0x2009, size - 0x2009); /* SEARCH KINROU5 TITLE (for MPK2KSS) */

  if (kss->title[0] == '\0')
    get_information(kss, data, size);

  for (i = 0; i < 4; i++)
    kss->idstr[i] = data[i];
  kss->idstr[i] = '\0';

  kss->loop_detectable = 0;
  kss->stop_detectable = 0;
  kss->type = KSSDATA;
}

KSS *KSS_kss2kss(uint8_t *data, uint32_t size) {
  KSS *kss;

  if (size < 16)
    return NULL;
  kss = KSS_new(data, size);

  return kss;
}
