#ifndef _WIN32

#ifndef APE_NOWINDOWS_H
#define APE_NOWINDOWS_H

#include "MACUtils.h"

#define FALSE    0
#define TRUE     1

#define NEAR
#define FAR

#define __forceinline inline

#ifdef MACLIB_COMPILE

typedef unsigned long       uint64;
typedef long                int64;
typedef unsigned int        uint32;
typedef int                 int32;
typedef unsigned short      uint16;
typedef short               int16;
typedef unsigned char       uint8;
typedef char                int8;
typedef int                 BOOL;

#endif

typedef char                str_ansi;
typedef unsigned char       str_utf8;
typedef wchar_t             str_utf16;


typedef unsigned long       DWORD;

typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef float               FLOAT;
typedef void *              HANDLE;
typedef unsigned int        UINT;
typedef unsigned int        WPARAM;
typedef long                LPARAM;
typedef const char *        LPCSTR;
typedef char *              LPSTR;
typedef long                LRESULT;
typedef unsigned char       UCHAR;
typedef const wchar_t *     LPCWSTR;

#define ZeroMemory(POINTER, BYTES) memset(POINTER, 0, BYTES);
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))

#define __stdcall
#define CALLBACK

#define _stricmp strcasecmp
#define _strnicmp strncasecmp

#ifdef HAVE_WCSCASECMP

#define wcsnicmp wcsncasecmp
#define _wcsicmp wcscasecmp
#define wcsicmp wcscasecmp

#else

#define wcsnicmp mac_wcsncasecmp
#define _wcsicmp mac_wcscasecmp
#define wcsicmp mac_wcscasecmp

#endif // HAVE_WCSCASECMP

#define _wtoi(ws) wcstol(ws, NULL, 2)

#include <locale.h> 

#define _FPOSOFF(fp) ((long)(fp).__pos)
#define MAX_PATH    4096

#ifndef _WAVEFORMATEX_
#define _WAVEFORMATEX_

typedef struct tWAVEFORMATEX
{
    uint16        wFormatTag;         /* format type */
    uint16        nChannels;          /* number of channels (i.e. mono, stereo...) */
    uint32       nSamplesPerSec;     /* sample rate */
    uint32       nAvgBytesPerSec;    /* for buffer estimation */
    uint16        nBlockAlign;        /* block size of data */
    uint16        wBitsPerSample;     /* number of bits per sample of mono data */
    uint16        cbSize;             /* the count in bytes of the size of */
                    /* extra information (after cbSize) */
} WAVEFORMATEX, *PWAVEFORMATEX, NEAR *NPWAVEFORMATEX, FAR *LPWAVEFORMATEX;
typedef const WAVEFORMATEX FAR *LPCWAVEFORMATEX;

#endif // #ifndef _WAVEFORMATEX_

#endif // #ifndef APE_NOWINDOWS_H

#endif // #ifndef _WIN32
