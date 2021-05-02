#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kss.h"

static uint32_t readDwordLE(uint8_t *data) { return (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0]; }

KSS *KSS_new(uint8_t *data, uint32_t size) {
  KSS *kss;

  if ((kss = malloc(sizeof(KSS))) == 0)
    return NULL;
  memset(kss, 0, sizeof(KSS));
  if ((kss->data = malloc(size)) == 0) {
    free(kss);
    return NULL;
  }

  memcpy(kss->data, data, size);
  kss->size = size;
  kss->type = 0;
  kss->title[0] = '\0';
  kss->idstr[0] = '\0';
  kss->extra = NULL;
  kss->loop_detectable = 0;
  kss->stop_detectable = 0;

  return kss;
}

const char *KSS_get_title(KSS *kss) { return (char *)kss->title; }

void KSS_delete(KSS *kss) {
  if (kss) {
    free(kss->data);
    free(kss->extra);
    free(kss->info);
    free(kss);
  }
}

int KSS_check_type(uint8_t *data, uint32_t size, const char *filename) {
  char *p;

  if (size < 0x4)
    return KSSDATA;

  if (KSS_isMGSdata(data, size))
    return MGSDATA;
  else if (KSS_isMPK106data(data, size))
    return MPK106DATA;
  else if (KSS_isMPK103data(data, size))
    return MPK103DATA;
  else if (!strncmp("KSCC", (const char *)data, 4))
    return KSSDATA;
  else if (!strncmp("KSSX", (const char *)data, 4))
    return KSSDATA;
  else if (KSS_isOPXdata(data, size))
    return OPXDATA;
  else if (KSS_isBGMdata(data, size))
    return BGMDATA;
  else if (filename) {
    p = strrchr(filename, '.');
    if (p && (strcmp(p, ".MBM") == 0 || strcmp(p, ".mbm") == 0))
      return MBMDATA;
  }

  return KSS_TYPE_UNKNOWN;
}

void KSS_make_header(uint8_t *header, uint16_t load_adr, uint16_t load_size, uint16_t init_adr, uint16_t play_adr) {
  header[0x00] = 'K';
  header[0x01] = 'S';
  header[0x02] = 'S';
  header[0x03] = 'X';
  header[0x04] = (uint8_t)(load_adr & 0xff);
  header[0x05] = (uint8_t)(load_adr >> 8);
  header[0x06] = (uint8_t)(load_size & 0xff);
  header[0x07] = (uint8_t)(load_size >> 8);
  header[0x08] = (uint8_t)(init_adr & 0xff);
  header[0x09] = (uint8_t)(init_adr >> 8);
  header[0x0A] = (uint8_t)(play_adr & 0xff);
  header[0x0B] = (uint8_t)(play_adr >> 8);

  header[0x0C] = 0x00;
  header[0x0D] = 0x00;
  header[0x0E] = 0x10;
  header[0x0F] = 0x05;

  header[0x1C] = 0x00;
  header[0x1D] = 0x00;
  header[0x1E] = 0x00;
  header[0x1F] = 0x00;
}

static void get_legacy_header(KSS *kss) {
  kss->load_adr = (kss->data[0x5] << 8) + kss->data[0x4];
  kss->load_len = (kss->data[0x7] << 8) + kss->data[0x6];
  kss->init_adr = (kss->data[0x9] << 8) + kss->data[0x8];
  kss->play_adr = (kss->data[0xB] << 8) + kss->data[0xA];
  kss->bank_offset = kss->data[0xC];
  kss->bank_num = kss->data[0xD] & 0x7F;
  kss->bank_mode = (kss->data[0xD] >> 7) ? KSS_8K : KSS_16K;
  kss->extra_size = kss->data[0xE];
  kss->device_flag = kss->data[0x0F];
}

static void check_device(KSS *kss, uint32_t flag) {
  kss->sn76489 = flag & 2;
  if (flag & 2) {
    kss->fmunit = kss->fmpac = flag & 1;
    kss->stereo = (flag & 4) >> 2;
    kss->ram_mode = (flag & 8) >> 3;
    kss->pal_mode = (flag & 64) >> 6;
    kss->mode = KSS_SEGA;
  } else {
    if ((flag & 0x18) == 0x10)
      kss->DA8_enable = 1;
    else
      kss->DA8_enable = 0;

    kss->fmpac = kss->fmunit = flag & 1;
    kss->ram_mode = (flag & 4) >> 2;
    kss->msx_audio = (flag & 8) >> 3;
    if (kss->msx_audio)
      kss->stereo = (flag & 16) >> 4;
    else
      kss->stereo = 0;
    kss->pal_mode = (flag & 64) >> 6;
    kss->mode = KSS_MSX;
  }
}

static uint8_t guarded_read(uint8_t *data, uint32_t pos, uint32_t size) {
  if (pos < size)
    return data[pos];
  else
    return 0;
}

