#include "opnafm.h"
#ifdef LIBOPNA_ENABLE_OSCILLO
#include "oscillo/oscillo.h"
#endif

#include "opnatables.h"

#if 1
#define LIBOPNA_DEBUG(...)
#else
#include <stdio.h>
#define LIBOPNA_DEBUG(...) fprintf(stderr, __VA_ARGS__)
#endif

//#define LIBOPNA_ENABLE_HIRES_SIN
//#define LIBOPNA_ENABLE_HIRES_ENV

enum {
  ENV_MAX_HIRES = LIBOPNA_FM_ENV_MAX * 4
};

enum {
  CH3_MODE_NORMAL = 0,
  CH3_MODE_CSM    = 1,
  CH3_MODE_SE     = 2
};

static void opna_fm_slot_reset(struct opna_fm_slot *slot) {
  slot->env = LIBOPNA_FM_ENV_MAX;
  slot->env_hires = ENV_MAX_HIRES;
  slot->env_state = ENV_RELEASE;
}


void opna_fm_chan_reset(struct opna_fm_channel *chan) {
#ifdef LIBOPNA_ENABLE_LEVELDATA
  leveldata_init(&chan->leveldata);
#endif
  for (int i = 0; i < 4; i++) {
    opna_fm_slot_reset(&chan->slot[i]);
  }
}

void opna_fm_reset(struct opna_fm *fm) {
  *fm = (struct opna_fm) {0};
  for (int i = 0; i < 6; i++) {
    opna_fm_chan_reset(&fm->channel[i]);
    fm->lselect[i] = true;
    fm->rselect[i] = true;
  }
  fm->blkfnum_h = 0;
  fm->env_div3 = 0;
  
  fm->ch3.mode = CH3_MODE_NORMAL;
  for (int i = 0; i < 3; i++) {
    fm->ch3.fnum[i] = 0;
    fm->ch3.blk[i] = 0;
  }
  fm->mask = 0;
}
// maximum output: 2042<<2 = 8168
static int16_t opna_fm_slotout(struct opna_fm_slot *slot, int16_t modulation,
  bool hires_sin, bool hires_env
) {
  int logout;
  bool minus;
  if (hires_sin) {
    unsigned pind_hires = (slot->phase >> 8);
    pind_hires += modulation << 1;
    minus = pind_hires & (1<<(LOGSINTABLEHIRESBIT+1));
    bool reverse = pind_hires & (1<<LOGSINTABLEHIRESBIT);
    if (reverse) pind_hires = ~pind_hires;
    pind_hires &= (1<<LOGSINTABLEHIRESBIT)-1;

    logout = logsintable_hires[pind_hires];
  } else {
    unsigned pind = (slot->phase >> 10);
    pind += modulation >> 1;
    minus = pind & (1<<(LOGSINTABLEBIT+1));
    bool reverse = pind & (1<<LOGSINTABLEBIT);
    if (reverse) pind = ~pind;
    pind &= (1<<LOGSINTABLEBIT)-1;

    logout = logsintable[pind];
  }
//  if (slot->env == LIBOPNA_FM_ENV_MAX) {
  if (hires_env) {
    logout += slot->env_hires;
  } else {
    logout += (slot->env << 2);
  }
//  }
  logout += (slot->tl << 5);

  int selector = logout & ((1<<EXPTABLEBIT)-1);
  int shifter = logout >> EXPTABLEBIT;
  if (shifter > 13) shifter = 13; 

  int16_t out = (exptable[selector] << 2) >> shifter;
  if (minus) out = -out;
  slot->prevout = out;
  return out;
}

static unsigned blkfnum2freq(unsigned blk, unsigned fnum) {
  return (fnum << blk) >> 1;
}

#define F(n) (!!(fnum & (1 << ((n)-1))))

static unsigned blkfnum2keycode(unsigned blk, unsigned fnum) {
  unsigned keycode = blk<<2;
  keycode |= F(11) << 1;
  keycode |= (F(11) && (F(10)||F(9)||F(8))) || ((!F(11))&&F(10)&&F(9)&&F(8));
  return keycode;
}

#undef F

