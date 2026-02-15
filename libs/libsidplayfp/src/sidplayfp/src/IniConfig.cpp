/*
 * This file is part of sidplayfp, a console SID player.
 *
 * Copyright 2011-2025 Leandro Nini
 * Copyright 2000-2001 Simon White
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "IniConfig.h"

#include <string>

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cerrno>

#ifndef _WIN32
#  include <sys/types.h>
#  include <sys/stat.h>  /* mkdir */
#  include <dirent.h>    /* opendir */
#else
#  include <windows.h>
#endif

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include "utils.h"
#include "dataParser.h"

#include "sidcxx11.h"

const TCHAR* colorStrings[16] =
{
    TEXT("black"),
    TEXT("red"),
    TEXT("green"),
    TEXT("yellow"),
    TEXT("blue"),
    TEXT("magenta"),
    TEXT("cyan"),
    TEXT("white"),
    TEXT("bright black"),
    TEXT("bright red"),
    TEXT("bright green"),
    TEXT("bright yellow"),
    TEXT("bright blue"),
    TEXT("bright magenta"),
    TEXT("bright cyan"),
    TEXT("bright white")
};

inline void debug(MAYBE_UNUSED const TCHAR *msg, MAYBE_UNUSED const TCHAR *val)
{
#ifndef NDEBUG
    SID_COUT << msg << val << std::endl;
#endif
}

inline void error(const TCHAR *msg)
{
    SID_CERR << msg << std::endl;
}

inline void error(const TCHAR *msg, const TCHAR *val)
{
    SID_CERR << msg << val << std::endl;
}

const TCHAR *DIR_NAME = TEXT("sidplayfp");
const TCHAR *FILE_NAME = TEXT("sidplayfp.ini");


IniConfig::IniConfig()
{   // Initialise everything else
    clear();
}


IniConfig::~IniConfig()
{
    clear();
}


void IniConfig::clear()
{
    sidplay2_s.version      = 1;           // INI File Version
    sidplay2_s.database.clear();
    sidplay2_s.playLength   = 0;           // INFINITE
    sidplay2_s.recordLength = (3 * 60 + 30) * 1000; // 3.5 minutes
    sidplay2_s.kernalRom.clear();
    sidplay2_s.basicRom.clear();
    sidplay2_s.chargenRom.clear();
    sidplay2_s.verboseLevel = 0;

    console_s.ansi          = false;
    console_s.topLeft       = '+';
    console_s.topRight      = '+';
    console_s.bottomLeft    = '+';
    console_s.bottomRight   = '+';
    console_s.vertical      = '|';
    console_s.horizontal    = '-';
    console_s.junctionLeft  = '+';
    console_s.junctionRight = '+';
    console_s.decorations   = color_t::bright_white;
    console_s.title         = color_t::white;
    console_s.label_core    = color_t::bright_green;
    console_s.text_core     = color_t::bright_yellow;
    console_s.label_extra   = color_t::bright_magenta;
    console_s.text_extra    = color_t::bright_cyan;
    console_s.notes         = color_t::bright_blue;
    console_s.control_on    = color_t::bright_green;
    console_s.control_off   = color_t::bright_red;

    audio_s.frequency = SidConfig::DEFAULT_SAMPLING_FREQ;
    audio_s.channels  = 0;
    audio_s.precision = 16;
    audio_s.bufLength = 250;

    emulation_s.modelDefault  = SidConfig::PAL;
    emulation_s.modelForced   = false;
    emulation_s.sidModel      = SidConfig::MOS6581;
    emulation_s.forceModel    = false;
    emulation_s.ciaModel      = SidConfig::MOS6526;
    emulation_s.filter        = true;
    emulation_s.engine.clear();

    emulation_s.bias            = 0.5;
    emulation_s.filterCurve6581 = 0.5;
#ifdef FEAT_FILTER_RANGE
    emulation_s.filterRange6581 = 0.5;
#endif
    emulation_s.filterCurve8580 = 0.5;
#ifdef FEAT_CW_STRENGTH
    emulation_s.combinedWaveformsStrength = SidConfig::AVERAGE;
#endif
    emulation_s.powerOnDelay = -1;
    emulation_s.samplingMethod = SidConfig::RESAMPLE_INTERPOLATE;
    emulation_s.fastSampling = false;
}


// static helpers

const TCHAR* readKey(iniHandler &ini, const TCHAR *key)
{
    const TCHAR* value = ini.getValue(key);
    if (value == nullptr)
    {   // Doesn't exist, add it
        ini.addValue(key, TEXT(""));
        debug(TEXT("Key doesn't exist: "), key);
    }
    else if (!value[0])
    {
        // Ignore empty values
        return nullptr;
    }

    return value;
}

