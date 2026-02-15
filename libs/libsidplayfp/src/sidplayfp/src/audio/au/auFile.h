/*
 * This file is part of sidplayfp, a SID player.
 *
 * Copyright 2011-2018 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000-2004 Simon White
 * Copyright 2000 Michael Schwendt
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

#ifndef AU_FILE_H
#define AU_FILE_H

#include <iostream>
#include <string>

#include "../AudioBase.h"

struct auHeader                         // little endian format
{
    char mainChunkID[4];                // '.snd' (ASCII)

    unsigned char dataOffset[4];        // data offset

    unsigned char dataSize[4];          // data size
    unsigned char encoding[4];          // 3 = 16-bit linear PCM, 6 = 32-bit IEEE floating point

    unsigned char sampleRate[4];        // sample rate
    unsigned char channels[4];          // 1 = mono, 2 = stereo
};

/*
 * A basic AU output file type
 */
class auFile: public AudioBase
{
private:
    std::string name;

    unsigned long int byteCount;

    static const auHeader defaultAuHdr;
    auHeader auHdr;

    std::ostream *file;
    bool headerWritten;
    int m_precision;
    int m_channels;

public:
    auFile(const std::string &name);
    ~auFile() override { close(); }

    static const char *extension () { return ".au"; }

    // Only signed 16-bit and 32bit float samples are supported.
    // Endian-ess is adjusted if necessary.

    bool open(AudioConfig &cfg) override;

    // After write call old buffer is invalid and you should
    // use the new buffer provided instead.
    bool write(uint_least32_t size) override;
    void close() override;
    void pause() override {}
    void reset() override {}

    // Stream state.
    bool fail() const { return (file->fail() != 0); }
    bool bad()  const { return (file->bad()  != 0); }
};

#endif /* AU_FILE_H */
