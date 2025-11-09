/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2025 Leandro Nini <drfiemost@users.sourceforge.net>
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


#ifndef SIMPLEMIXER_H
#define SIMPLEMIXER_H

#include <vector>
#ifdef __has_include
#  if __has_include(<version>)
#    include <version>
#  endif
#endif
#include <cstdint>

#ifdef __cpp_lib_math_constants
#  include <numbers>
#endif

namespace libsidplayfp
{

/**
 * This class implements the mixer.
 */
class SimpleMixer
{
private:
    static constexpr int_least32_t SCALE_FACTOR = 1 << 16;

#ifdef __cpp_lib_math_constants
    static constexpr double SQRT_2 = std::numbers::sqrt2;
    static constexpr double SQRT_3 = std::numbers::sqrt3;
#else
    static constexpr double SQRT_2 = 1.41421356237;
    static constexpr double SQRT_3 = 1.73205080757;
#endif

    static constexpr int_least32_t SCALE[3] = {
        SCALE_FACTOR,                                               // 1 chip, no scale
        static_cast<int_least32_t>((1.0 / SQRT_2) * SCALE_FACTOR),  // 2 chips, scale by sqrt(2)
        static_cast<int_least32_t>((1.0 / SQRT_3) * SCALE_FACTOR)   // 3 chips, scale by sqrt(3)
    };

private:
    using mixer_func_t = int_least32_t (SimpleMixer::*)() const;

private:
    std::vector<short*> m_buffers;

    std::vector<int_least32_t> m_iSamples;

    std::vector<mixer_func_t> m_mix;

private:
    void updateParams();

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

private:
    SimpleMixer() = delete;
    SimpleMixer(const SimpleMixer&) = delete;

public:
    /**
     * Create a new mixer.
     */
    SimpleMixer(bool stereo, short** buffers, int chips);

    /**
     * Do the mixing.
     */
    unsigned int doMix(short *buffer, unsigned int samples);
};

}

#endif // MIXER_H
