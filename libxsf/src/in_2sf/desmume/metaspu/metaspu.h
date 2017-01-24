/*  Copyright 2009-2010 DeSmuME team

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

// -------------------------
// this file contains the METASPU system
// which is designed to handle the task of audio synchronization
// and is designed to be as portable between multiple emulators
// -------------------------

#pragma once

#include "../types.h"

class ISynchronizingAudioBuffer
{
public:
	virtual ~ISynchronizingAudioBuffer() { }

	virtual void enqueue_samples(int16_t *buf, int samples_provided) = 0;

	// returns the number of samples actually supplied, which may not match the number requested
	virtual int output_samples(int16_t *buf, int samples_requested) = 0;
};

enum ESynchMode
{
	ESynchMode_DualSynchAsynch,
	ESynchMode_Synchronous
};

enum ESynchMethod
{
	ESynchMethod_N, // nitsuja's
	ESynchMethod_Z, // zero's
	ESynchMethod_P // PCSX2 spu2-x
};

ISynchronizingAudioBuffer *metaspu_construct(ESynchMethod method);
