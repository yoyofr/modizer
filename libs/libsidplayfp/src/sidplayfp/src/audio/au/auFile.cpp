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

#include "auFile.h"

#include <vector>
#include <iomanip>
#include <fstream>
#include <new>

/// Set the lo byte (8 bit) in a word (16 bit)
inline void endian_16lo8 (uint_least16_t &word, uint8_t byte)
{
    word &= 0xff00;
    word |= byte;
}

/// Get the lo byte (8 bit) in a word (16 bit)
inline uint8_t endian_16lo8 (uint_least16_t word)
{
    return (uint8_t) word;
}

/// Set the hi byte (8 bit) in a word (16 bit)
inline void endian_16hi8 (uint_least16_t &word, uint8_t byte)
{
    word &= 0x00ff;
    word |= (uint_least16_t) byte << 8;
}

/// Set the hi byte (8 bit) in a word (16 bit)
inline uint8_t endian_16hi8 (uint_least16_t word)
{
    return (uint8_t) (word >> 8);
}

/// Swap word endian.
inline void endian_16swap8 (uint_least16_t &word)
{
    uint8_t lo = endian_16lo8 (word);
    uint8_t hi = endian_16hi8 (word);
    endian_16lo8 (word, hi);
    endian_16hi8 (word, lo);
}

/// Convert high-byte and low-byte to 16-bit word.
inline uint_least16_t endian_16 (uint8_t hi, uint8_t lo)
{
    uint_least16_t word = 0;
    endian_16lo8 (word, lo);
    endian_16hi8 (word, hi);
    return word;
}

inline void endian_16 (uint8_t ptr[2], uint_least16_t word)
{
#if defined(WORDS_BIGENDIAN)
    ptr[0] = endian_16hi8 (word);
    ptr[1] = endian_16lo8 (word);
#else
    ptr[0] = endian_16lo8 (word);
    ptr[1] = endian_16hi8 (word);
#endif
}

inline void endian_16 (char ptr[2], uint_least16_t word)
{
    endian_16 ((uint8_t *) ptr, word);
}

/// Convert high-byte and low-byte to 16-bit big endian word.
inline uint_least16_t endian_big16 (const uint8_t ptr[2])
{
    return endian_16 (ptr[0], ptr[1]);
}

/// Write a little-big 16-bit word to two bytes in memory.
inline void endian_big16 (uint8_t ptr[2], uint_least16_t word)
{
    ptr[0] = endian_16hi8 (word);
    ptr[1] = endian_16lo8 (word);
}

/// Set the hi word (16bit) in a dword (32 bit)
inline void endian_32hi16 (uint_least32_t &dword, uint_least16_t word)
{
    dword &= (uint_least32_t) 0x0000ffff;
    dword |= (uint_least32_t) word << 16;
}

/// Get the hi word (16bit) in a dword (32 bit)
inline uint_least16_t endian_32hi16 (uint_least32_t dword)
{
    return (uint_least16_t) (dword >> 16);
}

/// Set the lo byte (8 bit) in a dword (32 bit)
inline void endian_32lo8 (uint_least32_t &dword, uint8_t byte)
{
    dword &= (uint_least32_t) 0xffffff00;
    dword |= (uint_least32_t) byte;
}

/// Get the lo byte (8 bit) in a dword (32 bit)
inline uint8_t endian_32lo8 (uint_least32_t dword)
{
    return (uint8_t) dword;
}

/// Set the hi byte (8 bit) in a dword (32 bit)
inline void endian_32hi8 (uint_least32_t &dword, uint8_t byte)
{
    dword &= (uint_least32_t) 0xffff00ff;
    dword |= (uint_least32_t) byte << 8;
}

/// Get the hi byte (8 bit) in a dword (32 bit)
inline uint8_t endian_32hi8 (uint_least32_t dword)
{
    return (uint8_t) (dword >> 8);
}

