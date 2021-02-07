/***************************************************************************
                          config.cpp  -  Library Configuration Code
                             -------------------
    begin                : Fri Jul 27 2001
    copyright            : (C) 2001 by Simon White
    email                : s_a_white@email.com
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/***************************************************************************
 *  $Log: config.cpp,v $
 *  Revision 1.41  2004/06/14 20:05:16  s_a_white
 *  Restore optimisation level support
 *
 *  Revision 1.40  2004/03/18 20:20:29  s_a_white
 *  Added sidmapper (so support more the 2 sids).
 *
 *  Revision 1.39  2004/01/08 09:01:34  s_a_white
 *  Support the TOD frequency divider.
 *
 *  Revision 1.38  2004/01/06 21:35:19  s_a_white
 *  Setup the TOD clocks.
 *
 *  Revision 1.37  2003/07/17 19:29:00  s_a_white
 *  Don't try to channel split stereo tunes in stereo audio mode.
 *
 *  Revision 1.36  2003/07/16 20:52:30  s_a_white
 *  Basic tunes can only run in a real C64 environment.
 *
 *  Revision 1.35  2003/07/10 07:32:01  s_a_white
 *  Added missed brace.
 *
 *  Revision 1.34  2003/07/10 07:20:00  s_a_white
 *  Moved sid creation to fix muting and volume problems when switching
 *  sid emulation types.
 *
 *  Revision 1.33  2003/06/27 21:15:26  s_a_white
 *  Tidy up mono to stereo sid conversion, only allowing it theres sufficient
 *  support from the emulation.  Allow user to override whether they want this
 *  to happen.
 *
 *  Revision 1.32  2003/06/27 18:37:50  s_a_white
 *  Remove mono to stereo sid address bodge.  Make the code that used this
 *  check the stereo flag.
 *
 *  Revision 1.31  2003/01/24 19:31:59  s_a_white
 *  An attempt at allowing configuration whilst paused without the tune restarting.
 *
 *  Revision 1.30  2003/01/23 17:33:22  s_a_white
 *  Redundent code removal.
 *
 *  Revision 1.29  2002/10/15 18:14:02  s_a_white
 *  Removed upper limit frequency limit check to allow for oversampling.
 *
 *  Revision 1.28  2002/10/02 19:46:36  s_a_white
 *  RSID support & fix sid model forced operation.
 *
 *  Revision 1.27  2002/09/09 18:09:06  s_a_white
 *  Added error message for psid specific flag set in strict real C64 mode.
 *
 *  Revision 1.26  2002/08/10 22:35:56  s_a_white
 *  Small clock speed fix for when both clockSpeed and clockDefault are set
 *  to SID2_CLOCK_CORRECT.
 *
 *  Revision 1.25  2002/07/17 19:25:28  s_a_white
 *  Temp fix to allow SID model to change for current builder.
 *
 *  Revision 1.24  2002/04/14 21:46:50  s_a_white
 *  PlaySID reads fixed to come from RAM only.
 *
 *  Revision 1.23  2002/03/11 18:01:30  s_a_white
 *  Prevent lockup if config call fails with existing and old configurations.
 *
 *  Revision 1.22  2002/03/04 19:05:49  s_a_white
 *  Fix C++ use of nothrow.
 *
 *  Revision 1.21  2002/03/03 22:01:58  s_a_white
 *  New clock speed & sid model interface.
 *
 *  Revision 1.20  2002/02/18 21:59:10  s_a_white
 *  Added two new clock modes (FIXED).  Seems to be a requirement for
 *  HVSC/sidplayw.
 *
 *  Revision 1.19  2002/02/04 22:08:14  s_a_white
 *  Fixed main voice/sample gains.
 *
 *  Revision 1.18  2002/01/29 21:50:33  s_a_white
 *  Auto switching to a better emulation mode.  m_tuneInfo reloaded after a
 *  config.  Initial code added to support more than two sids.
 *
 *  Revision 1.17  2002/01/16 19:11:38  s_a_white
 *  Always release sid emulations now on a call to sidCreate until a better
 *  method is implemented for hardware emulations with locked sids.
 *
 *  Revision 1.16  2002/01/16 08:23:45  s_a_white
 *  Force a clock speed when unknown.
 *
 *  Revision 1.15  2002/01/15 19:12:54  s_a_white
 *  PSID2NG update.
 *
 *  Revision 1.14  2002/01/14 23:18:56  s_a_white
 *  Make sure xsid releases the old sid emulation when there are errors gaining
 *  a new one.
 *
 *  Revision 1.13  2001/12/20 20:15:23  s_a_white
 *  Fixed bad environment initialisation when switching to legacy modes.
 *
 *  Revision 1.12  2001/12/13 08:28:08  s_a_white
 *  Added namespace support to fix problems with xsidplay.
 *
 *  Revision 1.11  2001/12/11 19:24:15  s_a_white
 *  More GCC3 Fixes.
 *
 *  Revision 1.10  2001/11/23 22:59:59  s_a_white
 *  Added new header
 *
 *  Revision 1.9  2001/10/02 18:27:55  s_a_white
 *  Updated to use new sidbuilder classes.
 *
 *  Revision 1.8  2001/09/20 20:33:54  s_a_white
 *  sid2 now gets correctly set to nullsid for a bad create call.
 *
 *  Revision 1.7  2001/09/20 19:34:11  s_a_white
 *  Error checking added for the builder create calls.
 *
 *  Revision 1.6  2001/09/17 19:02:38  s_a_white
 *  Now uses fixed point maths for sample output and rtc.
 *
 *  Revision 1.5  2001/09/01 11:13:56  s_a_white
 *  Fixes sidplay1 environment modes.
 *
 *  Revision 1.4  2001/08/20 18:24:50  s_a_white
 *  tuneInfo in the info structure now correctly has the sid revision setup.
 *
 *  Revision 1.3  2001/08/12 18:22:45  s_a_white
 *  Fixed bug in Player::sidEmulation call.
 *
 *  Revision 1.2  2001/07/27 12:51:40  s_a_white
 *  Removed warning.
 *
 *  Revision 1.1  2001/07/27 12:12:23  s_a_white
 *  Initial release.
 *
 ***************************************************************************/

