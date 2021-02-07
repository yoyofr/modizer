/*
 freeverb.cpp - wrapper function for freeverb
 Made by Daisuke Nagano <breeze.nagano@nifty.com>
 Feb.26.2006

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "revmodel.hpp"

#define PCMBUFSIZE (8192)

extern "C" {
typedef struct _freeverb_t {
  revmodel* model;
} freeverb_t;

void*
create_freeverb(void)
{
  freeverb_t* self = NULL;
  revmodel* model = NULL;

  self = (freeverb_t *)malloc(sizeof(freeverb_t));
  if (!self) {
    return NULL;
  }

  model = new revmodel;
  if (!model) {
    free(self);
    return NULL;
  }

  model->setroomsize(0.5f);
  model->setdamp(0.1f);
  model->setwidth(0.8f);
  model->setmode(0);
  model->setwet(0.1f);
  model->setdry(0.95f);

  self->model = model;
  return (void *)self;
}

void
delete_freeverb(void* oself)
{
  freeverb_t* self = (freeverb_t *)oself;

  if (self) {
    if (self->model) {
      delete self->model;
    }
    free(self);
  }
}

void
setparams_freeverb(void* oself, int srate, float predelay, float roomsize, float damp, float width, float dry, float wet)
{
  freeverb_t* self = (freeverb_t *)oself;

  self->model->setsrate(srate);
  self->model->setpredelay(predelay);

  self->model->setroomsize(roomsize);
  self->model->setdamp(damp);
  self->model->setwidth(width);
  self->model->setmode(0);
  self->model->setdry(dry);
  self->model->setwet(wet);
}

void
process_freeverb(void* oself, unsigned char* in_data, int in_len)
{
  freeverb_t* self = (freeverb_t *)oself;

  self->model->processreplace(in_data, in_len/4);
}

void
reset_freeverb(void* oself)
{
  freeverb_t* self = (freeverb_t *)oself;

  self->model->mute();
}

}
