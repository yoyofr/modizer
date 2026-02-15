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

#include "player.h"

#include <cstdlib>
#include <cmath>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <memory>
#include <new>

using std::cout;
using std::cerr;
using std::endl;

#include <cstdlib>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include "utils.h"
#include "keyboard.h"
#include "audio/AudioDrv.h"
#include "audio/au/auFile.h"
#include "audio/wav/WavFile.h"
#include "ini/types.h"

#include "sidcxx11.h"

#include <sidplayfp/sidbuilder.h>
#include <sidplayfp/SidInfo.h>
#include <sidplayfp/SidTuneInfo.h>


#include <unordered_map>

using filter_map_t = std::unordered_map<std::string, double>;
using filter_map_iter_t = std::unordered_map<std::string, double>::const_iterator;


// Previous song select timeout (4 secs)
constexpr uint_least32_t SID2_PREV_SONG_TIMEOUT = 4000;
constexpr uint_least32_t FREQ_PAL = 50;
constexpr uint_least32_t FREQ_NTSC = 60;


const char* ERR_NOT_ENOUGH_MEMORY = "ERROR: Not enough memory.";


#ifdef HAVE_SIDPLAYFP_BUILDERS_RESIDFP_H
#  include <sidplayfp/builders/residfp.h>
const char ConsolePlayer::RESIDFP_ID[] = "ReSIDfp";
#endif

#ifdef HAVE_SIDPLAYFP_BUILDERS_SIDLITE_H
#  include <sidplayfp/builders/sidlite.h>
const char ConsolePlayer::SIDLITE_ID[] = "SidLite";
#endif

#ifdef HAVE_SIDPLAYFP_BUILDERS_RESID_H
#  include <sidplayfp/builders/resid.h>
const char ConsolePlayer::RESID_ID[] = "ReSID";
#endif

#ifdef HAVE_SIDPLAYFP_BUILDERS_HARDSID_H
#   include <sidplayfp/builders/hardsid.h>
const char ConsolePlayer::HARDSID_ID[] = "HardSID";
#endif

#ifdef HAVE_SIDPLAYFP_BUILDERS_EXSID_H
#   include <sidplayfp/builders/exsid.h>
const char ConsolePlayer::EXSID_ID[] = "exSID";
#endif

#ifdef HAVE_SIDPLAYFP_BUILDERS_USBSID_H
#   include <sidplayfp/builders/usbsid.h>
const char ConsolePlayer::USBSID_ID[] = "USBSID";
#endif

#ifdef FEAT_REGS_DUMP_SID
uint16_t freqTablePal[]
{
    // C       C#      D       D#      E       F       F#      G       G#      A       A#      B
    0x0117, 0x0127, 0x0139, 0x014b, 0x015f, 0x0174, 0x018a, 0x01a1, 0x01ba, 0x01d4, 0x01f0, 0x020e, // 1
    0x022d, 0x024e, 0x0271, 0x0296, 0x02be, 0x02e8, 0x0314, 0x0343, 0x0374, 0x03a9, 0x03e1, 0x041c, // 2
    0x045a, 0x049c, 0x04e2, 0x052d, 0x057c, 0x05cf, 0x0628, 0x0685, 0x06e8, 0x0752, 0x07c1, 0x0837, // 3
    0x08b4, 0x0939, 0x09c5, 0x0a5a, 0x0af7, 0x0b9e, 0x0c4f, 0x0d0a, 0x0dd1, 0x0ea3, 0x0f82, 0x106e, // 4
    0x1168, 0x1271, 0x138a, 0x14b3, 0x15ee, 0x173c, 0x189e, 0x1a15, 0x1ba2, 0x1d46, 0x1f04, 0x20dc, // 5
    0x22d0, 0x24e2, 0x2714, 0x2967, 0x2bdd, 0x2e79, 0x313c, 0x3429, 0x3744, 0x3a8d, 0x3e08, 0x41b8, // 6
    0x45a1, 0x49c5, 0x4e28, 0x52cd, 0x57ba, 0x5cf1, 0x6278, 0x6853, 0x6e87, 0x751a, 0x7c10, 0x8371, // 7
    0x8b42, 0x9389, 0x9c4f, 0xa59b, 0xaf74, 0xb9e2, 0xc4f0, 0xd0a6, 0xdd0e, 0xea33, 0xf820, 0xffff, // 8
};

uint16_t freqTableNtsc[]
{
    // C       C#      D       D#      E       F       F#      G       G#      A       A#      B
    0x010c, 0x011c, 0x012d, 0x013f, 0x0152, 0x0166, 0x017b, 0x0192, 0x01aa, 0x01c3, 0x01de, 0x01fa, // 1
    0x0218, 0x0238, 0x025a, 0x027e, 0x02a4, 0x02cc, 0x02f7, 0x0324, 0x0354, 0x0386, 0x03bc, 0x03f5, // 2
    0x0431, 0x0471, 0x04b5, 0x04fc, 0x0548, 0x0598, 0x05ee, 0x0648, 0x06a9, 0x070d, 0x0779, 0x07ea, // 3
    0x0862, 0x08e2, 0x096a, 0x09f8, 0x0a90, 0x0b30, 0x0bdc, 0x0c90, 0x0d52, 0x0e1a, 0x0ef2, 0x0fd4, // 4
    0x10c4, 0x11c4, 0x12d4, 0x13f0, 0x1520, 0x1660, 0x17b8, 0x1920, 0x1aa4, 0x1c34, 0x1de4, 0x1fa8, // 5
    0x2188, 0x2388, 0x25a8, 0x27e0, 0x2a40, 0x2cc0, 0x2f70, 0x3240, 0x3548, 0x3868, 0x3bc8, 0x3f50, // 6
    0x4310, 0x4710, 0x4b50, 0x4fc0, 0x5480, 0x5980, 0x5ee0, 0x6480, 0x6a90, 0x70d0, 0x7790, 0x7ea0, // 7
    0x8620, 0x8e20, 0x96a0, 0x9f80, 0xa900, 0xb300, 0xbdc0, 0xc900, 0xd520, 0xe1a0, 0xef20, 0xfd40, // 8
};
#endif

