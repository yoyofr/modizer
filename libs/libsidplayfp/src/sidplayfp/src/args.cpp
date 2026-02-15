/*
 * This file is part of sidplayfp, a console SID player.
 *
 * Copyright 2011-2026 Leandro Nini
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

#include "dataParser.h"

#include <iostream>

#include <climits>
#include <cstdlib>
#include <cstring>

#include "ini/types.h"

#include "sidlib_features.h"

#include "sidcxx11.h"

using std::cout;
using std::cerr;
using std::endl;

#ifdef HAVE_SIDPLAYFP_BUILDERS_HARDSID_H
#  include <sidplayfp/builders/hardsid.h>
#endif

#ifdef HAVE_SIDPLAYFP_BUILDERS_EXSID_H
#  include <sidplayfp/builders/exsid.h>
#endif

#ifdef HAVE_SIDPLAYFP_BUILDERS_USBSID_H
#  include <sidplayfp/builders/usbsid.h>
#endif

// Wide-chars are not yet supported here
#undef SEPARATOR
#define SEPARATOR "/"

/**
 * Try load SID tune from HVSC_BASE
 */
bool ConsolePlayer::tryOpenTune(const char *hvscBase)
{
    std::string newFileName(hvscBase);

    newFileName.append(SEPARATOR).append(m_filename);
    m_tune.load(newFileName.c_str());
    if (!m_tune.getStatus())
    {
        return false;
    }

    m_filename.assign(newFileName);
    return true;
}

/**
 * Try load songlength DB from HVSC_BASE
 */
bool ConsolePlayer::tryOpenDatabase(const char *hvscBase, const char *suffix)
{
    std::string newFileName(hvscBase);

    newFileName.append(SEPARATOR).append("DOCUMENTS").append(SEPARATOR).append("Songlengths.").append(suffix);
    return m_database.open(newFileName.c_str());
}

// Convert time from integer
bool parseTime(const char *str, uint_least32_t &time)
{
    // Check for empty string
    if (*str == '\0')
        return false;

    uint_least32_t _time;
    uint_least32_t milliseconds = 0;

    char *sep = (char *) strstr (str, ":");
    if (!sep)
    {   // User gave seconds
        _time = std::atoi (str);
    }
    else
    {   // Read in MM:SS[.mmm] format
        int val;
        *sep = '\0';
        val  = std::atoi (str);
        if (val < 0 || val > 99)
            return false;
        _time = (uint_least32_t) val * 60;
        // parse milliseconds
        char *milli = (char *) strstr (sep+1, ".");
        if (milli)
        {
            char *start = milli + 1;
            char *end;
            milliseconds = strtol(start, &end, 10);
            switch (end - start)
            {
            case 1: milliseconds *= 100; break;
            case 2: milliseconds *= 10; break;
            case 3: break;
            default: return false;
            }

            if (milliseconds > 999)
                return false;

            *milli = '\0';
        }
        val   = std::atoi (sep + 1);
        if (val < 0 || val > 59)
            return false;
        _time += (uint_least32_t) val;
    }

    time = _time * 1000 + milliseconds;
    return true;
}

bool parseAddress(const char *str, uint_least16_t &address)
{
    if (*str == '\0')
        return false;

    long x = strtol(str, nullptr, 0);

    address = x;
    return true;
}

void displayDebugArgs()
{
    std::ostream &out = cout;

    out << "Debug Options:" << endl
        << " --cpu-debug   display cpu register and assembly dumps" << endl
        << " --delay=<num> simulate c64 power on delay (default: random)" << endl
        << " --noaudio     no audio output device" << endl
        << " --nosid       no sid emulation" << endl
        << " --none        no audio output device and no sid emulation" << endl;
}

