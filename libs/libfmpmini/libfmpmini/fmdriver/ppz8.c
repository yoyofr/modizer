#include "ppz8.h"
#include "fmdriver_common.h"
#include <string.h>
#include "ppz8-sinctable.inc"

unsigned ppz8_get_mask(const struct ppz8 *ppz8) {
  return ppz8->mask;
}
void ppz8_set_mask(struct ppz8 *ppz8, unsigned mask) {
  ppz8->mask = mask & 0xffu;
}

void ppz8_init(struct ppz8 *ppz8, uint16_t srate, uint16_t mix_volume) {
  for (int i = 0; i < 2; i++) {
    struct ppz8_pcmbuf *buf = &ppz8->buf[i];
    buf->data = 0;
    buf->buflen = 0;
    for (int j = 0; j < 128; j++) {
      struct ppz8_pcmvoice *voice = &buf->voice[j];
      voice->start = 0;
      voice->len = 0;
      voice->loopstart = 0;
      voice->loopend = 0;
      voice->origfreq = 0;
    }
  }
  for (int i = 0; i < 8; i++) {
    struct ppz8_channel *channel = &ppz8->channel[i];
    channel->ptr = -1;
    channel->loopstartptr = -1;
    channel->loopendptr = -1;
    channel->endptr = 0;
    channel->freq = 0;
    channel->loopstartoff = -1;
    channel->loopendoff = -1;
    channel->vol = 8;
    channel->pan = 5;
    channel->voice = 0;
    channel->looped = false;
    leveldata_init(&channel->leveldata);
  }
  ppz8->srate = srate;
  ppz8->totalvol = 12;
  ppz8->mix_volume = mix_volume;
  ppz8->interp = PPZ8_INTERP_SINC;
}

static uint64_t ppz8_loop(const struct ppz8_channel *channel, uint64_t ptr) {
  if (channel->loopstartptr != (uint64_t)-1) {
    uint64_t loopendptr = (channel->loopendptr == (uint64_t)-1) ?
        channel->endptr : channel->loopendptr;
    if (ptr >= loopendptr) {
      if (loopendptr == channel->loopstartptr) return (uint64_t)-1;
      uint32_t offset = (ptr - loopendptr) >> 16;
      offset %= (uint32_t)((loopendptr - channel->loopstartptr) >> 16);
      offset += (uint32_t)(channel->loopstartptr >> 16);
      return (ptr & ((1<<16)-1)) | (((uint64_t)offset) << 16);
    }
  }
  uint64_t limit = channel->endptr;
  if ((channel->loopstartptr != (uint64_t)-1) &&
      (channel->loopendptr != (uint64_t)-1) &&
      (channel->loopendptr > channel->endptr)) {
    limit = channel->loopendptr;
  }
  if (ptr >= limit) {
    return (uint64_t)-1;
  }
  return ptr;
}

static void ppz8_channel_get_centered_samples(
    struct ppz8 *ppz8,
    struct ppz8_channel *channel,
    int16_t *obuf, int samples) {
  struct ppz8_pcmbuf *buf = &ppz8->buf[channel->voice>>7];
  uint32_t currind = channel->ptr >> 16;
  for (int i = 0; i < samples; i++) {
    int indoff = i - (samples - 1)/2;
    uint32_t ind = currind + indoff;
    if (indoff < 0) {
      if ((channel->loopstartptr != (uint64_t)-1) && channel->looped) {
        uint32_t loopendind = ((channel->loopendptr == (uint64_t)-1) ?
            channel->endptr : channel->loopendptr) >> 16;
        uint32_t loopperiod = loopendind - (channel->loopstartptr >> 16);
        if (!loopperiod) {
          ind = -1;
        } else {
          uint32_t loopoffset = -(ind - ((uint32_t)(channel->loopstartptr >> 16)) - loopperiod);
          loopoffset %= loopperiod;
          if (!loopoffset) loopoffset = loopperiod;
          ind = (channel->loopstartptr >> 16) + loopperiod - loopoffset;
        }
      } else {
        if (((uint32_t)-indoff) > currind) {
          ind = -1;
        }
      }
    } else {
      uint32_t loopendind = (channel->loopendptr == (uint64_t)-1) ?
          channel->endptr : channel->loopendptr;
      if ((channel->loopstartptr != (uint64_t)-1) && (ind >= loopendind)) {
        uint32_t loopperiod = loopendind - (channel->loopstartptr >> 16);
        if (!loopperiod) {
          ind = -1;
        } else {
          uint32_t loopoffset = ind - (channel->loopstartptr >> 16);
          loopoffset %= loopperiod;
          ind = (channel->loopstartptr >> 16) + loopoffset;
        }
      }
    }
    if ((ind >= (channel->endptr >> 16)) || (ind >= buf->buflen) || !buf->data) {
      obuf[i] = 0;
    } else {
      obuf[i] = buf->data[ind];
    }
  }
}

