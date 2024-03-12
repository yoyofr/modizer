#ifndef _KSS2VGM_H_
#define _KSS2VGM_H_

#include "kssplay.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagKSS2VGM_DataArray {
  uint8_t *buffer;
  uint32_t allocated;
  uint32_t size;
} KSS2VGM_DataArray;

typedef struct tagKSS2VGM {
  KSSPLAY *kssplay;

  KSS2VGM_DataArray *ini_data;
  KSS2VGM_DataArray *vgm_data;

  uint32_t opll_adr;
  uint32_t psg_adr;
  uint32_t opl_adr;
  uint32_t total_samples;
  uint32_t max_samples;
  uint32_t last_write_clock;
  int completed;

  int scc_initialized;
  int use_sng;
  int use_opll;
  int use_psg;
  int use_scc;
  int use_scc_plus;
  int use_opl;
  int use_y8950_adpcm;

  int loop;
  int volume;
} KSS2VGM;

typedef struct tagKSS2VGM_Result {
  uint8_t *vgm;
  uint32_t vgm_size;
} KSS2VGM_Result;

KSS2VGM *KSS2VGM_new(void);
KSS2VGM_Result *KSS2VGM_kss2vgm(KSS2VGM *_this, KSS *kss, int duration, int song, int loop, int volume);

void KSS2VGM_setup(KSS2VGM *_this, KSS *kss, int duration, int song, int loop, int volume);
int KSS2VGM_process(KSS2VGM *_this);
KSS2VGM_Result *KSS2VGM_get_result(KSS2VGM *_this);

void KSS2VGM_delete(KSS2VGM *_this);
void KSS2VGM_Result_delete(KSS2VGM_Result *obj);
uint8_t *KSS2VGM_Result_vgm_ptr(KSS2VGM_Result *obj);
uint32_t KSS2VGM_Result_vgm_size(KSS2VGM_Result *obj);

#ifdef __cplusplus
}
#endif
#endif