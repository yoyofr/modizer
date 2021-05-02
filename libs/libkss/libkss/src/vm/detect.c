#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "detect.h"

LPDETECT *LPDETECT_new() {
  LPDETECT *p = malloc(sizeof(LPDETECT));
  memset(p, 0, sizeof(LPDETECT));
  p->m_bufsize = 1 << 16;
  p->m_bufmask = p->m_bufsize - 1;
  p->m_stream_buf = malloc(sizeof(int) * (p->m_bufsize));
  p->m_time_buf = malloc(sizeof(int) * (p->m_bufsize));
  return p;
}

void LPDETECT_delete(LPDETECT *__this) {
  if (__this) {
    free(__this->m_stream_buf);
    free(__this->m_time_buf);
    free(__this);
  }
}

void LPDETECT_reset(LPDETECT *__this) {
  int i;
  for (i = 0; i < __this->m_bufsize; i++) {
    __this->m_stream_buf[i] = -i;
    __this->m_time_buf[i] = 0;
  }

  __this->m_current_time = 0;
  __this->m_wspeed = 0;
  __this->m_bidx = 0;
  __this->m_blast = 0;
  __this->m_loop_start = -1;
  __this->m_loop_end = -1;
  __this->m_empty = 1; // true
}

int LPDETECT_write(LPDETECT *__this, uint32_t adr, uint32_t val) {
  __this->m_empty = 0; /* false */
  __this->m_time_buf[__this->m_bidx] = __this->m_current_time;
  __this->m_stream_buf[__this->m_bidx] = ((adr & 0xffff) << 8) | (val & 0xff);
  __this->m_bidx = (__this->m_bidx + 1) & __this->m_bufmask;
  return 0; /* false */
}

int LPDETECT_count(LPDETECT *__this, int32_t time_in_ms) {
  if ((__this->m_loop_end - __this->m_loop_start) <= 0)
    return 0;
  if (time_in_ms < __this->m_loop_end)
    return 0;
  time_in_ms -= __this->m_loop_end;
  return 1 + time_in_ms / (__this->m_loop_end - __this->m_loop_start);
}

int LPDETECT_update(LPDETECT *__this, int32_t time_in_ms, int32_t match_second, int32_t match_interval) {
  int i, j;
  int match_size, match_length;

  if (__this->m_loop_start >= 0)
    return 1;

  if (time_in_ms - __this->m_current_time < match_interval)
    return 0;

  __this->m_current_time = time_in_ms;

  if (__this->m_bidx <= __this->m_blast)
    return 0;

  if (__this->m_wspeed)
    __this->m_wspeed = (__this->m_wspeed + __this->m_bidx - __this->m_blast) / 2;
  else
    __this->m_wspeed = __this->m_bidx - __this->m_blast; /* ‰‰ñ */

  __this->m_blast = __this->m_bidx;

  match_size = __this->m_wspeed * match_second / match_interval;
  match_length = __this->m_bufsize - match_size;

  if (match_length < 0)
    return 0;

  for (i = 0; i < match_length; i++) {
    for (j = 0; j < match_size; j++) {
      if (__this->m_stream_buf[(__this->m_bidx + j + match_length) & __this->m_bufmask] !=
          __this->m_stream_buf[(__this->m_bidx + i + j) & __this->m_bufmask])
        break;
    }
    if (j == match_size) {
      __this->m_loop_start = __this->m_time_buf[(__this->m_bidx + i) & __this->m_bufmask];
      __this->m_loop_end = __this->m_time_buf[(__this->m_bidx + match_length) & __this->m_bufmask];
      return 1;
    }
  }

  return 0;
}