#include "sid2types.h"
#include "player.h"

#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif

SIDPLAY2_NAMESPACE_START

// An instance of this structure is used to transport emulator settings
// to and from the interface class.

int Player::config (const sid2_config_t &cfg)
{
    if (m_running)
    {
        m_errorString = ERR_CONF_WHILST_ACTIVE;
        goto Player_configure_error;
    }

    // Check for base sampling frequency
    if (cfg.frequency < 4000)
    {   // Rev 1.6 (saw) - Added descriptive error
        m_errorString = ERR_UNSUPPORTED_FREQ;
        goto Player_configure_error;
    }

    // Only do these if we have a loaded tune
    if (m_tune)
    {
        if (m_playerState != sid2_paused)
            m_tune->getInfo(m_tuneInfo);

        // SID emulation setup (must be performed before the
        // environment setup call)
        if (sidCreate (cfg.sidEmulation, cfg.sidModel, cfg.sidDefault) < 0)
        {
            m_errorString      = cfg.sidEmulation->error ();
            m_cfg.sidEmulation = NULL;
            goto Player_configure_restore;
        }

        if (m_playerState != sid2_paused)
        {
            float64_t cpuFreq;
            // Must be this order:
            // Determine clock speed
            cpuFreq = clockSpeed (cfg.clockSpeed, cfg.clockDefault,
                                  cfg.clockForced);
            // Setup fake cia
            sid6526.clock ((uint_least16_t)(cpuFreq / VIC_FREQ_PAL + 0.5));
            if (m_tuneInfo.songSpeed  == SIDTUNE_SPEED_CIA_1A ||
                m_tuneInfo.clockSpeed == SIDTUNE_CLOCK_NTSC)
            {
                sid6526.clock ((uint_least16_t)(cpuFreq / VIC_FREQ_NTSC + 0.5));
            }

            // @FIXME@ see mos6526.h for details. Setup TOD clock
            if (m_tuneInfo.clockSpeed == SIDTUNE_CLOCK_PAL)
            {
                cia.clock  (cpuFreq / VIC_FREQ_PAL);
                cia2.clock (cpuFreq / VIC_FREQ_PAL);
            }
            else
            {
                cia.clock  (cpuFreq / VIC_FREQ_NTSC);
                cia2.clock (cpuFreq / VIC_FREQ_NTSC);
            }

            /* inform ReSID of the desired sampling rate */
            for (int i = 0; i < SID2_MAX_SIDS; i += 1)
                sid[i]->sampling(cpuFreq, cfg.frequency, cfg.samplingMethod, cfg.fastSampling);

            // Start the real time clock event
            rtc.clock(cpuFreq);

            // Configure, setup and install C64 environment/events
            if (environment (cfg.environment) < 0)
                goto Player_configure_restore;
        }
    }
    sidSamples (cfg.sidSamples);

    // Setup sid mapping table
    // Note this should be based on m_tuneInfo.sidChipBase1
    // but this is only temporary code anyway
    {
        for (int i = 0; i < SID2_MAPPER_SIZE; i++)
            m_sidmapper[i] = 0;
    }

    m_info.channels = 1;
    if (m_tune && m_tuneInfo.sidChipBase2) {
        // Assumed to be in d4xx-d7xx range
        m_sidmapper[(m_tuneInfo.sidChipBase2 >> 5) &
                    (SID2_MAPPER_SIZE - 1)] = 1;
        m_info.channels = 2;
    } else if (cfg.forceDualSids) {
        /* Tune didn't tell us where; let's put the second SID
         * in every slot apart from 0xd400 - 0xd420. */
        for (int i = 0xd420; i < 0xd7ff; i += 0x20)
            m_sidmapper[(i >> 5) & (SID2_MAPPER_SIZE - 1)] = 1;
        m_info.channels = 2;
    }

    m_leftVolume  = cfg.leftVolume;
    m_rightVolume = cfg.rightVolume;

    /* without stereo SID mode, we don't emulate the second chip! */
    if (m_info.channels == 1)
        sid[1] = &nullsid;

    // Update Configuration
    m_cfg = cfg;

return 0;

Player_configure_restore:
    // Try restoring old configuration
    if (&m_cfg != &cfg)
        config (m_cfg);
Player_configure_error:
    return -1;
}