static void opna_fm_slot_phase(struct opna_fm_slot *slot, unsigned freq) {
// TODO: detune
//  freq += slot->dt;
  unsigned det = dettable[slot->det & 0x3][slot->keycode];
  if (slot->det & 0x4) det = -det;
  freq += det;
  freq &= (1U<<17)-1;
  int mul = slot->mul << 1;
  if (!mul) mul = 1;
  slot->phase += ((freq * mul)>>1);
}

void opna_fm_chan_phase(struct opna_fm_channel *chan) {
  unsigned freq = blkfnum2freq(chan->blk, chan->fnum);
  for (int i = 0; i < 4; i++) {
    opna_fm_slot_phase(&chan->slot[i], freq);
  }
}

static void opna_fm_chan_phase_se(struct opna_fm_channel *chan, struct opna_fm *fm) {
  unsigned freq;
  freq = blkfnum2freq(fm->ch3.blk[0], fm->ch3.fnum[0]);
  opna_fm_slot_phase(&chan->slot[0], freq);
  freq = blkfnum2freq(fm->ch3.blk[1], fm->ch3.fnum[1]);
  opna_fm_slot_phase(&chan->slot[1], freq);
  freq = blkfnum2freq(fm->ch3.blk[2], fm->ch3.fnum[2]);
  opna_fm_slot_phase(&chan->slot[2], freq);
  freq = blkfnum2freq(chan->blk, chan->fnum);
  opna_fm_slot_phase(&chan->slot[3], freq);
}

struct opna_fm_frame opna_fm_chanout(struct opna_fm_channel *chan,
  bool hires_sin, bool hires_env) {
  int16_t slot0 = chan->slot[0].prevout;
  int16_t slot1 = chan->slot[1].prevout;
  int16_t slot2 = chan->slot[2].prevout;
  int16_t slot3 = chan->slot[3].prevout;
  int16_t fb = chan->fbmem + chan->slot[0].prevout;
  chan->fbmem = slot0;
  if (!chan->fb) fb = 0;
  opna_fm_slotout(&chan->slot[0], fb >> (9 - chan->fb), hires_sin, hires_env);

  int16_t prev_alg_mem = chan->alg_mem;
  struct opna_fm_frame ret;
  switch (chan->alg) {
  // this looks ugly, but is verified with actual YMF288 and YM2608
  case 0:
    opna_fm_slotout(&chan->slot[1], chan->slot[0].prevout, hires_sin, hires_env);
    opna_fm_slotout(&chan->slot[2], slot1, hires_sin, hires_env);
    opna_fm_slotout(&chan->slot[3], slot2, hires_sin, hires_env);
    ret.data[0] = ret.data[1] = chan->slot[3].prevout >> 1;
    break;
  case 1:
    opna_fm_slotout(&chan->slot[1], 0, hires_sin, hires_env);
    opna_fm_slotout(&chan->slot[2], prev_alg_mem, hires_sin, hires_env);
    opna_fm_slotout(&chan->slot[3], slot2, hires_sin, hires_env);
    chan->alg_mem = chan->slot[0].prevout;
    chan->alg_mem += chan->slot[1].prevout;
    chan->alg_mem &= ~1;
    ret.data[0] = ret.data[1] = chan->slot[3].prevout >> 1;
    break;
  case 2:
    opna_fm_slotout(&chan->slot[1], 0, hires_sin, hires_env);
    opna_fm_slotout(&chan->slot[2], slot1, hires_sin, hires_env);
    opna_fm_slotout(&chan->slot[3], slot0 + slot2, hires_sin, hires_env);
    ret.data[0] = ret.data[1] = chan->slot[3].prevout >> 1;
    break;
  case 3:
    opna_fm_slotout(&chan->slot[1], chan->slot[0].prevout, hires_sin, hires_env);
    opna_fm_slotout(&chan->slot[2], 0, hires_sin, hires_env);
    opna_fm_slotout(&chan->slot[3], slot2 + prev_alg_mem, hires_sin, hires_env);
    chan->alg_mem = slot1;
    ret.data[0] = ret.data[1] = chan->slot[3].prevout >> 1;
    break;
  case 4:
    opna_fm_slotout(&chan->slot[1], slot0, hires_sin, hires_env);
    opna_fm_slotout(&chan->slot[2], 0, hires_sin, hires_env);
    opna_fm_slotout(&chan->slot[3], chan->slot[2].prevout, hires_sin, hires_env);
    ret.data[0] = ret.data[1] = slot3 >> 1;
    ret.data[0] += chan->slot[1].prevout >> 1;
    ret.data[1] += slot1 >> 1;
    break;
  case 5:
    opna_fm_slotout(&chan->slot[1], slot0, hires_sin, hires_env);
    opna_fm_slotout(&chan->slot[2], slot0, hires_sin, hires_env);
    opna_fm_slotout(&chan->slot[3], slot0, hires_sin, hires_env);
    chan->alg_mem = slot2;
    chan->alg_mem &= ~1;
    ret.data[0] = ret.data[1] = slot3 >> 1;
    ret.data[0] += (chan->slot[1].prevout >> 1) + (slot2 >> 1);
    ret.data[1] += (slot1 >> 1) + (prev_alg_mem >> 1);
    break;
  case 6:
    opna_fm_slotout(&chan->slot[1], slot0, hires_sin, hires_env);
    opna_fm_slotout(&chan->slot[2], 0, hires_sin, hires_env);
    opna_fm_slotout(&chan->slot[3], 0, hires_sin, hires_env);
    chan->alg_mem = slot2;
    chan->alg_mem &= ~1;
    ret.data[0] = ret.data[1] = slot3 >> 1;
    ret.data[0] += (chan->slot[1].prevout >> 1) + (slot2 >> 1);
    ret.data[1] += (slot1 >> 1) + (prev_alg_mem >> 1);
    break;
  case 7:
    opna_fm_slotout(&chan->slot[1], 0, hires_sin, hires_env);
    opna_fm_slotout(&chan->slot[2], 0, hires_sin, hires_env);
    opna_fm_slotout(&chan->slot[3], 0, hires_sin, hires_env);
    chan->alg_mem = chan->slot[1].prevout + chan->slot[2].prevout;
    chan->alg_mem &= ~1;
    ret.data[0] = ret.data[1] =
        (chan->slot[0].prevout >> 1) + (chan->slot[3].prevout >> 1);
    ret.data[0] += chan->alg_mem >> 1;
    ret.data[1] += prev_alg_mem >> 1;
    // when int = 32bit, this is implementation-defined, not UB
    ret.data[0] <<= 1;
    ret.data[0] >>= 1;
    ret.data[1] <<= 1;
    ret.data[1] >>= 1;
    break;
  }
  return ret;
}