// This tables contains chip profiles which allow us to adjust
// certain settings that varied wildly between 6581 chips, even
// those made in the same factory on the same day.
//
// This works under the assumption that the authors used the
// same SID chip their entire career.
//
// taken from https://github.com/reFX/libSidplayEZ/blob/modernized/src/chip-profiles.h
#ifdef FEAT_FILTER_RANGE
static const filter_map_t filterRangeMap =
{
    { "Anthony Lees",                        1.3 },
    { "Antony Crowther (Ratt)",              1.1 },
    { "Barry Leitch (The Jackal)",           0.3 },
    { "Ben Daglish",                         0.6 },
    { "Carsten Berggreen (Scarzix)",         0.7 },
    { "Charles Deenen",                      0.2 },
    { "Chris H\xFClsbeck",                   0.9 },
    { "David Dunn",                          0.1 },
    { "David Dunn & Aidan Bell",             0.1 },
    { "David Whittaker",                     0.15 },
    { "Edwin van Santen",                    0.5 },
    { "Edwin van Santen & Falco Paul",       0.4 }, // 20th Century Composers
    { "Edwin van Santen & Venom",            0.4 }, // 20th Century Composers
    { "Falco Paul",                          0.15 },
    { "Falco Paul & Edwin van Santen",       0.4 }, // 20th Century Composers
    { "Figge Wasberger (Fegolhuzz)",         0.25 },
    { "Fred Gray",                           0.4 },
    { "Geir Tjelta",                         0.5 },
    { "Geoff Follin",                        0.85 },
    { "Georg Feil",                          0.2 },
    { "Glenn Rune Gallefoss",                1.3 },
    { "Graham Jarvis & Rob Hartshorne",      0.25 }, // Clever Music
    { "Jason Page",                          0.35 },
    { "Jeroen Tel",                          0.35 },
    { "Johannes Bjerregaard",                0.35 },
    { "Jonathan Dunn",                       0.25 },
    { "Jouni Ikonen (Mixer)",                0.25 },
    { "Jori Olkkonen",                       0.15 },
    { "Jori Olkkonen (Yip)",                 0.35 },
    { "Kim Christensen (Future Freak)",      0.35 },
    { "Linus \xC5kesson (lft)",              0.3 },
    { "Mark Cooksey",                        0.4 },
    { "Mark Wilson",                         0.2 },
    { "Markus M\xFCller (Superbrain)",       0.5 },
    { "Martin Galway",                       0.65 },
    { "Martin Walker",                       0.15 },
    { "Matt Gray",                           0.3 },
    { "Michael Hendriks",                    0.35 },
    { "Mitch & Dane",                        0.85 },
    { "M. Nilsson-Vonderburgh (Mic)",        0.3 },
    { "M. Nilsson-Vonderburgh (Mitch)",      0.3 },
    { "M. Nilsson-Vonderburgh (Yankee)",     0.3 },
    { "NM156",                               0.7 },
    { "Neil Brennan",                        0.25 },
    { "Peter Clarke",                        0.2 },
    { "Pex Tufvesson (Mahoney)",             0.35 },
    { "Pex Tufvesson (Zax)",                 0.35 },
    { "Renato Brosowski (Zoci-Joe)",         0.3 },
    { "Reyn Ouwehand",                       0.8 },
    { "Richard Joseph",                      0.3 },
    { "Rob Hubbard",                         0.35 },
    { "Russell Lieblich",                    0.25 },
    { "Stellan Andersson (Dane)",            0.85 },
    { "Steve Turner",                        0.6 },
    { "Tim Follin",                          0.5 },
    { "Thomas E. Petersen (Laxity)",         0.3 },
    { "Thomas E. Petersen (TSS)",            0.3 },
    { "Thomas Mogensen (DRAX)",              0.3 },
};
#else
static const filter_map_t filterCurveMap =
{
    { "Anthony Lees",                        0.450 },
    { "Antony Crowther (Ratt)",              0.400 },
    { "Ben Daglish",                         0.900 },
    { "Charles Deenen",                      1.000 },
    { "Chris H\xFClsbeck",                   0.600 },
    { "David Dunn",                          1.100 },
    { "David Dunn & Aidan Bell",             1.100 },
    { "David Whittaker",                     1.000 },
    { "Thomas Mogensen (DRAX)",              0.450 },
    { "Edwin van Santen",                    0.650 },
    { "Falco Paul",                          1.100 },
    { "Figge Wasberger (Fegolhuzz)",         1.100 },
    { "Fred Gray",                           1.250 },
    { "Geir Tjelta",                         0.700 },
    { "Georg Feil",                          1.250 },
    { "Glenn Rune Gallefoss",                1.250 },
    { "Graham Jarvis & Rob Hartshorne",      1.000 },
    { "Jason Page",                          1.000 },
    { "Jeroen Tel",                          0.825 },
    { "Johannes Bjerregaard",                0.700 },
    { "Jonathan Dunn",                       0.750 },
    { "Jouni Ikonen (Mixer)",                0.600 },
    { "Kim Christensen (Future Freak)",      0.300 },
    { "Linus \xC5kesson (lft)",              0.900 },
    { "Mark Cooksey",                        1.250 },
    { "Markus M\xFCller (Superbrain)",       0.800 },
    { "Martin Walker",                       1.000 },
    { "Matt Gray",                           1.100 },
    { "Michael Hendriks",                    0.900 },
    { "M. Nilsson-Vonderburgh (Mic)",        0.700 },
    { "M. Nilsson-Vonderburgh (Mitch)",      0.700 },
    { "M. Nilsson-Vonderburgh (Yankee)",     0.700 },
    { "Neil Brennan",                        0.750 },
    { "Peter Clarke",                        0.600 },
    { "Pex Tufvesson (Mahoney)",             0.400 },
    { "Pex Tufvesson (Zax)",                 0.400 },
    { "Renato Brosowski (Zoci-Joe)",         1.400 },
    { "Reyn Ouwehand",                       1.000 },
    { "Richard Joseph",                      0.700 },
    { "Rob Hubbard",                         0.700 },
    { "Russell Lieblich",                    0.400 },
    { "Stellan Andersson (Dane)",            0.350 },
    { "Steve Turner",                        0.550 },
    { "Tim Follin",                          0.300 },
    { "Thomas E. Petersen (Laxity)",         1.550 },
    { "Thomas E. Petersen (TSS)",            1.550 },
};
#endif

