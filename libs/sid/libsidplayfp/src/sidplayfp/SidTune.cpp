 /*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012-2021 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include "SidTune.h"

#include "sidtune/SidTuneBase.h"

#include "sidcxx11.h"

using namespace libsidplayfp;

const char MSG_NO_ERRORS[] = "No errors";

// Default sidtune file name extensions. This selection can be overriden
// by specifying a custom list in the constructor.
const char* defaultFileNameExt[] =
{
    // Preferred default file extension for single-file sidtunes
    // or sidtune description files in SIDPLAY INFOFILE format.
    ".sid", ".SID",
    // File extensions used (and created) by various C64 emulators and
    // related utilities. These extensions are recommended to be used as
    // a replacement for ".dat" in conjunction with two-file sidtunes.
    ".c64", ".prg", ".p00", ".C64", ".PRG", ".P00",
    // Stereo Sidplayer (.mus/.MUS ought not be included because
    // these must be loaded first; it sometimes contains the first
    // credit lines of a MUS/STR pair).
    ".str", ".STR", ".mus", ".MUS",
    // End.
    0
};

const char** SidTune::fileNameExtensions = defaultFileNameExt;

SidTune::SidTune(const char* fileName, const char **fileNameExt, bool separatorIsSlash) :
    SidTune(nullptr, fileName, fileNameExt, separatorIsSlash)
{
}

SidTune::SidTune(LoaderFunc loader, const char* fileName, const char **fileNameExt, bool separatorIsSlash) :
    tune(nullptr)
{
    setFileNameExtensions(fileNameExt);
    load(loader, fileName, separatorIsSlash);
}

SidTune::SidTune(const uint_least8_t* oneFileFormatSidtune, uint_least32_t sidtuneLength) :
    tune(nullptr)
{
    read(oneFileFormatSidtune, sidtuneLength);
}

SidTune::~SidTune()
{
    delete tune;
}

void SidTune::setFileNameExtensions(const char **fileNameExt)
{
    fileNameExtensions = ((fileNameExt != nullptr) ? fileNameExt : defaultFileNameExt);
}

void SidTune::load(const char* fileName, bool separatorIsSlash)
{
    load(nullptr, fileName, separatorIsSlash);
}

void SidTune::load(LoaderFunc loader, const char* fileName, bool separatorIsSlash)
{
    try
    {
        delete tune;
        tune = SidTuneBase::load(loader, fileName, fileNameExtensions, separatorIsSlash);
        m_status = true;
        m_statusString = MSG_NO_ERRORS;
    }
    catch (loadError const &e)
    {
        tune =  nullptr;
        m_status = false;
        m_statusString = e.message();
    }
}

void SidTune::read(const uint_least8_t* sourceBuffer, uint_least32_t bufferLen)
{
    try
    {
        delete tune;
        tune = SidTuneBase::read(sourceBuffer, bufferLen);
        m_status = true;
        m_statusString = MSG_NO_ERRORS;
    }
    catch (loadError const &e)
    {
        tune =  nullptr;
        m_status = false;
        m_statusString = e.message();
    }
}

unsigned int SidTune::selectSong(unsigned int songNum)
{
    return tune != nullptr ? tune->selectSong(songNum) : 0;
}

const SidTuneInfo* SidTune::getInfo() const
{
    return tune != nullptr ? tune->getInfo() : nullptr;
}

const SidTuneInfo* SidTune::getInfo(unsigned int songNum)
{
    return tune != nullptr ? tune->getInfo(songNum) : nullptr;
}

bool SidTune::getStatus() const { return m_status; }

const char* SidTune::statusString() const { return m_statusString; }

bool SidTune::placeSidTuneInC64mem(sidmemory& mem)
{
    if (tune == nullptr)
        return false;

    tune->placeSidTuneInC64mem(mem);
    return true;
}

const char* SidTune::createMD5(char *md5)
{
    return tune != nullptr ? tune->createMD5(md5) : nullptr;
}

const char* SidTune::createMD5New(char *md5)
{
    return tune != nullptr ? tune->createMD5New(md5) : nullptr;
}

const uint_least8_t* SidTune::c64Data() const
{
    return tune != nullptr ? tune->c64Data() : nullptr;
}
