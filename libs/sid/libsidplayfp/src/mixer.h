/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2023 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright (C) 2000 Simon White
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef MIXER_H
#define MIXER_H

#include "sidcxx11.h"

#include <stdint.h>

#include <vector>

namespace libsidplayfp
{

class sidemu;

/**
 * This class implements the mixer.
 */
class Mixer
{
private:
    // random number generator for dithering
    template <int MAX_VAL>
    class randomLCG
    {
    static_assert((MAX_VAL != 0) && ((MAX_VAL & (MAX_VAL - 1)) == 0), "MAX_VAL must be a power of two");

    private:
        uint32_t rand_seed;

    public:
        randomLCG(uint32_t seed) :
            rand_seed(seed)
        {}

        int get()
        {
            rand_seed = (214013 * rand_seed + 2531011);
            return static_cast<int>((rand_seed >> 16) & (MAX_VAL-1));
        }
    };

public:
    class badBufferSize {};

public:
    /// Maximum number of supported SIDs
    static constexpr unsigned int MAX_SIDS = 3;

private:
    static constexpr int_least32_t SCALE_FACTOR = 1 << 16;

    static constexpr double SQRT_2 = 1.41421356237;
    static constexpr double SQRT_3 = 1.73205080757;

    static constexpr int_least32_t SCALE[3] = {
        SCALE_FACTOR,                                               // 1 chip, no scale
        static_cast<int_least32_t>((1.0 / SQRT_2) * SCALE_FACTOR),  // 2 chips, scale by sqrt(2)
        static_cast<int_least32_t>((1.0 / SQRT_3) * SCALE_FACTOR)   // 3 chips, scale by sqrt(3)
    };

private:
    using mixer_func_t = int_least32_t (Mixer::*)() const;

    using scale_func_t = int (Mixer::*)(unsigned int);

public:
    /// Maximum allowed volume, must be a power of 2.
    static constexpr int_least32_t VOLUME_MAX = 1024;

private:
    std::vector<sidemu*> m_chips;

    std::vector<int_least32_t> m_iSamples;
    std::vector<int_least32_t> m_volume;

    std::vector<mixer_func_t> m_mix;
    std::vector<scale_func_t> m_scale;

    int m_oldRandomValue = 0;
    int m_fastForwardFactor = 1;

    // Mixer settings
    short         *m_sampleBuffer = nullptr;
    uint_least32_t m_sampleCount = 0;
    uint_least32_t m_sampleIndex = 0;

    bool m_stereo = false;

    bool m_wait = false;

    randomLCG<VOLUME_MAX> m_rand;

private:
    void updateParams();

    int triangularDithering()
    {
        const int prevValue = m_oldRandomValue;
        m_oldRandomValue = m_rand.get();
        return m_oldRandomValue - prevValue;
    }

    int scale(unsigned int ch)
    {
        const int_least32_t sample = (this->*(m_mix[ch]))();
        return (sample * m_volume[ch] + triangularDithering()) / VOLUME_MAX;
    }

    int noScale(unsigned int ch)
    {
        return (this->*(m_mix[ch]))();
    }

    /*
     * Channel matrix
     *
     *   C1
     * L 1.0
     * R 1.0
     *
     *   C1    C2
     * L 1.0   0.5
     * R 0.5   1.0
     *
     *   C1    C2    C3
     * L 1.0   1.0   0.5
     * R 0.5   1.0   1.0
     */

    // Mono mixing
    template <int Chips>
    int_least32_t mono() const
    {
        int_least32_t res = 0;
        for (int i = 0; i < Chips; i++)
            res += m_iSamples[i];
        return res * SCALE[Chips-1] / SCALE_FACTOR;
    }

    // Stereo mixing
    int_least32_t stereo_OneChip() const { return m_iSamples[0]; }

    int_least32_t stereo_ch1_TwoChips() const
    {
        return (m_iSamples[0] + 0.5*m_iSamples[1]) * SCALE[1] / SCALE_FACTOR;
    }
    int_least32_t stereo_ch2_TwoChips() const
    {
        return (0.5*m_iSamples[0] + m_iSamples[1]) * SCALE[1] / SCALE_FACTOR;
    }

    int_least32_t stereo_ch1_ThreeChips() const
    {
        return (m_iSamples[0] + m_iSamples[1] + 0.5*m_iSamples[2]) * SCALE[2] / SCALE_FACTOR;
    }
    int_least32_t stereo_ch2_ThreeChips() const
    {
        return (0.5*m_iSamples[0] + m_iSamples[1] + m_iSamples[2]) * SCALE[2] / SCALE_FACTOR;
    }

public:
    /**
     * Create a new mixer.
     */
    Mixer() :
        m_rand(257254)
    {
        m_mix.push_back(&Mixer::mono<1>);
    }

    /**
     * Do the mixing.
     */
    void doMix();

    //YOYOFR
    int samplesReady();
    //YOYOFR
    
    /**
     * This clocks the SID chips to the present moment, if they aren't already.
     */
    void clockChips();

    /**
     * Reset sidemu buffer position discarding produced samples.
     */
    void resetBufs();

    /**
     * Prepare for mixing cycle.
     *
     * @param buffer output buffer
     * @param count size of the buffer in samples
     *
     * @throws badBufferSize
     */
    void begin(short *buffer, uint_least32_t count);

    /**
     * Remove all SIDs from the mixer.
     */
    void clearSids() { m_chips.clear(); }

    /**
     * Add a SID to the mixer.
     *
     * @param chip the sid emu to add
     */
    void addSid(sidemu *chip);

    /**
     * Get a SID from the mixer.
     *
     * @param i the number of the SID to get
     * @return a pointer to the requested sid emu or 0 if not found
     */
    sidemu* getSid(unsigned int i) const { return (i < m_chips.size()) ? m_chips[i] : nullptr; }

    /**
     * Set the fast forward ratio.
     *
     * @param ff the fast forward ratio, from 1 to 32
     * @return true if parameter is valid, false otherwise
     */
    bool setFastForward(int ff);

    /**
     * Set mixing volumes, from 0 to #VOLUME_MAX.
     *
     * @param left volume for left or mono channel
     * @param right volume for right channel in stereo mode
     */
    void setVolume(int_least32_t left, int_least32_t right);

    /**
     * Set mixing mode.
     *
     * @param stereo true for stereo mode, false for mono
     */
    void setStereo(bool stereo);

    /**
     * Check if the buffer have been filled.
     */
    bool notFinished() const { return m_sampleIndex < m_sampleCount; }

    /**
     * Get the number of samples generated up to now.
     */
    uint_least32_t samplesGenerated() const { return m_sampleIndex; }

    /*
     * Wait till we consume the buffer.
     */
    bool wait() const { return m_wait; }
};

}

#endif // MIXER_H
