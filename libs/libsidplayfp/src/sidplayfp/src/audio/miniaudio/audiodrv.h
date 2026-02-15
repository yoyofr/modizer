/*
 * This file is part of sidplayfp, a console SID player.
 *
 * Copyright 2000-2005 Simon White
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

#ifndef AUDIO_MINIAUDIO_H
#define AUDIO_MINIAUDIO_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "osaudio.h"

#include "../AudioBase.h"


class Audio_Miniaudio: public AudioBase
{
private:  // ------------------------------------------------------- private
    osaudio_t m_audioHandle;

private:
    void outOfOrder();

public:  // --------------------------------------------------------- public
    Audio_Miniaudio();
    ~Audio_Miniaudio() override;

    bool open  (AudioConfig &cfg) override;
    void close () override;
    void reset () override {}
    bool write (uint_least32_t frames) override;
    void pause () override {}
};

#endif // AUDIO_ALSA_H
