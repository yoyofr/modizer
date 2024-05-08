#ifndef MYON_LEVELDATA_H_INCLUDED
#define MYON_LEVELDATA_H_INCLUDED

#include <stdatomic.h>

struct leveldata {
  atomic_flag flag;
  unsigned level;
  bool read;
};

static inline unsigned leveldata_read(struct leveldata *data) {
  while (atomic_flag_test_and_set_explicit(&data->flag, memory_order_acquire));
  unsigned level = data->level;
  data->read = true;
  atomic_flag_clear_explicit(&data->flag, memory_order_release);
  return level;
}

static inline void leveldata_update(struct leveldata *data, unsigned level) {
  while (atomic_flag_test_and_set_explicit(&data->flag, memory_order_acquire));
  if (data->read || (level > data->level)) data->level = level;
  data->read = false;
  atomic_flag_clear_explicit(&data->flag, memory_order_release);
}

static inline void leveldata_init(struct leveldata *data) {
  *data = (struct leveldata) {0};
  atomic_flag_clear_explicit(&data->flag, memory_order_relaxed);
}

#endif // MYON_LEVELDATA_H_INCLUDED
