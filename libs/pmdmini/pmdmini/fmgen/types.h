#if !defined(win32_types_h)
#define win32_types_h

#include "headers.h"

//  固定長型とか
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int  uint32;

typedef signed char sint8;
typedef signed short sint16;
typedef signed int sint32;

typedef signed char int8;
typedef signed short int16;
typedef signed int int32;


#ifdef _WIN32
	typedef _int64 int64;

#else
	#define _T(x)			x
	typedef char			TCHAR;

	typedef int64_t			int64;
	typedef int64_t			_int64;

	typedef unsigned char	BYTE;
	typedef unsigned short	WORD;
//	typedef unsigned long	DWORD;
	typedef unsigned int		DWORD;


#endif





#endif // win32_types_h
