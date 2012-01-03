/*
  I/O access routines.

  Made by Daisuke Nagano <breeze.nagano@nifty.ne.jp>
  Jan.16.1999

  WARNING! This code only work with Linux.
 */

#ifndef _IOACCESS_H_
#define _IOACCESS_H_

extern int regopen( int, int );
extern int regclose( void );
extern int regwr( int, int );
extern int regrd( int );

#endif /* _IOACCESS_H_ */
