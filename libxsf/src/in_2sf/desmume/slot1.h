/*
	Copyright (C) 2010-2011 DeSmuME team

	This file is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with the this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "types.h"

struct SLOT1INTERFACE
{
	// The name of the plugin, this name will appear in the plugins list
	const char *name;

	// called once when the plugin starts up
	bool (*init)();

	// called when the emulator resets
	void (*reset)();

	// called when the plugin shuts down
	void (*close)();

	// called when the user configurating plugin
	void (*config)();

	// called when the emulator write to addon
	void (*write08)(uint8_t PROCNUM, uint32_t adr, uint8_t val);
	void (*write16)(uint8_t PROCNUM, uint32_t adr, uint16_t val);
	void (*write32)(uint8_t PROCNUM, uint32_t adr, uint32_t val);

	// called when the emulator read from addon
	uint8_t  (*read08)(uint8_t PROCNUM, uint32_t adr);
	uint16_t (*read16)(uint8_t PROCNUM, uint32_t adr);
	uint32_t (*read32)(uint8_t PROCNUM, uint32_t adr);

	// called when the user get info about addon pak (description)
	void (*info)(char *info);
};

extern SLOT1INTERFACE slot1_device; // current slot1 device

enum NDS_SLOT1_TYPE
{
	NDS_SLOT1_RETAIL,
	NDS_SLOT1_COUNT // use for counter addons - MUST TO BE LAST!!!
};