// Clock speed changes due to loading a new song
float64_t Player::clockSpeed (sid2_clock_t userClock, sid2_clock_t defaultClock,
                              bool forced)
{
    float64_t cpuFreq = CLOCK_FREQ_PAL;

    // Detect the Correct Song Speed
    // Determine song speed when unknown
    if (m_tuneInfo.clockSpeed == SIDTUNE_CLOCK_UNKNOWN)
    {
        switch (defaultClock)
        {
        case SID2_CLOCK_PAL:
            m_tuneInfo.clockSpeed = SIDTUNE_CLOCK_PAL;
            break;
        case SID2_CLOCK_NTSC:
            m_tuneInfo.clockSpeed = SIDTUNE_CLOCK_NTSC;
            break;
        case SID2_CLOCK_CORRECT:
            // No default so base it on emulation clock
            m_tuneInfo.clockSpeed = SIDTUNE_CLOCK_ANY;
        }
    }

    // Since song will run correct at any clock speed
    // set tune speed to the current emulation
    if (m_tuneInfo.clockSpeed == SIDTUNE_CLOCK_ANY)
    {
        if (userClock == SID2_CLOCK_CORRECT)
            userClock  = defaultClock;

        switch (userClock)
        {
        case SID2_CLOCK_NTSC:
            m_tuneInfo.clockSpeed = SIDTUNE_CLOCK_NTSC;
            break;
        case SID2_CLOCK_PAL:
        default:
            m_tuneInfo.clockSpeed = SIDTUNE_CLOCK_PAL;
            break;
        }
    }

    if (userClock == SID2_CLOCK_CORRECT)
    {
        switch (m_tuneInfo.clockSpeed)
        {
        case SIDTUNE_CLOCK_NTSC:
            userClock = SID2_CLOCK_NTSC;
            break;
        case SIDTUNE_CLOCK_PAL:
            userClock = SID2_CLOCK_PAL;
            break;
        }
    }

    if (forced)
    {
        m_tuneInfo.clockSpeed = SIDTUNE_CLOCK_PAL;
        if (userClock == SID2_CLOCK_NTSC)
            m_tuneInfo.clockSpeed = SIDTUNE_CLOCK_NTSC;
    }

    if (m_tuneInfo.clockSpeed == SIDTUNE_CLOCK_PAL)
        vic.chip (MOS6569);
    else // if (tuneInfo.clockSpeed == SIDTUNE_CLOCK_NTSC)
        vic.chip (MOS6567R8);

    if (userClock == SID2_CLOCK_PAL)
    {
        cpuFreq = CLOCK_FREQ_PAL;
        m_tuneInfo.speedString = TXT_PAL_VBI;
        if (m_tuneInfo.songSpeed == SIDTUNE_SPEED_CIA_1A)
            m_tuneInfo.speedString = TXT_PAL_CIA;
        else if (m_tuneInfo.clockSpeed == SIDTUNE_CLOCK_NTSC)
            m_tuneInfo.speedString = TXT_PAL_VBI_FIXED;
    }
    else // if (userClock == SID2_CLOCK_NTSC)
    {
        cpuFreq = CLOCK_FREQ_NTSC;
        m_tuneInfo.speedString = TXT_NTSC_VBI;
        if (m_tuneInfo.songSpeed == SIDTUNE_SPEED_CIA_1A)
            m_tuneInfo.speedString = TXT_NTSC_CIA;
        else if (m_tuneInfo.clockSpeed == SIDTUNE_CLOCK_PAL)
            m_tuneInfo.speedString = TXT_NTSC_VBI_FIXED;
    }
    return cpuFreq;
}

