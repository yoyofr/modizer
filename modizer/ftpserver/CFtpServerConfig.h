/**@file
 * @brief      CFtpServer - Custom compilation
 *
 * @author     Mail :: thebrowser@gmail.com
 *
 * @copyright  Copyright (C) 2007 Julien Poumailloux
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 * 4. If you like this software, a fee would be appreciated, particularly if
 *    you use this software in a commercial product, but is not required.
 */

// Compilation Options !:


/**
 * @def  CFTPSERVER_ENABLE_EXTRACMD
 * Enable or disable extra commands support.
 * Default: Disabled.
 */
//#define CFTPSERVER_ENABLE_EXTRACMD

/**
 * @def  CFTPSERVER_ENABLE_EVENTS
 * Enable or disable events.
 */
#define CFTPSERVER_ENABLE_EVENTS

/**
 * @def  CFTPSERVER_ENABLE_ZLIB
 * Enable or disable data transfer compression support using the Zlib library.
 */
//#define CFTPSERVER_ENABLE_ZLIB

/**
 * @def  CFTPSERVER_ZLIB_H_PATH
 * Define the path to the Zlib header file.
 */
#define CFTPSERVER_ZLIB_H_PATH		"../zlib-1.2.3/zlib.h"

/**
 * @def  CFTPSERVER_ENABLE_IPV6
 * Enable or disable IPV6 support.
 * @note  Not yet implemented.
 */
//#define CFTPSERVER_ENABLE_IPV6 to be implemented.

/**
 * @def  __USE_FILE_OFFSET64
 * Enable or disable large file support under Win32.
 * @note  Under Linux and other stuff, just add -D_FILE_OFFSET_BITS=64 to the compilation line.
 */
#ifdef WIN32
	#define __USE_FILE_OFFSET64
#endif