static void opna_fm_slot_setrate(struct opna_fm_slot *slot, int status) {
  int r;
  switch (status) {
  case ENV_ATTACK:
    r = slot->ar;
    break;
  case ENV_DECAY:
    r = slot->dr;
    break;
  case ENV_SUSTAIN:
    r = slot->sr;
    break;
  case ENV_RELEASE:
    r = (slot->rr*2+1);
    break;
  default:
    return;
  }

  if (!r) {
    slot->rate_selector = 0;
    slot->rate_mul = 0;
    slot->rate_shifter = 0;
    slot->rate_selector_hires = 0;
    slot->rate_mul_hires = 0;
    slot->rate_shifter_hires = 0;
    return;
  }

  int rate = 2*r + (slot->keycode >> (3 - slot->ks));

  if (rate > 63) rate = 63;
  LIBOPNA_DEBUG("rate: %d\n", rate);
  if (status == ENV_ATTACK && rate >= 62) rate += 4;
  int rate_shifter = 11 - (rate >> 2);
  if (rate_shifter < 0) {
    slot->rate_selector = (rate & ((1<<2)-1)) + 4;
    slot->rate_mul = 1<<(-rate_shifter-1);
    slot->rate_shifter = 0;
  } else {
    slot->rate_selector = rate & ((1<<2)-1);
    slot->rate_mul = 1;
    slot->rate_shifter = rate_shifter;
  }
  
  int rate_hires = rate + 8;
  int rate_shifter_hires = 11 - (rate_hires >> 2);
  if (rate_shifter_hires < 0) {
    slot->rate_selector_hires = (rate_hires & ((1<<2)-1)) + 4;
    slot->rate_mul_hires = 1<<(-rate_shifter_hires-1);
    slot->rate_shifter_hires = 0;
  } else {
    slot->rate_selector_hires = rate_hires & ((1<<2)-1);
    slot->rate_mul_hires = 1;
    slot->rate_shifter_hires = rate_shifter_hires;
  }
  LIBOPNA_DEBUG("status: %d\n", status);
  LIBOPNA_DEBUG("rate_selector: %d\n", slot->rate_selector);
  LIBOPNA_DEBUG("rate_mul:      %d\n", slot->rate_mul);
  LIBOPNA_DEBUG("rate_shifter:  %d\n\n", slot->rate_shifter);
}

