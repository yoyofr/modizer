//
// /home/ms/source/sidplay/libsidplay/include/RCS/fformat.h,v
//

#ifndef SIDPLAY1_FFORMAT_H
#define SIDPLAY1_FFORMAT_H


#include "compconf.h"
#include "mytypes.h"

#ifdef SID_HAVE_SSTREAM
#include <sstream>
using std::istringstream;
#else
#include <strstream.h>
#endif

// Wrapper for ``strnicmp'' without third argument.
extern int myStrNcaseCmp(const char* s1, const char* s2);

// Wrapper for ``stricmp''.
extern int myStrCaseCmp(const char* s1, const char* s2);

// Own version of strdup, which uses new instead of malloc.
extern char* myStrDup(const char *source);

// Return pointer to file name position in complete path.
extern char* fileNameWithoutPath(char* s);

// Return pointer to file name position in complete path.
// Special version: file separator = forward slash.
extern char* slashedFileNameWithoutPath(char* s);

// Return pointer to file name extension in path.
// Searching backwards until first dot is found.
extern char* fileExtOfPath(char* s);

// Parse input string stream. Read and convert a hexa-decimal number up 
// to a ``,'' or ``:'' or ``\0'' or end of stream.
#ifdef SID_HAVE_SSTREAM
extern udword readHex(istringstream& parseStream);
#else
extern udword readHex(istrstream& parseStream);
#endif

// Parse input string stream. Read and convert a decimal number up 
// to a ``,'' or ``:'' or ``\0'' or end of stream.
#ifdef SID_HAVE_SSTREAM
extern udword readDec(istringstream& parseStream);
#else
extern udword readDec(istrstream& parseStream);
#endif

// Search terminated string for next newline sequence.
// Skip it and return pointer to start of next line.
extern const char* returnNextLine(const char* pBuffer);

// Skip any characters in an input string stream up to '='.
#ifdef SID_HAVE_SSTREAM
extern void skipToEqu(istringstream& parseStream);
#else
extern void skipToEqu(istrstream& parseStream);
#endif

// Start at first character behind '=' and copy rest of string.
extern void copyStringValueToEOL(const char* pSourceStr, char* pDestStr, int destMaxLen);


#endif  /* SIDPLAY1_FFORMAT_H */