#ifdef FEAT_FILTER_RANGE
double getRecommendedFilterRange(const std::string& author)
{
    filter_map_iter_t it = filterRangeMap.find(author);
    if (it != filterRangeMap.end())
    {
        double adjustment = it->second;
        return (adjustment * 20. - 1.) / 39.;
    }
    return -1.;
}
#else
double getRecommendedFilterCurve(const std::string& author)
{
    filter_map_iter_t it = filterCurveMap.find(author);
    return (it != filterCurveMap.end()) ? it->second : -1.;
}
#endif

std::unique_ptr<uint8_t[]> loadRom(const SID_STRING &romPath, const int size)
{
    SID_IFSTREAM is(romPath.c_str(), std::ios::binary);

    if (is.is_open())
    {
        try
        {
            std::unique_ptr<uint8_t[]> buffer(new uint8_t[size]);

            is.read((char*)buffer.get(), size);
            if (!is.fail())
            {
                is.close();
                return buffer;
            }
        }
        catch (std::bad_alloc const &ba) {}
    }

    return nullptr;
}


std::unique_ptr<uint8_t[]> loadRom(const SID_STRING &romPath, const int size, const TCHAR* defaultRom)
{
    // Try to load given rom
    if (!romPath.empty())
    {
        std::unique_ptr<uint8_t[]> buffer = loadRom(romPath, size);
        if (buffer)
            return buffer;
    }

    // Fallback to default rom path
    try
    {
#ifdef _WIN32
        {
            // Try exec dir first
            SID_STRING execPath(utils::getExecPath());
            execPath.append(SEPARATOR).append(defaultRom);
            std::unique_ptr<uint8_t[]> buffer = loadRom(execPath, size);
            if (buffer)
                return buffer;
        }
#endif
        SID_STRING dataPath(utils::getDataPath());

        dataPath.append(SEPARATOR).append(TEXT("sidplayfp")).append(SEPARATOR).append(defaultRom);

#if !defined _WIN32 && defined HAVE_UNISTD_H
        if (::access(dataPath.c_str(), R_OK) != 0)
        {
            dataPath = PKGDATADIR;
            dataPath.append(defaultRom);
        }
#endif

        return loadRom(dataPath, size);
    }
    catch (utils::error const &e)
    {
        return nullptr;
    }
}


ConsolePlayer::ConsolePlayer (const char * const name) :
    m_name(name),
    m_tune(nullptr),
    m_state(playerStopped),
    m_outfile(nullptr),
    m_filename(""),
    m_quietLevel(0),
    m_showhelp(false),
    songlengthDB(sldb_t::NONE),
    m_cpudebug(false),
    m_autofilter(false)
{
#ifdef FEAT_REGS_DUMP_SID
    std::memset(m_registers, 0, 32*3);
#endif
    // Other defaults
    m_filter.enabled = true;
    m_driver.device  = nullptr;
    m_driver.sid     = EMU_RESIDFP;
    m_timer.start    = 0;
    m_timer.length   = 0; // FOREVER
    m_timer.valid    = false;
    m_timer.starting = false;
    m_track.first    = 0;
    m_track.selected = 0;
    m_track.loop     = false;
    m_track.single   = false;
    m_speed.current  = 1;
    m_speed.max      = 32;

    // Read default configuration
    m_iniCfg.read ();
    m_engCfg = m_engine.config ();

    {   // Load ini settings
        IniConfig::audio_section     audio     = m_iniCfg.audio();
        IniConfig::emulation_section emulation = m_iniCfg.emulation();

        // INI Configuration Settings
        m_engCfg.forceC64Model   = emulation.modelForced;
        m_engCfg.defaultC64Model = emulation.modelDefault;
        m_engCfg.defaultSidModel = emulation.sidModel;
        m_engCfg.forceSidModel   = emulation.forceModel;
        m_engCfg.ciaModel        = emulation.ciaModel;
        m_engCfg.frequency    = audio.frequency;
        m_engCfg.samplingMethod = emulation.samplingMethod;
#ifdef HAVE_SIDPLAYFP_BUILDERS_RESID_H
        m_engCfg.fastSampling = emulation.fastSampling;
#endif
        m_channels            = audio.channels;
        m_precision           = audio.precision;
        m_buffer_size         = audio.getBufSize();
        m_filter.enabled      = emulation.filter;
        m_filter.bias         = emulation.bias;
        m_filter.filterCurve6581 = emulation.filterCurve6581;
#ifdef FEAT_FILTER_RANGE
        m_filter.filterRange6581 = emulation.filterRange6581;
#endif
        m_filter.filterCurve8580 = emulation.filterCurve8580;
#ifdef FEAT_CW_STRENGTH
        m_combinedWaveformsStrength = emulation.combinedWaveformsStrength;
#endif

        if (emulation.powerOnDelay >= 0)
            m_engCfg.powerOnDelay    = emulation.powerOnDelay;

        // Set default output
        m_driver.output = output_t::SOUNDCARD;

        if (!emulation.engine.empty())
        {
            if (emulation.engine.compare(TEXT("RESIDFP")) == 0)
            {
                m_driver.sid    = EMU_RESIDFP;
            }
            else if (emulation.engine.compare(TEXT("SIDLITE")) == 0)
            {
                m_driver.sid    = EMU_SIDLITE;
            }
            else if (emulation.engine.compare(TEXT("RESID")) == 0)
            {
                m_driver.sid    = EMU_RESID;
            }
#ifdef HAVE_SIDPLAYFP_BUILDERS_HARDSID_H
            else if (emulation.engine.compare(TEXT("HARDSID")) == 0)
            {
                m_driver.sid    = EMU_HARDSID;
                m_driver.output = output_t::NONE;
            }
#endif
#ifdef HAVE_SIDPLAYFP_BUILDERS_EXSID_H
            else if (emulation.engine.compare(TEXT("EXSID")) == 0)
            {
                m_driver.sid    = EMU_EXSID;
                m_driver.output = output_t::NONE;
            }
#endif
#ifdef HAVE_SIDPLAYFP_BUILDERS_USBSID_H
            else if (emulation.engine.compare(TEXT("USBSID")) == 0)
            {
                m_driver.sid    = EMU_USBSID;
                m_driver.output = output_t::NONE;
            }
#endif
            else if (emulation.engine.compare(TEXT("NONE")) == 0)
            {
                m_driver.sid    = EMU_NONE;
            }
        }
    }

    m_verboseLevel = (m_iniCfg.sidplay2()).verboseLevel;
#ifdef FEAT_NEW_PLAY_API
    m_fadeoutTime = 0;
    m_fadeAfter = false;
#endif
    createOutput (output_t::NONE, nullptr);
    createSidEmu (EMU_NONE, nullptr);

    std::unique_ptr<uint8_t[]> kernalRom = loadRom((m_iniCfg.sidplay2()).kernalRom, 8192, TEXT("kernal"));
    std::unique_ptr<uint8_t[]> basicRom = loadRom((m_iniCfg.sidplay2()).basicRom, 8192, TEXT("basic"));
    std::unique_ptr<uint8_t[]> chargenRom = loadRom((m_iniCfg.sidplay2()).chargenRom, 4096, TEXT("chargen"));
    m_engine.setRoms(kernalRom.get(), basicRom.get(), chargenRom.get());
}