void opna_fm_slot_env(struct opna_fm_slot *slot, bool hires_env) {
//  if (!(slot->env_count & ((1<<slot->rate_shifter)-1))) {
  int rate_shifter = hires_env ? slot->rate_shifter_hires : slot->rate_shifter;
  int rate_selector = hires_env ? slot->rate_selector_hires : slot->rate_selector;
  int rate_mul = hires_env ? slot->rate_mul_hires : slot->rate_mul;
  if ((slot->env_count & ((1<<rate_shifter)-1)) == ((1<<rate_shifter)-1)) {
    int rate_index = (slot->env_count >> rate_shifter) & 7;
    int env_inc = rateinctable[rate_selector][rate_index];
    env_inc *= rate_mul;

    if (hires_env) {
      switch (slot->env_state) {
      int newenv;
      int sl;
      case ENV_ATTACK:
        newenv = slot->env_hires + (((-slot->env_hires-1) * env_inc) >> 6);
        if (newenv <= 0) {
          slot->env_hires = 0;
          slot->env_state = ENV_DECAY;
          opna_fm_slot_setrate(slot, ENV_DECAY);
        } else {
          slot->env_hires = newenv;
        }
        break;
      case ENV_DECAY:
        slot->env_hires += env_inc;
        sl = slot->sl;
        if (sl == 0xf) sl = 0x1f;
        if (slot->env_hires >= (sl << 7)) {
          slot->env_state = ENV_SUSTAIN;
          opna_fm_slot_setrate(slot, ENV_SUSTAIN);
        }
        break;
      case ENV_SUSTAIN:
        slot->env_hires += env_inc;
        if (slot->env_hires >= ENV_MAX_HIRES) slot->env_hires = ENV_MAX_HIRES;
        break;
      case ENV_RELEASE:
        slot->env_hires += env_inc;
        if (slot->env_hires >= ENV_MAX_HIRES) {
          slot->env_hires = ENV_MAX_HIRES;
          slot->env_state = ENV_OFF;
        }
        break;
      }
      slot->env = slot->env_hires >> 2;
    } else {
      switch (slot->env_state) {
      int newenv;
      int sl;
      case ENV_ATTACK:
        newenv = slot->env + (((-slot->env-1) * env_inc) >> 4);
        if (newenv <= 0) {
          slot->env = 0;
          slot->env_state = ENV_DECAY;
          opna_fm_slot_setrate(slot, ENV_DECAY);
        } else {
          slot->env = newenv;
        }
        break;
      case ENV_DECAY:
        slot->env += env_inc;
        sl = slot->sl;
        if (sl == 0xf) sl = 0x1f;
        if (slot->env >= (sl << 5)) {
          slot->env_state = ENV_SUSTAIN;
          opna_fm_slot_setrate(slot, ENV_SUSTAIN);
        }
        break;
      case ENV_SUSTAIN:
        slot->env += env_inc;
        if (slot->env >= LIBOPNA_FM_ENV_MAX) slot->env = LIBOPNA_FM_ENV_MAX;
        break;
      case ENV_RELEASE:
        slot->env += env_inc;
        if (slot->env >= LIBOPNA_FM_ENV_MAX) {
          slot->env = LIBOPNA_FM_ENV_MAX;
          slot->env_state = ENV_OFF;
        }
        break;
      }
      slot->env_hires = slot->env << 2;
    }
  }
  slot->env_count++;
}