void readDouble(iniHandler &ini, const TCHAR *key, double &result)
{
    const TCHAR* value = readKey(ini, key);
    if (value == nullptr)
        return;

    try
    {
        result = dataParser::parseDouble(value);
    }
    catch (dataParser::parseError const &e)
    {
        error(TEXT("Error parsing double at "), key);
    }
}


void readInt(iniHandler &ini, const TCHAR *key, int &result)
{
    const TCHAR* value = readKey(ini, key);
    if (value == nullptr)
        return;

    try
    {
        result = dataParser::parseInt(value);
    }
    catch (dataParser::parseError const &e)
    {
        error(TEXT("Error parsing int at "), key);
    }
}


void readBool(iniHandler &ini, const TCHAR *key, bool &result)
{
    const TCHAR* value = readKey(ini, key);
    if (value == nullptr)
        return;

    try
    {
        result = dataParser::parseBool(value);
    }
    catch (dataParser::parseError const &e)
    {
        error(TEXT("Error parsing bool at "), key);
    }
}


SID_STRING readString(iniHandler &ini, const TCHAR *key)
{
    const TCHAR* value = ini.getValue(key);
    if (value == nullptr)
    {
        // Doesn't exist, add it
        ini.addValue(key, TEXT(""));
        debug(TEXT("Key doesn't exist: "), key);
        return SID_STRING();
    }

    return SID_STRING(value);
}


void readChar(iniHandler &ini, const TCHAR *key, char &ch)
{
    SID_STRING str = readString(ini, key);
    if (str.empty())
        return;

    TCHAR c = 0;

    // Check if we have an actual character
    if (str[0] == '\'')
    {
        if (str[2] != '\'')
            return;
        else
            c = str[1];
    } // Nope is number
    else
    {
        try
        {
            c = dataParser::parseInt(str.c_str());
        }
        catch (dataParser::parseError const &e)
        {
            error(TEXT("Error parsing int at "), key);
        }
    }

    // Clip off special characters
    if ((unsigned) c >= 32)
        ch = c;
}


bool readTime(iniHandler &ini, const TCHAR *key, int &value)
{
    SID_STRING str = readString(ini, key);
    if (str.empty())
        return false;

    int time;
    int milliseconds = 0;
    const size_t sep = str.find_first_of(':');
    const size_t dot = str.find_first_of('.');
    try
    {
        if (sep == SID_STRING::npos)
        {   // User gave seconds
            time = dataParser::parseInt(str.c_str());
        }
        else
        {   // Read in MM:SS.mmm format
            const int min = dataParser::parseInt(str.substr(0, sep).c_str());
            if (min < 0 || min > 99)
                goto IniCofig_readTime_error;
            time = min * 60;

            int sec;
            if (dot == SID_STRING::npos)
            {
                sec  = dataParser::parseInt(str.substr(sep + 1).c_str());
            }
            else
            {
                sec  = dataParser::parseInt(str.substr(sep + 1, dot - sep).c_str());
                SID_STRING msec = str.substr(dot + 1);
                milliseconds = dataParser::parseInt(msec.c_str());
                switch (msec.length())
                {
                case 1: milliseconds *= 100; break;
                case 2: milliseconds *= 10; break;
                case 3: break;
                default: goto IniCofig_readTime_error;
                }
            }

            if (sec < 0 || sec > 59)
                goto IniCofig_readTime_error;
            time += sec;
        }
    }
    catch (dataParser::parseError const &e)
    {
        error(TEXT("Error parsing time at "), key);
        return false;
    }

    value = time * 1000 + milliseconds;
    return true;

IniCofig_readTime_error:
    error(TEXT("Invalid time at "), key);
    return false;
}

bool readColor(iniHandler &ini, const TCHAR *key, color_t &ch)
{
    SID_STRING str = readString(ini, key);
    if (str.empty())
        return false;

    for (int i=0; i<16; i++)
    {
        if (str.compare(colorStrings[i]) == 0)
        {
            ch = static_cast<color_t>(i);
            return true;
        }
    }

    return false;
}