// Parse command line arguments
int ConsolePlayer::args(int argc, const char *argv[])
{
    if (argc == 0) // at least one argument required
    {
        displayArgs ();
        return -1;
    }

    // default arg options
    m_driver.file   = false;
    m_driver.info   = false;

    m_mute_channel.reset();

    int  infile = 0;
    int  i      = 0;
    bool err    = false;

    // parse command line arguments
    while ((i < argc) && (argv[i] != nullptr))
    {
        if ((argv[i][0] == '-') && (argv[i][1] != '\0'))
        {
            // help options
            if ((argv[i][1] == 'h') || !std::strcmp(&argv[i][1], "-help"))
            {
                displayArgs ();
                return 0;
            }
            else if (!std::strcmp(&argv[i][1], "-help-debug"))
            {
                displayDebugArgs ();
                return 0;
            }
            else if (!std::strcmp(&argv[i][1], "-version"))
            {
                displayVersion();
                return 0;
            }

            else if (argv[i][1] == 'b')
            {
                if (!parseTime (&argv[i][2], m_timer.start))
                    err = true;
            }
            else if (std::strncmp (&argv[i][1], "ds", 2) == 0)
            {   // Override sidTune and enable the second sid
                if (!parseAddress (&argv[i][3], m_engCfg.secondSidAddress))
                    err = true;
            }
            else if (std::strncmp (&argv[i][1], "ts", 2) == 0)
            {   // Override sidTune and enable the third sid
                if (!parseAddress (&argv[i][3], m_engCfg.thirdSidAddress))
                    err = true;
            }
#ifdef FEAT_NEW_PLAY_API
            else if (std::strncmp (&argv[i][1], "fo", 2) == 0)
            {
                m_fadeAfter = true;
                int j = 3;
                if (argv[i][j] == 'a')
                {
                    j++;
                }
                else if (argv[i][j] == 'b')
                {
                    j++;
                    m_fadeAfter = false;
                }
                if (argv[i][j] == '\0')
                    err = true;
                int fadeoutTime = (uint_least32_t) std::atoi (&argv[i][j]);
                if (fadeoutTime < 0)
                {
                    err = true;
                }
                m_fadeoutTime = static_cast<uint_least32_t>(fadeoutTime);
            }
#endif
            else if (argv[i][1] == 'f')
            {
                if (argv[i][2] == '\0')
                    err = true;
                m_engCfg.frequency = (uint_least32_t) std::atoi (argv[i]+2);
            }

            // No filter options
            else if (std::strncmp (&argv[i][1], "nf", 2) == 0)
            {
                if (argv[i][3] == '\0')
                    m_filter.enabled = false;
            }

            // Track options
            else if (std::strncmp (&argv[i][1], "ols", 3) == 0)
            {
                m_track.loop   = true;
                m_track.single = true;
                m_track.first  = std::atoi(&argv[i][4]);
            }
            else if (std::strncmp (&argv[i][1], "ol", 2) == 0)
            {
                m_track.loop  = true;
                m_track.first = std::atoi(&argv[i][3]);
            }
            else if (std::strncmp (&argv[i][1], "os", 2) == 0)
            {
                m_track.single = true;
                m_track.first  = std::atoi(&argv[i][3]);
            }
            else if (argv[i][1] == 'o')
            {   // User forgot track number ?
                if (argv[i][2] == '\0')
                    err = true;
                m_track.first = std::atoi(&argv[i][2]);
            }

            // Channel muting
            else if (argv[i][1] == 'u')
            {
                if (argv[i][2] == '\0')
                    err = true;
                else
                {
                    const unsigned int voice = std::atoi(&argv[i][2]);
                    if (voice > 0 && voice <= m_mute_channel.size())
                        m_mute_channel[voice-1] = true;
                }
            }

#ifdef FEAT_SAMPLE_MUTE
            // Sample muting
            else if (argv[i][1] == 'g')
            {
                if (argv[i][2] == '\0')
                    err = true;
                else
                {
                    const unsigned int chip = std::atoi(&argv[i][2]);
                    if (chip > 0 && chip <= m_mute_samples.size())
                        m_mute_samples[chip-1] = true;
                }
            }
#endif
            else if (argv[i][1] == 'p')
            {   // User forgot precision
                if (argv[i][2] == '\0')
                    err = true;
                {
                    uint_least8_t precision = std::atoi(&argv[i][2]);
                    if (precision <= 16)
                        m_precision = 16;
                    else
                        m_precision = 32;
                }
            }

            else if (argv[i][1] == 'q')
            {
                if (argv[i][2] == '\0')
                    m_quietLevel = 1;
                else
                    m_quietLevel = std::atoi(&argv[i][2]);
            }

            else if (argv[i][1] == 't')
            {
                if (!parseTime (&argv[i][2], m_timer.length))
                    err = true;
                m_timer.valid = true;
            }

            // Resampling Options ----------
#ifdef HAVE_SIDPLAYFP_BUILDERS_RESID_H
            else if (std::strcmp (&argv[i][1], "rif") == 0)
            {
                m_engCfg.samplingMethod = SidConfig::INTERPOLATE;
                m_engCfg.fastSampling = true;

            }
            else if (std::strcmp (&argv[i][1], "rrf") == 0)
            {
                m_engCfg.samplingMethod = SidConfig::RESAMPLE_INTERPOLATE;
                m_engCfg.fastSampling = true;
            }
#endif
            else if (std::strcmp (&argv[i][1], "ri") == 0)
            {
                m_engCfg.samplingMethod = SidConfig::INTERPOLATE;
            }
            else if (std::strcmp (&argv[i][1], "rr") == 0)
            {
                m_engCfg.samplingMethod = SidConfig::RESAMPLE_INTERPOLATE;
            }

            // SID model options
            else if (std::strcmp (&argv[i][1], "mof") == 0)
            {
                m_engCfg.defaultSidModel = SidConfig::MOS6581;
                m_engCfg.forceSidModel = true;
            }
            else if (std::strcmp (&argv[i][1], "mnf") == 0)
            {
                m_engCfg.defaultSidModel = SidConfig::MOS8580;
                m_engCfg.forceSidModel = true;
            }
            else if (std::strcmp (&argv[i][1], "mo") == 0)
            {
                m_engCfg.defaultSidModel = SidConfig::MOS6581;
            }
            else if (std::strcmp (&argv[i][1], "mn") == 0)
            {
                m_engCfg.defaultSidModel = SidConfig::MOS8580;
            }

            else if (argv[i][1] == 's')
            {   // Stereo Playback
                m_channels = 2;
            }
            else if (argv[i][1] == 'm')
            {   // Mono Playback
                m_channels = 1;
            }

            else if (std::strcmp (&argv[i][1], "-digiboost") == 0)
            {
                m_engCfg.digiBoost = true;
            }

            // Video/Verbose Options
            else if (std::strcmp (&argv[i][1], "vnf") == 0)
            {
                m_engCfg.forceC64Model = true;
                m_engCfg.defaultC64Model  = SidConfig::NTSC;
            }
            else if (std::strcmp (&argv[i][1], "vpf") == 0)
            {
                m_engCfg.forceC64Model = true;
                m_engCfg.defaultC64Model  = SidConfig::PAL;
            }
            else if (std::strcmp (&argv[i][1], "vf") == 0)
            {
                m_engCfg.forceC64Model = true;
            }
            else if (std::strcmp (&argv[i][1], "vn") == 0)
            {
                m_engCfg.defaultC64Model  = SidConfig::NTSC;
            }
            else if (std::strcmp (&argv[i][1], "vp") == 0)
            {
                m_engCfg.defaultC64Model  = SidConfig::PAL;
            }
            else if (argv[i][1] == 'v')
            {
                if (argv[i][2] == '\0')
                    m_verboseLevel = 1;
                else
                    m_verboseLevel = std::atoi(&argv[i][2]);
            }
            else if (std::strncmp (&argv[i][1], "-delay=", 7) == 0)
            {
                m_engCfg.powerOnDelay = (uint_least16_t) std::atoi(&argv[i][8]);
            }
            else if (std::strncmp (&argv[i][1], "-fcurve=", 8) == 0)
            {
                if (std::strncmp (&argv[i][9], "auto", 4) == 0)
                {
                    m_autofilter = true;
                }
                else
                {
                    m_fcurve = dataParser::parseDouble(&argv[i][9]);
                }
            }
#ifdef FEAT_FILTER_RANGE
            else if (std::strncmp (&argv[i][1], "-frange=", 8) == 0)
            {
                if (std::strncmp (&argv[i][9], "auto", 4) == 0)
                {
                    m_autofilter = true;
                }
                else
                {
                    m_frange = dataParser::parseDouble(&argv[i][9]);
                }
            }
#endif
#ifdef FEAT_CW_STRENGTH
            else if (std::strcmp (&argv[i][1], "cww") == 0)
            {
                m_combinedWaveformsStrength  = SidConfig::WEAK;
            }
            else if (std::strcmp (&argv[i][1], "cwa") == 0)
            {
                m_combinedWaveformsStrength  = SidConfig::AVERAGE;
            }
            else if (std::strcmp (&argv[i][1], "cws") == 0)
            {
                m_combinedWaveformsStrength  = SidConfig::STRONG;
            }
#endif
            // File format conversions
            else if (argv[i][1] == 'w')
            {
                m_driver.output = output_t::WAV;
                m_driver.file   = true;
                if (argv[i][2] != '\0')
                    m_outfile = &argv[i][2];
            }
            else if (std::strncmp (&argv[i][1], "-wav", 4) == 0)
            {
                m_driver.output = output_t::WAV;
                m_driver.file   = true;
                if (argv[i][5] != '\0')
                    m_outfile = &argv[i][5];
            }
            else if (std::strncmp (&argv[i][1], "-au", 3) == 0)
            {
                m_driver.output = output_t::AU;
                m_driver.file   = true;
                if (argv[i][4] != '\0')
                    m_outfile = &argv[i][4];
            }
            else if (std::strncmp (&argv[i][1], "-info", 5) == 0)
            {
                m_driver.info   = true;
            }
#ifdef HAVE_SIDPLAYFP_BUILDERS_RESIDFP_H
            else if (std::strcmp (&argv[i][1], "-residfp") == 0)
            {
                m_driver.sid    = EMU_RESIDFP;
            }
#endif // HAVE_SIDPLAYFP_BUILDERS_RESIDFP_H

#ifdef HAVE_SIDPLAYFP_BUILDERS_SIDLITE_H
            else if (std::strcmp (&argv[i][1], "-sidlite") == 0)
            {
                m_driver.sid    = EMU_SIDLITE;
            }
#endif // HAVE_SIDPLAYFP_BUILDERS_SIDLITE_H

#ifdef HAVE_SIDPLAYFP_BUILDERS_RESID_H
            else if (std::strcmp (&argv[i][1], "-resid") == 0)
            {
                m_driver.sid    = EMU_RESID;
            }
#endif // HAVE_SIDPLAYFP_BUILDERS_RESID_H

            // Hardware selection
#ifdef HAVE_SIDPLAYFP_BUILDERS_HARDSID_H
            else if (std::strcmp (&argv[i][1], "-hardsid") == 0)
            {
                m_driver.sid    = EMU_HARDSID;
                m_driver.output = output_t::NONE;
            }
#endif // HAVE_SIDPLAYFP_BUILDERS_HARDSID_H

#ifdef HAVE_SIDPLAYFP_BUILDERS_EXSID_H
            else if (std::strcmp (&argv[i][1], "-exsid") == 0)
            {
                m_driver.sid    = EMU_EXSID;
                m_driver.output = output_t::NONE;
            }
#endif // HAVE_SIDPLAYFP_BUILDERS_EXSID_H

#ifdef HAVE_SIDPLAYFP_BUILDERS_USBSID_H
            else if (std::strcmp (&argv[i][1], "-usbsid") == 0)
            {
                m_driver.sid    = EMU_USBSID;
                m_driver.output = output_t::NONE;
            }
#endif // HAVE_SIDPLAYFP_BUILDERS_USBSID_H

            // These are for debug
            else if (std::strcmp (&argv[i][1], "-none") == 0)
            {
                m_driver.sid    = EMU_NONE;
                m_driver.output = output_t::NONE;
            }
            else if (std::strcmp (&argv[i][1], "-nosid") == 0)
            {
                m_driver.sid = EMU_NONE;
            }
            else if (std::strcmp (&argv[i][1], "-noaudio") == 0)
            {
                m_driver.output = output_t::NONE;
            }
            else if (std::strcmp (&argv[i][1], "-cpu-debug") == 0)
            {
                m_cpudebug = true;
            }

            else
            {
                err = true;
            }

        }
        else
        {   // Reading file name
            if (infile == 0)
                infile = i;
            else
                err = true;
        }

        if (err)
        {
            displayArgs (argv[i]);
            return -1;
        }

        i++;  // next index
    }
#ifdef FEAT_NEW_PLAY_API
    // convert to milliseconds
    m_fadeoutTime *= 1000;
#endif
    const char* hvscBase = getenv("HVSC_BASE");

    // Load the tune
    m_filename = argv[infile];
    m_tune.load(m_filename.c_str());
    if (!m_tune.getStatus())
    {
        std::string errorString(m_tune.statusString());

        // Try prepending HVSC_BASE
        if (!hvscBase || !tryOpenTune(hvscBase))
        {
            displayError(errorString.c_str());
            return -1;
        }
    }

    // If filename specified we can only convert one song
    if (m_outfile != nullptr)
        m_track.single = true;

    // Can only loop if not creating audio files
    if (m_driver.output > output_t::SOUNDCARD)
        m_track.loop = false;

    // Check to see if we are trying to generate an audio file
    // whilst using a hardware emulation
    if (m_driver.file && (m_driver.sid >= EMU_HARDSID))
    {
        displayError ("ERROR: Cannot generate audio files using hardware emulations");
        return -1;
    }

    if (m_driver.info && m_driver.file)
    {
        displayError ("WARNING: metadata can be added only to wav files");
    }

    // Select the desired track
    m_track.first    = m_tune.selectSong (m_track.first);
    m_track.selected = m_track.first;
    if (m_track.single)
        m_track.songs = 1;

    // If user provided no time then load songlength database
    // and set default lengths in case it's not found in there.
    {
        if (m_driver.file && m_timer.valid && !m_timer.length)
        {   // Time of 0 provided for wav generation
            displayError ("ERROR: -t0 invalid in record mode");
            return -1;
        }
        if (!m_timer.valid)
        {
            m_timer.length = m_driver.file
                ? (m_iniCfg.sidplay2()).recordLength
                : (m_iniCfg.sidplay2()).playLength;

            songlengthDB = sldb_t::NONE;
            bool dbOpened = false;
            if (hvscBase)
            {
                if (tryOpenDatabase(hvscBase, "md5"))
                {
                    dbOpened = true;
                    songlengthDB = sldb_t::MD5;
                }
                else if (tryOpenDatabase(hvscBase, "txt"))
                {
                    dbOpened = true;
                    songlengthDB = sldb_t::TXT;
                }
            }

            if (!dbOpened)
            {
                // Try load user configured songlength DB
                if ((m_iniCfg.sidplay2()).database.length() != 0)
                {
                    // Try loading the database specificed by the user
#if defined(_WIN32) && defined(UNICODE)
#  ifdef FEAT_DB_WCHAR_OPEN
                    const wchar_t *database = (m_iniCfg.sidplay2()).database.c_str();
#  else
                    char database[MAX_PATH];
                    const int ret = wcstombs(database, (m_iniCfg.sidplay2()).database.c_str(), sizeof(database));
                    if (ret >= MAX_PATH)
                        database[0] = '\0';
#  endif
#else
                    const char *database = (m_iniCfg.sidplay2()).database.c_str();
#endif
                    if (!m_database.open(database))
                    {
                        displayError (m_database.error ());
                        return -1;
                    }

                    if ((m_iniCfg.sidplay2()).database.find(TEXT(".md5")) != SID_STRING::npos)
                        songlengthDB = sldb_t::MD5;
                    else
                        songlengthDB = sldb_t::TXT;
                }
            }
        }
    }

#if HAVE_TSID == 1
    // Set TSIDs base directory
    if (!m_tsid.setBaseDir(true))
    {
        displayError (m_tsid.getError ());
        return -1;
    }
#endif

    // Configure engine with settings
    if (!m_engine.config (m_engCfg))
    {   // Config failed
        displayError (m_engine.error ());
        return -1;
    }
    return 1;
}


