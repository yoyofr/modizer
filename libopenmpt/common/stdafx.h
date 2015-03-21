/*
 * StdAfx.h
 * --------
 * Purpose: Include file for standard system include files, or project specific include files that are used frequently, but are changed infrequently. Also includes the global build settings from BuildSettings.h.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


// has to be first
#include "BuildSettings.h"


#if defined(MODPLUG_TRACKER)

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcmn.h>			// MFC support for Windows Common Controls
#include <afxcview.h>
#include <afxdlgs.h>
#include <afxole.h>

#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <mmsystem.h>

#endif // MODPLUG_TRACKER


#if MPT_COMPILER_MSVC
#include <intrin.h>
#endif


#if defined(MPT_USE_WINDOWS_H)
#include <windows.h>
#endif

// this will be available everywhere

#include "../common/typedefs.h"
// <memory>
// <new>
// <type_traits> // if available
// <cstdarg>
// <cstdint>
// <stdint.h>

#include "../common/mptString.h"
// <limits>
// <string>
// <type_traits>
// <cstring>

#include "../common/mptPathString.h"

#include "../common/Logging.h"

#include "../common/misc_util.h"
// <limits>
// <string>
// <type_traits>
// <vector>
// <cmath>
// <cstring>
// <time.h>

// for std::abs
#include <cstdlib>
#include <stdlib.h>

#ifndef MODPLUG_NO_FILESAVE
// for FILE* definition (which cannot be forward-declared in a portable way)
#include <stdio.h>
#endif


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
