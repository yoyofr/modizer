#ifndef WIN_HEADERS_H
#define WIN_HEADERS_H


#ifndef STRICT
#define STRICT
#endif

#define WIN32_LEAN_AND_MEAN

#include "portability.h"

#ifdef _WIN32
	#include <windows.h>
	#include <objbase.h>
	#include <TCHAR.H>
	
#else
	#include <unistd.h>
	#include <fcntl.h>
	#include <sys/stat.h>
//	#include <jni.h>
#endif



#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

/*
#ifdef _MSC_VER
	#undef max
	#define max _MAX
	#undef min
	#define min _MIN
#endif
*/

#endif	// WIN_HEADERS_H