void opna_fm_slot_key(struct opna_fm_channel *chan, int slotnum, bool keyon) {
  struct opna_fm_slot *slot = &chan->slot[slotnum];
  //LIBOPNA_DEBUG("%d: %d\n", slotnum, keyon);
  if (keyon) {
    if (!slot->keyon) {
      slot->keyon = true;
      slot->env_state = ENV_ATTACK;
      slot->env_count = 0;
      slot->phase = 0;
      slot->prevout = 0;
      opna_fm_slot_setrate(slot, ENV_ATTACK);
    }
  } else {
    if ((slot->env_state != ENV_OFF) && slot->keyon) {
      slot->keyon = false;
      slot->env_state = ENV_RELEASE;
      opna_fm_slot_setrate(slot, ENV_RELEASE);
    }
  }
}

void opna_fm_slot_set_det(struct opna_fm_slot *slot, unsigned det) {
  det &= 0x7;
  slot->det = det;
}

void opna_fm_slot_set_mul(struct opna_fm_slot *slot, unsigned mul) {
  mul &= 0xf;
  slot->mul = mul;
}

void opna_fm_slot_set_tl(struct opna_fm_slot *slot, unsigned tl) {
  tl &= 0x7f;
  slot->tl = tl;
}

void opna_fm_slot_set_ks(struct opna_fm_slot *slot, unsigned ks) {
  ks &= 0x3;
  slot->ks = ks;
}

void opna_fm_slot_set_ar(struct opna_fm_slot *slot, unsigned ar) {
  ar &= 0x1f;
  slot->ar = ar;
  if (slot->env_state == ENV_ATTACK) {
    opna_fm_slot_setrate(slot, ENV_ATTACK);
  }
}

void opna_fm_slot_set_dr(struct opna_fm_slot *slot, unsigned dr) {
  dr &= 0x1f;
  slot->dr = dr;
  if (slot->env_state == ENV_DECAY) {
    opna_fm_slot_setrate(slot, ENV_DECAY);
  }
}

void opna_fm_slot_set_sr(struct opna_fm_slot *slot, unsigned sr) {
  sr &= 0x1f;
  slot->sr = sr;
  if (slot->env_state == ENV_SUSTAIN) {
    opna_fm_slot_setrate(slot, ENV_SUSTAIN);
  }
}

void opna_fm_slot_set_sl(struct opna_fm_slot *slot, unsigned sl) {
  sl &= 0xf;
  slot->sl = sl;
}

void opna_fm_slot_set_rr(struct opna_fm_slot *slot, unsigned rr) {
  rr &= 0xf;
  slot->rr = rr;
  if (slot->env_state == ENV_RELEASE) {
    opna_fm_slot_setrate(slot, ENV_RELEASE);
  }
}

void opna_fm_chan_set_blkfnum(struct opna_fm_channel *chan, unsigned blk, unsigned fnum) {
  blk &= 0x7;
  fnum &= 0x7ff;
  chan->blk = blk;
  chan->fnum = fnum;
  for (int i = 0; i < 4; i++) {
    chan->slot[i].keycode = blkfnum2keycode(chan->blk, chan->fnum);
    opna_fm_slot_setrate(&chan->slot[i], chan->slot[i].env_state);
  }
}

void opna_fm_chan_set_alg(struct opna_fm_channel *chan, unsigned alg) {
  alg &= 0x7;
  chan->alg = alg;
}

void opna_fm_chan_set_fb(struct opna_fm_channel *chan, unsigned fb) {
  fb &= 0x7;
  chan->fb = fb;
}

