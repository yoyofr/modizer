/*
 * This file is part of sidplayfp, a console SID player.
 *
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

#ifndef AUDIODRV_H
#define AUDIODRV_H

#include "IAudio.h"

#include <memory>

#include "AudioBase.h"

class audioDrv : public IAudio
{
private:
    std::unique_ptr<AudioBase> audio;

public:
    ~audioDrv() override = default;

    bool open(AudioConfig &cfg) override;
    void reset() override { audio->reset(); }
    bool write(uint_least32_t frames) override { return audio->write(frames); }
    void close() override { audio->close(); }
    void pause() override { audio->pause(); }
    short *buffer() const override { return audio->buffer(); }
    void clearBuffer() override { audio->clearBuffer(); }
    void getConfig(AudioConfig &cfg) const override { audio->getConfig(cfg); }
    const char *getErrorString() const override { return audio->getErrorString(); }
    const char *getDriverString() const override { return audio->getDriverString(); }
};

#endif // AUDIODRV_H