static int32_t ppz8_channel_calc_nearest_neighbor(struct ppz8 *ppz8, struct ppz8_channel *channel) {
  int16_t sample;
  ppz8_channel_get_centered_samples(ppz8, channel, &sample, 1);
  return sample;
}

static int32_t ppz8_channel_calc_linear(struct ppz8 *ppz8, struct ppz8_channel *channel) {
  int32_t out = 0;
  int16_t samples[2];
  ppz8_channel_get_centered_samples(ppz8, channel, samples, 2);
  uint16_t coeff = channel->ptr & 0xffffu;
  out += (int32_t)samples[0] * (0x10000u - coeff);
  out += (int32_t)samples[1] * coeff;
  out >>= 16;
  return out;
}

static int32_t ppz8_channel_calc_sinc(struct ppz8 *ppz8, struct ppz8_channel *channel) {
  int16_t samples[7];
  uint8_t frac = channel->ptr >> 8;
  ppz8_channel_get_centered_samples(ppz8, channel, samples, 7);
  const int16_t *sinctable = ppz8_sinctable[frac];
  int32_t out = 0;
  for (int i = 0; i < 7; i++) {
    out += samples[i] * sinctable[i];
  }
  out >>= 15;
  return out;
}

static int32_t ppz8_channel_calc(struct ppz8 *ppz8, struct ppz8_channel *channel) {
  struct ppz8_pcmbuf *buf = &ppz8->buf[channel->voice>>7];
  struct ppz8_pcmvoice *voice = &buf->voice[channel->voice & 0x7f];
  int32_t out = 0;
  if (channel->vol) {
    switch (ppz8->interp) {
    case PPZ8_INTERP_SINC:
      out = ppz8_channel_calc_sinc(ppz8, channel);
      break;
    case PPZ8_INTERP_LINEAR:
      out = ppz8_channel_calc_linear(ppz8, channel);
      break;
    default:
      out = ppz8_channel_calc_nearest_neighbor(ppz8, channel);
      break;
    }
    // volume: out * 2**((volume-15)/2)
    out >>= (7 - ((channel->vol&0xf)>>1));
    if (!(channel->vol&1)) {
      out *= 0xb505;
      out >>= 16;
    }
  }

  uint64_t ptrdiff = (((uint64_t)channel->freq * voice->origfreq) << 1) / ppz8->srate;
  uint64_t newptr = channel->ptr + ptrdiff;
  channel->ptr = ppz8_loop(channel, newptr);
  if (newptr != channel->ptr) channel->looped = true;
  if (channel->ptr == (uint64_t)-1) channel->playing = false;
  return out;
}

void ppz8_mix(struct ppz8 *ppz8, int16_t *buf, unsigned samples) {
  unsigned level[8] = {0};
  static const uint8_t pan_vol[10][2] = {
    {0, 0},
    {4, 0},
    {4, 1},
    {4, 2},
    {4, 3},
    {4, 4},
    {3, 4},
    {2, 4},
    {1, 4},
    {0, 4}
  };
  for (unsigned i = 0; i < samples; i++) {
    int32_t lo = buf[i*2+0];
    int32_t ro = buf[i*2+1];
    for (int p = 0; p < 8; p++) {
      struct ppz8_channel *channel = &ppz8->channel[p];
      if (!channel->playing) continue;
      int32_t out = ppz8_channel_calc(ppz8, channel);
      {
        unsigned uout = out > 0 ? out : -out;
        if (uout > level[p]) level[p] = uout;
      }
      if ((1u << p) & (ppz8->mask)) continue;
      out *= ppz8->mix_volume;
      out >>= 15;
      lo += (out * pan_vol[channel->pan][0]) >> 2;
      ro += (out * pan_vol[channel->pan][1]) >> 2;
    }
    if (lo < INT16_MIN) lo = INT16_MIN;
    if (lo > INT16_MAX) lo = INT16_MAX;
    if (ro < INT16_MIN) ro = INT16_MIN;
    if (ro > INT16_MAX) ro = INT16_MAX;
    buf[i*2+0] = lo;
    buf[i*2+1] = ro;
  }
  for (int p = 0; p < 8; p++) {
    leveldata_update(&ppz8->channel[p].leveldata, level[p]);
  }
}