void opna_fm_writereg(struct opna_fm *fm, unsigned reg, unsigned val) {
  val &= (1<<8)-1;

  if (reg > 0x1ff) return;

  switch (reg) {
  case 0x27:
    {
      unsigned mode = val >> 6;
      if (mode != fm->ch3.mode) {
//        LIBOPNA_DEBUG("0x27\n");
//        LIBOPNA_DEBUG("  mode = %d\n", mode);
        fm->ch3.mode = mode;
        for (int c = 0; c < 2; c++) {
          unsigned blk, fnum;
          if (fm->ch3.mode == CH3_MODE_NORMAL) {
            blk = fm->channel[2].blk;
            fnum = fm->channel[2].fnum;
          } else {
            blk = fm->ch3.blk[c];
            fnum = fm->ch3.fnum[c];
          }
          fm->channel[2].slot[c].keycode = blkfnum2keycode(blk, fnum);
          opna_fm_slot_setrate(&fm->channel[2].slot[c],
                               fm->channel[2].slot[c].env_state);
        }
      }
    }
    return;
  case 0x28:
    {
//      LIBOPNA_DEBUG("%02x\n", val);
      int c = val & 0x3;
      if (c == 3) return;
      if (val & 0x4) c += 3;
      for (int i = 0; i < 4; i++) {
        bool keyon = val & (1<<(4+i));
        fm->channel[c].slot[i].keyon_ext = keyon;
        if (!keyon) {
          opna_fm_slot_key(&fm->channel[c], i, false);
        }
      }
    }
    return;
  }

  int c = reg & 0x3;
  if (c == 3) return;
  if (reg & (1<<8)) c += 3;
  int s = ((reg & (1<<3)) >> 3) | ((reg & (1<<2)) >> 1);
  struct opna_fm_channel *chan = &fm->channel[c];
  struct opna_fm_slot *slot = &chan->slot[s];
  switch (reg & 0xf0) {
  case 0x30:
    opna_fm_slot_set_det(slot, (val >> 4) & 0x7);
    opna_fm_slot_set_mul(slot, val & 0xf);
    break;
  case 0x40:
    opna_fm_slot_set_tl(slot, val & 0x7f);
    break;
  case 0x50:
    opna_fm_slot_set_ks(slot, (val >> 6) & 0x3);
    opna_fm_slot_set_ar(slot, val & 0x1f);
    break;
  case 0x60:
    opna_fm_slot_set_dr(slot, val & 0x1f);
    break;
  case 0x70:
    opna_fm_slot_set_sr(slot, val & 0x1f);
    break;
  case 0x80:
    opna_fm_slot_set_sl(slot, (val >> 4) & 0xf);
    opna_fm_slot_set_rr(slot, val & 0xf);
    break;
  case 0xa0:
    {
      unsigned blk = (fm->blkfnum_h >> 3) & 0x7;
      unsigned fnum = ((fm->blkfnum_h & 0x7) << 8) | (val & 0xff);
      switch (reg & 0xc) {
      case 0x0:
        if (c != 2 || fm->ch3.mode == CH3_MODE_NORMAL) {
//          LIBOPNA_DEBUG("fnum: ch%d, mode: %d\n", c, fm->ch3.mode);
          opna_fm_chan_set_blkfnum(chan, blk, fnum);
        } else {
//          LIBOPNA_DEBUG("fnum: ch2, slot3\n");
          chan->blk = blk;
          chan->fnum = fnum;
          chan->slot[3].keycode = blkfnum2keycode(blk, fnum);
          opna_fm_slot_setrate(&chan->slot[3], chan->slot[3].env_state);
        }
        break;
      case 0x8:
        c = (c + 2) % 3;
        fm->ch3.blk[c] = blk;
        fm->ch3.fnum[c] = fnum;
        if (fm->ch3.mode != CH3_MODE_NORMAL) {
          fm->channel[2].slot[c].keycode = blkfnum2keycode(blk, fnum);
          opna_fm_slot_setrate(&fm->channel[2].slot[c],
                               fm->channel[2].slot[c].env_state);
        }
        break;
      case 0x4:
      case 0xc:
        fm->blkfnum_h = val & 0x3f;
        break;
      }
    }
    break;
  case 0xb0:
    switch (reg & 0xc) {
    case 0x0:
      opna_fm_chan_set_alg(chan, val & 0x7);
      opna_fm_chan_set_fb(chan, (val >> 3) & 0x7);
      break;
    case 0x4:
      fm->lselect[c] = val & 0x80;
      fm->rselect[c] = val & 0x40;
      break;
    }
    break;
  }
}

#ifdef LIBOPNA_ENABLE_OSCILLO
static int gcd(int a, int b) {
  if (a < b) {
    int t = a;
    a = b;
    b = t;
  }
  for (;;) {
    int r = a % b;
    if (!r) break;
    a = b;
    b = r;
  }
  return b;
}
#endif

