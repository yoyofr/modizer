/* Author: Peter Sovietov */

#ifndef LOAD_TEXT_H
#define LOAD_TEXT_H

struct ay_data {
  int sample_rate;
  int is_ym;
  int clock_rate;
  double frame_rate;
  double pan[3];
  int eqp_stereo_on;
  int dc_filter_on;
  int note_table;
};

struct text_parser {
  int index;
  int size;
  char* text;
};

int load_text_file(const char* name, struct ay_data* t);

#endif