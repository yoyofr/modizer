/*
 * This file is part of sidplayfp, a console SID player.
 *
 * Copyright 2021-2025 Leandro Nini
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SIDLIB_FEATURES_H
#define SIDLIB_FEATURES_H

#include <sidplayfp/sidplayfp.h>

#if LIBSIDPLAYFP_VERSION_MAJ > 2 || (LIBSIDPLAYFP_VERSION_MAJ == 2 && LIBSIDPLAYFP_VERSION_MIN >= 2)
#  define FEAT_REGS_DUMP_SID
#  define FEAT_DB_WCHAR_OPEN
#endif

#if LIBSIDPLAYFP_VERSION_MAJ > 2 || (LIBSIDPLAYFP_VERSION_MAJ == 2 && LIBSIDPLAYFP_VERSION_MIN >= 7)
#  define FEAT_FILTER_RANGE
#  define FEAT_CW_STRENGTH
#endif

#if LIBSIDPLAYFP_VERSION_MAJ > 2 || (LIBSIDPLAYFP_VERSION_MAJ == 2 && LIBSIDPLAYFP_VERSION_MIN >= 10)
#  define FEAT_SAMPLE_MUTE
#  define FEAT_FILTER_DISABLE
#endif

#if LIBSIDPLAYFP_VERSION_MAJ > 2 || (LIBSIDPLAYFP_VERSION_MAJ == 2 && LIBSIDPLAYFP_VERSION_MIN >= 14)
#  define FEAT_NEW_PLAY_API
#endif

#if LIBSIDPLAYFP_VERSION_MAJ > 2 || (LIBSIDPLAYFP_VERSION_MAJ == 2 && LIBSIDPLAYFP_VERSION_MIN >= 16)
#  define FEAT_SID_MODEL
#endif

#if LIBSIDPLAYFP_VERSION_MAJ >= 3
#  define FEAT_NO_CREATE
#endif

#endif
