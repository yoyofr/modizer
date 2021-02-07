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
*/

#ifndef ___X_SRY_H_
#define ___X_SRY_H_

extern void x_sry_wrdt_apply(uint8 *data, int len);
extern void CloseSryWindow(void);
extern int OpenSryWindow(char *opts);
extern void x_sry_redraw_ctl(int);
extern void x_sry_close(void);
extern void x_sry_update(void);
extern void x_sry_event(void);

#endif /* ___X_SRY_H_ */