static void scan_info(KSS *kss) {
  uint32_t i, j, k;

  if (strncmp("KSCC", (const char *)(kss->data), 4) == 0) {
    kss->kssx = 0;
    get_legacy_header(kss);
    check_device(kss, kss->device_flag);
    kss->trk_min = 0;
    kss->trk_max = 255;
    for (i = 0; i < EDSC_MAX; i++)
      kss->vol[i] = 0x80;
  } else if (strncmp("KSSX", (const char *)(kss->data), 4) == 0) {
    kss->kssx = 1;
    get_legacy_header(kss);
    check_device(kss, kss->device_flag);
    if (kss->data[0x0E] < 0x10) {
      kss->trk_min = 0;
      kss->trk_max = 255;
      for (i = 0; i < EDSC_MAX; i++)
        kss->vol[i] = 0x80;
    } else {
      kss->trk_min = (kss->data[0x19] << 8) + kss->data[0x18];
      kss->trk_max = (kss->data[0x1B] << 8) + kss->data[0x1A];
      for (i = 0; i < EDSC_MAX; i++)
        kss->vol[i] = kss->data[0x1C + i];

      i = (kss->data[0x13] << 24) + (kss->data[0x12] << 16) + (kss->data[0x11] << 8) + kss->data[0x10] + 0x10 +
          kss->extra_size;
      if (i == 0x10 + kss->extra_size)
        return;

      if ((i + 0x10 + 9) <= kss->size && memcmp(kss->data + i, "INFO", 4) == 0) {

        kss->info_num = (kss->data[i + 8]) + (kss->data[i + 9] << 8);

        /*  DEBUG_OUT("INFONUM:%d\n",kss->info_num);*/

        if (kss->info_num > 0) {

          i += 0x10;
          kss->info = malloc(sizeof(KSSINFO) * kss->info_num);
          memset(kss->info, 0, sizeof(KSSINFO) * kss->info_num);

          for (j = 0; j < kss->info_num && i + 4 <= kss->size; j++) {
            kss->info[j].song = kss->data[i++];
            kss->info[j].type = kss->data[i++];
            kss->info[j].time_in_ms = (int)readDwordLE(kss->data + i);
            i += 4;
            kss->info[j].fade_in_ms = (int)readDwordLE(kss->data + i);
            i += 4;

            for (k = 0; k < KSS_TITLE_MAX - 1; k++) {
              kss->info[j].title[k] = guarded_read(kss->data, i++, kss->size);
              if (kss->info[j].title[k] == '\0')
                break;
            }
            if (KSS_TITLE_MAX <= k)
              break;
          }
        }
      }
    }
  }
}

static int is_sjis_prefix(int c) {
  if ((0x81 <= c && c <= 0x9F) || (0xE0 <= c && c <= 0xFC))
    return 1;
  else
    return 0;
}

static void msx_kanji_fix(unsigned char *title) {
  unsigned char *p = title;

  while (p[0]) {
    if (p[0] == 0x81 && 0xAF <= p[1] && p[1] <= 0xB8) {
      p[0] = 0x87;
      p[1] = p[1] - 0xAF + 0x54;
      p += 2;
    } else if (is_sjis_prefix(p[0]))
      p += 2;
    else
      p += 1;
  }

  return;
}

KSS *KSS_bin2kss(uint8_t *data, uint32_t data_size, const char *filename) {
  KSS *kss;
  int type;

  if (data == NULL)
    return NULL;

  type = KSS_check_type(data, data_size, filename);

  switch (type) {
  case MBMDATA:
    kss = KSS_mbm2kss(data, data_size);
    if (kss)
      KSS_get_info_mbmdata(kss, data, data_size);
    break;

  case MGSDATA:
    kss = KSS_mgs2kss(data, data_size);
    if (kss)
      KSS_get_info_mgsdata(kss, data, data_size);
    break;

  case BGMDATA:
    kss = KSS_bgm2kss(data, data_size);
    if (kss)
      KSS_get_info_bgmdata(kss, data, data_size);
    break;

  case MPK103DATA:
    kss = KSS_mpk1032kss(data, data_size);
    if (kss)
      KSS_get_info_mpkdata(kss, data, data_size);
    break;

  case MPK106DATA:
    kss = KSS_mpk1062kss(data, data_size);
    if (kss)
      KSS_get_info_mpkdata(kss, data, data_size);
    break;

  case OPXDATA:
    kss = KSS_opx2kss(data, data_size);
    if (kss)
      KSS_get_info_opxdata(kss, data, data_size);
    break;

  case KSSDATA:
    kss = KSS_kss2kss(data, data_size);
    if (kss)
      KSS_get_info_kssdata(kss, data, data_size);
    break;

  default:
    return NULL;
  }

  if (kss == NULL)
    return NULL;
  kss->type = type;
  msx_kanji_fix(kss->title);
  scan_info(kss);
  return kss;
}
