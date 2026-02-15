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

#ifndef WAV_FILE_H
#define WAV_FILE_H

#include <iostream>
#include <string>

#include "../AudioBase.h"

struct riffHeader                       // little endian format
{
    char mainChunkID[4];                // 'RIFF' (ASCII)
    unsigned char length[4];            // file length
    char chunkID[4];                    // 'WAVE' (ASCII)
};

struct wavHeader                        // little endian format
{
    char subChunkID[4];                 // 'fmt ' (ASCII)
    char subChunkLen[4];                // length of subChunk, always 16 bytes
    unsigned char format[2];            // 1 = PCM, 3 = IEEE float

    unsigned char channels[2];          // 1 = mono, 2 = stereo
    unsigned char sampleFreq[4];        // sample-frequency
    unsigned char bytesPerSec[4];       // sampleFreq * blockAlign
    unsigned char blockAlign[2];        // bytes per sample * channels
    unsigned char bitsPerSample[2];

    char dataChunkID[4];                // keyword, begin of data chunk; = 'data' (ASCII)

    unsigned char dataChunkLen[4];      // length of data
};

struct listInfo                         // little endian format
{
    char mainChunkID[4];                // 'LIST' (ASCII)

    unsigned char length[4];            // chunk length

    char chunkID[4];                    // 'INFO' (ASCII)
    char namChunkID[4];                 // 'INAM' (ASCII)
    char namChunkLen[4];                // length of subChunk, always 32 bytes
    char name[32];

    char artChunkID[4];                 // 'IART' (ASCII)
    char artChunkLen[4];                // length of subChunk, always 32 bytes
    char artist[32];

    char copChunkID[4];                 // 'ICOP' (ASCII)
    char copChunkLen[4];                // length of subChunk, always 32 bytes
    char released[32];
};

/*
 * A basic WAV output file type
 * Initial implementation by Michael Schwendt <mschwendt@yahoo.com>
 */
class WavFile: public AudioBase
{
private:
    std::string name;

    unsigned long int dataSize;

    static const riffHeader defaultRiffHdr;
    riffHeader riffHdr;

    static const wavHeader defaultWavHdr;
    wavHeader wavHdr;

    static const listInfo defaultListInfo;
    listInfo listHdr;

    std::ostream *file;
    bool headerWritten;
    bool hasListInfo;
    int m_precision;
    int m_channels;

public:
    WavFile(const std::string &name);
    ~WavFile() override { close(); }

    static const char *extension () { return ".wav"; }

    // Only signed 16-bit and 32bit float samples are supported.
    // Endian-ess is adjusted if necessary.
    //
    // If number of sample bytes is given, this can speed up the
    // process of closing a huge file on slow storage media.

    bool open(AudioConfig &cfg) override;

    // After write call old buffer is invalid and you should
    // use the new buffer provided instead.
    bool write(uint_least32_t frames) override;
    void close() override;
    void pause() override {}
    void reset() override {}

    // Stream state.
    bool fail() const { return (file->fail() != 0); }
    bool bad()  const { return (file->bad()  != 0); }

    void setInfo(const char* title, const char* author, const char* released);
};

#endif /* WAV_FILE_H */
