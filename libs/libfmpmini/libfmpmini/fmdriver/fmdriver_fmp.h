#ifndef MYON_FMDRIVER_FMP_H_INCLUDED
#define MYON_FMDRIVER_FMP_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "fmdriver.h"
#include <stddef.h>

enum {
  FMP_PART_FM_1,
  FMP_PART_FM_2,
  FMP_PART_FM_3,
  FMP_PART_FM_4,
  FMP_PART_FM_5,
  FMP_PART_FM_6,
  FMP_PART_FM_EX1,
  FMP_PART_FM_EX2,
  FMP_PART_FM_EX3,
  FMP_PART_PPZ8_1,
  FMP_PART_PPZ8_2,
  FMP_PART_PPZ8_3,
  FMP_PART_PPZ8_4,
  FMP_PART_PPZ8_5,
  FMP_PART_PPZ8_6,
  FMP_PART_PPZ8_7,
  FMP_PART_PPZ8_8,
  FMP_PART_RESERVED,
  FMP_PART_SSG_1,
  FMP_PART_SSG_2,
  FMP_PART_SSG_3,
  FMP_PART_ADPCM,
  FMP_PART_NUM
};

enum {
  FMP_RHYTHM_NUM = 6
};

enum {
  FMP_COMMENT_BUFLEN = 256,
};

enum {
  FMP_DATA_FM_1,
  FMP_DATA_FM_2,
  FMP_DATA_FM_3,
  FMP_DATA_FM_4,
  FMP_DATA_FM_5,
  FMP_DATA_FM_6,
  FMP_DATA_SSG_1,
  FMP_DATA_SSG_2,
  FMP_DATA_SSG_3,
  FMP_DATA_RHYTHM,
  FMP_DATA_ADPCM,
  FMP_DATA_FM_EX1,
  FMP_DATA_FM_EX2,
  FMP_DATA_FM_EX3,
  FMP_DATA_NUM
};

enum {
  FMP_LFO_P = 0,
  FMP_LFO_Q = 1,
  FMP_LFO_R = 2,
};

enum {
  FMP_LFOWF_TRIANGLE = 0,
  FMP_LFOWF_SAWTOOTH = 1,
  FMP_LFOWF_SQUARE = 2,
  FMP_LFOWF_LINEAR = 3,
  FMP_LFOWF_STAIRCASE = 4,
  FMP_LFOWF_TRIANGLE2 = 5,
  FMP_LFOWF_RANDOM = 6,
};

struct fmp_lfo {
  // 0016
  uint8_t delay;
  // 0017
  uint8_t speed;
  // 0018
  uint8_t delay_cnt;
  // 0019
  uint8_t speed_cnt;
  // 001a
  uint8_t depth;
  // 001b
  uint8_t depth_cnt;
  // 001c
  uint16_t rate;
  // 001e
  uint16_t rate2;
  // 0020
  uint8_t waveform;
};

struct fmp_ssgenv {
  // 0056
  uint8_t startvol;
  // 0057
  uint8_t attack_rate;
  // 0058
  uint8_t decay_rate;
  // 0059
  uint8_t sustain_lv;
  // 005a
  uint8_t sustain_rate;
  // 005b
  uint8_t release_rate;
};

enum {
  PDZF_ENV_VOL_MIN = -15,
};

