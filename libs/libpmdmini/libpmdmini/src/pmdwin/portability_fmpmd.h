#ifndef PORTABILITY_FMPMD_H
#define PORTABILITY_FMPMD_H

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <cassert>
#include <climits>
#include <cwchar>

#ifdef __APPLE__
	#ifndef PATH_MAX
		#ifdef _POSIX_PATH_MAX
			#define PATH_MAX (_POSIX_PATH_MAX)
		#endif
	#endif
#endif



#ifdef _WIN32

	#if _MSC_VER >= 1600

		#include <cstdint>

	#elif _MSC_VER >= 1310

		typedef unsigned char		uint8_t;
		typedef unsigned short		uint16_t;
		typedef unsigned int		uint32_t;
		typedef unsigned long long	uint64_t;
		typedef signed char			int8_t;
		typedef signed short		int16_t;
		typedef signed int			int32_t;
		typedef signed long long	int64_t;

	#else
		typedef unsigned char		uint8_t;
		typedef unsigned short		uint16_t;
		typedef unsigned int		uint32_t;
		typedef unsigned __int64	uint64_t;
		typedef signed char			int8_t;
		typedef signed short		int16_t;
		typedef signed int			int32_t;
		typedef __int64				int64_t;

	#endif

#else

	//	呼出規約の無効化

	#define	__stdcall
	#define	WINAPI
	#define __cdecl
	
	#define _T(x)			x
	typedef char			TCHAR;

	#define	_MAX_PATH	((PATH_MAX) > ((FILENAME_MAX)*4) ? (PATH_MAX) : ((FILENAME_MAX)*4))
	#define	_MAX_DIR	FILENAME_MAX
	#define	_MAX_FNAME	FILENAME_MAX
	#define	_MAX_EXT	FILENAME_MAX

	#include <cstdint>

#endif


#endif	// PORTABILITY_FMPMD_H