static int16_t calc_acc(int16_t acc, uint16_t adpcmd, uint8_t data) {
  data &= 0xf;
  int32_t acc_d = (((data&7)<<1)|1);
  if (data&8) acc_d = -acc_d;
  int32_t newacc = acc + (acc_d*adpcmd/8);
  if (newacc < -32768) newacc = -32768;
  if (newacc > 32767) newacc = 32767;
  return newacc;
}

static uint16_t calc_adpcmd(uint16_t adpcmd, uint8_t data) {
  static const uint8_t adpcm_table[8] = {
    57, 57, 57, 57, 77, 102, 128, 153,
  };
  uint32_t newadpcmd = adpcmd*adpcm_table[data&7]/64;
  if (newadpcmd < 127) newadpcmd = 127;
  if (newadpcmd > 24576) newadpcmd = 24576;
  return newadpcmd;
}

bool ppz8_pvi_load(struct ppz8 *ppz8, uint8_t bnum,
                   const uint8_t *pvidata, uint32_t pvidatalen,
                   int16_t *decodebuf) {
  if (bnum >= 2) return false;
  //if (pvidatalen > (0x210+(1<<18))) return false;
  struct ppz8_pcmbuf *buf = &ppz8->buf[bnum];
  //uint16_t origfreq = ((uint32_t)read16le(&pvidata[0x8]) * 55467) >> 16;
  uint16_t origfreq = (0x49ba*55467)>>16;
  uint32_t lastaddr = 0;
  for (int i = 0; i < 0x80; i++) {
    uint32_t startaddr = read16le(&pvidata[0x10+i*4+0]) << 6;
    uint32_t endaddr = (read16le(&pvidata[0x10+i*4+2])+1) << 6;
    if (startaddr != lastaddr) break;
    if (startaddr >= endaddr) break;
    struct ppz8_pcmvoice *voice = &buf->voice[i];
    voice->start = startaddr<<1;
    voice->len = (endaddr-startaddr)<<1;
    voice->loopstart = (uint32_t)-1;
    voice->loopend = (uint32_t)-1;
    voice->origfreq = origfreq;

    int16_t acc = 0;
    uint16_t adpcmd = 127;
    for (uint32_t a = startaddr; a < endaddr; a++) {
      if (pvidatalen <= (0x210+(a>>1))) return false;
      uint8_t data = pvidata[0x210+(a>>1)];
      if (a&1) {
        data &= 0xf;
      } else {
        data >>= 4;
      }
      acc = calc_acc(acc, adpcmd, data);
      adpcmd = calc_adpcmd(adpcmd, data);
      decodebuf[a] = acc;
    }
    lastaddr = endaddr;
  }
  buf->data = decodebuf;
  buf->buflen = ppz8_pvi_decodebuf_samples(pvidatalen);
  return true;
}

bool ppz8_pzi_load(struct ppz8 *ppz8, uint8_t bnum,
                   const uint8_t *pzidata, uint32_t pzidatalen,
                   int16_t *decodebuf) {
  if (bnum >= 2) return false;
  if (pzidatalen < (0x20+(18*128))) return false;
  if (memcmp(pzidata, "PZI0", 4) && memcmp(pzidata, "PZI1", 4)) return false;
  struct ppz8_pcmbuf *buf = &ppz8->buf[bnum];
  for (int i = 0; i < 0x80; i++) {
    struct ppz8_pcmvoice *voice = &buf->voice[i];
    voice->start = read32le(&pzidata[0x20+18*i+0]) * 2;
    voice->len = read32le(&pzidata[0x20+18*i+4]) * 2;
    voice->loopstart = read32le(&pzidata[0x20+18*i+8]);
    voice->loopend = read32le(&pzidata[0x20+18*i+12]);
    voice->origfreq = read16le(&pzidata[0x20+18*i+16]);/*
    if (voice->origfreq) {
      voice->origfreq = 0x100000000 / voice->origfreq;
    }*/
    //voice->origfreq = 15974;
  }
  buf->data = decodebuf;
  buf->buflen = pzidatalen - (0x20+(18*128));
  for (uint32_t i = 0; i < buf->buflen; i++) {
    buf->data[i] = (pzidata[0x20+18*128+i] - 0x80) << 8;
  }
  return true;
}

