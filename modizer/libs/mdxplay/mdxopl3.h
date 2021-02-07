/*
  MDXplayer :  OPL3 access routines

  Made by Daisuke Nagano <breeze.nagano@nifty.ne.jp>
  Jan.16.1999
 */

#ifndef _MDXOPL3_H_
#define _MDXOPL3_H_

extern int opl3_open( void );
extern void opl3_close( void );
extern void opl_reg_init( MDX_DATA * );

extern void opl3_all_note_off( void );
extern void opl3_note_on( int, int );
extern void opl3_note_off( int );
extern void opl3_set_pan( int, int );
extern void opl3_set_volume( int, int );
extern void opl3_set_detune( int, int );
extern void opl3_set_portament( int, int );
extern void opl3_set_noise_freq( int );
extern void opl3_set_voice( int, VOICE_DATA * );
extern void opl3_set_freq_reg( int, int );

extern void opl3_set_plfo( int, int, int, int, int );
extern void opl3_set_alfo( int, int, int, int, int );
extern void opl3_set_lfo_delay( int, int );

extern void opl3_set_freq_volume( int );
extern void opl3_set_master_volume( int );

#endif /* _MDXOPL3_H_ */