int Player::environment (sid2_env_t env)
{
    switch (m_tuneInfo.compatibility)
    {
    case SIDTUNE_COMPATIBILITY_R64:
    case SIDTUNE_COMPATIBILITY_BASIC:
        env = sid2_envR;
        break;
    case SIDTUNE_COMPATIBILITY_PSID:
        if (env == sid2_envR)
            env  = sid2_envBS;
    }

    // Environment already set?
    if (!(m_ram && (m_info.environment == env)))
    {   // Setup new player environment
        m_info.environment = env;
        if (m_ram)
        {
            if (m_ram == m_rom)
               delete [] m_ram;
            else
            {
               delete [] m_rom;
               delete [] m_ram;
            }
        }

#ifdef HAVE_EXCEPTIONS
        m_ram = new(std::nothrow) uint8_t[0x10000];
#else
        m_ram = new uint8_t[0x10000];
#endif

        // Setup the access functions to the environment
        // and the properties the memory has.
        if (m_info.environment == sid2_envPS)
        {   // Playsid has no roms and SID exists in ram space
            m_rom = m_ram;
            m_readMemByte     = &Player::readMemByte_plain;
            m_writeMemByte    = &Player::writeMemByte_playsid;
            m_readMemDataByte = &Player::readMemByte_plain;
        }
        else
        {
#ifdef HAVE_EXCEPTIONS
            m_rom = new(std::nothrow) uint8_t[0x10000];
#else
            m_rom = new uint8_t[0x10000];
#endif

            switch (m_info.environment)
            {
            case sid2_envTP:
                m_readMemByte     = &Player::readMemByte_plain;
                m_writeMemByte    = &Player::writeMemByte_sidplay;
                m_readMemDataByte = &Player::readMemByte_sidplaytp;
            break;

            //case sid2_envTR:
            case sid2_envBS:
                m_readMemByte     = &Player::readMemByte_plain;
                m_writeMemByte    = &Player::writeMemByte_sidplay;
                m_readMemDataByte = &Player::readMemByte_sidplaybs;
            break;

            case sid2_envR:
            default: // <-- Just to please compiler
                m_readMemByte     = &Player::readMemByte_sidplaybs;
                m_writeMemByte    = &Player::writeMemByte_sidplay;
                m_readMemDataByte = &Player::readMemByte_sidplaybs;
            break;
            }
        }
    }

    {   // Have to reload the song into memory as
        // everything has changed
        int ret;
        sid2_env_t old = m_info.environment;
        m_info.environment = env;
        ret = initialise ();
        m_info.environment = old;
        return ret;
    }
}