static void ppz8_channel_play(struct ppz8 *ppz8, uint8_t ch, uint8_t v) {
  if (ch >= 8) return;
  struct ppz8_channel *channel = &ppz8->channel[ch];
  channel->voice = v;
  struct ppz8_pcmbuf *buf = &ppz8->buf[channel->voice>>7];
  struct ppz8_pcmvoice *voice = &buf->voice[channel->voice & 0x7f];
  channel->ptr = ((uint64_t)(voice->start)>>1)<<16;
  channel->endptr = ((uint64_t)(voice->start+voice->len)>>1)<<16;
/*  
  channel->loopstartptr = ((uint64_t)(voice->loopstart)>>1)<<16;
  channel->loopendptr = ((uint64_t)(voice->loopend)>>1)<<16;
*/
  channel->loopstartptr = (channel->loopstartoff == (uint32_t)-1) ? (uint64_t)-1
    : channel->ptr + (((uint64_t)(channel->loopstartoff))<<16);
  channel->loopendptr = (channel->loopendoff == (uint32_t)-1) ? (uint64_t)-1
    : channel->ptr + (((uint64_t)(channel->loopendoff))<<16);
  channel->playing = true;
  channel->looped = false;
}

static void ppz8_channel_stop(struct ppz8 *ppz8, uint8_t ch) {
  if (ch >= 8) return;
  struct ppz8_channel *channel = &ppz8->channel[ch];
  channel->playing = false;
  channel->ptr = -1;
}

static void ppz8_channel_volume(struct ppz8 *ppz8, uint8_t ch, uint8_t vol) {
  if (ch >= 8) return;
  if (vol >= 16) return;
  struct ppz8_channel *channel = &ppz8->channel[ch];
  channel->vol = vol;
}

static void ppz8_channel_freq(struct ppz8 *ppz8, uint8_t ch, uint32_t freq) {
  if (ch >= 8) return;
  struct ppz8_channel *channel = &ppz8->channel[ch];
  channel->freq = freq;
}

static void ppz8_channel_loopoffset(struct ppz8 *ppz8, uint8_t ch,
                                    uint32_t startoff, uint32_t endoff) {
  if (ch >= 8) return;
  struct ppz8_channel *channel = &ppz8->channel[ch];
  channel->loopstartoff = startoff;
  channel->loopendoff = endoff;
//  fprintf(stderr, "channel: %d, start: %08x, end: %08x\n", ch, startoff, endoff);
}

static void ppz8_channel_pan(struct ppz8 *ppz8, uint8_t ch, uint8_t pan) {
  if (ch >= 8) return;
  if (pan >= 10) return;
  struct ppz8_channel *channel = &ppz8->channel[ch];
  channel->pan = pan;
}

static void ppz8_total_volume(struct ppz8 *ppz8, uint8_t vol) {
  if (vol >= 16) return;
  ppz8->totalvol = vol;
}

static void ppz8_channel_loop_voice(struct ppz8 *ppz8, uint8_t ch, uint8_t v) {
  if (ch >= 8) return;
  struct ppz8_channel *channel = &ppz8->channel[ch];
  struct ppz8_pcmbuf *buf = &ppz8->buf[v>>7];
  struct ppz8_pcmvoice *voice = &buf->voice[v & 0x7f];
  channel->loopstartoff = voice->loopstart;
  channel->loopendoff = voice->loopend;
}

static uint32_t ppz8_voice_length(struct ppz8 *ppz8, uint8_t v) {
  struct ppz8_pcmbuf *buf = &ppz8->buf[v>>7];
  struct ppz8_pcmvoice *voice = &buf->voice[v & 0x7f];
  return voice->len;
}

const struct ppz8_functbl ppz8_functbl = {
  ppz8_channel_play,
  ppz8_channel_stop,
  ppz8_channel_volume,
  ppz8_channel_freq,
  ppz8_channel_loopoffset,
  ppz8_channel_pan,
  ppz8_total_volume,
  ppz8_channel_loop_voice,
  ppz8_voice_length
};