std::string ConsolePlayer::getFileName(const SidTuneInfo *tuneInfo, const char* ext)
{
    std::string title;

    if (m_outfile != nullptr)
    {
        title = m_outfile;
        if (title.compare("-") != 0
                && title.find_last_of('.') == std::string::npos)
            title.append(ext);
    }
    else
    {
        // Generate a name for the wav file
        title = tuneInfo->dataFileName();

        title.erase(title.find_last_of('.'));

        // Change name based on subtune
        if (tuneInfo->songs() > 1)
        {
            std::ostringstream sstream;
            sstream << "[" << tuneInfo->currentSong() << "]";
            title.append(sstream.str());
        }
        title.append(ext);
    }

    return title;
}

// Create the output object to process sound buffer
bool ConsolePlayer::createOutput (output_t driver, const SidTuneInfo *tuneInfo)
{
    // Remove old audio driver
    m_driver.null.close ();
    m_driver.selected = &m_driver.null;
    if (m_driver.device != nullptr)
    {
        if (m_driver.device != &m_driver.null)
            delete m_driver.device;
        m_driver.device = nullptr;
    }

    // Create audio driver
    switch (driver)
    {
    case output_t::NONE:
        m_driver.device = &m_driver.null;
    break;

    case output_t::SOUNDCARD:
        try
        {
            m_driver.device = new audioDrv();
        }
        catch (std::bad_alloc const &ba)
        {
            m_driver.device = nullptr;
        }
    break;

    case output_t::WAV:
        try
        {
            std::string title = getFileName(tuneInfo, WavFile::extension());
            WavFile* wav = new WavFile(title);
            if (m_driver.info && (tuneInfo->numberOfInfoStrings() == 3))
                wav->setInfo(tuneInfo->infoString(0), tuneInfo->infoString(1), tuneInfo->infoString(2));
            m_driver.device = wav;
        }
        catch (std::bad_alloc const &ba)
        {
            m_driver.device = nullptr;
        }
    break;

    case output_t::AU:
        try
        {
            std::string title = getFileName(tuneInfo, auFile::extension());
            m_driver.device = new auFile(title);
        }
        catch (std::bad_alloc const &ba)
        {
            m_driver.device = nullptr;
        }
    break;

    default:
        break;
    }

    // Audio driver failed
    if (!m_driver.device)
    {
        m_driver.device = &m_driver.null;
        displayError (ERR_NOT_ENOUGH_MEMORY);
        return false;
    }

    int tuneChannels = (tuneInfo && (tuneInfo->sidChips() > 1)) ? 2 : 1;

    // Configure with user settings
    m_driver.cfg.frequency = m_engCfg.frequency;
    m_driver.cfg.channels  = m_channels ? m_channels : tuneChannels;
    m_driver.cfg.precision = m_precision;
    m_driver.cfg.bufSize   = m_buffer_size;

    {   // Open the hardware
        bool err = false;
        if (!m_driver.device->open(m_driver.cfg))
            err = true;

        // Can't open the same driver twice
        if (driver != output_t::NONE)
        {
            if (!m_driver.null.open(m_driver.cfg))
                err = true;
        }

        if (err)
        {
            displayError(m_driver.device->getErrorString());
            return false;
        }

        if (m_verboseLevel && (driver != output_t::NONE))
            cerr << "Using audio driver: " << m_driver.device->getDriverString() << endl;
    }

    // See what we got
    m_engCfg.frequency = m_driver.cfg.frequency;
    switch (m_driver.cfg.channels)
    {
    case 1:
        m_engCfg.playback  = SidConfig::MONO;
        break;
    case 2:
        m_engCfg.playback  = SidConfig::STEREO;
        break;
    default:
        cerr << m_name << ": " << "ERROR: " << m_channels
             << " audio channels not supported" << endl;
        return false;
    }

    return true;
}


