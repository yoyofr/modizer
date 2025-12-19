#ifndef APE_WINDOWS_ENVIRONMENT_H
#define APE_WINDOWS_ENVIRONMENT_H

/*****************************************************************************************
Windows version
*****************************************************************************************/
#ifndef WINVER
#ifndef __MINGW32__
    #define WINVER         0x0400
    #define _WIN32_WINNT   0x0400
    #define _WIN32_WINDOWS 0x0400
    #define _WIN32_IE      0x0400
#endif
#endif

/*****************************************************************************************
Unicode
*****************************************************************************************/
#ifndef UNICODE
    #define UNICODE
#endif
#ifndef _UNICODE
    #define _UNICODE
#endif

/*****************************************************************************************
Visual Studio 2008 defines
*****************************************************************************************/
#ifndef __MINGW32__
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS
#endif

#endif // #APE_WINDOWS_ENVIRONMENT_H
