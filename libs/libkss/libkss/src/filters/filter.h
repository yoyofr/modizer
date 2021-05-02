#ifndef _FIR_FILTER_H_
#define _FIR_FILTER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _tagFIR {
  double h[65];
  int32_t buf[65];
  double Wc;
  uint32_t M;
} FIR;

FIR *FIR_new(void);
int32_t FIR_calc(FIR *, int32_t data);
void FIR_delete(FIR *);
void FIR_reset(FIR *, uint32_t sam, uint32_t cut, uint32_t n);
void FIR_disable(FIR *);

#ifdef __cplusplus
}
#endif

#endif