// Create the sid emulation
bool ConsolePlayer::createSidEmu (SIDEMUS emu, const SidTuneInfo *tuneInfo)
{
    // Remove old driver and emulation
    if (m_engCfg.sidEmulation)
    {
        sidbuilder *builder   = m_engCfg.sidEmulation;
        m_engCfg.sidEmulation = nullptr;
        m_engine.config(m_engCfg);
        delete builder;
    }

    // Now setup the sid emulation
    switch (emu)
    {
#ifdef HAVE_SIDPLAYFP_BUILDERS_RESIDFP_H
    case EMU_RESIDFP:
    {
        try
        {
            ReSIDfpBuilder *rs = new ReSIDfpBuilder( RESIDFP_ID );

            m_engCfg.sidEmulation = rs;
#ifndef FEAT_NO_CREATE
            if (!rs->getStatus()) goto createSidEmu_error;
            rs->create ((m_engine.info ()).maxsids());
            if (!rs->getStatus()) goto createSidEmu_error;
#endif
#ifdef FEAT_CW_STRENGTH
            rs->combinedWaveformsStrength(m_combinedWaveformsStrength);
#endif

#ifdef FEAT_FILTER_RANGE
            double frange = m_filter.filterRange6581;

            if (m_autofilter && (tuneInfo->numberOfInfoStrings() == 3))
            {
                double rfr = getRecommendedFilterRange(tuneInfo->infoString(1));
                if (rfr < 0.)
                {
                    if (m_verboseLevel > 1)
                        cerr << "No recommended filter range available" << endl;
                }
                else
                {
                    if (m_verboseLevel > 1)
                        cerr << "Recommended filter range: " << rfr << endl;
                    frange = rfr;
                }
            }

            if (m_frange.has_value())
            {
                frange = m_frange.value();
            }

            if ((frange < 0.0) || (frange > 1.0))
            {
                cerr << "Invalid 6581 filter range: " << frange << endl;
                exit(EXIT_FAILURE);
            }

            if (m_verboseLevel)
                cerr << "6581 filter range: " << frange << endl;
            rs->filter6581Range(frange);
#endif

            // 6581
            double fcurve = m_filter.filterCurve6581;
#ifndef FEAT_FILTER_RANGE
            if (m_autofilter && (tuneInfo->numberOfInfoStrings() == 3))
            {
                double rfc = getRecommendedFilterCurve(tuneInfo->infoString(1));
                if (rfc < 0.)
                {
                    if (m_verboseLevel > 1)
                        cerr << "No recommended filter curve available" << endl;
                }
                else
                {
                    if (m_verboseLevel > 1)
                        cerr << "Recommended filter curve: " << rfc << endl;
                    fcurve = rfc;
                }
            }
#endif
            if (m_fcurve.has_value())
            {
                fcurve = m_fcurve.value();
            }

            if ((fcurve < 0.0) || (fcurve > 1.0))
            {
                cerr << "Invalid 6581 filter curve: " << fcurve << endl;
                exit(EXIT_FAILURE);
            }

            if (m_verboseLevel)
                cerr << "6581 filter curve: " << fcurve << endl;
            rs->filter6581Curve(fcurve);

            // 8580
            fcurve = m_filter.filterCurve8580;
            if (m_fcurve.has_value())
            {
                fcurve = m_fcurve.value();
            }

            if ((fcurve < 0.0) || (fcurve > 1.0))
            {
                cerr << "Invalid 8580 filter curve: " << fcurve << endl;
                exit(EXIT_FAILURE);
            }

            if (m_verboseLevel)
                cerr << "8580 filter curve: " << fcurve << endl;
            rs->filter8580Curve(fcurve);
        }
        catch (std::bad_alloc const &ba) {}
        break;
    }
#endif // HAVE_SIDPLAYFP_BUILDERS_RESIDFP_H

#ifdef HAVE_SIDPLAYFP_BUILDERS_SIDLITE_H
    case EMU_SIDLITE:
    {
        try
        {
            SIDLiteBuilder *rs = new SIDLiteBuilder( SIDLITE_ID );

            m_engCfg.sidEmulation = rs;
        }
        catch (std::bad_alloc const &ba) {}
        break;
    }
#endif // HAVE_SIDPLAYFP_BUILDERS_SIDLITE_H

#ifdef HAVE_SIDPLAYFP_BUILDERS_RESID_H
    case EMU_RESID:
    {
        try
        {
            ReSIDBuilder *rs = new ReSIDBuilder( RESID_ID );

            m_engCfg.sidEmulation = rs;
            if (!rs->getStatus()) goto createSidEmu_error;
            rs->create ((m_engine.info ()).maxsids());
            if (!rs->getStatus()) goto createSidEmu_error;
            rs->bias(m_filter.bias);
        }
        catch (std::bad_alloc const &ba) {}
        break;
    }
#endif // HAVE_SIDPLAYFP_BUILDERS_RESID_H

#ifdef HAVE_SIDPLAYFP_BUILDERS_HARDSID_H
    case EMU_HARDSID:
    {
        try
        {
            HardSIDBuilder *hs = new HardSIDBuilder( HARDSID_ID );

            m_engCfg.sidEmulation = hs;
            if (!hs->getStatus()) goto createSidEmu_error;
            hs->create ((m_engine.info ()).maxsids());
            if (!hs->getStatus()) goto createSidEmu_error;
        }
        catch (std::bad_alloc const &ba) {}
        break;
    }
#endif // HAVE_SIDPLAYFP_BUILDERS_HARDSID_H

#ifdef HAVE_SIDPLAYFP_BUILDERS_EXSID_H
    case EMU_EXSID:
    {
        try
        {
            exSIDBuilder *es = new exSIDBuilder( EXSID_ID );

            m_engCfg.sidEmulation = es;
#ifndef FEAT_NO_CREATE
            if (!es->getStatus()) goto createSidEmu_error;
            es->create ((m_engine.info ()).maxsids());
            if (!es->getStatus()) goto createSidEmu_error;
#endif
        }
        catch (std::bad_alloc const &ba) {}
        break;
    }
#endif // HAVE_SIDPLAYFP_BUILDERS_EXSID_H

#ifdef HAVE_SIDPLAYFP_BUILDERS_USBSID_H
    case EMU_USBSID:
    {
        try
        {
            USBSIDBuilder *us = new USBSIDBuilder( USBSID_ID );

            m_engCfg.sidEmulation = us;
#ifndef FEAT_NO_CREATE
            if (!us->getStatus()) goto createSidEmu_error;
            us->create ((m_engine.info ()).maxsids());
            if (!us->getStatus()) goto createSidEmu_error;
#endif
        }
        catch (std::bad_alloc const &ba) {}
        break;
    }
#endif // HAVE_SIDPLAYFP_BUILDERS_USBSID_H

    default:
        // Emulation Not yet handled
        // This default case results in the default
        // emulation
        break;
    }

    if (!m_engCfg.sidEmulation)
    {
        if (emu > EMU_DEFAULT)
        {   // No sid emulation?
            displayError (ERR_NOT_ENOUGH_MEMORY);
            return false;
        }
    }

#ifndef FEAT_FILTER_DISABLE
    if (m_engCfg.sidEmulation) {
        /* set up SID filter. HardSID just ignores call with def. */
        m_engCfg.sidEmulation->filter(m_filter.enabled);
    }
#endif

    return true;
#ifndef FEAT_NO_CREATE
createSidEmu_error:
    displayError (m_engCfg.sidEmulation->error ());
    delete m_engCfg.sidEmulation;
    m_engCfg.sidEmulation = nullptr;
    return false;
#endif
}


