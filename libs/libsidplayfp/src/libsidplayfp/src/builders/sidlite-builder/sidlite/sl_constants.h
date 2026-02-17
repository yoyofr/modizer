/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright (C) 2025-2026 Leandro Nini
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

// Based on cRSID lightweight RealSID by Hermit (Mihaly Horvath)

#ifndef SIDLITE_CONSTANTS_H
#define SIDLITE_CONSTANTS_H

namespace SIDLite
{

constexpr int SID_CHANNEL_COUNT = 3;

//attenuates wave-generator output not to overdrive resampler-input (and maybe filter-input):
constexpr int CRSID_WAVGEN_PRESHIFT = 3;
constexpr int CRSID_WAVGEN_PREDIV = 1 << CRSID_WAVGEN_PRESHIFT; //shift-value can be 1..4 (1..16x division)

}

#endif