sid2_model_t Player::getModel(int sidModel,
                              sid2_model_t userModel,
                              sid2_model_t defaultModel)
{
    if (sidModel == SIDTUNE_SIDMODEL_UNKNOWN)
    {
        switch (defaultModel)
        {
        case SID2_MOS6581:
            sidModel = SIDTUNE_SIDMODEL_6581;
            break;
        case SID2_MOS8580:
            sidModel = SIDTUNE_SIDMODEL_8580;
            break;
        case SID2_MODEL_CORRECT:
            // No default so base it on emulation clock
            sidModel = SIDTUNE_SIDMODEL_ANY;
        }
    }

    // Since song will run correct on any sid model
    // set it to the current emulation
    if (sidModel == SIDTUNE_SIDMODEL_ANY)
    {
        if (userModel == SID2_MODEL_CORRECT)
            userModel  = defaultModel;

        switch (userModel)
        {
        case SID2_MOS8580:
            sidModel = SIDTUNE_SIDMODEL_8580;
            break;
        case SID2_MOS6581:
        default:
            sidModel = SIDTUNE_SIDMODEL_6581;
            break;
        }
    }

    switch (userModel)
    {
    case SID2_MODEL_CORRECT:
        switch (sidModel)
        {
        case SIDTUNE_SIDMODEL_8580:
            userModel = SID2_MOS8580;
            break;
        case SIDTUNE_SIDMODEL_6581:
            userModel = SID2_MOS6581;
            break;
        }
    break;
    // Fixup tune information if model is forced
    case SID2_MOS6581:
        sidModel = SIDTUNE_SIDMODEL_6581;
        break;
    case SID2_MOS8580:
        sidModel = SIDTUNE_SIDMODEL_8580;
        break;
    }

    return userModel;
}

// Integrate SID emulation from the builder class into
// libsidplay2
int Player::sidCreate (sidbuilder *builder, sid2_model_t userModel,
                       sid2_model_t defaultModel)
{
    sid[0] = xsid.emulation ();
   /* @FIXME@ Removed as prevents SID
    * Model being updated
    ****************************************
    // If we are already using the emulation
    // then don't change
    if (builder == sid[0]->builder ())
    {
        sid[0] = &xsid;
        return 0;
    }
    ****************************************/

    // Make xsid forget it's emulation
    xsid.emulation (&nullsid);

    {   // Release old sids
        for (int i = 0; i < SID2_MAX_SIDS; i++)
        {
            sidbuilder *b;
            b = sid[i]->builder ();
            if (b)
                b->unlock (sid[i]);
        }
    }

    if (!builder)
    {   // No sid
        for (int i = 0; i < SID2_MAX_SIDS; i++)
            sid[i] = &nullsid;
    }
    else
    {   // Detect the Correct SID model
        // Determine model when unknown
        sid2_model_t userModels[SID2_MAX_SIDS];
        userModels[0] = getModel(m_tuneInfo.sidModel1, userModel, defaultModel);
        // If bits 6-7 are set to Unknown then the second SID will be set to the same SID
        // model as the first SID.
        userModels[1] = getModel(m_tuneInfo.sidModel2, userModel, userModels[0]);

        for (int i = 0; i < SID2_MAX_SIDS; i++)
        {   // Get first SID emulation
            sid[i] = builder->lock (this, userModels[i]);
            if (!sid[i])
                sid[i] = &nullsid;
            if ((i == 0) && !*builder)
                return -1;
        }
    }
    xsid.emulation (sid[0]);
    sid[0] = &xsid;
    return 0;
}

void Player::sidSamples (bool enable)
{
    //int_least8_t gain = 0;
    xsid.sidSamples (enable);
}

SIDPLAY2_NAMESPACE_STOP
