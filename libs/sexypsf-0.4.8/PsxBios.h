/*  Pcsx - Pc Psx Emulator
 *  Copyright (C) 1999-2002  Pcsx Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __PSXBIOS_H__
#define __PSXBIOS_H__

extern char *biosA0n[256];
extern char *biosB0n[256];
extern char *biosC0n[256];

void psxBiosInit();
void psxBiosShutdown();
void psxBiosException();

extern void (*biosA0[256])();
extern void (*biosB0[256])();
extern void (*biosC0[256])();

#endif /* __PSXBIOS_H__ */
