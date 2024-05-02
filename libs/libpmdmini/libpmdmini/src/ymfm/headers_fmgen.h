#ifndef HEADERS_FMGEN_H
#define HEADERS_FMGEN_H


#ifndef STRICT
#define STRICT
#endif

#define WIN32_LEAN_AND_MEAN

#ifdef _WIN32
	#include <windows.h>
	#include <objbase.h>
	#include <tchar.h>
	
#else
	#include <sys/types.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include <sys/stat.h>
#endif



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

/*
#ifdef _MSC_VER
	#undef max
	#define max _MAX
	#undef min
	#define min _MIN
#endif
*/

#endif	// HEADERS_FMGEN_H