void IniConfig::readSidplay2(iniHandler &ini)
{
    if (!ini.setSection(TEXT("SIDPlayfp")))
        ini.addSection(TEXT("SIDPlayfp"));

    int version = sidplay2_s.version;
    readInt(ini, TEXT("Version"), version);
    if (version > 0)
        sidplay2_s.version = version;

    sidplay2_s.database = readString(ini, TEXT("Songlength Database"));

#if !defined _WIN32
    if (sidplay2_s.database.empty())
    {
        SID_STRING buffer(utils::getDataPath());
        buffer.append(SEPARATOR).append(DIR_NAME).append(SEPARATOR).append("Songlengths.txt");
        if (::access(buffer.c_str(), R_OK) == 0)
            sidplay2_s.database.assign(buffer);
    }
#endif

    int time;
    if (readTime(ini, TEXT("Default Play Length"), time))
        sidplay2_s.playLength = time;
    if (readTime(ini, TEXT("Default Record Length"), time))
        sidplay2_s.recordLength = time;

    sidplay2_s.kernalRom = readString(ini, TEXT("Kernal Rom"));
    sidplay2_s.basicRom = readString(ini, TEXT("Basic Rom"));
    sidplay2_s.chargenRom = readString(ini, TEXT("Chargen Rom"));

    readInt(ini, TEXT("VerboseLevel"), sidplay2_s.verboseLevel);
}


void IniConfig::readConsole(iniHandler &ini)
{
    if (!ini.setSection (TEXT("Console")))
        ini.addSection(TEXT("Console"));

    readBool(ini, TEXT("Ansi"),                console_s.ansi);
    readChar(ini, TEXT("Char Top Left"),       console_s.topLeft);
    readChar(ini, TEXT("Char Top Right"),      console_s.topRight);
    readChar(ini, TEXT("Char Bottom Left"),    console_s.bottomLeft);
    readChar(ini, TEXT("Char Bottom Right"),   console_s.bottomRight);
    readChar(ini, TEXT("Char Vertical"),       console_s.vertical);
    readChar(ini, TEXT("Char Horizontal"),     console_s.horizontal);
    readChar(ini, TEXT("Char Junction Left"),  console_s.junctionLeft);
    readChar(ini, TEXT("Char Junction Right"), console_s.junctionRight);

    readColor(ini, TEXT("Color Decorations"),  console_s.decorations);
    readColor(ini, TEXT("Color Title"),        console_s.title);
    readColor(ini, TEXT("Color Label Core"),   console_s.label_core);
    readColor(ini, TEXT("Color Text Core"),    console_s.text_core);
    readColor(ini, TEXT("Color Label Extra"),  console_s.label_extra);
    readColor(ini, TEXT("Color Text Extra"),   console_s.text_extra);
    readColor(ini, TEXT("Color Notes"),        console_s.notes);
    readColor(ini, TEXT("Color Control On"),   console_s.control_on);
    readColor(ini, TEXT("Color Control Off"),  console_s.control_off);
}


void IniConfig::readAudio(iniHandler &ini)
{
    if (!ini.setSection (TEXT("Audio")))
        ini.addSection(TEXT("Audio"));

    readInt(ini, TEXT("Frequency"), audio_s.frequency);

    readInt(ini, TEXT("Channels"),  audio_s.channels);

    readInt(ini, TEXT("BitsPerSample"), audio_s.precision);

    readInt(ini, TEXT("BufferLength"), audio_s.bufLength);
}


