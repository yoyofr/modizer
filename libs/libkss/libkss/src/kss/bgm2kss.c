#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kss.h"

static uint8_t KINROU[8192] = {
#include "drivers/kinrou5.h"
};
static uint32_t kinrou_size = sizeof(KINROU);

static uint8_t kinrou_init[0x100] = {
    0xCD, 0x20, 0x60,       /* CALL	6020H */
    0x3E, 0x3F,             /* LD	A,3FH */
    0x32, 0x10, 0x60,       /* LD	(6010H),A	; OPLL SLOT */
    0x32, 0x11, 0x60,       /* LD	(6011H),A	; SCC SLOT */
    0x21, 0x07, 0x80,       /* LD HL,A007H ; DATA ADDRESS */
    0xED, 0x5B, 0x01, 0x80, /* LD	DE,(A001H) ; COMPILE ADDRESS */
    /* INIT FLAG WORK */
    0x3E, 0x00,             /* LD	A,0 ; LOOP NUMBER */
    0xCD, 0x26, 0x60,       /* CALL	6026H */
    0xAF, 0xCD, 0x38, 0x60, /* CALL	6038H	*/

    0x3E, 0x7F, /* LD  A,0FFH */
    0xD3, 0x40, /* OUT (040H),A */
    0xAF,       /* XOR A */
    0xD3, 0x41, /* OUT (041H),A ; PLAY FLAG  */
    0xD3, 0x42, /* OUT (042H),A ; LOOP COUNT */

    0xC9 /* RET */
};

static uint8_t kinrou_play[0x100] = {
    0xCD, 0x29, 0x60, /* CALL PLAY */
    0xCD, 0x32, 0x60, /* CALL PLAYCHK */
    0xE6, 0x01,       /* AND 01H */
    0xAF,             /* XOR A */
    0xD3, 0x41,       /* OUT (041H),A ; STOP FLAG */
    0x2B,             /* DEC HL */
    0x7D,             /* LD A,L */
    0xD3, 0x42,       /* OUT (042H),A ; LOOP COUNT */
    0xC9              /* RET */
};

int KSS_isBGMdata(uint8_t *data, uint32_t size) {
  uint32_t top, end;

  if (data[0] == 0xfe && size >= 8) {
    if ((size >= 0x60) && !strncmp((const char *)(data + 0x50), "BTO", 3))
      return 1;
    top = (data[2] << 8) + data[1];
    end = (data[4] << 8) + data[3];
    if (size >= (8 + end - top) && data[7] < 2)
      return 1;
    if (top == 0xA5B7 && data[7] < 2)
      return 1;
  }

  return 0;
}

int KSS_set_kinrou(const uint8_t *data, uint32_t size) {
  if (size > 8192)
    return 1;
  memcpy(KINROU, data, size);
  return 0;
}

int KSS_load_kinrou(const char *kinrou) {
  FILE *fp;

  if ((fp = fopen(kinrou, "rb")) == NULL)
    return 1;

  fseek(fp, 0, SEEK_END);
  kinrou_size = ftell(fp);

  if (kinrou_size > 8192) {
    fclose(fp);
    return 1;
  }

  fseek(fp, 0, SEEK_SET);
  fread(KINROU, 1, kinrou_size, fp);

  fclose(fp);

  return 0;
}

void KSS_get_info_bgmdata(KSS *kss, uint8_t *data, uint32_t size) {
  static char extra[256];
  uint32_t offset, i;

  if (size > 0x60 && !strncmp((const char *)(data + 0x50), "BTO", 3)) {
    strcpy((char *)kss->idstr, "BTO");
    offset = data[0x5B] + (data[0x5C] << 8) - (data[0x59] + (data[0x5a] << 8)) + 7;
    for (i = 0; i < 255; i++) {
      if ((offset + i) >= size)
        break;
      if (data[offset + i] < 0x20)
        break;
      kss->title[i] = data[offset + i];
    }
    kss->title[i] = '\0';

    offset = data[0x5D] + (data[0x5E] << 8) - (data[0x59] + (data[0x5a] << 8)) + 7;
    for (i = 0; i < 255; i++) {
      if ((offset + i) >= size)
        break;
      if (data[offset + i] < 0x20)
        break;
      extra[i] = data[offset + i];
    }
    extra[i] = '\0';
    kss->extra = malloc(strlen(extra) + 1);
    if (kss->extra != NULL)
      strcpy((char *)kss->extra, extra);
  } else {
    kss->title[0] = '\0';
    kss->extra = NULL;
  }

  kss->loop_detectable = 1;
  kss->stop_detectable = 1;
}

KSS *KSS_bgm2kss(uint8_t *data, uint32_t size) {
  KSS *kss;
  uint8_t *buf;
  uint16_t load_adr = 0x6000, load_size = 0x8000 + size - load_adr;
  uint16_t init_adr = 0x7E00, play_adr = 0x7F00;

  if ((size < 16) || (kinrou_size == 0))
    return NULL;

  if ((buf = malloc(load_size + KSS_HEADER_SIZE)) == NULL)
    return NULL;

  KSS_make_header(buf, load_adr, load_size, init_adr, play_adr);
  buf[0x0F] = 0x01;
  memcpy(buf + KSS_HEADER_SIZE, KINROU + 0x07, kinrou_size - 7);
  memcpy(buf + KSS_HEADER_SIZE + 0x1E00, kinrou_init, 0x100);
  memcpy(buf + KSS_HEADER_SIZE + 0x1F00, kinrou_play, 0x100);
  memcpy(buf + KSS_HEADER_SIZE + 0x8000 - load_adr, data, size);

  kss = KSS_new(buf, (load_size + KSS_HEADER_SIZE));

  free(buf);
  return kss;
}