bool ConsolePlayer::open (void)
{
    if ((m_state & ~playerFast) == playerRestart)
    {
        if (m_quietLevel < 2)
            cerr << endl;
        if (m_state & playerFast)
            m_driver.selected->reset ();
        m_state = playerStopped;
    }

    // Select the required song
    m_track.selected = m_tune.selectSong(m_track.selected);
    if (!m_engine.load (&m_tune))
    {
        displayError (m_engine.error());
        return false;
    }

    // Get tune details
    const SidTuneInfo *tuneInfo = m_tune.getInfo();
    if (!m_track.single)
        m_track.songs = tuneInfo->songs();
    if (!createOutput(m_driver.output, tuneInfo))
        return false;
    if (!createSidEmu(m_driver.sid, tuneInfo))
        return false;

    // Configure engine with settings
    if (!m_engine.config(m_engCfg))
    {   // Config failed
        displayError(m_engine.error ());
        return false;
    }

#ifdef FEAT_FILTER_DISABLE
    for (int chip=0; chip<3; chip++)
    {
        m_engine.filter(chip, m_filter.enabled);
    }
#endif
#ifdef FEAT_REGS_DUMP_SID
    if (
            (
                (m_engCfg.defaultC64Model == SidConfig::NTSC) &&
                (m_engCfg.forceC64Model || (tuneInfo->clockSpeed() != SidTuneInfo::CLOCK_PAL))
            ) ||
            (tuneInfo->clockSpeed() == SidTuneInfo::CLOCK_NTSC)
    )
        m_freqTable = freqTableNtsc;
    else
        m_freqTable = freqTablePal;
#endif
#ifdef FEAT_NEW_PLAY_API
    m_mixer.initialize(m_engine.installedSIDs(), m_engCfg.playback == SidConfig::STEREO);
#endif

    // Start the player.  Do this by fast
    // forwarding to the start position
    m_driver.selected = &m_driver.null;
    m_speed.current   = m_speed.max;
#ifdef FEAT_NEW_PLAY_API
    m_mixer.clear();
    m_mixer.setFastForward(m_speed.current);
    m_mixer.setVolume(Mixer::VOLUME_MAX);
#else
    m_engine.fastForward(100 * m_speed.current);
#endif

    for (int chip=0; chip<3; chip++)
    {
        for (int channel=0; channel<3; channel++)
        {
            m_engine.mute(chip, channel, m_mute_channel[chip*3 + channel]);
        }
#ifdef FEAT_SAMPLE_MUTE
        m_engine.mute(chip, 3, m_mute_samples[chip]);
#endif
    }

    // As yet we don't have a required songlength
    // so try the songlength database or keep the default
    if (!m_timer.valid)
    {
        int_least32_t length = songlengthDB == sldb_t::MD5
            ? m_database.lengthMs(m_tune)
            : (m_database.length(m_tune) * 1000);
        if (length > 0)
        {
            if (m_engCfg.forceC64Model)
            {
                // The model is forced. Adjust the song length
                // if it doesn't match what the tune is made for
                length *=
                    (tuneInfo->clockSpeed() != SidTuneInfo::CLOCK_PAL)
                    ? FREQ_NTSC
                    : FREQ_PAL;
                length /=
                    (m_engCfg.defaultC64Model == SidConfig::NTSC) ||
                    (m_engCfg.defaultC64Model == SidConfig::OLD_NTSC)
                    ? FREQ_NTSC
                    : FREQ_PAL;
            }
            m_timer.length = length;
        }
    }

    // Set up the play timer
    m_timer.stop = m_timer.length;
#ifdef FEAT_NEW_PLAY_API
    if (m_fadeAfter)
        m_timer.stop += m_fadeoutTime;
#endif

    if (m_timer.valid)
    {   // Length relative to start
        if (m_timer.stop > 0)
            m_timer.stop += m_timer.start;
    }
    else
    {   // Check to make start time dosen't exceed end
        if ((m_timer.stop != 0) && (m_timer.start >= m_timer.stop))
        {
            displayError ("ERROR: Start time exceeds song length!");
            return false;
        }
    }

    m_timer.current = ~0;
    m_timer.starting = true;
    m_state = playerRunning;

    // Update display
    menu();
    updateDisplay();
    return true;
}

