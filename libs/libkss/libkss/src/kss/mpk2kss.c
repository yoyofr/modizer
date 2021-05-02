#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kss.h"

static uint8_t MPKDRV106[8192] = {
#include "drivers/mpk106.h"
};
static uint32_t mpkdrv106_size = sizeof(MPKDRV106);

static uint8_t MPKDRV103[8192] = {
#include "drivers/mpk103.h"
};
static uint32_t mpkdrv103_size = sizeof(MPKDRV103);

static uint8_t mpk106_init[0x100] = {
    0x0E, 0x00,       /* LD C,00H */
    0xCD, 0x10, 0x40, /* CALL 04010H */
    0x3E, 0x01,       /* LD A,01H */
    0x32, 0x49, 0x5D, /* LD (05D49H),A ; FM  SLOT*/
    0x32, 0x4A, 0x5D, /* LD (05D4AH),A ; SCC SLOT */
    0x21, 0x00, 0xA0, /* LD HL,A000H */
    0x0E, 0x01,       /* LD C,01H */
    0xAF,             /* XOR A */
    0xCD, 0x10, 0x40, /* CALL 04010H */
    0x0E, 0x0C,       /* LD C,12 */
    0xCD, 0x10, 0x40, /* CALL 04010H */

    0x3E, 0x7F, /* LD  A,0FFH */
    0xD3, 0x40, /* OUT (040H),A */
    0xAF,       /* XOR A */
    0xD3, 0x41, /* OUT (041H),A ; PLAY FLAG  */
    0xD3, 0x42, /* OUT (042H),A ; LOOP COUNT */

    0xC9, /* C9 */
};

static uint8_t mpk106_play[0x100] = {
    0xCD, 0x13, 0x40, /* CALL 04013H */
    0x0E, 0x0E,       /* LD C,0EH */
    0xAF,             /* XOR A */
    0xCD, 0x10, 0x40, /* CALL 04010H */
    0xD3, 0x42,       /* OUT (042H),A */
    0xC9,             /* RET */
};

/*
 * MPK103.BIN --> need to patch: 04364H :: 0x00,0x3E,0x01
 */
static uint8_t mpk103_init[0x100] = {
    0x0E, 0x00,       /* LD C,00H */
    0xCD, 0x10, 0x40, /* CALL 04010H */
    0x21, 0x00, 0xA0, /* LD HL,A000H */
    0x0E, 0x01,       /* LD C,01H */
    0xAF,             /* XOR A */
    0xCD, 0x10, 0x40, /* CALL 04010H */
    0x0E, 0x0C,       /* LD C,012 */
    0xCD, 0x10, 0x40, /* CALL 04010H */

    0x3E, 0x7F, /* LD  A,0FFH */
    0xD3, 0x40, /* OUT (040H),A */
    0xAF,       /* XOR A */
    0xD3, 0x41, /* OUT (041H),A ; PLAY FLAG  */
    0xD3, 0x42, /* OUT (042H),A ; LOOP COUNT */

    0xC9, /* C9 */
};

static uint8_t mpk103_play[0x100] = {
    0xCD, 0x13, 0x40, /* CALL 04013H */
    0x0E, 0x0E,       /* LD C,0EH */
    0xAF,             /* XOR A */
    0xCD, 0x10, 0x40, /* CALL 04010H */
    0xD3, 0x42,       /* OUT (042H),A */
    0xC9,             /* RET */
};

int KSS_isMPK103data(uint8_t *data, uint32_t size) {
  char ver[4] = {0, 0, 0, 0};

  if ((size >= 6) && !strncmp((const char *)data, "MPK", 3)) {
    ver[0] = data[3];
    ver[1] = data[4];
    ver[2] = data[5];
    if (atoi(ver) <= 103)
      return 1;
    else
      return 0;
  } else
    return 0;
}

int KSS_isMPK106data(uint8_t *data, uint32_t size) {
  char ver[4] = {0, 0, 0, 0};

  if ((size >= 6) && !strncmp((const char *)data, "MPK", 3)) {
    ver[0] = data[3];
    ver[1] = data[4];
    ver[2] = data[5];
    if (103 < atoi(ver))
      return 1;
    else
      return 0;
  } else
    return 0;
}

int KSS_set_mpk106(const uint8_t *data, uint32_t size) {
  if (size > 8192)
    return 1;
  memcpy(MPKDRV106, data, size);
  return 0;
}

int KSS_load_mpk106(const char *mpkdrv) {
  FILE *fp;

  if ((fp = fopen(mpkdrv, "rb")) == NULL)
    return 1;

  fseek(fp, 0, SEEK_END);
  mpkdrv106_size = ftell(fp);

  if (mpkdrv106_size > 8192) {
    fclose(fp);
    return 1;
  }

  fseek(fp, 0, SEEK_SET);
  fread(MPKDRV106, 1, mpkdrv106_size, fp);

  fclose(fp);

  return 0;
}

int KSS_set_mpk103(const uint8_t *data, uint32_t size) {
  if (size > 8192)
    return 1;
  memcpy(MPKDRV103, data, size);
  return 0;
}

int KSS_load_mpk103(const char *mpkdrv) {
  FILE *fp;

  if ((fp = fopen(mpkdrv, "rb")) == NULL)
    return 1;

  fseek(fp, 0, SEEK_END);
  mpkdrv103_size = ftell(fp);

  if (mpkdrv103_size > 8192) {
    fclose(fp);
    return 1;
  }

  fseek(fp, 0, SEEK_SET);
  fread(MPKDRV103, 1, mpkdrv103_size, fp);

  fclose(fp);

  return 0;
}

