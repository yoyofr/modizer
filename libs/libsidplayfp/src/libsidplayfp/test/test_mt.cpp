/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012-2023 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include <cstdlib>
#include <cstring>
#include <cstdlib>

#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
#include <string>

#include <sidplayfp/sidplayfp.h>
#include <sidplayfp/SidTune.h>
#include <sidplayfp/SidInfo.h>
#include <sidplayfp/builders/residfp.h>

/**
 * Compile with
 *     g++ -std=c++11 `pkg-config --cflags libsidplayfp` `pkg-config --libs libsidplayfp` -fopenmp test_mt.cpp
 */

/*
 * Adjust these paths to point to existing ROM dumps if needed.
 */
#define KERNAL_PATH  ""
#define BASIC_PATH   ""
#define CHARGEN_PATH ""

constexpr int SAMPLERATE = 48000;

/*
 * Load ROM dump from file.
 * Allocate the buffer if file exists, otherwise return 0.
 */
char* loadRom(const char* path, size_t romSize)
{
    char* buffer = 0;
    std::ifstream is(path, std::ios::binary);
    if (is.good())
    {
        buffer = new char[romSize];
        is.read(buffer, romSize);
    }
    is.close();
    return buffer;
}

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

riffHeader riffHdr;

wavHeader wavHdr;

const riffHeader defaultRiffHdr =
{
    // ASCII keywords are hexified.
    {0x52,0x49,0x46,0x46}, // 'RIFF'
    {0,0,0,0},             // length
    {0x57,0x41,0x56,0x45}, // 'WAVE'
};

const wavHeader defaultWavHdr =
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

void writeWavHeader()
{
    constexpr unsigned short bits       = 16;
    constexpr unsigned short format     = (bits == 16) ? 1 : 3;
    constexpr unsigned short channels   = 1;
    constexpr unsigned short blockAlign = (bits>>3)*channels;

    // Fill in header with parameters and expected file size.
    riffHdr = defaultRiffHdr;
    endian_little32(riffHdr.length, sizeof(riffHeader)+sizeof(wavHeader)-8);

    wavHdr = defaultWavHdr;
    endian_little16(wavHdr.channels, channels);
    endian_little16(wavHdr.format, 1);
    endian_little32(wavHdr.sampleFreq, SAMPLERATE);
    endian_little32(wavHdr.bytesPerSec, SAMPLERATE*blockAlign);
    endian_little16(wavHdr.blockAlign, blockAlign);
    endian_little16(wavHdr.bitsPerSample, 16);
    endian_little32(wavHdr.dataChunkLen, 0);
}

void run(int i, const char* tuneName)
{
    constexpr int bufferSize = 4096;

    std::string filename;
    filename.append("test_").append(std::to_string(i)).append(".wav");
    std::ofstream handle(filename.c_str(), std::ios::out | std::ios::binary);

    writeWavHeader();
    handle.write((char*)&riffHdr, sizeof(riffHeader));
    handle.write((char*)&wavHdr, sizeof(wavHeader));

    uint_least32_t bufferSamples = static_cast<uint_least32_t>(bufferSize) / sizeof(short);

    // Load tune from file
    std::unique_ptr<SidTune> tune(new SidTune(tuneName));

    // CHeck if the tune is valid
    if (!tune->getStatus())
    {
        std::cerr << tune->statusString() << std::endl;
        exit(EXIT_FAILURE);
    }

    // Select default song
    tune->selectSong(0);

    sidplayfp m_engine;

    // Configure the engine
    SidConfig cfg;
    cfg.frequency = SAMPLERATE;
    cfg.samplingMethod = SidConfig::INTERPOLATE;
    cfg.playback = SidConfig::MONO;
    cfg.sidEmulation = new ReSIDfpBuilder("Test-MT");;

    if (!m_engine.config(cfg))
    {
        std::cerr <<  m_engine.error() << std::endl;
        exit(EXIT_FAILURE);
    }

    // Load tune into engine
    if (!m_engine.load(tune.get()))
    {
        std::cerr <<  m_engine.error() << std::endl;
        exit(EXIT_FAILURE);
    }

    // Play
    m_engine.initMixer(false);

    std::vector<short> buffer(bufferSamples);
    for (int i=0; i<1000; i++)
    {
        int res = m_engine.play(5000);
        if (res < 0)
        {
            std::cerr << m_engine.error() << std::endl;
            break;
        }
        unsigned int s = m_engine.mix(&buffer.front(), res);

        handle.write((char*)&buffer.front(), s*sizeof(short));
    }
    handle.close();
}

/*
 * Multithreading test application.
 */
int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Missing argument" << std::endl;
        exit(EXIT_FAILURE);
    }

    /*
    { // Load ROM files
    char *kernal = loadRom(KERNAL_PATH, 8192);
    char *basic = loadRom(BASIC_PATH, 8192);
    char *chargen = loadRom(CHARGEN_PATH, 4096);

    m_engine.setRoms((const uint8_t*)kernal, (const uint8_t*)basic, (const uint8_t*)chargen);

    delete [] kernal;
    delete [] basic;
    delete [] chargen;
    }
*/
    #pragma omp parallel for
    for (int j = 0; j < 10; j++)
    {
        run(j, argv[1]);
    }
}