void ConsolePlayer::close ()
{
#ifndef FEAT_NEW_PLAY_API
    m_engine.stop();
#endif
    if (m_state == playerExit)
    {   // Natural finish
        emuflush ();
        if (m_driver.file)
            cerr << (char) 7; // Bell
    }
    else // Destroy buffers
        m_driver.selected->reset ();

    // Shutdown drivers, etc
    createOutput    (output_t::NONE, nullptr);
    createSidEmu    (EMU_NONE, nullptr);
    m_engine.load   (nullptr);
    m_engine.config (m_engCfg);
}

// Flush any hardware sid fifos so all music is played
void ConsolePlayer::emuflush ()
{
    switch (m_driver.sid)
    {
#ifdef HAVE_SIDPLAYFP_BUILDERS_HARDSID_H
    case EMU_HARDSID:
        ((HardSIDBuilder *)m_engCfg.sidEmulation)->flush ();
        break;
#endif // HAVE_SIDPLAYFP_BUILDERS_HARDSID_H
#ifdef HAVE_SIDPLAYFP_BUILDERS_EXSID_H
    case EMU_EXSID:
        ((exSIDBuilder *)m_engCfg.sidEmulation)->flush ();
        break;
#endif // HAVE_SIDPLAYFP_BUILDERS_EXSID_H
#ifdef HAVE_SIDPLAYFP_BUILDERS_USBSID_H
    case EMU_USBSID:
        ((USBSIDBuilder *)m_engCfg.sidEmulation)->flush ();
        break;
#endif // HAVE_SIDPLAYFP_BUILDERS_USBSID_H
    default:
        break;
    }
}

// Out play loop to be externally called
bool ConsolePlayer::play()
{
    uint_least32_t frames = 0;
    if (m_state == playerRunning) LIKELY
    {
        updateDisplay();
#ifdef FEAT_NEW_PLAY_API
        // fadeout
        const uint_least32_t fadeoutTime = m_fadeoutTime;
        if (fadeoutTime && (m_timer.stop > fadeoutTime)) UNLIKELY
        {
            const uint_least32_t timeleft = m_timer.stop - m_timer.current;
            if (timeleft <= fadeoutTime) UNLIKELY
            {
                double a = (double)timeleft / fadeoutTime;
                double v = a / (1. + (1.-a)*0.25);
                m_mixer.setVolume(Mixer::VOLUME_MAX * v);
            }
        }
#endif
        // Fill buffer
        // getBufSize returns the number of frames
        // multiply by number of channels to get the count of 16bit samples
        const uint_least32_t length = getBufSize() * m_driver.cfg.channels;
        short *buffer = m_driver.selected->buffer();
#ifdef FEAT_NEW_PLAY_API
        m_mixer.begin(buffer, length);
        short* buffers[3];
        m_engine.buffers(buffers);

        do
        {
            int samples = m_engine.play(2000);
            if (samples < 0) UNLIKELY
            {
                displayError (m_engine.error());
                m_state = playerError;
                return false;
            }
            if (!buffer) UNLIKELY
                break;
            if (samples > 0)
                m_mixer.doMix(buffers, samples);
            else break;
        }
        while (!m_mixer.isFull());

        // m_engine.play returns the number of 16bit samples
        // divide by number of channels to get the count of frames
        frames = length / m_driver.cfg.channels;
#else
        uint_least32_t samples = m_engine.play(buffer, length);
        if ((samples < length) || !m_engine.isPlaying()) UNLIKELY
        {
            displayError (m_engine.error());
            m_state = playerError;
            return false;
        }
        // m_engine.play returns the number of 16bit samples
        // divide by number of channels to get the count of frames
        frames = samples / m_driver.cfg.channels;
#endif
    }
#ifdef HAVE_UNISTD_H
    else
        usleep(100000);
#endif

    switch (m_state)
    {
    LIKELY case playerRunning:
        if (!m_driver.selected->write(frames)) UNLIKELY
        {
            cerr << m_driver.selected->getErrorString();
            m_state = playerError;
            return false;
        }
        // fall-through
    case playerPaused:
        // Check for a keypress (approx 250ms rate, but really depends
        // on music buffer sizes).  Don't do this for high quiet levels
        // as chances are we are under remote control.
        if ((m_quietLevel < 2) && _kbhit ())
            decodeKeys ();
        return true;
    default:
        if (m_quietLevel < 2)
            cerr << endl;
#ifndef FEAT_NEW_PLAY_API
        m_engine.stop ();
#endif
#if HAVE_TSID == 1
        if (m_tsid)
        {
            m_tsid.addTime((int)(m_timer.current/1000), m_track.selected, m_filename);
        }
#elif HAVE_TSID == 2
        if (m_tsid)
        {
            char md5[SidTune::MD5_LENGTH + 1];
            if (newSonglengthDB)
                m_tune.createMD5New(md5);
            else
                m_tune.createMD5(md5);
            int_least32_t length = m_database.lengthMs(md5, m_track.selected);
            // ignore errors
            if (length < 0)
                length = 0;
            m_tsid.addTime(md5, m_filename, (uint)(m_timer.current/1000),
                            m_track.selected, (uint)(length/1000));
        }
#endif
        break;
    }
    return false;
}


void ConsolePlayer::stop ()
{
    m_state = playerStopped;
#ifndef FEAT_NEW_PLAY_API
    m_engine.stop ();
#endif
}


uint_least32_t ConsolePlayer::getBufSize()
{
    if (m_timer.starting && (m_timer.current >= m_timer.start)) UNLIKELY
    {   // Switch audio drivers.
        m_timer.starting = false;
        m_driver.selected = m_driver.device;
        m_driver.selected->clearBuffer();
#ifdef FEAT_NEW_PLAY_API
        m_mixer.clear();
        m_mixer.setFastForward(1);
#else
        m_engine.fastForward(100);
#endif
        m_speed.current = 1;
        if (m_cpudebug)
            m_engine.debug (true, nullptr);
    }
    else if ((m_timer.stop != 0) && (m_timer.current >= m_timer.stop)) UNLIKELY
    {
        m_state = playerExit;
        if (m_track.loop)
        {
            m_state = playerRestart;
        }
        else
        {
            if (m_track.single)
                return 0;
            // Move to next track
            m_track.selected++;
            if (m_track.selected > m_track.songs)
                m_track.selected = 1;
            if (m_track.selected == m_track.first)
                return 0;
            m_state = playerRestart;
        }
    }
    else
    {
        uint_least32_t remainingMs = m_timer.stop - m_timer.current;
        uint_least32_t bufSize = (remainingMs * m_driver.cfg.frequency) / 1000;
        if (bufSize < m_driver.cfg.bufSize)
            return bufSize;
    }

    return m_driver.cfg.bufSize;
}


