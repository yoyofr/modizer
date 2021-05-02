#ifndef _DC_FILTER_H_
#define _DC_FILTER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _tagDCF {
  uint32_t enable;
  double weight;
  double in, out;
} DCF;

DCF *DCF_new(void);
int32_t DCF_calc(DCF *, int32_t data);
void DCF_delete(DCF *);
void DCF_reset(DCF *, double rate);
void DCF_disable(DCF *);

#ifdef __cplusplus
}
#endif

#endif
