/*
  I/O access routines.

  Made by Daisuke Nagano <breeze.nagano@nifty.ne.jp>
  Jan.16.1999

  WARNING! This code only work with Linux.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef CAN_HANDLE_OPL3

#include <stdio.h>
#include <stdlib.h>
#define extern
#define REALLY_SLOW_IO 1

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <sys/io.h>

static int isioopened;
static int opened_address;
static int opened_address_num;

static int regopen_io( int address, int num ) {

  int ret;
  if (ioperm(0x80, 1, 1)!=0) {
    ret=1;
    isioopened = 0;
  } else if (ioperm(address, num, 1)!=0) {
    ret = 1;
    isioopened = 0;
  }
  else {
    opened_address = address;
    opened_address_num = num;
    ret=0;
    isioopened = 1;
  }

  return ret;
}

static int regclose_io(void)
{
  /* nothing to do */
  return 1;
}

static int regwr_io( int address, int val ) {

  if ( isioopened==0 ) return 1;
  if ( opened_address <= address &&
       opened_address+opened_address_num > address ) {
    outb_p(val, address);
  }

  return 0;
}

static int regrd_io( int address ) {

  int ret=0;
  if ( isioopened==0 ) return 0;

  if ( opened_address <= address &&
       opened_address+opened_address_num > address ) {
    ret = inb_p(address);
  }
  else {
    ret = 0;
  }
  return ret;
}

static int regwait_io( int f ) {

  return 0;
  /*
  if ( f== 1 ) {
    inb(0x80);
    inb(0x80);
    inb(0x80);
    inb(0x80);
    inb(0x80);
  }
  else {
    inb(0x80);
    inb(0x80);
  }
  */
}
#endif /* CAN_HANDLE_OPL3 */

/* interfaces */

int regopen( int a , int b ) {
#if defined (CAN_HANDLE_OPL3)
  return regopen_io(a, b);
#else
  return -1;
#endif
}

int regclose(void) {
#if defined (CAN_HANDLE_OPL3)
  return regclose_io();
#else
  return -1;
#endif
}

int regwr( int a , int b ) {
#if defined (CAN_HANDLE_OPL3)
  return regwr_io(a, b);
#else
  return -1;
#endif
}

int regrd( int a ) {
#if defined (CAN_HANDLE_OPL3)
  return regrd_io(a);
#else
  return -1;
#endif
}