void KSS_get_info_mpkdata(KSS *kss, uint8_t *data, uint32_t size) {
  uint32_t i, j, offset;
  static char extra[3][256] = {"", "", ""};

  for (i = 0; i < 6; i++)
    kss->idstr[i] = data[i];
  kss->idstr[i] = '\0';

  /* GET TITILE STRING */
  offset = 8;
  i = 0;
  while ((data[offset + i] != 0x0D) && (i < KSS_TITLE_MAX - 1) && ((offset + i) < size) && i < 255) {
    kss->title[i] = data[offset + i];
    i++;
  }
  kss->title[i] = '\0';
  i++;
  if (data[offset + i++] != 0x0A)
    return;

  /* GET COMPOSER, ARRANGER, MEMO (Max 256 bytes each) */
  for (j = 0; j < 3; j++) {
    offset += i;
    i = 0;
    while ((data[offset + i] != 0x0D) && (i < KSS_TITLE_MAX - 1) && ((offset + i) < size) && i < 256) {
      extra[j][i] = data[offset + i];
      i++;
    }
    extra[j][i] = '\0';
    i++;
    if (data[offset + i++] != 0x0A)
      break;
  }

  kss->extra = malloc(strlen(extra[0]) + strlen(extra[1]) + strlen(extra[2]) + 256);
  if (kss->extra == NULL)
    return;
  sprintf((char *)kss->extra, "Composer: %s\r\nArranger: %s\r\n%s", extra[0], extra[1], extra[2]);

  kss->loop_detectable = 1;
  kss->stop_detectable = 1;
}

KSS *KSS_mpk1062kss(uint8_t *data, uint32_t size) {
  KSS *kss;

  uint8_t *buf;
  uint16_t data_adr, load_adr, load_size;
  uint16_t drv_adr = 0x4000, init_adr = 0x6000, play_adr = 0x6100;

  if (size < 16 || mpkdrv106_size == 0)
    return NULL;

  if (size < (0x4000 - 0x200)) {
    data_adr = 0x200;
    load_adr = 0x200;
    load_size = play_adr + 0x100 - load_adr;
  } else {
    data_adr = 0x6200;
    load_adr = 0x4000;
    load_size = data_adr + size - load_adr;
    init_adr = 0x6000;
    play_adr = 0x6100;
  }

  if ((buf = malloc(load_size + KSS_HEADER_SIZE)) == NULL)
    return NULL;

  KSS_make_header(buf, load_adr, load_size, init_adr, play_adr);

  memcpy(buf + KSS_HEADER_SIZE + drv_adr - load_adr, MPKDRV106, 0x2000);
  memcpy(buf + KSS_HEADER_SIZE + init_adr - load_adr, mpk106_init, 0x100);
  memcpy(buf + KSS_HEADER_SIZE + play_adr - load_adr, mpk106_play, 0x100);
  memcpy(buf + KSS_HEADER_SIZE + data_adr - load_adr, data, size);
  buf[KSS_HEADER_SIZE + init_adr + (0xE) - load_adr] = data_adr & 0xFF;
  buf[KSS_HEADER_SIZE + init_adr + 0xF - load_adr] = data_adr >> 8;
  kss = KSS_new(buf, (load_size + KSS_HEADER_SIZE));

  free(buf);
  return kss;
}

KSS *KSS_mpk1032kss(uint8_t *data, uint32_t size) {
  KSS *kss;

  uint8_t *buf;
  uint16_t data_adr, load_adr, load_size;
  uint16_t drv_adr = 0x4000, init_adr = 0x6000, play_adr = 0x6100;

  if (size < 16 || mpkdrv103_size == 0)
    return NULL;

  if (size < (0x4000 - 0x200)) {
    data_adr = 0x200;
    load_adr = 0x200;
    load_size = play_adr + 0x100 - load_adr;
  } else {
    data_adr = 0x6200;
    load_adr = 0x4000;
    load_size = data_adr + size - load_adr;
    init_adr = 0x6000;
    play_adr = 0x6100;
  }

  if ((buf = malloc(load_size + KSS_HEADER_SIZE)) == NULL)
    return NULL;

  KSS_make_header(buf, load_adr, load_size, init_adr, play_adr);
  buf[0x0F] = 0x05;

  memcpy(buf + KSS_HEADER_SIZE + drv_adr - load_adr, MPKDRV103, 0x2000);
  memcpy(buf + KSS_HEADER_SIZE + 0x4364 - load_adr, "\x00\x3E\x01", 3);
  /* patch for driver */
  memcpy(buf + KSS_HEADER_SIZE + init_adr - load_adr, mpk103_init, 0x100);
  memcpy(buf + KSS_HEADER_SIZE + play_adr - load_adr, mpk103_play, 0x100);
  memcpy(buf + KSS_HEADER_SIZE + data_adr - load_adr, data, size);
  buf[KSS_HEADER_SIZE + init_adr + 0x6 - load_adr] = data_adr & 0xFF;
  buf[KSS_HEADER_SIZE + init_adr + 0x7 - load_adr] = data_adr >> 8;

  kss = KSS_new(buf, (load_size + KSS_HEADER_SIZE));

  free(buf);
  return kss;
}