void opna_fm_mix(struct opna_fm *fm, int16_t *buf, unsigned samples,
                 struct oscillodata *oscillo, unsigned offset) {
#ifdef LIBOPNA_ENABLE_OSCILLO
  if (oscillo) {
    for (unsigned c = 0; c < 6; c++) {
      const struct opna_fm_channel *ch = &fm->channel[c];
      unsigned freq = blkfnum2freq(ch->blk, ch->fnum);
      int mul[4];
      for (int i = 0; i < 4; i++) {
        mul[i] = ch->slot[i].mul << 1;
        if (!mul[i]) mul[i] = 1;
      }
      freq *= gcd(gcd(gcd(mul[0], mul[1]), mul[2]), mul[3]);
      freq /= 2;
      unsigned period = 0;
      if (freq) period = (1u<<(20+OSCILLO_OFFSET_SHIFT)) / freq;
      if (period) {
        oscillo[c].offset += (samples << OSCILLO_OFFSET_SHIFT);
        oscillo[c].offset %= period;
      } else {
        oscillo[c].offset = 0;
      }
    }
  }
#else
  (void)oscillo;
  (void)offset;
#endif
  unsigned level[6] = {0};
  for (unsigned i = 0; i < samples; i++) {
    if (!fm->env_div3) {
      for (int c = 0; c < 6; c++) {
        for (int s = 0; s < 4; s++) {
          if (fm->channel[c].slot[s].keyon_ext) {
            opna_fm_slot_key(&fm->channel[c], s, true);
            opna_fm_slot_env(&fm->channel[c].slot[s], fm->hires_env);
          }
          //opna_fm_slot_env(&fm->channel[c].slot[s]);
        }
      }
      //LIBOPNA_DEBUG("e %04d\n", fm->channel[0].slot[3].env);
    }
    
    int32_t lo = buf[i*2+0];
    int32_t ro = buf[i*2+1];

    for (int c = 0; c < 6; c++) {
      struct opna_fm_frame o = opna_fm_chanout(&fm->channel[c], fm->hires_sin, fm->hires_env);
      unsigned nlevel[2];
      nlevel[0] = o.data[0] > 0 ? o.data[0] : -o.data[0];
      nlevel[1] = o.data[1] > 0 ? o.data[1] : -o.data[1];
      if (nlevel[1] > nlevel[0]) nlevel[0] = nlevel[1];
      if (nlevel[0] > level[c]) level[c] = nlevel[0];
#ifdef LIBOPNA_ENABLE_OSCILLO
      if (oscillo) oscillo[c].buf[offset+i] = o.data[0] + o.data[1];
#endif
      // TODO: CSM
      if (c == 2 && fm->ch3.mode != CH3_MODE_NORMAL) {
        opna_fm_chan_phase_se(&fm->channel[c], fm);
      } else {
        opna_fm_chan_phase(&fm->channel[c]);
      }
      if (fm->mask & (1<<c)) continue;
      if (fm->lselect[c]) lo += o.data[1];
      if (fm->rselect[c]) ro += o.data[0];
    }

    if (lo < INT16_MIN) lo = INT16_MIN;
    if (lo > INT16_MAX) lo = INT16_MAX;
    if (ro < INT16_MIN) ro = INT16_MIN;
    if (ro > INT16_MAX) ro = INT16_MAX;
    buf[i*2+0] = lo;
    buf[i*2+1] = ro;
    if (lo == 1 || lo == 3 || lo == 5) {
      //if (fm->channel[0].slot[3].env == 0511) {
        //LIBOPNA_DEBUG("l:%6d %4d %4d\n", lo, fm->channel[0].slot[3].phase >> 10, fm->channel[0].slot[3].env);
      //}
    }
    if (!fm->env_div3) {
      for (int c = 0; c < 6; c++) {
        for (int s = 0; s < 4; s++) {
          if (fm->channel[c].slot[s].keyon_ext) {
            fm->channel[c].slot[s].keyon_ext = false;
          } else {
            opna_fm_slot_env(&fm->channel[c].slot[s], fm->hires_env);
          }
          //opna_fm_slot_env(&fm->channel[c].slot[s]);
        }
      }
      fm->env_div3 = 3;
    }
    fm->env_div3--;
  }
#ifdef LIBOPNA_ENABLE_LEVELDATA
  for (int c = 0; c < 6; c++) {
    leveldata_update(&fm->channel[c].leveldata, level[c]);
  }
#endif
}
