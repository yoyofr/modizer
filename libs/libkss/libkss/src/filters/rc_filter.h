#ifndef _RC_FILTER_H_
#define _RC_FILTER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _tagRCF {
  uint32_t enable;
  double out;
  double a;
} RCF;

RCF *RCF_new(void);
int32_t RCF_calc(RCF *, int32_t data);
void RCF_delete(RCF *);
void RCF_reset(RCF *, double rate, double R, double C);
void RCF_disable(RCF *);

#ifdef __cplusplus
}
#endif

#endif