/// Convert high-byte and low-byte to 32-bit word.
inline uint_least32_t endian_32 (uint8_t hihi, uint8_t hilo, uint8_t hi, uint8_t lo)
{
    uint_least32_t dword = 0;
    uint_least16_t word  = 0;
    endian_32lo8  (dword, lo);
    endian_32hi8  (dword, hi);
    endian_16lo8  (word,  hilo);
    endian_16hi8  (word,  hihi);
    endian_32hi16 (dword, word);
    return dword;
}

/// Convert high-byte and low-byte to 32-bit big endian word.
inline uint_least32_t endian_big32 (const uint8_t ptr[4])
{
    return endian_32 (ptr[0], ptr[1], ptr[2], ptr[3]);
}

// Write a big-endian 32-bit word to four bytes in memory.
inline void endian_big32 (uint8_t ptr[4], uint_least32_t dword)
{
    uint_least16_t word = endian_32hi16 (dword);
    ptr[0] = endian_16hi8 (word);
    ptr[1] = endian_16lo8 (word);
    ptr[2] = endian_32hi8 (dword);
    ptr[3] = endian_32lo8 (dword);
}

const auHeader auFile::defaultAuHdr =
{
    // ASCII keywords are hexified.
    {0x2e,0x73,0x6e,0x64}, // '.snd'
    {0,0,0,24},            // data offset
    {0,0,0,0},             // data size
    {0,0,0,0},             // encoding
    {0,0,0,0},             // Samplerate
    {0,0,0,0},             // Channels
};

auFile::auFile(const std::string &name) :
    AudioBase("AUFILE"),
    name(name),
    auHdr(defaultAuHdr),
    file(nullptr),
    headerWritten(false),
    m_precision(32)
{}

bool auFile::open(AudioConfig &cfg)
{
    m_precision = cfg.precision;
    m_channels = cfg.channels;

    unsigned short bits       = m_precision;
    unsigned long  format     = (m_precision == 16) ? 3 : 6;
    unsigned long  channels   = m_channels;
    unsigned long  freq       = cfg.frequency;
    unsigned short blockAlign = (bits>>3)*channels;
    unsigned long  bufSize    = freq * blockAlign;
    cfg.bufSize = freq;

    if (name.empty())
        return false;

    if (file && !file->fail())
        close();

    byteCount = 0;

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
    endian_big32(auHdr.encoding, format);
    endian_big32(auHdr.sampleRate, freq);
    endian_big32(auHdr.channels, channels);

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

bool auFile::write(uint_least32_t frames)
{
    if (file && !file->fail())
    {
        uint_least32_t size = frames * m_channels;
        unsigned long int bytes = size;
        if (!headerWritten)
        {
            file->write((char*)&auHdr, sizeof(auHeader));
            headerWritten = true;
        }

        if (m_precision == 16)
        {
            std::vector<uint_least16_t> buffer(size);
            bytes *= 2;
            for (unsigned long i=0; i<size; i++)
            {
                uint_least16_t temp = m_sampleBuffer[i];
                buffer[i] = endian_big16((uint8_t*)&temp);
            }
            file->write((char*)&buffer.front(), bytes);
        }
        else
        {
            std::vector<float> buffer(size);
            bytes *= 4;
            // normalize floats
            for (unsigned long i=0; i<size; i++)
            {
                float temp = ((float)m_sampleBuffer[i])/32768.f;
                buffer[i] = endian_big32((uint8_t*)&temp);
            }
            file->write((char*)&buffer.front(), bytes);
        }
        byteCount += bytes;

    }
    return true;
}

void auFile::close()
{
    if (file && !file->fail())
    {
        // update length field in header
        endian_big32(auHdr.dataSize, byteCount);
        if (file != &std::cout)
        {
            file->seekp(0, std::ios::beg);
            file->write((char*)&auHdr, sizeof(auHeader));
            delete file;
        }
        file = nullptr;
        delete[] m_sampleBuffer;
    }
}