struct fmp_part {
  // 0000
  struct {
    // 0x01
    bool mask: 1;
    // 0x02
    bool lfo: 1; // ?
    // 0x04
    bool e: 1;
    // 0x08
    bool w: 1;
    // 0x10
    bool a: 1;
    // 0x20
    bool r: 1;
    // 0x40
    bool q: 1;
    // 0x80
    bool p: 1;
  } lfo_f;
  // 0001
  uint8_t default_len;
  // 0002
  uint8_t current_vol;
  // 0003
  // q command without Q
  // noteoff when equal to gate_cmp
  uint8_t gate_cmp;
  // 0004
  uint8_t tonelen_cnt;
  // 0005
  uint8_t gate_cnt;
  // 0006
  uint8_t actual_vol;
  // 0007
  // command k
  uint8_t keyon_delay;
  // 0008
  uint8_t keyon_delay_cnt;
  // 0009
  uint8_t prev_note;
  // 000a
  struct {
    // 0x01
    bool keyon: 1;
    // 0x02
    bool slur: 1;
    // 0x04
    bool tie_cont: 1;
    // 0x08
    bool off: 1;
    // 0x10
    bool lfo_sync: 1;
    // 0x20
    bool pitchbend: 1;
    // 0x40
    bool rest: 1;
    // 0x80
    bool tie: 1;
  } status;
  // 000b
  uint8_t sync;
  // 000c
  uint16_t detune;
  struct {
    // 000e
    uint8_t rate;
    // 000f
    uint8_t delay;
    // 0010
    uint8_t speed;
    // 0011
    uint8_t speed_cnt;
    // 0012
    uint16_t target_freq;
    // 0014
    uint8_t target_note;
  } pit;
  // 0016
  struct fmp_lfo lfo[3];
  // 003a
  uint16_t actual_freq;
  // 003c
  uint16_t prev_freq;
  // 003e
  uint8_t note_diff;
  // 003f
  uint8_t tone;
  // 0040
  uint16_t ext_keyon;
  // 0042
  uint8_t pan_ams_pms;
  // 0043
  uint8_t slot_vol_mask;
  // 0046
  uint16_t current_ptr;
  // 0048
  uint16_t loop_ptr;
  // 004a
  struct {
    // 0x01
    bool fm: 1;
    // 0x02
    bool fm_3: 1;
    // 0x04
    bool ssg: 1;
    // 0x08
    bool rhythm: 1;
    // 0x10
    bool adpcm: 1;
    // 0x20
    // SSG backup??
  } type;
  // 0b00xx x0xx xxxx xxxx
  //     || | || |||| ||||
  //     || | || |||| |||`- FM#1
  //     || | || |||| ||`-- FM#2
  //     || | || |||| |`--- FM#3
  //     || | || |||| `---- FM#4
  //     || | || |||`------ FM#5
  //     || | || ||`------- FM#6
  //     || | || |`-------- SSG#1
  //     || | || `--------- SSG#2
  //     || | |`----------- SSG#3
  //     || | `------------ rhythm
  //     || `-------------- FMEX#1
  //     |`---------------- FMEX#2
  //     `----------------- FMEX#3
  // 004c
  uint16_t part_bit;
  // 004e
  //   SSG:    1420
  //   FM:     141e
  //   ADPCM:  1422
  //   RHYTHM: 1424
  // 0050
  uint8_t opna_keyon_out; // output this to 0x28
  // when SSG, this can be PPZ8 channel num
  
  // 0051
  uint8_t eff_chan;
  union {
    struct {
      // 0052 (ptr)
      uint8_t tone_tl[4];
      struct {
        // 0054
        uint8_t delay;
        // 0055
        uint8_t speed;
        // 0056
        uint8_t delay_cnt;
        // 0057
        uint8_t speed_cnt;
        // 0058
        uint8_t depth;
        // 0059
        uint8_t depth_cnt;
        // 005a
        uint8_t rate;
        // 005b
        uint8_t rate_orig;
      } alfo;
      // 005c
      struct fmp_wlfo {
        // 005c
        uint8_t delay;
        // 005d
        uint8_t speed;
        // 005e
        uint8_t delay_cnt;
        // 005f
        uint8_t speed_cnt;
        // 0060
        uint8_t depth;
        // 0061
        uint8_t depth_cnt;
        // 0062
        uint8_t rate;
        // 0063
        uint8_t rate_orig;
        // 0064
        uint8_t rate_curr;
        // 0065
        uint8_t sync;
      } wlfo;
      // 0066
      uint8_t hlfo_delay;
      // 0067
      uint8_t hlfo_delay_cnt;
      // 0068
      // value written to register 0x22
      uint8_t hlfo_freq;
      // 0069
      uint8_t hlfo_apms;
      // 006a
      // 0b1234????
      uint8_t slot_mask;
      // 006b
      uint8_t slot_rel_vol[4];
    } fm;
    struct {
      // 0052
      uint8_t curr_vol;
      // 0053
      struct {
        // 0x01
        bool ppz: 1;
        // 0x02
        bool noise: 1;
        // 0x04
        bool portamento: 1; // ??
        // 0x10
        bool release: 1;
        // 0x20
        bool sustain: 1;
        // 0x40
        bool decay: 1;
        // 0x80
        bool attack: 1;
      } env_f;
      /*
      struct {
        // 0x01
        //bool 1: 1;
      } env_flag;
      */
      // 0054
      uint8_t octave;
      // 0055
      uint8_t vol;
      // 0056
      struct fmp_ssgenv env;
      // pointer at 005e
      struct fmp_ssgenv envbak;
    } ssg;
    struct {
      // 0052
      uint16_t startaddr;
      // 0054
      uint16_t endaddr;
      // 0056
      uint16_t deltat;
    } adpcm;
  } u;

  uint8_t tonelen;
  struct {
    uint32_t loopstart32;
    uint32_t loopend32;
    uint8_t mode;
    uint8_t ppz8_channel;
    uint8_t voice;
    uint8_t vol;
    uint8_t prev_note;
    int8_t pan;
    struct {
      uint8_t al;
      int8_t dd;
      uint8_t sr;
      uint8_t rr;
    } env_param;
    struct {
      enum {
        PDZF_ENV_ATT,
        PDZF_ENV_DEC,
        PDZF_ENV_REL,
        PDZF_ENV_OFF,
      } status;
      uint8_t cnt;
      int8_t vol;
    } env_state;
    bool keyon;
    // detune = (pitchdiff*64) + (pitchdiff*(2**(octave-5)))
    // octave = prevnote/0xc;
    struct {
      bool on;
      uint8_t rate;
      uint8_t delay;
      uint8_t speed;
      uint8_t speed_cnt;
      uint32_t target_freq;
      uint8_t target_note;
      int32_t pitchdiff;
    } pit;
    int32_t lfodiff;
    uint32_t lastfreq;
  } pdzf;
};

