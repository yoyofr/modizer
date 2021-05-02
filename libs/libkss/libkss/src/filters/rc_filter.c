#include <stdio.h>
#include <stdlib.h>
#include "rc_filter.h"

RCF *RCF_new(void) {
  RCF *rcf = malloc(sizeof(RCF));
  return rcf;
}

void RCF_reset(RCF *rcf, double rate, double R, double C) {
  rcf->a = 1.0 / R / C / rate;
  rcf->out = 0;
  rcf->enable = 1;
}

int32_t RCF_calc(RCF *rcf, int32_t wav) {
  if (rcf->enable) {
    rcf->out += rcf->a * (wav - rcf->out);
    return (int32_t)rcf->out;
  } else {
    return wav;
  }
}

void RCF_delete(RCF *rcf) { free(rcf); }

void RCF_disable(RCF *rcf) { rcf->enable = 0; }