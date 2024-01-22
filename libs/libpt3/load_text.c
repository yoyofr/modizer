/* Author: Peter Sovietov */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "load_text.h"

static int load_file(const char* name, char** buffer, int* size) {
  FILE* f = fopen(name, "rb");
  if (f == NULL) {
    return 0;
  }
  fseek(f, 0, SEEK_END);
  *size = ftell(f);
  rewind(f);
  *buffer = (char*) malloc(*size + 1);
  if (*buffer == NULL) {
    fclose(f);
    return 0;
  }
  if ((int) fread(*buffer, 1, *size, f) != *size) {
    free(*buffer);
    fclose(f);
    return 0;
  }
  fclose(f);
  (*buffer)[*size] = 0;
  return 1;
}

static void skip(struct text_parser* p, int (*is_valid)(int)) {
  for (; p->index < p->size && is_valid(p->text[p->index]); p->index += 1) {
  }
}

static int parse_next(struct text_parser* p, const char* name) {
  int size = (int) strlen(name);
  skip(p, isspace);
  if (strncmp(&p->text[p->index], name, size)) {
    return 0;
  }
  p->index += size;
  return 1;
}

int parse_int(struct text_parser* p, int* n) {
  int size;
  char* start;
  char* end;
  skip(p, isspace);
  start = &p->text[p->index];
  *n = strtol(start, &end, 0);
  size = (int) (end - start);
  if (size > 0) {
    p->index += size;
    return 1;
  }
  return 0;
}

int parse_float(struct text_parser* p, double* n) {
  int size;
  char* start;
  char* end;
  skip(p, isspace);
  start = &p->text[p->index];
  *n = strtod(start, &end);
  size = (int) (end - start);
  if (size > 0) {
    p->index += size;
    return 1;
  }
  return 0;
}

static int parse_frames(struct text_parser* p, int* frame_data, int frame_count) {
  int i;
  int* buffer = frame_data;
  for (i = 0; i < frame_count * 16; i += 1) {
    if(!parse_int(p, buffer)) {
      return 0;
    }
    buffer += 1;
  }
  return 1;
}

static int is_not_space(int c) {
  return !isspace(c);
}

int load_text_file(const char* name, struct ay_data* t) {
  int n;
  double f;
  struct text_parser p;
  int ok = 1;
  if (!load_file(name, &p.text, &p.size)) {
    return 0;
  }
  p.index = 0;
  while (p.index < p.size) {
    if (parse_next(&p, "sample_rate") && parse_int(&p, &n)) {
      t->sample_rate = n;
    } else if (parse_next(&p, "is_ym") && parse_int(&p, &n)) {
      t->is_ym = n;
    } else if (parse_next(&p, "clock_rate") && parse_int(&p, &n)) {
      t->clock_rate = n;
    } else if (parse_next(&p, "frame_rate") && parse_float(&p, &f)) {
      t->frame_rate = f;
    } else if (parse_next(&p, "pan_a") && parse_int(&p, &n)) {
      t->pan[0] = n / 100.;
    } else if (parse_next(&p, "pan_b") && parse_int(&p, &n)) {
      t->pan[1] = n / 100.;
    } else if (parse_next(&p, "pan_c") && parse_int(&p, &n)) {
      t->pan[2] = n / 100.;
    } else if (parse_next(&p, "eqp_stereo_on") && parse_int(&p, &n)) {
      t->eqp_stereo_on = n;
    } else if (parse_next(&p, "dc_filter_on") && parse_int(&p, &n)) {
      t->dc_filter_on = n;
    } else if (parse_next(&p, "note_table") && parse_int(&p, &n)) {
      t->note_table = n;
    } else {
      skip(&p, is_not_space);
    }
  }
  free(p.text);
  return ok;
}