/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2019 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
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

#include "sidplayfp/SidTune.h"
#include "sidplayfp/sidbuilder.h"

#include "sidemu.h"
#include "psiddrv.h"
#include "romCheck.h"

#include "sidcxx11.h"

namespace libsidplayfp
{

// Speed strings
const char TXT_PAL_VBI[]        = "50 Hz VBI (PAL)";
const char TXT_PAL_VBI_FIXED[]  = "60 Hz VBI (PAL FIXED)";
const char TXT_PAL_CIA[]        = "CIA (PAL)";
const char TXT_PAL_UNKNOWN[]    = "UNKNOWN (PAL)";
const char TXT_NTSC_VBI[]       = "60 Hz VBI (NTSC)";
const char TXT_NTSC_VBI_FIXED[] = "50 Hz VBI (NTSC FIXED)";
const char TXT_NTSC_CIA[]       = "CIA (NTSC)";
const char TXT_NTSC_UNKNOWN[]   = "UNKNOWN (NTSC)";

// Error Strings
const char ERR_NA[]                   = "NA";
const char ERR_UNSUPPORTED_FREQ[]     = "SIDPLAYER ERROR: Unsupported sampling frequency.";
const char ERR_UNSUPPORTED_SID_ADDR[] = "SIDPLAYER ERROR: Unsupported SID address.";
const char ERR_UNSUPPORTED_SIZE[]     = "SIDPLAYER ERROR: Size of music data exceeds C64 memory.";
const char ERR_INVALID_PERCENTAGE[]   = "SIDPLAYER ERROR: Percentage value out of range.";

/**
 * Configuration error exception.
 */
class configError
{
private:
    const char* m_msg;

public:
    configError(const char* msg) : m_msg(msg) {}
    const char* message() const { return m_msg; }
};

Player::Player() :
    // Set default settings for system
    m_tune(nullptr),
    m_errorString(ERR_NA),
    m_isPlaying(STOPPED),
    m_rand((unsigned int)::time(0))
{
    m_c64.setRoms(nullptr, nullptr, nullptr);
    config(m_cfg);

    // Get component credits
    m_info.m_credits.push_back(m_c64.cpuCredits());
    m_info.m_credits.push_back(m_c64.ciaCredits());
    m_info.m_credits.push_back(m_c64.vicCredits());
}

template<class T>
inline void checkRom(const uint8_t* rom, std::string &desc)
{
    if (rom != nullptr)
    {
        T romCheck(rom);
        desc.assign(romCheck.info());
    }
    else
        desc.clear();
}

void Player::setRoms(const uint8_t* kernal, const uint8_t* basic, const uint8_t* character)
{
    checkRom<kernalCheck>(kernal, m_info.m_kernalDesc);
    checkRom<basicCheck>(basic, m_info.m_basicDesc);
    checkRom<chargenCheck>(character, m_info.m_chargenDesc);

    m_c64.setRoms(kernal, basic, character);
}

bool Player::fastForward(unsigned int percent)
{
    if (!m_mixer.setFastForward(percent / 100))
    {
        m_errorString = ERR_INVALID_PERCENTAGE;
        return false;
    }

    return true;
}

void Player::initialise()
{
    m_isPlaying = STOPPED;

    m_c64.reset();

    const SidTuneInfo* tuneInfo = m_tune->getInfo();

    const uint_least32_t size = static_cast<uint_least32_t>(tuneInfo->loadAddr()) + tuneInfo->c64dataLen() - 1;
    if (size > 0xffff)
    {
        throw configError(ERR_UNSUPPORTED_SIZE);
    }

    uint_least16_t powerOnDelay = m_cfg.powerOnDelay;
    // Delays above MAX result in random delays
    if (powerOnDelay > SidConfig::MAX_POWER_ON_DELAY)
    {   // Limit the delay to something sensible.
        powerOnDelay = (uint_least16_t)((m_rand.next() >> 3) & SidConfig::MAX_POWER_ON_DELAY);
    }

    psiddrv driver(m_tune->getInfo());
    driver.powerOnDelay(powerOnDelay);
    if (!driver.drvReloc())
    {
        throw configError(driver.errorString());
    }

    m_info.m_driverAddr = driver.driverAddr();
    m_info.m_driverLength = driver.driverLength();
    m_info.m_powerOnDelay = powerOnDelay;

    driver.install(m_c64.getMemInterface(), videoSwitch);

    if (!m_tune->placeSidTuneInC64mem(m_c64.getMemInterface()))
    {
        throw configError(m_tune->statusString());
    }

    m_c64.resetCpu();
}

bool Player::load(SidTune *tune)
{
    m_tune = tune;

    if (tune != nullptr)
    {
        // Must re-configure on fly for stereo support!
        if (!config(m_cfg, true))
        {
            // Failed configuration with new tune, reject it
            m_tune = nullptr;
            return false;
        }
    }
    return true;
}

void Player::mute(unsigned int sidNum, unsigned int voice, bool enable)
{
    sidemu *s = m_mixer.getSid(sidNum);
    if (s != nullptr)
        s->voice(voice, enable);
}

/**
 * @throws MOS6510::haltInstruction
 */
void Player::run(unsigned int events)
{
    for (unsigned int i = 0; m_isPlaying && i < events; i++)
        m_c64.clock();
}

uint_least32_t Player::play(short *buffer, uint_least32_t count)
{
    // Make sure a tune is loaded
    if (m_tune == nullptr)
        return 0;

    // Start the player loop
    if (m_isPlaying == STOPPED)
        m_isPlaying = PLAYING;

    if (m_isPlaying == PLAYING)
    {
        m_mixer.begin(buffer, count);

        try
        {
            if (m_mixer.getSid(0) != nullptr)
            {
                if (count && buffer != nullptr)
                {
                    // Clock chips and mix into output buffer
                    while (m_isPlaying && m_mixer.notFinished())
                    {
                        run(sidemu::OUTPUTBUFFERSIZE);

                        m_mixer.clockChips();
                        m_mixer.doMix();
                    }
                    count = m_mixer.samplesGenerated();
                }
                else
                {
                    // Clock chips and discard buffers
                    int size = m_c64.getMainCpuSpeed() / m_cfg.frequency;
                    while (m_isPlaying && --size)
                    {
                        run(sidemu::OUTPUTBUFFERSIZE);

                        m_mixer.clockChips();
                        m_mixer.resetBufs();
                    }
                }
            }
            else
            {
                // Clock the machine
                int size = m_c64.getMainCpuSpeed() / m_cfg.frequency;
                while (m_isPlaying && --size)
                {
                    run(sidemu::OUTPUTBUFFERSIZE);
                }
            }
        }
        catch (MOS6510::haltInstruction const &)
        {
            m_errorString = "Illegal instruction executed";
            m_isPlaying = STOPPING;
        }
    }

    if (m_isPlaying == STOPPING)
    {
        try
        {
            initialise();
        }
        catch (configError const &) {}
        m_isPlaying = STOPPED;
    }

    return count;
}

void Player::stop()
{
    if (m_tune != nullptr && m_isPlaying == PLAYING)
    {
        m_isPlaying = STOPPING;
    }
}

bool Player::config(const SidConfig &cfg, bool force)
{
    // Check if configuration have been changed or forced
    if (!force && !m_cfg.compare(cfg))
    {
        return true;
    }

    // Check for base sampling frequency
    if (cfg.frequency < 8000)
    {
        m_errorString = ERR_UNSUPPORTED_FREQ;
        return false;
    }

    // Only do these if we have a loaded tune
    if (m_tune != nullptr)
    {
        const SidTuneInfo* tuneInfo = m_tune->getInfo();

        try
        {
            sidRelease();

            std::vector<unsigned int> addresses;
            const uint_least16_t secondSidAddress = tuneInfo->sidChipBase(1) != 0 ?
                tuneInfo->sidChipBase(1) :
                cfg.secondSidAddress;
            if (secondSidAddress != 0)
                addresses.push_back(secondSidAddress);

            const uint_least16_t thirdSidAddress = tuneInfo->sidChipBase(2) != 0 ?
                tuneInfo->sidChipBase(2) :
                cfg.thirdSidAddress;
            if (thirdSidAddress != 0)
                addresses.push_back(thirdSidAddress);

            // SID emulation setup (must be performed before the
            // environment setup call)
            sidCreate(cfg.sidEmulation, cfg.defaultSidModel, cfg.digiBoost, cfg.forceSidModel, addresses);

            // Determine clock speed
            const c64::model_t model = c64model(cfg.defaultC64Model, cfg.forceC64Model);

            m_c64.setModel(model);
            m_c64.setCiaModel(cfg.ciaModel);

            sidParams(m_c64.getMainCpuSpeed(), cfg.frequency, cfg.samplingMethod, cfg.fastSampling);

            // Configure, setup and install C64 environment/events
            initialise();
        }
        catch (configError const &e)
        {
            m_errorString = e.message();
            m_cfg.sidEmulation = 0;
            if (&m_cfg != &cfg)
            {
                config(m_cfg);
            }
            return false;
        }
    }

    const bool isStereo = cfg.playback == SidConfig::STEREO;
    m_info.m_channels = isStereo ? 2 : 1;

    m_mixer.setStereo(isStereo);
    m_mixer.setVolume(cfg.leftVolume, cfg.rightVolume);

    // Update Configuration
    m_cfg = cfg;

    return true;
}

// Clock speed changes due to loading a new song
c64::model_t Player::c64model(SidConfig::c64_model_t defaultModel, bool forced)
{
    const SidTuneInfo* tuneInfo = m_tune->getInfo();

    SidTuneInfo::clock_t clockSpeed = tuneInfo->clockSpeed();

    c64::model_t model;

    // Use preferred speed if forced or if song speed is unknown
    if (forced || clockSpeed == SidTuneInfo::CLOCK_UNKNOWN || clockSpeed == SidTuneInfo::CLOCK_ANY)
    {
        switch (defaultModel)
        {
        case SidConfig::PAL:
            clockSpeed = SidTuneInfo::CLOCK_PAL;
            model = c64::PAL_B;
            videoSwitch = 1;
            break;
        case SidConfig::DREAN:
            clockSpeed = SidTuneInfo::CLOCK_PAL;
            model = c64::PAL_N;
            videoSwitch = 1; // TODO verify
            break;
        case SidConfig::NTSC:
            clockSpeed = SidTuneInfo::CLOCK_NTSC;
            model = c64::NTSC_M;
            videoSwitch = 0;
            break;
        case SidConfig::OLD_NTSC:
            clockSpeed = SidTuneInfo::CLOCK_NTSC;
            model = c64::OLD_NTSC_M;
            videoSwitch = 0;
            break;
        case SidConfig::PAL_M:
            clockSpeed = SidTuneInfo::CLOCK_NTSC;
            model = c64::PAL_M;
            videoSwitch = 0; // TODO verify
            break;
        }
    }
    else
    {
        switch (clockSpeed)
        {
        default:
        case SidTuneInfo::CLOCK_PAL:
            model = c64::PAL_B;
            videoSwitch = 1;
            break;
        case SidTuneInfo::CLOCK_NTSC:
            model = c64::NTSC_M;
            videoSwitch = 0;
            break;
        }
    }

    switch (clockSpeed)
    {
    case SidTuneInfo::CLOCK_PAL:
        if (tuneInfo->songSpeed() == SidTuneInfo::SPEED_CIA_1A)
            m_info.m_speedString = TXT_PAL_CIA;
        else if (tuneInfo->clockSpeed() == SidTuneInfo::CLOCK_NTSC)
            m_info.m_speedString = TXT_PAL_VBI_FIXED;
        else
            m_info.m_speedString = TXT_PAL_VBI;
        break;
    case SidTuneInfo::CLOCK_NTSC:
        if (tuneInfo->songSpeed() == SidTuneInfo::SPEED_CIA_1A)
            m_info.m_speedString = TXT_NTSC_CIA;
        else if (tuneInfo->clockSpeed() == SidTuneInfo::CLOCK_PAL)
            m_info.m_speedString = TXT_NTSC_VBI_FIXED;
        else
            m_info.m_speedString = TXT_NTSC_VBI;
        break;
    default:
        break;
    }

    return model;
}

/**
 * Get the SID model.
 *
 * @param sidModel the tune requested model
 * @param defaultModel the default model
 * @param forced true if the default model shold be forced in spite of tune model
 */
SidConfig::sid_model_t getSidModel(SidTuneInfo::model_t sidModel, SidConfig::sid_model_t defaultModel, bool forced)
{
    SidTuneInfo::model_t tuneModel = sidModel;

    // Use preferred speed if forced or if song speed is unknown
    if (forced || tuneModel == SidTuneInfo::SIDMODEL_UNKNOWN || tuneModel == SidTuneInfo::SIDMODEL_ANY)
    {
        switch (defaultModel)
        {
        case SidConfig::MOS6581:
            tuneModel = SidTuneInfo::SIDMODEL_6581;
            break;
        case SidConfig::MOS8580:
            tuneModel = SidTuneInfo::SIDMODEL_8580;
            break;
        default:
            break;
        }
    }

    SidConfig::sid_model_t newModel;

    switch (tuneModel)
    {
    default:
    case SidTuneInfo::SIDMODEL_6581:
        newModel = SidConfig::MOS6581;
        break;
    case SidTuneInfo::SIDMODEL_8580:
        newModel = SidConfig::MOS8580;
        break;
    }

    return newModel;
}

void Player::sidRelease()
{
    m_c64.clearSids();

    for (unsigned int i = 0; ; i++)
    {
        sidemu *s = m_mixer.getSid(i);
        if (s == nullptr)
            break;

        if (sidbuilder *b = s->builder())
        {
            b->unlock(s);
        }
    }

    m_mixer.clearSids();
}

void Player::sidCreate(sidbuilder *builder, SidConfig::sid_model_t defaultModel, bool digiboost,
                        bool forced, const std::vector<unsigned int> &extraSidAddresses)
{
    if (builder != nullptr)
    {
        const SidTuneInfo* tuneInfo = m_tune->getInfo();

        // Setup base SID
        const SidConfig::sid_model_t userModel = getSidModel(tuneInfo->sidModel(0), defaultModel, forced);
        sidemu *s = builder->lock(m_c64.getEventScheduler(), userModel, digiboost);
        if (!builder->getStatus())
        {
            throw configError(builder->error());
        }

        m_c64.setBaseSid(s);
        m_mixer.addSid(s);

        // Setup extra SIDs if needed
        if (extraSidAddresses.size() != 0)
        {
            // If bits 6-7 are set to Unknown then the second SID will be set to the same SID
            // model as the first SID.
            defaultModel = userModel;

            const unsigned int extraSidChips = extraSidAddresses.size();

            for (unsigned int i = 0; i < extraSidChips; i++)
            {
                const SidConfig::sid_model_t userModel = getSidModel(tuneInfo->sidModel(i+1), defaultModel, forced);

                sidemu *s = builder->lock(m_c64.getEventScheduler(), userModel, digiboost);
                if (!builder->getStatus())
                {
                    throw configError(builder->error());
                }

                if (!m_c64.addExtraSid(s, extraSidAddresses[i]))
                    throw configError(ERR_UNSUPPORTED_SID_ADDR);

                m_mixer.addSid(s);
            }
        }
    }
}

void Player::sidParams(double cpuFreq, int frequency,
                        SidConfig::sampling_method_t sampling, bool fastSampling)
{
    for (unsigned int i = 0; ; i++)
    {
        sidemu *s = m_mixer.getSid(i);
        if (s == nullptr)
            break;

        s->sampling((float)cpuFreq, frequency, sampling, fastSampling);
    }
}

}
