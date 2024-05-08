#ifndef LIBOPNA_OPNA_TIMER_H_INCLUDED
#define LIBOPNA_OPNA_TIMER_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*opna_timer_int_cb_t)(void *ptr);
typedef void (*opna_timer_mix_cb_t)(void *ptr, int16_t *buf, unsigned samples);

struct opna;

struct opna_timer {
  struct opna *opna;
  uint8_t status;
  opna_timer_int_cb_t interrupt_cb;
  void *interrupt_userptr;
  opna_timer_mix_cb_t mix_cb;
  void *mix_userptr;
  uint16_t timera;
  uint8_t timerb;
  bool timera_load;
  bool timera_enable;
  bool timerb_load;
  bool timerb_enable;
  uint16_t timerb_cnt;
};

void opna_timer_reset(struct opna_timer *timer, struct opna *opna);
uint8_t opna_timer_status(const struct opna_timer *timer);
void opna_timer_set_int_callback(struct opna_timer *timer,
                                 opna_timer_int_cb_t func, void *userptr);
void opna_timer_set_mix_callback(struct opna_timer *timer,
                                 opna_timer_mix_cb_t func, void *userptr);
void opna_timer_writereg(struct opna_timer *timer, unsigned reg, unsigned val);
void opna_timer_mix(struct opna_timer *timer, int16_t *buf, unsigned samples);
struct oscillodata;
void opna_timer_mix_oscillo(struct opna_timer *timer, int16_t *buf, unsigned samples, struct oscillodata *oscillo);

void opna_timer_nomix(struct opna_timer *timer, unsigned samples); //YOYOFR

#ifdef __cplusplus
}
#endif

#endif // LIBOPNA_OPNA_TIMER_H_INCLUDED
