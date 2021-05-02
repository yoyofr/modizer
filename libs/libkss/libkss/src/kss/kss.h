#ifndef _KSSOBJ_H_
#define _KSSOBJ_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum EmuDeviceSerialCode {
  EDSC_PSG = 0,
  EDSC_SCC = 1,
  EDSC_OPLL = 2,
  EDSC_OPL = 3,
  EDSC_MAX = 4,
};

enum { KSS_16K = 0, KSS_8K = 1 };

enum { KSS_MSX = 0, KSS_SEGA = 1 };

#define KSS_TITLE_MAX 256
#define KSS_HEADER_SIZE 0x20

typedef struct tagKSSINFO {
  int song;
  int type;
  char title[KSS_TITLE_MAX];
  int time_in_ms;
  int fade_in_ms;
} KSSINFO;

typedef struct tagKSS {
  uint32_t type;
  uint32_t stop_detectable;
  uint32_t loop_detectable;
  uint8_t title[KSS_TITLE_MAX]; /* the title */
  uint8_t idstr[8];             /* ID */
  uint8_t *data;                /* the KSS binary */
  uint32_t size;                /* the size of KSS binary */
  uint8_t *extra;               /* Infomation text for KSS info dialog */

  int kssx; /* 0:KSCC, 1:KSSX */
  int mode; /* 0:MSX 1:SMS 2:GG */

  int fmpac;
  int fmunit;
  int sn76489;
  int ram_mode;
  int msx_audio;
  int stereo;
  int pal_mode;

  int DA8_enable;

  uint16_t load_adr;
  uint16_t load_len;
  uint16_t init_adr;
  uint16_t play_adr;

  uint8_t bank_offset;
  uint8_t bank_num;
  uint8_t bank_mode;
  uint8_t extra_size;
  uint8_t device_flag;
  uint8_t trk_min;
  uint8_t trk_max;
  uint8_t vol[EDSC_MAX];

  uint16_t info_num;
  KSSINFO *info;

} KSS;

KSS *KSS_new(uint8_t *data, uint32_t size);
void KSS_delete(KSS *kss);

const char *KSS_get_title(KSS *kss);

enum { KSSDATA, MGSDATA, MBMDATA, MPK106DATA, MPK103DATA, BGMDATA, OPXDATA, FMDATA, KSS_TYPE_UNKNOWN };

int KSS_check_type(uint8_t *data, uint32_t size, const char *filename);

int KSS_load_mgsdrv(const char *filename);
int KSS_set_mgsdrv(const uint8_t *data, uint32_t size);
int KSS_load_kinrou(const char *filename);
int KSS_set_kinrou(const uint8_t *data, uint32_t size);
int KSS_load_mpk106(const char *filename);
int KSS_set_mpk106(const uint8_t *data, uint32_t size);
int KSS_load_mpk103(const char *filename);
int KSS_set_mpk103(const uint8_t *data, uint32_t size);
int KSS_load_opxdrv(const char *filename);
int KSS_set_opxdrv(const uint8_t *data, uint32_t size);
int KSS_load_fmbios(const char *filename);
int KSS_set_fmbios(const uint8_t *data, uint32_t size);
int KSS_load_mbmdrv(const char *filename);
int KSS_set_mbmdrv(const uint8_t *data, uint32_t size);

void KSS_get_info_mgsdata(KSS *, uint8_t *data, uint32_t size);
void KSS_get_info_bgmdata(KSS *, uint8_t *data, uint32_t size);
void KSS_get_info_mpkdata(KSS *, uint8_t *data, uint32_t size);
void KSS_get_info_opxdata(KSS *, uint8_t *data, uint32_t size);
void KSS_get_info_kssdata(KSS *, uint8_t *data, uint32_t size);
void KSS_get_info_mbmdata(KSS *, uint8_t *data, uint32_t size);

void KSS_make_header(uint8_t *header, uint16_t load_adr, uint16_t load_size, uint16_t init_adr, uint16_t play_adr);

KSS *KSS_mgs2kss(uint8_t *data, uint32_t size);
KSS *KSS_bgm2kss(uint8_t *data, uint32_t size);
KSS *KSS_opx2kss(uint8_t *data, uint32_t size);
KSS *KSS_mpk1032kss(uint8_t *data, uint32_t size);
KSS *KSS_mpk1062kss(uint8_t *data, uint32_t size);
KSS *KSS_kss2kss(uint8_t *data, uint32_t size);
KSS *KSS_mbm2kss(const uint8_t *data, uint32_t size);
void KSS_set_mbmparam(int m, int, int s);
KSS *KSS_bin2kss(uint8_t *data, uint32_t size, const char *filename);
int KSS_load_mbk(const char *filename);
int KSS_autoload_mbk(const char *mbmfile, const char *extra_path, const char *dummy_file);

int KSS_isMGSdata(uint8_t *data, uint32_t size);
int KSS_isBGMdata(uint8_t *data, uint32_t size);
int KSS_isMPK106data(uint8_t *data, uint32_t size);
int KSS_isMPK103data(uint8_t *data, uint32_t size);
int KSS_isOPXdata(uint8_t *data, uint32_t size);

KSS *KSS_load_file(char *fn);

void KSS_info2pls(KSS *kss, char *base, int index, char *buf, int buflen, int playtime, int fadetime);

#ifdef __cplusplus
}
#endif

#endif