struct fmp_rhythm {
  // 0acf
  uint8_t mask;
  // 0ad0
  uint8_t len_cnt;
  // 0ad1
  uint8_t default_len;
  // 0ad2
  uint8_t tl_volume;
  // 0ad3
  uint8_t volumes[FMP_RHYTHM_NUM];
  // 0ad9
  uint8_t pans[FMP_RHYTHM_NUM];
  // 0adf
  //uint8_t loop_now
  // 0ae0
  uint8_t sync;
  // 0ae1
  bool status;
  struct fmp_part part;
};

struct driver_fmp {
  const uint8_t *data;
  uint16_t datalen;
  // 0103
  uint8_t timerb;
  struct {
    // 0104
    uint8_t data;
    // 0105
    uint16_t cnt;
  } sync;
  // 010d
  struct {
    // 0x4000
    bool looped: 1;
    // 0x8000
    bool stopped: 1;
  } status;
  // 010f
  uint16_t total_clocks;
  // 0111
  uint8_t clock_divider;
  // 0112
  uint8_t fm_vol;
  // 011a
  uint8_t loop_cnt;
  // 011d
  uint8_t timerb_bak;
  // 011e
  // 0b00FEDFED
  uint8_t ssg_mix;
  // 011f
  // output this to 0x27 to reset timer
  uint8_t timer_ch3;
  // 0120
  uint8_t ssg_noise_freq;
  // 012f
  uint8_t ssg_mix_se;
  // 0131
  bool se_proc;
  // 0b54
  uint8_t loop_dec;
  // 0b55
  uint8_t loop_times;
  // 0b5e
  uint8_t data_version;
  // 0b6c
  uint8_t bar_tickcnt;
  // 0b92
  uint8_t fm3_slot_keyon;
  // 0b93
  uint8_t fm3_alg;
  // 0ba4
  // see fmp_part::part_bit
  uint16_t part_playing_bit;
  // 0ba6
  // see fmp_part::part_bit
  uint16_t part_loop_bit;
  // 0b9c
  int16_t ch3_se_freqdiff[4];

  // 0cee
  uint16_t adpcm_deltat;
  // 0cf0
  uint8_t adpcm_c1;
  // 0cf6
  uint16_t adpcm_startaddr[0x80];
  uint16_t adpcm_endaddr[0x80];
  // 0ff6
  // title
  
  // filename without extension .PVI
  char ppz_name[9];
  char pvi_name[9];

  struct fmp_part parts[FMP_PART_NUM];
  struct fmp_rhythm rhythm;
  struct {
    uint16_t partptr[FMP_DATA_NUM];
    uint16_t loopptr[FMP_DATA_NUM];
    // 4ebe (005e)
    // default len: bar / 4
    uint8_t bar;
    uint16_t fmtoneptr;
    uint16_t ssgtoneptr;
    uint16_t adpcmptr;
    // *4ebf & 0x01
    struct {
      // 0x4ebf
      // 0x01
      bool q: 1;
      // 0x02
      bool ppz: 1;
      // 0x04
      bool lfo_octave_fix: 1;
    } flags;
  } datainfo;
  uint8_t rand71;
  
  struct {
    // when 0, no PDZF
    // when 1, PDZF Normal mode
    // when 2, PDZF Enhanced mode
    uint8_t mode;
    struct pdzf_rhythm {
      uint8_t voice[2];
      int8_t pan;
      uint8_t note;
      bool enabled;
    } rhythm[2];
    uint8_t rhythm_current_note;
  } pdzf;

  uint8_t comment[3][FMP_COMMENT_BUFLEN];
};

// first: call fmp_load with zero_initialized struct driver_fmp and data
// returns true if valid data
// warning: will overwrite data during playback
bool fmp_load(struct driver_fmp *fmp, uint8_t *data, uint16_t datalen);
// then call fmp_init
// this will set the fmp pointer to fmdriver_work::driver
// this function will access opna
void fmp_init(struct fmdriver_work *work, struct driver_fmp *fmp);
// load adpcm data
// this function will access opna
bool fmp_adpcm_load(struct fmdriver_work *work,
                    uint8_t *data, size_t datalen);

// 1da8
// 6190: fmp external characters

#ifdef __cplusplus
}
#endif

#endif // MYON_FMDRIVER_FMP_H_INCLUDED
