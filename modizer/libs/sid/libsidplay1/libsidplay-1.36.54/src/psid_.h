//
// /home/ms/source/sidplay/libsidplay/fformat/RCS/psid_.h,v
//

#ifndef SIDPLAY1_PSID__H
#define SIDPLAY1_PSID__H


#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstring>

#include "mytypes.h"
#include "myendian.h"
#include "sidtune.h"

struct psidHeader
{
    //
    // All values in big-endian order.
    //
    char id[4];          // 'PSID'
    ubyte version[2];    // 0x0001 or 0x0002
    ubyte data[2];       // 16-bit offset to binary data in file
    ubyte load[2];       // 16-bit C64 address to load file to
    ubyte init[2];       // 16-bit C64 address of init subroutine
    ubyte play[2];       // 16-bit C64 address of play subroutine
    ubyte songs[2];      // number of songs
    ubyte start[2];      // start song (1-256 !)
    ubyte speed[4];      // 32-bit speed info
                         // bit: 0=50 Hz, 1=CIA 1 Timer A (default: 60 Hz)
    char name[32];       // ASCII strings, 31 characters long and
    char author[32];     // terminated by a trailing zero
    char copyright[32];  //
    ubyte flags[2];      // only version 0x0002
    ubyte relocStartPage;
    ubyte relocPages;
    ubyte reserved[2];   // only version 0x0002
};


#endif  /* SIDPLAY1_PSID__H */
