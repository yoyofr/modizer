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

#include "WavFile.h"

#include <vector>
#include <iomanip>
#include <fstream>
#include <new>

#include <cstring>

// Get the lo byte (8 bit) in a dword (32 bit)
inline uint8_t endian_32lo8 (uint_least32_t dword)
{
    return (uint8_t) dword;
}

// Get the hi byte (8 bit) in a dword (32 bit)
inline uint8_t endian_32hi8 (uint_least32_t dword)
{
    return (uint8_t) (dword >> 8);
}

// Get the hi word (16bit) in a dword (32 bit)
inline uint_least16_t endian_32hi16 (uint_least32_t dword)
{
    return (uint_least16_t) (dword >> 16);
}

// Get the lo byte (8 bit) in a word (16 bit)
inline uint8_t endian_16lo8 (uint_least16_t word)
{
    return (uint8_t) word;
}

// Set the hi byte (8 bit) in a word (16 bit)
inline uint8_t endian_16hi8 (uint_least16_t word)
{
    return (uint8_t) (word >> 8);
}

// Write a little-endian 16-bit word to two bytes in memory.
inline void endian_little16 (uint8_t ptr[2], uint_least16_t word)
{
    ptr[0] = endian_16lo8 (word);
    ptr[1] = endian_16hi8 (word);
}

// Write a little-endian 32-bit word to four bytes in memory.
inline void endian_little32 (uint8_t ptr[4], uint_least32_t dword)
{
    uint_least16_t word = 0;
    ptr[0] = endian_32lo8  (dword);
    ptr[1] = endian_32hi8  (dword);
    word   = endian_32hi16 (dword);
    ptr[2] = endian_16lo8  (word);
    ptr[3] = endian_16hi8  (word);
}

const riffHeader WavFile::defaultRiffHdr =
{
    // ASCII keywords are hexified.
    {0x52,0x49,0x46,0x46}, // 'RIFF'
    {0,0,0,0},             // length
    {0x57,0x41,0x56,0x45}, // 'WAVE'
};

const wavHeader WavFile::defaultWavHdr =
{
    {0x66,0x6d,0x74,0x20}, // 'fmt '
    {16,0,0,0},            // length
    {1,0},                 // AudioFormat (PCM)
    {0,0},                 // Channels
    {0,0,0,0},             // Samplerate
    {0,0,0,0},             // ByteRate
    {0,0},                 // BlockAlign
    {0,0},                 // BitsPerSample
    {0x64,0x61,0x74,0x61}, // 'data'
    {0,0,0,0}              // length
};

const listInfo WavFile::defaultListInfo =
{
    // ASCII keywords are hexified.
    {0x4C,0x49,0x53,0x54}, // 'LIST'
    {124,0,0,0},           // length
    {0x49,0x4E,0x46,0x4F}, // 'INFO'
    {0x49,0x4E,0x41,0x4D}, // 'INAM'
    {32,0,0,0},            // length
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0x49,0x41,0x52,0x54}, // 'IART'
    {32,0,0,0},            // length
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0x49,0x43,0x4F,0x50}, // 'ICOP'
    {32,0,0,0},            // length
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

WavFile::WavFile(const std::string &name) :
    AudioBase("WAVFILE"),
    name(name),
    riffHdr(defaultRiffHdr),
    wavHdr(defaultWavHdr),
    listHdr(defaultListInfo),
    file(nullptr),
    headerWritten(false),
    hasListInfo(false),
    m_precision(32)
{}

bool WavFile::open(AudioConfig &cfg)
{
    m_precision = cfg.precision;
    m_channels = cfg.channels;

    unsigned short bits       = m_precision;
    unsigned short format     = (m_precision == 16) ? 1 : 3;
    unsigned short channels   = m_channels;
    unsigned long  freq       = cfg.frequency;
    unsigned short blockAlign = (bits>>3)*channels;
    unsigned long  bufSize    = freq * blockAlign;
    cfg.bufSize = freq;

    if (name.empty())
        return false;

    if (file && !file->fail())
        close();

    dataSize = 0;

    // We need to make a buffer for the user
    try
    {
        m_sampleBuffer = new short[bufSize/2];
    }
    catch (std::bad_alloc const &ba)
    {
        setError("Unable to allocate memory for sample buffers.");
        return false;
    }

    // Fill in header with parameters and expected file size.
    endian_little32(riffHdr.length, sizeof(riffHeader)+sizeof(wavHeader)-8);
    endian_little16(wavHdr.channels, channels);
    endian_little16(wavHdr.format, format);
    endian_little32(wavHdr.sampleFreq, freq);
    endian_little32(wavHdr.bytesPerSec, freq*blockAlign);
    endian_little16(wavHdr.blockAlign, blockAlign);
    endian_little16(wavHdr.bitsPerSample, bits);
    endian_little32(wavHdr.dataChunkLen, 0);

    if (name.compare("-") == 0)
    {
        file = &std::cout;
    }
    else
    {
        file = new std::ofstream(name.c_str(), std::ios::out|std::ios::binary|std::ios::trunc);
    }

    m_settings = cfg;
    return true;
}

bool WavFile::write(uint_least32_t frames)
{
    if (file && !file->fail())
    {
        uint_least32_t size = frames * m_channels;
        unsigned long int bytes = size;
        if (!headerWritten)
        {
            file->write((char*)&riffHdr, sizeof(riffHeader));
            if (hasListInfo)
                file->write((char*)&listHdr, sizeof(listInfo));
            file->write((char*)&wavHdr, sizeof(wavHeader));
            headerWritten = true;
        }

        /* XXX endianness... */
        if (m_precision == 16)
        {
            bytes *= 2;
            file->write((char*)m_sampleBuffer, bytes);
        }
        else
        {
            std::vector<float> buffer(size);
            bytes *= 4;
            // normalize floats
            for (unsigned long i=0; i<size; i++)
            {
                buffer[i] = ((float)m_sampleBuffer[i])/32768.f;
            }
            file->write((char*)&buffer.front(), bytes);
        }
        dataSize += bytes;
    }
    return true;
}

void WavFile::close()
{
    if (file && !file->fail())
    {
        // update length fields in header
        unsigned long int headerSize = sizeof(riffHeader)+sizeof(wavHeader)-8;
        if (hasListInfo)
            headerSize += sizeof(listInfo);
        endian_little32(riffHdr.length, headerSize+dataSize);
        endian_little32(wavHdr.dataChunkLen, dataSize);
        if (file != &std::cout)
        {
            file->seekp(0, std::ios::beg);
            file->write((char*)&riffHdr, sizeof(riffHeader));
            if (hasListInfo)
                file->write((char*)&listHdr, sizeof(listInfo));
            file->write((char*)&wavHdr, sizeof(wavHeader));
            delete file;
        }
        file = nullptr;
        delete[] m_sampleBuffer;
    }
}

void WavFile::setInfo(const char* title, const char* author, const char* released)
{
    hasListInfo = true;
    std::memcpy(listHdr.name, title, 32);
    std::memcpy(listHdr.artist, author, 32);
    std::memcpy(listHdr.released, released, 32);
}
