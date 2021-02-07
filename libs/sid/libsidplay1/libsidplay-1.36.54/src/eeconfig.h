//
// /home/ms/source/sidplay/libsidplay/eeconfig.h,v
//

#ifndef SIDPLAY1_EECONFIG_H
#define SIDPLAY1_EECONFIG_H


#include "compconf.h"
#ifdef SID_HAVE_EXCEPTIONS
#include <new>
#endif
#include <cmath>
#include <ctime>

#include "emucfg.h"
#include "6510_.h"
#include "mytypes.h"
#include "opstruct.h"
#include "samples.h"
#include "version.h"

extern sbyte* ampMod1x8;       // -> 6581_.cpp

extern sbyte* signedPanMix8;   // -> 6581_.cpp
extern sword* signedPanMix16;  //


#endif  /* SIDPLAY1_EECONFIG_H */
