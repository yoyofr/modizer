/* MGS2KSS CONVERTER */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kss.h"

static uint8_t MGSDRV[0x2000] = {
#include "drivers/mgsdrv.h"
};
static uint32_t mgsdrv_size = sizeof(MGSDRV);

static uint8_t mgsdrv_init[0x100] = {
    /* FORCE FMPAC RYTHM MUTE */
    0x3E, 0x0E, /* LD A,0EH */
    0xD3, 0x7C, /* OUT(C),A */
    0x3E, 0x20, /* LD A,020H */
    0xD3, 0x7D, /* OUT(C),A */

    /* INIT */
    0xCD, 0x10, 0x60, /* CALL 06010H */
    0x3E, 0x01,       /* LD A,1 */
    0xDD, 0x77, 0x00, /* LD (IX+0), A */
    0xDD, 0x77, 0x01, /* LD (IX+1), A */

    /* SAVE MIB POINTER TO WORK AREA */
    0xDD, 0x22, 0xF0, 0x7F, /* LD (07FF0H),IX */

    /* FORCE ACTIVATE SCC(MEGAROM MODE) */
    0xAF,             /* XOR A */
    0x32, 0xFE, 0xBF, /* LD (0BFFEH), A */
    0x3E, 0x3F,       /* LD A, 03FH */
    0x32, 0x00, 0x90, /* LD (09000H), A */

    /* PLAY */
    0x11, 0x00, 0x02, /* LD DE,0200H */
    0x06, 0xFF,       /* LD B,0FFH */
    0x60,             /* LD H,B */
    0x68,             /* LD L,B */
    0xCD, 0x16, 0x60, /* CALL 06016H */

    /* INIT FLAG */
    0x3E, 0x7F, /* LD  A,07FH */
    0xD3, 0x40, /* OUT (040H),A ; SEL EXT I/O */
    0xAF,       /* XOR A */
    0xD3, 0x41, /* OUT (041H),A ; STOP FLAG  */
    0xD3, 0x42, /* OUT (042H),A ; LOOP COUNT */
    0xD3, 0x43, 0xD3, 0x44,

    0xC9 /* RET */
};

static uint8_t mgsdrv_play[0x100] = {
    0xCD, 0x1F, 0x60,       /* CALL 0601FH */
    0xDD, 0x2A, 0xF0, 0x7F, /* LD IX,(07FF0H) */
    0xDD, 0x7E, 0x08,       /* LD A,(IX+8) ; */
    0xFE, 0x00,             /* CP A,0 */
    0x20, 0x04,             /* JR NZ,+04 */
    0x3E, 0x01, 0xD3, 0x41, /* OUT (041H),A */
    0xDD, 0x7E, 0x05,       /* LD A,(IX+5) ; LOOP COUNTER */
    0xD3, 0x42,             /* OUT (042H),A */
    0xDD, 0x7E, 10,         /* LD A,(IX+10) ; @m ADDRESS(L) */
    0xD3, 0x43,             /* OUT (043H),A */
    0xDD, 0x7E, 11,         /* LD A,(IX+11) ; @m ADDRESS(H) */
    0xD3, 0x44,             /* OUT (044H),A */
    0xAF,                   /* XOR A */
    0xDD, 0x77, 10,         /* LD (IX+10), A */
    0xDD, 0x77, 11,         /* LD (IX+11), A */
    0xC9                    /* RET */
};

int KSS_isMGSdata(uint8_t *data, uint32_t size) {
  if (size > 32 && !strncmp((const char *)data, "MGS", 3))
    return 1;
  else
    return 0;
}

int KSS_set_mgsdrv(const uint8_t *data, uint32_t size) {
  if (size > 8192)
    return 1;
  memcpy(MGSDRV, data, size);
  return 0;
}

int KSS_load_mgsdrv(const char *mgsdrv) {
  FILE *fp;

  if ((fp = fopen(mgsdrv, "rb")) == NULL)
    return 1;

  fseek(fp, 0, SEEK_END);
  mgsdrv_size = ftell(fp);

  if (mgsdrv_size > sizeof(MGSDRV)) {
    fclose(fp);
    return 1;
  }

  fseek(fp, 0, SEEK_SET);
  fread(MGSDRV, 1, mgsdrv_size, fp);

  fclose(fp);

  return 0;
}

void KSS_get_info_mgsdata(KSS *kss, uint8_t *data, uint32_t size) {
  uint32_t i = 0, offset = 0;

  for (i = 0; i < 6; i++)
    kss->idstr[i] = data[i];
  kss->idstr[i] = '\0';

  offset = 8;
  for (i = 0; i < 128; i++) {
    if (data[offset + i] == 0x0D)
      break;
    kss->title[i] = data[offset + i];
    if (kss->title[i] == 0x09)
      kss->title[i] = ' ';
  }
  kss->title[i] = '\0';

  kss->loop_detectable = 1;
  kss->stop_detectable = 1;
}

KSS *KSS_mgs2kss(uint8_t *data, uint32_t size) {
  KSS *kss;
  uint8_t *buf;
  uint16_t load_adr = 0x200, load_size = 0x8000 + 0x200 - 0x200;
  uint16_t init_adr = 0x8000, play_adr = init_adr + 0x100;

  if (mgsdrv_size == 0)
    return NULL;

  if ((buf = malloc(load_size + KSS_HEADER_SIZE)) == NULL)
    return NULL;

  KSS_make_header(buf, load_adr, load_size, init_adr, play_adr);
  memcpy(buf + KSS_HEADER_SIZE, data, size);
  memcpy(buf + KSS_HEADER_SIZE + 0x6000 - load_adr, MGSDRV + 0x0D, 0x2000);
  memcpy(buf + KSS_HEADER_SIZE + init_adr - load_adr, mgsdrv_init, 0x100);
  memcpy(buf + KSS_HEADER_SIZE + init_adr + 0x100 - load_adr, mgsdrv_play, 0x100);

  kss = KSS_new(buf, (load_size + KSS_HEADER_SIZE));

  free(buf);
  return kss;
}
