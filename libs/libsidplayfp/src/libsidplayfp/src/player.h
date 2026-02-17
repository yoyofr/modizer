/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2025 Leandro Nini <drfiemost@users.sourceforge.net>
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


#ifndef PLAYER_H
#define PLAYER_H

#include <cstdint>
#include <cstdio>

#include "sidplayfp/siddefs.h"
#include "sidplayfp/SidConfig.h"
#include "sidplayfp/SidTuneInfo.h"

#include "SidInfoImpl.h"
#include "sidrandom.h"
#include "simpleMixer.h"
#include "c64/c64.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <atomic>
#include <memory>
#include <vector>

class SidTune;
class SidInfo;
class sidbuilder;


namespace libsidplayfp
{

class sidemu;

class Player
{
private:
    /// Commodore 64 emulator
    c64 m_c64;

    /// Emulator info
    SidTune *m_tune;

    /// User Configuration Settings
    SidInfoImpl m_info;

    /// User Configuration Settings
    SidConfig m_cfg;

    /// Error message
    std::string m_errorString;

    sidrandom m_rand;

    uint_least32_t m_startTime = 0;

    /// PAL/NTSC switch value
    uint8_t m_videoSwitch;

    std::vector<sidemu*> m_chips;

    std::unique_ptr<SimpleMixer> m_simpleMixer;

private:
    /**
     * Get the C64 model for the current loaded tune.
     *
     * @param defaultModel the default model
     * @param forced true if the default model shold be forced in spite of tune model
     */
    c64::model_t c64model(SidConfig::c64_model_t defaultModel, bool forced);

    /**
     * Initialize the emulation.
     *
     * @throw configError
     */
    void initialise();

    /**
     * Release the SID builders.
     */
    void sidRelease();

    /**
     * Create the SID emulation(s).
     *
     * @throw configError
     */
    void sidCreate(sidbuilder *builder, SidConfig::sid_model_t defaultModel, bool digiboost,
                    bool forced, const std::vector<unsigned int> &extraSidAddresses);

    /**
     * Set the SID emulation parameters.
     *
     * @param cpuFreq the CPU clock frequency
     * @param frequency the output sampling frequency
     * @param sampling the sampling method to use
     */
    void sidParams(double cpuFreq, int frequency,
                    SidConfig::sampling_method_t sampling);

    inline void run(unsigned int events);

public:
    Player();
    ~Player() = default;

    const SidConfig &config() const { return m_cfg; }

    const SidInfo &info() const { return m_info; }

    bool config(const SidConfig &cfg, bool force=false);

    bool load(SidTune *tune);

    void buffers(short** buffers) const;

    int play(unsigned int cycles);

    uint_least32_t timeMs() const { return m_c64.getTimeMs() - m_startTime; }

    void debug(const bool enable, FILE *out) { m_c64.debug(enable, out); }

    void mute(unsigned int sidNum, unsigned int voice, bool enable);

    void filter(unsigned int sidNum, bool enable);

    const char *error() const { return m_errorString.c_str(); }

    void setKernal(const uint8_t* rom);
    void setBasic(const uint8_t* rom);
    void setChargen(const uint8_t* rom);

    uint_least16_t getCia1TimerA() const { return m_c64.getCia1TimerA(); }

    bool getSidStatus(unsigned int sidNum, uint8_t regs[32]);

    unsigned int installedSIDs() const { return m_chips.size(); }

    void initMixer(bool stereo);

    unsigned int mix(short *buffer, unsigned int samples);

    bool reset();
};

}

#endif // PLAYER_H