// External Timer Event
void ConsolePlayer::updateDisplay()
{
    const uint_least32_t milliseconds = m_engine.timeMs();
    const uint_least32_t seconds = milliseconds / 1000;

    refreshRegDump();

    if (!m_quietLevel && (seconds != (m_timer.current / 1000)))
    {
        //cerr << "\b\b\b\b\b";
        cerr << std::setw(2) << std::setfill('0')
             << ((seconds / 60) % 100) << ':' << std::setw(2)
             << std::setfill('0') << (seconds % 60) << std::flush;
    }

    m_timer.current = milliseconds;
}

void ConsolePlayer::displayError (const char *error)
{
    cerr << m_name << ": " << error << endl;
}

// Keyboard handling
void ConsolePlayer::decodeKeys ()
{
    do
    {
        const int action = keyboard_decode ();
        if (action == A_INVALID)
            continue;

        switch (action)
        {
        case A_RIGHT_ARROW:
            m_state = playerFastRestart;
            if (!m_track.single)
            {
                m_track.selected++;
                if (m_track.selected > m_track.songs)
                    m_track.selected = 1;
            }
        break;

        case A_LEFT_ARROW:
            m_state = playerFastRestart;
            if (!m_track.single)
            {   // Only select previous song if less than timeout
                // else restart current song
                const uint_least32_t milliseconds = m_engine.timeMs();
                if (milliseconds < SID2_PREV_SONG_TIMEOUT)
                {
                    m_track.selected--;
                    if (m_track.selected < 1)
                        m_track.selected = m_track.songs;
                }
            }
        break;

        case A_UP_ARROW:
            m_speed.current *= 2;
            if (m_speed.current > m_speed.max)
                m_speed.current = m_speed.max;

#ifdef FEAT_NEW_PLAY_API
            m_mixer.setFastForward(m_speed.current);
#else
            m_engine.fastForward(100 * m_speed.current);
#endif
        break;

        case A_DOWN_ARROW:
            m_speed.current = 1;
#ifdef FEAT_NEW_PLAY_API
            m_mixer.setFastForward(1);
#else
            m_engine.fastForward(100);
#endif
        break;

        case A_HOME:
            m_state = playerFastRestart;
            m_track.selected = 1;
        break;

        case A_END:
            m_state = playerFastRestart;
            m_track.selected = m_track.songs;
        break;

        case A_PAUSED:
            if (m_state == playerPaused)
            {
                cerr << "\b\b\b\b\b\b\b\b\b";
                // Just to make sure PAUSED is removed from screen
                cerr << "         ";
                cerr << "\b\b\b\b\b\b\b\b\b";
                m_state  = playerRunning;
            }
            else
            {
                cerr << " [PAUSED]";
                m_state = playerPaused;
                m_driver.selected->pause ();
            }
        break;

        case A_HELP:
            m_showhelp = !m_showhelp;
            menu();
        break;

        case A_TOGGLE_VOICE1:
            m_mute_channel.flip(0);
            m_engine.mute(0, 0, m_mute_channel[0]);
        break;

        case A_TOGGLE_VOICE2:
            m_mute_channel.flip(1);
            m_engine.mute(0, 1, m_mute_channel[1]);
        break;

        case A_TOGGLE_VOICE3:
            m_mute_channel.flip(2);
            m_engine.mute(0, 2, m_mute_channel[2]);
        break;

        case A_TOGGLE_VOICE4:
            m_mute_channel.flip(3);
            m_engine.mute(1, 0, m_mute_channel[3]);
        break;

        case A_TOGGLE_VOICE5:
            m_mute_channel.flip(4);
            m_engine.mute(1, 1, m_mute_channel[4]);
        break;

        case A_TOGGLE_VOICE6:
            m_mute_channel.flip(5);
            m_engine.mute(1, 2, m_mute_channel[5]);
        break;

        case A_TOGGLE_VOICE7:
            m_mute_channel.flip(6);
            m_engine.mute(2, 0, m_mute_channel[6]);
        break;

        case A_TOGGLE_VOICE8:
            m_mute_channel.flip(7);
            m_engine.mute(2, 1, m_mute_channel[7]);
        break;

        case A_TOGGLE_VOICE9:
            m_mute_channel.flip(8);
            m_engine.mute(2, 2, m_mute_channel[8]);
        break;
#ifdef FEAT_SAMPLE_MUTE
        case A_TOGGLE_SAMPLE1:
            m_mute_samples.flip(0);
            m_engine.mute(0, 3, m_mute_samples[0]);
        break;
        case A_TOGGLE_SAMPLE2:
            m_mute_samples.flip(1);
            m_engine.mute(1, 3, m_mute_samples[1]);
        break;
        case A_TOGGLE_SAMPLE3:
            m_mute_samples.flip(2);
            m_engine.mute(2, 3, m_mute_samples[2]);
        break;
#endif
        case A_TOGGLE_FILTER:
            m_filter.enabled = !m_filter.enabled;
#ifdef FEAT_FILTER_DISABLE
        for (int chip=0; chip<3; chip++)
        {
            m_engine.filter(chip, m_filter.enabled);
        }
#else
            m_engCfg.sidEmulation->filter(m_filter.enabled);
#endif
        break;

        case A_QUIT:
            m_state = playerFastExit;
            return;
        break;
        }
    } while (_kbhit ());
}
