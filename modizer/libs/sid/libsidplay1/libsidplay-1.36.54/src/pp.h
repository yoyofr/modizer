//
// /home/ms/source/sidplay/libsidplay/include/RCS/pp.h,v
//

#ifndef SIDPLAY1_PP_H
#define SIDPLAY1_PP_H


#include <fstream>
using std::ifstream;
#include "mytypes.h"

extern bool depp(ifstream& inputFile, ubyte** destBufRef);
extern bool ppIsCompressed();
extern udword ppUncompressedLen();
extern const char* ppErrorString;


#endif  /* SIDPLAY1_PP_H */
