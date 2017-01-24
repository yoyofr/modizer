////////////////////////////////////////////////////////////////////////////////
///
/// A header file for detecting the Intel MMX instructions set extension.
///
/// Please see 'mmx_win.cpp', 'mmx_cpp.cpp' and 'mmx_non_x86.cpp' for the
/// routine implementations for x86 Windows, x86 gnu version and non-x86
/// platforms, respectively.
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// Last changed  : $Date: 2006/02/05 16:44:06 $
// File revision : $Revision: 1.4 $
//
// $Id: cpu_detect.h,v 1.4 2006/02/05 16:44:06 Olli Exp $
//
////////////////////////////////////////////////////////////////////////////////
//
// License :
//
//  SoundTouch audio processing library
//  Copyright (c) Olli Parviainen
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "STTypes.h"

const uint32_t SUPPORT_MMX = 0x0001;
const uint32_t SUPPORT_3DNOW = 0x0002;
const uint32_t SUPPORT_SSE = 0x0004;

/// Checks which instruction set extensions are supported by the CPU.
///
/// \return A bitmask of supported extensions, see SUPPORT_... defines.
uint32_t detectCPUextensions();

/// Disables given set of instruction extensions. See SUPPORT_... defines.
void disableExtensions(uint32_t wDisableMask);
