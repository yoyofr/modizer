//
// /home/ms/source/sidplay/libsidplay/fformat/RCS/fformat_.cpp,v
//

#include "compconf.h"
#ifdef SID_HAVE_EXCEPTIONS
#include <new>
#endif
#include <cctype>
#include <cstring>

#include "fformat.h"
#include "myendian.h"

// Wrapper for ``strnicmp'' without third argument.
int myStrNcaseCmp(const char* s1, const char* s2)
{
#ifdef SID_HAVE_STRNICMP
    return strnicmp( s1, s2, strlen(s2) );
#else
#ifdef SID_HAVE_STRNCASECMP
    return strncasecmp( s1, s2, strlen(s2) );
#else
    // Simple replacement that works with old compilers too.
    char* tmp1 = myStrDup( s1 );
    char* tmp2 = myStrDup( s2 );
    char* tmp1bak = tmp1;
    char* tmp2bak = tmp2;
    while ( *tmp1!=0 )
    {
        *tmp1 = toupper(*tmp1);
        ++tmp1;
    }
    while ( *tmp2!=0 )
    {
        *tmp2 = toupper(*tmp2);
        ++tmp2;
    }
    tmp1 = tmp1bak;
    tmp2 = tmp2bak;
    int ret = strncmp( tmp1, tmp2, strlen(tmp2) );
    delete[] tmp2;
    delete[] tmp1;
    return ret;
#endif
#endif
}

// Wrapper for ``stricmp''.
int myStrCaseCmp(const char* s1, const char* s2)
{
#ifdef SID_HAVE_STRICMP
    return stricmp( s1, s2 );
#else
#ifdef SID_HAVE_STRCASECMP
    return strcasecmp( s1, s2 );
#else
    // Simple replacement that works with old compilers too.
    char* tmp1 = myStrDup( s1 );
    char* tmp2 = myStrDup( s2 );
    char* tmp1bak = tmp1;
    char* tmp2bak = tmp2;
    while ( *tmp1!=0 )
    {
        *tmp1 = toupper(*tmp1);
        ++tmp1;
    }
    while ( *tmp2!=0 )
    {
        *tmp2 = toupper(*tmp2);
        ++tmp2;
    }
    tmp1 = tmp1bak;
    tmp2 = tmp2bak;
    int ret = strcmp( tmp1, tmp2 );
    delete[] tmp2;
    delete[] tmp1;
    return ret;
#endif
#endif
}

// Own version of strdup, which uses new instead of malloc.
char* myStrDup(const char *source)
{
	char *dest;
#ifdef SID_HAVE_EXCEPTIONS
	if ( (dest = new(std::nothrow) char[strlen(source)+1]) != 0)
#else
	if ( (dest = new char[strlen(source)+1]) != 0)
#endif
	{
		strcpy(dest,source);
	}
	return dest;
}

// Return pointer to file name position in complete path.
char* fileNameWithoutPath(char* s)
{
	int last_slash_pos = -1;
	for ( uint pos = 0; pos < strlen(s); pos++ )
	{
#if defined(FS_IS_COLON_AND_BACKSLASH)
		if ( s[pos] == ':' || s[pos] == '\\' )
#elif defined(FS_IS_COLON_AND_SLASH)
		if ( s[pos] == ':' || s[pos] == '/' )
#elif defined(FS_IS_SLASH)
        if ( s[pos] == '/' )
#elif defined(FS_IS_BACKSLASH)
		if ( s[pos] == '\\' )
#elif defined(FS_IS_COLON)
		if ( s[pos] == ':' )
#else
#error Missing file/path separator definition in config.h.
#endif
		{
			last_slash_pos = pos;
		}
	}
	return( &s[last_slash_pos +1] );
}

// Return pointer to file name position in complete path.
// Special version: file separator = forward slash.
char* slashedFileNameWithoutPath(char* s)
{
	int last_slash_pos = -1;
	for ( uint pos = 0; pos < strlen(s); pos++ )
	{
        if ( s[pos] == '/' )
		{
			last_slash_pos = pos;
		}
	}
	return( &s[last_slash_pos +1] );
}

// Return pointer to file name extension in path.
// The backwards-version.
char* fileExtOfPath(char* s)
{
	uint last_dot_pos = strlen(s);  // assume no dot and append
	for ( int pos = last_dot_pos; pos >= 0; --pos )
	{
		if ( s[pos] == '.' )
		{
			last_dot_pos = pos;
			break;
		}
	}
	return( &s[last_dot_pos] );
}

// Parse input string stream. Read and convert a hexa-decimal number up 
// to a ``,'' or ``:'' or ``\0'' or end of stream.
#ifdef SID_HAVE_SSTREAM
udword readHex( istringstream& hexin )
#else
udword readHex( istrstream& hexin )
#endif
{
	udword hexLong = 0;
	char c;
	do
	{
		hexin >> c;
		if ( !hexin )
			break;
		if (( c != ',') && ( c != ':' ) && ( c != 0 ))
		{
			// machine independed to_upper
			c &= 0xdf;
			( c < 0x3a ) ? ( c &= 0x0f ) : ( c -= ( 0x41 - 0x0a ));
			hexLong <<= 4;
			hexLong |= (udword)c;
		}
		else
		{
			if ( c == 0 )
				hexin.putback(c);
			break;
		}
	}  while ( hexin );
	return hexLong;
}

// Parse input string stream. Read and convert a decimal number up 
// to a ``,'' or ``:'' or ``\0'' or end of stream.
#ifdef SID_HAVE_SSTREAM
udword readDec( istringstream& decin )
#else
udword readDec( istrstream& decin )
#endif
{
	udword hexLong = 0;
	char c;
	do
	{
		decin >> c;
		if ( !decin )
			break;
		if (( c != ',') && ( c != ':' ) && ( c != 0 ))
		{
			c &= 0x0f;
			hexLong *= 10;
			hexLong += (udword)c;
		}
		else
		{ 
			if ( c == 0 )
				decin.putback(c);
			break;
		}
	}  while ( decin );
	return hexLong;
}

// Search terminated string for next newline sequence.
// Skip it and return pointer to start of next line.
const char* returnNextLine(const char* s)
{
	// Unix: LF = 0x0A
	// Windows, DOS: CR,LF = 0x0D,0x0A
	// Mac: CR = 0x0D
	char c;
	while ((c = *s) != 0)
	{
		s++;                            // skip read character
		if (c == 0x0A)
		{
			break;                      // LF found
		}
		else if (c == 0x0D)
		{
			if (*s == 0x0A)
			{
				s++;                    // CR,LF found, skip LF
			}
			break;                      // CR or CR,LF found
		}
	}
	if (*s == 0)                        // end of string ?
	{
		return 0;                       // no next line available
	}
	return s;                           // next line available
}

// Skip any characters in an input string stream up to '='.
#ifdef SID_HAVE_SSTREAM
void skipToEqu( istringstream& parseStream )
#else
void skipToEqu( istrstream& parseStream )
#endif
{
	char c;
	do
	{
		parseStream >> c;
	}
	while ( c != '=' );
}

void copyStringValueToEOL(const char* pSourceStr, char* pDestStr, int DestMaxLen )
{
	// Start at first character behind '='.
	while ( *pSourceStr != '=' )
	{
		pSourceStr++;
	}
	pSourceStr++;  // Skip '='.
	while (( DestMaxLen > 0 ) && ( *pSourceStr != 0 )
		   && ( *pSourceStr != '\n' ) && ( *pSourceStr != '\r' ))
	{
		*pDestStr++ = *pSourceStr++;
		DestMaxLen--;
	}
	*pDestStr++ = 0;
}
