/* 
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	Macintosh interface for TiMidity
	by T.Nogami	<t-nogami@happy.email.ne.jp>
	
    mac_com.h
*/

#ifndef	MAC_COM_H
#define	MAC_COM_H

#define SUPPORT_SOUNDSPEC
#undef  PATCH_EXT_LIST
#define PATCH_EXT_LIST { ".pat", 0 }
#define URL_DIR_CACHE_DISABLE

#undef  DEFAULT_RATE
#define DEFAULT_RATE	22050

#define	AU_MACOS
#undef  TILD_SCHEME_ENABLE
#undef  JAPANESE
#define ANOTHER_MAIN
#define DEFAULT_PATH	""
#undef  CONFIG_FILE
#define CONFIG_FILE DEFAULT_PATH "timidity.cfg"
#define MAC_SIGNATURE 'TIMI'
#define MAC_STARTUP_FOLDER_NAME "\pStartup items"

#define ENABLE_SHERRY
#define MAC_SOUNDBUF_QLENGTH (stdQLength*4)

extern int presence_balance;

#include "mac_util.h"
#endif /*MAC_COM_H*/
