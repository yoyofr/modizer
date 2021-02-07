//
// /home/ms/source/sidplay/libsidplay/fformat/RCS/sid_.h,v
//

#ifndef SIDPLAY1_SID__H
#define SIDPLAY1_SID__H


#include "compconf.h"
#ifdef SID_HAVE_EXCEPTIONS
#include <new>
#endif
#include <fstream>
#include <iostream>
#include <iomanip>
using namespace std;
#ifdef SID_HAVE_SSTREAM
#include <sstream>
using std::istringstream;
#else
#include <strstream.h>
#endif
#include <string>
using std::string;
#include <cctype>
#include <cstring>
#include "mytypes.h"
#include "myendian.h"
#include "fformat.h"
#include "sidtune.h"


#endif  /* SIDPLAY1_SID__H */
