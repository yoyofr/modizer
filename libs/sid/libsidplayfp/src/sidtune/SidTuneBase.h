/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2021 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
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

#ifndef SIDTUNEBASE_H
#define SIDTUNEBASE_H

#include <stdint.h>
#include <memory>
#include <vector>
#include <string>

#include "sidplayfp/SidTuneInfo.h"
#include "sidplayfp/siddefs.h"

#include "SmartPtr.h"
#include "SidTuneInfoImpl.h"

#include "sidcxx11.h"

namespace libsidplayfp
{

class sidmemory;
template <class T> class SmartPtr_sidtt;

/**
 * loadError
 */
class loadError
{
private:
    const char* m_msg;
public:
    loadError(const char* msg) : m_msg(msg) {}
    const char* message() const { return m_msg; }
};

/**
 * SidTuneBaseBase
 */
class SidTuneBase
{
protected:
    typedef std::vector<uint8_t> buffer_t;

protected:
    /// Also PSID file format limit.
    static const unsigned int MAX_SONGS = 256;

    // Generic error messages
    static const char ERR_TRUNCATED[];
    static const char ERR_INVALID[];

public:  // ----------------------------------------------------------------
    virtual ~SidTuneBase() {}

    typedef void (*LoaderFunc)(const char* fileName, buffer_t& bufferRef);

    /**
     * Load a sidtune from a file.
     *
     * To retrieve data from standard input pass in filename "-".
     * If you want to override the default filename extensions use this
     * contructor. Please note, that if the specified "sidTuneFileName"
     * does exist and the loader is able to determine its file format,
     * this function does not try to append any file name extension.
     * See "SidTune.cpp" for the default list of file name extensions.
     *
     * @param fileName
     * @param fileNameExt
     * @param separatorIsSlash
     * @return the sid tune
     * @throw loadError
     */
    static SidTuneBase* load(const char* fileName, const char **fileNameExt, bool separatorIsSlash);

    /**
     * Load a sidtune from a file, using a file access callback.
     *
     * Uses the same call methodology as the above function, only
     * a callback is supplied, which will be used to read every
     * file this function accesses.
     *
     * @param loader
     * @param fileName
     * @param fileNameExt
     * @param separatorIsSlash
     * @return the sid tune
     * @throw loadError
     */
    static SidTuneBase* load(LoaderFunc loader, const char* fileName, const char **fileNameExt, bool separatorIsSlash);

    /**
     * Load a single-file sidtune from a memory buffer.
     * Currently supported: PSID format
     *
     * @param sourceBuffer
     * @param bufferLen
     * @return the sid tune
     * @throw loadError
     */
    static SidTuneBase* read(const uint_least8_t* sourceBuffer, uint_least32_t bufferLen);

    /**
     * Select sub-song (0 = default starting song)
     * and return active song number out of [1,2,..,SIDTUNE_MAX_SONGS].
     *
     * @param songNum
     * @return the active song
     */
    unsigned int selectSong(unsigned int songNum);

    /**
     * Retrieve sub-song specific information.
     */
    const SidTuneInfo* getInfo() const;

    /**
     * Select sub-song (0 = default starting song)
     * and retrieve active song information.
     *
     * @param songNum
     */
    const SidTuneInfo* getInfo(unsigned int songNum);

    /**
     * Copy sidtune into C64 memory (64 KB).
     *
     * @param mem
     */
    virtual void placeSidTuneInC64mem(sidmemory& mem);

    /**
     * Calculates the MD5 hash of the tune.
     * Not providing an md5 buffer will cause the internal one to be used.
     * If provided, buffer must be MD5_LENGTH + 1
     *
     * @return a pointer to the buffer containing the md5 string.
     */
    virtual const char *createMD5(char *) { return nullptr; }

    /**
     * Calculates the MD5 hash of the tune.
     * Not providing an md5 buffer will cause the internal one to be used.
     * If provided, buffer must be MD5_LENGTH + 1
     *
     * @return a pointer to the buffer containing the md5 string.
     */
    virtual const char *createMD5New(char *) { return nullptr; }

    /**
     * Get the pointer to the tune data.
     */
    const uint_least8_t* c64Data() const { return &cache[fileOffset]; }

protected:  // -------------------------------------------------------------

    std::unique_ptr<SidTuneInfoImpl> info;

    uint_least8_t songSpeed[MAX_SONGS];
    SidTuneInfo::clock_t clockSpeed[MAX_SONGS];

    /// For files with header: offset to real data
    uint_least32_t fileOffset;

    buffer_t cache;

protected:
    SidTuneBase();

    /**
     * Does not affect status of object, and therefore can be used
     * to load files. Error string is put into info.statusString, though.
     *
     * @param fileName
     * @param bufferRef
     * @throw loadError
     */
    static void loadFile(const char* fileName, buffer_t& bufferRef);

    /**
     * Convert 32-bit PSID-style speed word to internal tables.
     *
     * @param speed
     * @param clock
     */
    void convertOldStyleSpeedToTables(uint_least32_t speed,
         SidTuneInfo::clock_t clock = SidTuneInfo::CLOCK_PAL);

    /**
     * Check if compatibility constraints are fulfilled.
     */
    bool checkCompatibility();

    /**
     * Check for valid relocation information.
     */
    bool checkRelocInfo();

    /**
     * Common address resolution procedure.
     *
     * @param c64data
     */
    void resolveAddrs(const uint_least8_t* c64data);

    /**
     * Cache the data of a single-file or two-file sidtune and its
     * corresponding file names.
     *
     * @param dataFileName
     * @param infoFileName
     * @param buf
     * @param isSlashedFileName If your opendir() and readdir()->d_name return path names
     * that contain the forward slash (/) as file separator, but
     * your operating system uses a different character, there are
     * extra functions that can deal with this special case. Set
     * separatorIsSlash to true if you like path names to be split
     * correctly.
     * You do not need these extra functions if your systems file
     * separator is the forward slash.
     * @throw loadError
     */
    virtual void acceptSidTune(const char* dataFileName, const char* infoFileName,
                        buffer_t& buf, bool isSlashedFileName);

    /**
     * Petscii to Ascii converter.
     */
    std::string petsciiToAscii(SmartPtr_sidtt<const uint8_t>& spPet);

private:  // ---------------------------------------------------------------

#if !defined(SIDTUNE_NO_STDIN_LOADER)
    static SidTuneBase* getFromStdIn();
#endif
    static SidTuneBase* getFromFiles(const char* name, const char **fileNameExtensions, bool separatorIsSlash);
    static SidTuneBase* getFromFiles(LoaderFunc loader, const char* name, const char **fileNameExtensions, bool separatorIsSlash);

    /**
     * Try to retrieve single-file sidtune from specified buffer.
     */
    static SidTuneBase* getFromBuffer(const uint_least8_t* const buffer, uint_least32_t bufferLen);

    /**
     * Get new file name with specified extension.
     *
     * @param destString destinaton string
     * @param sourceName original file name
     * @param sourceExt new extension
     */
    static void createNewFileName(std::string& destString,
                           const char* sourceName, const char* sourceExt);

private:
    // prevent copying
    SidTuneBase(const SidTuneBase&);
    SidTuneBase& operator=(SidTuneBase&);
};

}

#endif  /* SIDTUNEBASE_H */