void ConsolePlayer::displayArgs (const char *arg)
{
    std::ostream &out = arg ? cerr : cout;

    if (arg)
        out << "Option Error: " << arg << endl;
    else
        out << "Syntax: " << m_name << " [-<option>...] <datafile>" << endl;

    out << "Options:" << endl
        << " --help|-h    display this screen" << endl
        << " --help-debug debug help menu" << endl
        << " -b<num>      set start time in [mins:]secs[.milli] format (default: 0)" << endl
#ifdef FEAT_NEW_PLAY_API
        << " -fo[a|b]<num> set fade-out time in seconds, ends a(fter) or starts b(efore) actual song end time (default: 0=no fade out)" << endl
#endif
        << " -f<num>      set frequency in Hz (default: "
        << SidConfig::DEFAULT_SAMPLING_FREQ << ")" << endl
        << " -ds<addr>    set second sid address (e.g. -ds0xd420)" << endl
        << " -ts<addr>    set third sid address (e.g. -ts0xd440)" << endl

        << " -u<num>      mute voice <num> (e.g. -u1 -u2)" << endl
#ifdef FEAT_SAMPLE_MUTE
        << " -g<num>      mute samples <num> (e.g. -g1 -g2)" << endl
#endif
        << " -nf          no SID filter emulation" << endl

        << " -o<l|s>[num] looping and/or single track" << endl
        << " -o<num>      start track (default: preset)" << endl

        << " -p<num>      set format for file output (16 = signed 16 bit, 32 = 32 bit float)"
        << "(default: 16)" << endl

        << " -s           force stereo output" << endl
        << " -m           force mono output" << endl

        << " -t<num>      set play length in [mins:]secs[.milli] format (0 is endless)" << endl

        << " -<v|q>[x]    verbose or quiet output. x is the optional level, default=1" << endl
        << " -v[p|n][f]   set VIC PAL/NTSC clock speed (default: defined by song)" << endl
        << "              Use 'f' to force the clock by preventing speed fixing" << endl

        << " -m<o|n>[f]   set SID new/old chip model (default: old)" << endl
        << "              Use 'f' to force the model" << endl
        << " --digiboost  Enable digiboost for 8580 model" << endl
#ifdef HAVE_SIDPLAYFP_BUILDERS_RESID_H
        << " -r[i|r][f]   set resampling method (default: resample interpolate)" << endl
        << "              Use 'f' to enable fast resampling (only for reSID)" << endl
#else
        << " -r[i|r]      set resampling method (default: resample interpolate)" << endl
#endif
        << " --fcurve=<num>|auto Controls the filter curve in the ReSIDfp emulation (0.0 to 1.0, default: 0.5)" << endl
#ifdef FEAT_FILTER_RANGE
        << " --frange=<num>|auto Controls the filter range in the ReSIDfp emulation (0.0 to 1.0, default: 0.5)" << endl
#endif
#ifdef FEAT_CW_STRENGTH
        << " -cw<w|a|s>   Set the strength of combined waveforms, w(eak), a(verage) or s(trong)" << endl
#endif
        << " -w[name]     create wav file (default: <datafile>[n].wav)" << endl
        << " --au[name]   create au file (default: <datafile>[n].au)" << endl
        << " --info       add metadata to wav file" << endl;

#ifdef HAVE_SIDPLAYFP_BUILDERS_RESIDFP_H
    out << " --residfp    use reSIDfp emulation (default)" << endl;
#endif

#ifdef HAVE_SIDPLAYFP_BUILDERS_SIDLITE_H
    out << " --sidlite   use sidlite emulation" << endl;
#endif

#ifdef HAVE_SIDPLAYFP_BUILDERS_RESID_H
    out << " --resid      use reSID emulation" << endl;
#endif

#ifdef HAVE_SIDPLAYFP_BUILDERS_HARDSID_H
    {
        HardSIDBuilder hs("");
        if (hs.availDevices ())
            out << " --hardsid    enable hardsid support" << endl;
    }
#endif
#ifdef HAVE_SIDPLAYFP_BUILDERS_EXSID_H
    {
        exSIDBuilder es("");
#ifndef FEAT_NO_CREATE
        if (es.availDevices ())
#endif
            out << " --exsid      enable exSID support" << endl;
    }
#endif
#ifdef HAVE_SIDPLAYFP_BUILDERS_USBSID_H
    {
        USBSIDBuilder us("");
#ifndef FEAT_NO_CREATE
        if (us.availDevices ())
#endif
            out << " --usbsid     enable USBSID support" << endl;
    }
#endif
    out << endl
        << "Home Page: " PACKAGE_URL << endl;
}