void IniConfig::readEmulation(iniHandler &ini)
{
    if (!ini.setSection (TEXT("Emulation")))
        ini.addSection(TEXT("Emulation"));

    emulation_s.engine = readString (ini, TEXT("Engine"));

    {
        SID_STRING str = readString (ini, TEXT("C64Model"));
        if (!str.empty())
        {
            if (str.compare(TEXT("PAL")) == 0)
                emulation_s.modelDefault = SidConfig::PAL;
            else if (str.compare(TEXT("NTSC")) == 0)
                emulation_s.modelDefault = SidConfig::NTSC;
            else if (str.compare(TEXT("OLD_NTSC")) == 0)
                emulation_s.modelDefault = SidConfig::OLD_NTSC;
            else if (str.compare(TEXT("DREAN")) == 0)
                emulation_s.modelDefault = SidConfig::DREAN;
        }
    }

    readBool(ini, TEXT("ForceC64Model"), emulation_s.modelForced);
    readBool(ini, TEXT("DigiBoost"), emulation_s.digiboost);
    {
        SID_STRING str = readString(ini, TEXT("CiaModel"));
        if (!str.empty())
        {
            if (str.compare(TEXT("MOS6526")) == 0)
                emulation_s.ciaModel = SidConfig::MOS6526;
            else if (str.compare(TEXT("MOS8521")) == 0)
                emulation_s.ciaModel = SidConfig::MOS8521;
        }
    }
    {
        SID_STRING str = readString(ini, TEXT("SidModel"));
        if (!str.empty())
        {
            if (str.compare(TEXT("MOS6581")) == 0)
                emulation_s.sidModel = SidConfig::MOS6581;
            else if (str.compare(TEXT("MOS8580")) == 0)
                emulation_s.sidModel = SidConfig::MOS8580;
        }
    }

    readBool(ini, TEXT("ForceSidModel"), emulation_s.forceModel);

    readBool(ini, TEXT("UseFilter"), emulation_s.filter);

    readDouble(ini, TEXT("FilterBias"), emulation_s.bias);
    readDouble(ini, TEXT("FilterCurve6581"), emulation_s.filterCurve6581);
#ifdef FEAT_FILTER_RANGE
    {
        // For backward compatibility, remove for 3.0
        const TCHAR *key = TEXT("filterRange6581");
        const TCHAR* value = ini.getValue(key);
        if (value && value[0])
        {
            ini.addValue(TEXT("FilterRange6581"), value);
            ini.removeValue(key);
        }
    }
    readDouble(ini, TEXT("FilterRange6581"), emulation_s.filterRange6581);

#endif
    readDouble(ini, TEXT("FilterCurve8580"), emulation_s.filterCurve8580);

#ifdef FEAT_CW_STRENGTH
    {
        SID_STRING str = readString(ini, TEXT("CombinedWaveforms"));
        if (!str.empty())
        {
            if (str.compare(TEXT("AVERAGE")) == 0)
                emulation_s.combinedWaveformsStrength = SidConfig::AVERAGE;
            else if (str.compare(TEXT("WEAK")) == 0)
                emulation_s.combinedWaveformsStrength = SidConfig::WEAK;
            else if (str.compare(TEXT("STRONG")) == 0)
                emulation_s.combinedWaveformsStrength = SidConfig::STRONG;
        }
    }
#endif

    readInt(ini, TEXT("PowerOnDelay"), emulation_s.powerOnDelay);

    {
        SID_STRING str = readString(ini, TEXT("Sampling"));
        if (!str.empty())
        {
            if (str.compare(TEXT("INTERPOLATE")) == 0)
                emulation_s.samplingMethod = SidConfig::INTERPOLATE;
            else if (str.compare(TEXT("RESAMPLE")) == 0)
                emulation_s.samplingMethod = SidConfig::RESAMPLE_INTERPOLATE;
        }
    }

    readBool(ini, TEXT("ResidFastSampling"), emulation_s.fastSampling);
}

class iniError
{
private:
    const SID_STRING msg;

public:
    iniError(const TCHAR* msg) : msg(msg) {}
    const SID_STRING message() const { return msg; }
};

void createDir(const SID_STRING& path)
{
#ifndef _WIN32
    DIR *dir = opendir(path.c_str());
    if (dir)
    {
        closedir(dir);
    }
    else if (errno == ENOENT)
    {
        if (mkdir(path.c_str(), 0755) < 0)
        {
            throw iniError(strerror(errno));
        }
    }
    else
    {
        throw iniError(strerror(errno));
    }
#else
    if (GetFileAttributes(path.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        if (CreateDirectory(path.c_str(), NULL) == 0)
        {
            LPTSTR pBuffer;
            FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&pBuffer, 0, NULL);
            iniError err(pBuffer);
            LocalFree(pBuffer);
            throw err;
        }
    }
#endif
}

SID_STRING getConfigPath()
{
    SID_STRING configPath;

    try
    {
        configPath = utils::getConfigPath();
    }
    catch (utils::error const &e)
    {
        throw iniError(TEXT("Cannot get config path!"));
    }

    debug(TEXT("Config path: "), configPath.c_str());

    // Make sure the config path exists
    createDir(configPath);

    configPath.append(SEPARATOR).append(DIR_NAME);

    // Make sure the app config path exists
    createDir(configPath);

    configPath.append(SEPARATOR).append(FILE_NAME);

    debug(TEXT("Config file: "), configPath.c_str());

    return configPath;
}

bool tryOpen(MAYBE_UNUSED iniHandler &ini)
{
#ifdef _WIN32
    {
        // Try exec dir first
        SID_STRING execPath(utils::getExecPath());
        execPath.append(SEPARATOR).append(FILE_NAME);
        if (ini.tryOpen(execPath.c_str()))
            return true;
    }
#endif
    return false;
}

void IniConfig::read()
{
    clear();

    iniHandler ini;

    if (!tryOpen(ini))
    {
        try
        {
            SID_STRING configPath = getConfigPath();

            // Opens an existing file or creates a new one
            if (!ini.open(configPath.c_str()))
            {
                error(TEXT("Error reading config file!"));
                return;
            }
        } catch (iniError const &e) {
            error(e.message().c_str());
            return;
        }
    }

    readSidplay2  (ini);
    readConsole   (ini);
    readAudio     (ini);
    readEmulation (ini);

    m_fileName = ini.getFilename();

    ini.close();
}
