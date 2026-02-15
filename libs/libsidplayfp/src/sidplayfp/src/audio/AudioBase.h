/*
 * This file is part of sidplayfp, a console SID player.
 *
 * Copyright 2013-2016 Leandro Nini
 * Copyright 2000 Simon White
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

#ifndef AUDIOBASE_H
#define AUDIOBASE_H

#include <string>
#include <cstring>

#include "IAudio.h"
#include "AudioConfig.h"

#include "sidcxx11.h"

class AudioBase : public IAudio
{
protected:
    class error
    {
    private:
        const char* m_msg;

    public:
        error(const char* msg) : m_msg(msg) {}
        const char* message() const { return m_msg; }
    };

private:
    const char *m_backendName;
    std::string m_errorString;

protected:
    AudioConfig m_settings;
    short      *m_sampleBuffer;

protected:
    void setError(const char* msg)
    {
        m_errorString.assign(m_backendName).append(" ERROR: ").append(msg);
    }

    void clearError()
    {
        m_errorString.clear();
    }

public:
    AudioBase(const char* name) :
        m_backendName(name),
        m_sampleBuffer(nullptr) {}
    ~AudioBase() override = default;

    short *buffer() const override { return m_sampleBuffer; }

    void clearBuffer() override { std::memset(m_sampleBuffer, 0, m_settings.getBufBytes()); }

    void getConfig(AudioConfig &cfg) const override
    {
        cfg = m_settings;
    }

    const char *getErrorString() const override
    {
        return m_errorString.c_str();
    }

    const char *getDriverString() const override
    {
        return m_backendName;
    }
};

#endif // AUDIOBASE_H
