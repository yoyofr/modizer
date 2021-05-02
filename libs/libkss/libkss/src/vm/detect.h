#ifndef __DETECT_H__
#define __DETECT_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef struct __LPDETECT__ {
  int32_t m_bufsize, m_bufmask;
  int32_t *m_stream_buf;
  int32_t *m_time_buf;
  int32_t m_bidx;
  int32_t m_blast;
  int32_t m_wspeed;
  int32_t m_current_time;
  int32_t m_loop_start, m_loop_end;
  int32_t m_empty;
} LPDETECT;

LPDETECT *LPDETECT_new(void);
void LPDETECT_delete(LPDETECT *__this);
void LPDETECT_reset(LPDETECT *__this);
int LPDETECT_write(LPDETECT *__this, uint32_t adr, uint32_t val);
int LPDETECT_update(LPDETECT *__this, int32_t time_in_ms, int32_t match_second, int32_t match_interval);
int LPDETECT_count(LPDETECT *__this, int32_t time_in_ms);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DETECT_H__ */
