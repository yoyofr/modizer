/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2025 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2004 Dag Lem <resid@nimrod.no>
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

#ifndef FILTER_H
#define FILTER_H

//TODO:  MODIZER changes start / YOYOFR
extern "C" int sid_v4;
//TODO:  MODIZER changes start / YOYOFR


#include "FilterModelConfig.h"
#include "Voice.h"

#include "siddefs-fp.h"

namespace reSIDfp
{

/**
 * SID filter base class
 */
class Filter
{
private:
    unsigned short* mixer;
    unsigned short* summer;
    unsigned short* resonance;
    unsigned short* volume;

    FilterModelConfig& fmc;

    /// Current filter/voice mixer setting.
    unsigned short* currentMixer = nullptr;

    /// Filter input summer setting.
    unsigned short* currentSummer = nullptr;

    /// Filter resonance value.
    unsigned short* currentResonance = nullptr;

    /// Current volume amplifier setting.
    unsigned short* currentVolume = nullptr;

protected:
    /// Filter highpass state.
    int Vhp = 0;

    /// Filter bandpass state.
    int Vbp = 0;

    /// Filter lowpass state.
    int Vlp = 0;

private:
    /// Filter external input.
    int Ve = 0;
    
    //YOYOFR
        int Vhp2,Vbp2,Vlp2;
        //YOYOFR

    /// Filter cutoff frequency.
    unsigned int fc = 0;

    /// Routing to filter or outside filter
    //@{
    bool filt1 = false;
    bool filt2 = false;
    bool filt3 = false;
    bool filtE = false;
    //@}

    /// Switch voice 3 off.
    bool voice3off = false;

protected:
    /// Highpass, bandpass, and lowpass filter modes.
    //@{
    bool hp = false;
    bool bp = false;
    bool lp = false;
    //@}

private:
    /// Current volume.
    unsigned char vol = 0;

    /// Filter enabled.
    bool enabled = true;

    /// Selects which inputs to route through filter.
    unsigned char filt = 0;

private:
    inline int getNormalizedVoice(Voice& v) const
    {
        return fmc.getNormalizedVoice(v.output(), v.envelope()->output());
    }

    // If voice 3 is off we still need to clock the waveform generator
    inline int getSilentVoice(Voice& v) const
    {
        v.wave()->output();
        return 0;
    }

protected:
    /**
     * Update filter cutoff frequency.
     */
    virtual void updateCenterFrequency() = 0;

    /**
     * Update filter resonance.
     *
     * @param res the new resonance value
     */
    void updateResonance(unsigned char res) { currentResonance = resonance + (res * (1<<16)); }

    /**
     * Mixing configuration modified (offsets change)
     */
    void updateMixing();

    /**
     * Get the filter cutoff register value
     */
    inline unsigned int getFC() const { return fc; }

    virtual int solveIntegrators() = 0;

public:
    Filter(FilterModelConfig& fmc);

    virtual ~Filter() = default;

    /**
     * SID clocking - 1 cycle
     *
     * @param v1 voice 1 in
     * @param v2 voice 2 in
     * @param v3 voice 3 in
     * @return filtered output, unsigned 16 bit
     */
    unsigned short clock(Voice& v1, Voice& v2, Voice& v3);

    /**
     * Enable filter.
     *
     * @param enable
     */
    void enable(bool enable);

    /**
     * SID reset.
     */
    void reset();

    /**
     * Write Frequency Cutoff Low register.
     *
     * @param fc_lo Frequency Cutoff Low-Byte
     */
    void writeFC_LO(unsigned char fc_lo);

    /**
     * Write Frequency Cutoff High register.
     *
     * @param fc_hi Frequency Cutoff High-Byte
     */
    void writeFC_HI(unsigned char fc_hi);

    /**
     * Write Resonance/Filter register.
     *
     * @param res_filt Resonance/Filter
     */
    void writeRES_FILT(unsigned char res_filt);

    /**
     * Write filter Mode/Volume register.
     *
     * @param mode_vol Filter Mode/Volume
     */
    void writeMODE_VOL(unsigned char mode_vol);

    /**
     * Apply a signal to EXT-IN
     *
     * @param input a signed 16 bit sample
     */
    void input(short input) { Ve = fmc.getNormalizedVoice(input/32768.f, 0); }
};

} // namespace reSIDfp

#if RESIDFP_INLINING || defined(FILTER_CPP)

namespace reSIDfp
{

RESIDFP_INLINE
unsigned short Filter::clock(Voice& voice1, Voice& voice2, Voice& voice3)
{
    const int V1 = getNormalizedVoice(voice1);
    const int V2 = getNormalizedVoice(voice2);
    // Voice 3 is silenced by voice3off if it is not routed through the filter.
    const int V3 = (filt3 || !voice3off) ? getNormalizedVoice(voice3) : getSilentVoice(voice3);
    
    int Vsum = 0;
    int Vmix = 0;
    
    (filt1 ? Vsum : Vmix) += V1;
    (filt2 ? Vsum : Vmix) += V2;
    (filt3 ? Vsum : Vmix) += V3;
    (filtE ? Vsum : Vmix) += Ve;
    
    Vhp = currentSummer[currentResonance[Vbp] + Vlp + Vsum];
    
    Vmix += solveIntegrators();
    
    //YOYOFR
    //return currentVolume[currentMixer[Vmix]];
    unsigned short ret=currentVolume[currentMixer[Vmix]];
    
    Vsum = 0;
    Vmix = 0;
    
    (filtE ? Vsum : Vmix) += Ve;
    
    sid_v4=currentVolume[currentMixer[Vmix]];
    return ret;
    //YOYOFR
}

} // namespace reSIDfp

#endif

#endif
