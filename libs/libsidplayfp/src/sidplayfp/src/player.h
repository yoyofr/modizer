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

#ifndef PLAYER_H
#define PLAYER_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <sidplayfp/SidTune.h>
#include <sidplayfp/sidplayfp.h>
#include <sidplayfp/SidConfig.h>
#include <sidplayfp/SidTuneInfo.h>
#include <sidplayfp/SidDatabase.h>

#include "audio/IAudio.h"
#include "audio/AudioConfig.h"
#include "audio/null/null.h"
#include "IniConfig.h"

#include "setting.h"

#ifdef FEAT_NEW_PLAY_API
#  include <mixer.h>
#endif

#include "siddefines.h"
#include "sidlib_features.h"

#include <string>
#include <bitset>

#ifdef HAVE_TSID
#  if HAVE_TSID > 1
#    include <tsid2/tsid2.h>
#    define TSID TSID2
#  else
#    include <tsid/tsid.h>
#  endif
#endif

typedef enum
{
    playerError = 0,
    playerRunning,
    playerPaused,
    playerStopped,
    playerRestart,
    playerExit,
    playerFast = 128,
    playerFastRestart = playerRestart | playerFast,
    playerFastExit = playerExit | playerFast
} player_state_t;

typedef enum
{
    /* Same as EMU_DEFAULT except no soundcard.
    Still allows wav generation */
    EMU_NONE = 0,
    /* The following require a soundcard */
    EMU_DEFAULT,
    EMU_RESIDFP,
    EMU_SIDLITE,
    EMU_RESID,
    /* The following should disable the soundcard */
    EMU_HARDSID,
    EMU_EXSID,
    EMU_USBSID,
    EMU_SIDSTATION,
    EMU_COMMODORE,
    EMU_SIDSYN,
    EMU_END
} SIDEMUS;

enum class output_t
{
    /* Define possible output sources */
    NONE,
    /* Hardware */
    SOUNDCARD,
    /* File creation support */
    WAV,
    AU,
    END
};

// Songlength DB.
enum class sldb_t
{
    NONE,
    TXT,
    MD5
};

// Grouped global variables
class ConsolePlayer
{
private:
#ifdef HAVE_SIDPLAYFP_BUILDERS_RESIDFP_H
    static const char  RESIDFP_ID[];
#endif
#ifdef HAVE_SIDPLAYFP_BUILDERS_SIDLITE_H
    static const char  SIDLITE_ID[];
#endif
#ifdef HAVE_SIDPLAYFP_BUILDERS_RESID_H
    static const char  RESID_ID[];
#endif
#ifdef HAVE_SIDPLAYFP_BUILDERS_HARDSID_H
    static const char  HARDSID_ID[];
#endif
#ifdef HAVE_SIDPLAYFP_BUILDERS_EXSID_H
    static const char  EXSID_ID[];
#endif
#ifdef HAVE_SIDPLAYFP_BUILDERS_USBSID_H
    static const char  USBSID_ID[];
#endif
#ifdef HAVE_TSID
    TSID               m_tsid;
#endif

    const char* const  m_name;
    sidplayfp          m_engine;
    SidConfig          m_engCfg;
    SidTune            m_tune;
    player_state_t     m_state;
    const char*        m_outfile;
    std::string        m_filename;

    IniConfig          m_iniCfg;
    SidDatabase        m_database;

    Setting<double>    m_fcurve;
#ifdef FEAT_FILTER_RANGE
    Setting<double>    m_frange;
#endif

#ifdef FEAT_CW_STRENGTH
    SidConfig::sid_cw_t m_combinedWaveformsStrength;
#endif
#ifdef FEAT_NEW_PLAY_API
    uint_least32_t     m_fadeoutTime;
    /*
     * fade out after song end
     * if true fade out time will be added to song duration
     */
    bool               m_fadeAfter;
#endif
#ifdef FEAT_REGS_DUMP_SID
    uint8_t            m_registers[3][32];
    uint16_t*          m_freqTable;
#endif

    // Display parameters
    uint_least8_t      m_quietLevel;
    uint_least8_t      m_verboseLevel;
    bool               m_showhelp;

    sldb_t             songlengthDB;

    bool               m_cpudebug;

    bool               m_autofilter;

    std::bitset<9>     m_mute_channel;
#ifdef FEAT_SAMPLE_MUTE
    std::bitset<3>     m_mute_samples;
#endif

    int  m_channels;
    int  m_precision;
    int  m_buffer_size;
#ifdef FEAT_NEW_PLAY_API
    Mixer m_mixer;
#endif
    struct m_filter_t
    {
        // Filter parameter for reSID
        double         bias;
        // Filter parameters for reSIDfp
        double         filterCurve6581;
#ifdef FEAT_FILTER_RANGE
        double         filterRange6581;
#endif
        double         filterCurve8580;

        bool           enabled;
    } m_filter;

    struct m_driver_t
    {
        output_t       output;   // Selected output type
        SIDEMUS        sid;      // Sid emulation
        bool           file;     // File based driver
        bool           info;     // File metadata
        AudioConfig    cfg;
        IAudio*        selected; // Selected Output Driver
        IAudio*        device;   // HW/File Driver
        Audio_Null     null;     // Used for everything
    } m_driver;

    struct m_timer_t
    {   // secs
        uint_least32_t start;
        uint_least32_t current;
        uint_least32_t stop;
        uint_least32_t length;
        bool           valid;
        bool           starting;
    } m_timer;

    struct m_track_t
    {
        uint_least16_t first;
        uint_least16_t selected;
        uint_least16_t songs;
        bool           loop;
        bool           single;
    } m_track;

    struct m_speed_t
    {
        uint_least8_t current;
        uint_least8_t max;
    } m_speed;

private:
    // Console
    void consoleColour  (color_t colour);
    void consoleTable   (table_t table);
    void consoleRestore (void);

    // Command line args
    void displayArgs    (const char *arg = nullptr);
    void displayVersion ();

    bool createOutput   (output_t driver, const SidTuneInfo *tuneInfo);
    bool createSidEmu   (SIDEMUS emu, const SidTuneInfo *tuneInfo);
    void decodeKeys     (void);
    void updateDisplay();
    void emuflush       (void);
    void menu           (void);
    void refreshRegDump ();

    uint_least32_t getBufSize();

    const char *getNote(uint16_t freq);

    std::string getFileName(const SidTuneInfo *tuneInfo, const char* ext);

    inline bool tryOpenTune(const char *hvscBase);
    inline bool tryOpenDatabase(const char *hvscBase, const char *suffix);

public:
    ConsolePlayer (const char * const name);
    virtual ~ConsolePlayer() = default;

    void displayError(const char *error);

    int  args  (int argc, const char *argv[]);
    bool open  (void);
    void close (void);
    bool play  (void);
    void stop  (void);

    player_state_t state (void) const { return m_state; }

    bool isInteractive() const { return !m_driver.file; }
};

#endif // PLAYER_H
