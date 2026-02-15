/*
 * This file is part of sidplayfp, a console SID player.
 *
 * Copyright 2013-2024 Leandro Nini
 * Copyright 2000-2006 Simon White
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

#include "audiodrv.h"

#include <new>

Audio_Miniaudio::Audio_Miniaudio() :
    AudioBase("MINIAUDIO")
{
    // Reset everything.
    outOfOrder();
}

Audio_Miniaudio::~Audio_Miniaudio()
{
    close();
}

void Audio_Miniaudio::outOfOrder()
{
    // Reset everything.
    clearError();
    m_audioHandle = nullptr;
}

bool Audio_Miniaudio::open(AudioConfig &cfg)
{
    if (m_audioHandle != nullptr)
    {
        setError("Device already in use");
        return false;
    }

    osaudio_config_t config;
    osaudio_config_init(&config, OSAUDIO_OUTPUT);
    config.format   = OSAUDIO_FORMAT_S16;
    config.channels = cfg.channels;
    config.rate     = cfg.frequency;

    int res = osaudio_open(&m_audioHandle, &config);
    if (res != OSAUDIO_SUCCESS) {
        if (res == OSAUDIO_FORMAT_NOT_SUPPORTED)
            setError("Audio format not supported.");
        else
            setError("Failed to open audio device.");
        return false;
    }

    try
    {
        m_sampleBuffer = new short[cfg.bufSize*cfg.channels];
    }
    catch (std::bad_alloc const &ba)
    {
        setError("Unable to allocate memory for sample buffers.");
        return false;
    }

    // Force precision
    cfg.precision = 16;
    // Setup internal Config
    m_settings = cfg;
    return true;
}

// Close an opened audio device, free any allocated buffers and
// reset any variables that reflect the current state.
void Audio_Miniaudio::close()
{
    if (m_audioHandle != nullptr)
    {
        osaudio_close(m_audioHandle);
        delete[] m_sampleBuffer;
        outOfOrder();
    }
}

bool Audio_Miniaudio::write(uint_least32_t frames)
{
    if (m_audioHandle == nullptr)
    {
        setError("Device not open.");
        return false;
    }

    int res = osaudio_write(m_audioHandle, m_sampleBuffer, frames);
    if (res != OSAUDIO_SUCCESS && res != OSAUDIO_XRUN)
    {
        setError("Error writing to audio device.");
        return false;
    }
    if (res == OSAUDIO_XRUN)
    {
        // An underrun or overrun occurred
    }
    return true;
}
