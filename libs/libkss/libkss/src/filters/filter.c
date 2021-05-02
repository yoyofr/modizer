#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include "filter.h"

#define PI (3.14159265358979)

/* N : tap number (must be odd) */
FIR *FIR_new(void) {
  FIR *fir;

  fir = malloc(sizeof(FIR));
  if (fir != NULL)
    FIR_disable(fir);

  return fir;
}

static double HAMMING_window(int n, int M) { return 0.54 + 0.46 * cos(PI * n / M); }

/**
static double HANNING_window(int n, int M)
{
  return 0.5 * (1.0 + cos(PI*n/M));
}

static double BERTLET_window(int n, int M)
{
  return 1.0 - (double)n/M;
}

static double SQR_window(int n, int M)
{
  return 1.0;
}
*/

void FIR_disable(FIR *fir) { FIR_reset(fir, 0, 0, 0); }

/* Reset sample rate and cutoff frequency */
void FIR_reset(FIR *fir, uint32_t sam, uint32_t cut, uint32_t N) {
  uint32_t i;

  /* assert(fir); */
  memset(fir->buf, 0, sizeof(fir->buf));
  if (sam == 0 || cut == 0 || N == 0 || sam < cut) {
    fir->M = 0;
    return;
  }

  fir->Wc = 2.0 * PI * cut / sam;
  fir->M = (N - 1) / 2;

  fir->h[0] = fir->Wc / PI;
  fir->h[0] *= HAMMING_window(0, fir->M);

  for (i = 1; i <= fir->M; i++) {
    fir->h[i] = (1.0 / (PI * i)) * sin(fir->Wc * i);
    fir->h[i] *= HAMMING_window(i, fir->M); /* Window */
  }
}

int32_t FIR_calc(FIR *fir, int32_t data) {
  uint32_t i, M = fir->M;
  double s;

  /* assert(fir); */
  if (fir->M == 0)
    return data;

  /* Shift Buffer */
  for (i = 2 * M; i > 0; i--)
    fir->buf[i] = fir->buf[i - 1];

  /* Add new data */
  fir->buf[0] = data;

  /* Apply */
  s = fir->h[0] * fir->buf[M];
  for (i = 1; i <= M; i++)
    s += fir->h[i] * (fir->buf[M + i] + fir->buf[M - i]);

  return (int32_t)s;
}

void FIR_delete(FIR *fir) {
  if (fir)
    free(fir);
}