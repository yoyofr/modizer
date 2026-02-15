/*
 * This file is part of sidplayfp, a console SID player.
 *
 * Copyright 2000 Simon White
 * Copyright 2013-2026 Leandro Nini
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "AudioDrv.h"

#include "miniaudio/audiodrv.h"

#include "null/null.h"

#include <iostream>

bool audioDrv::open(AudioConfig &cfg)
{
    bool res = false;
    {
        audio.reset(new Audio_Miniaudio());
        res = audio->open(cfg);
    }
    if (!res)
    {
        audio.reset(new Audio_Null());
        res = audio->open(cfg);
    }

    return res;
}
