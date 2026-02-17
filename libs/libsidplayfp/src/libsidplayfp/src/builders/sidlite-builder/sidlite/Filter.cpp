/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright (C) 2025-2026 Leandro Nini
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

// Based on cRSID lightweight RealSID by Hermit (Mihaly Horvath)

#include "Filter.h"

#include "sl_defs.h"
#include "sl_constants.h"
#include "filt_tables.h"
#include "sl_constants.h"
#include "sl_settings.h"

#include <map>
#include <mutex>
#include <cmath>

//YOYOFR
extern "C" int sid_v4;
//YOYOFR

namespace SIDLite
{

constexpr int VOLUME_MAX = 0xF;
constexpr int CHANNELS = 3+1;
constexpr int SID_FULLVOLUME = CHANNELS*VOLUME_MAX; /*64*/

constexpr int CRSID_PRESAT_ATT_NOM = 20;
constexpr int CRSID_PRESAT_ATT_DENOM = 16;

constexpr int SID_CUTOFF_BITS = 11;
constexpr int SID_CUTOFF_RANGE = (1 << SID_CUTOFF_BITS);
constexpr int SID_CUTOFF_MAX = SID_CUTOFF_RANGE - 1;

constexpr int FRACTIONAL_BITS = 12;
constexpr int FRACTIONAL_SHIFTS = FRACTIONAL_BITS;
constexpr int CRSID_FILTERTABLE_RESOLUTION = 12;
constexpr int CRSID_FILTERTABLE_SHIFTS = CRSID_FILTERTABLE_RESOLUTION;
constexpr int CRSID_FILTERTABLE_MAGNITUDE = 1 << CRSID_FILTERTABLE_RESOLUTION;

constexpr int D418_DIGI_VOL = 1*16;
constexpr int D418_DIGI_MUL = D418_DIGI_VOL / CRSID_WAVGEN_PREDIV;

constexpr int VUMETER_LOWPASS_DIV = 16;
constexpr int VUMETER_DIVSHIFTS = 4 - CRSID_WAVGEN_PRESHIFT;

constexpr int Attenuation = ((SID_FULLVOLUME+26) * CRSID_PRESAT_ATT_NOM) / (CRSID_PRESAT_ATT_DENOM * CRSID_WAVGEN_PREDIV);

int Filter::clock(int FilterInput, int NonFiltered)
{
    enum SIDspecs { HIGHPASS_BITVAL=0x40, BANDPASS_BITVAL=0x20, LOWPASS_BITVAL=0x10 };

    //Filter
    unsigned char FilterSwitchReso = regs[0x17];
    unsigned char VolumeBand = regs[0x18];

    int Cutoff = (regs[0x16] << 3) + (regs[0x15] & 7);
    int Resonance = FilterSwitchReso >> 4;
    if (LIKELY(s->is8580()))
    {
        Cutoff = CutoffMul8580[Cutoff];
        Resonance = Resonances8580[Resonance];
    }
    else
    { //6581
        // MOSFET-VCR control-voltage calculation (resistance-modulation aka 6581 filter distortion) emulation
        Cutoff += (FilterInput*105)>>16;
        if (Cutoff > SID_CUTOFF_MAX)
            Cutoff = SID_CUTOFF_MAX;
        else if (Cutoff < 0)
            Cutoff = 0;
        Cutoff = CutoffMul6581[Cutoff];
        Resonance = Resonances6581[Resonance];
    }

    int FilterOutput = 0;
    {
        int Tmp = FilterInput + ((PrevBandPass * Resonance) / CRSID_FILTERTABLE_MAGNITUDE) + PrevLowPass;
        if (VolumeBand & HIGHPASS_BITVAL)
            FilterOutput -= Tmp;
        Tmp = PrevBandPass - ((Tmp * Cutoff) / CRSID_FILTERTABLE_MAGNITUDE);
        PrevBandPass = Tmp;
        if (VolumeBand & BANDPASS_BITVAL)
            FilterOutput -= Tmp;
        Tmp = PrevLowPass + ((Tmp * Cutoff) / CRSID_FILTERTABLE_MAGNITUDE);
        PrevLowPass = Tmp;
        if (VolumeBand & LOWPASS_BITVAL)
            FilterOutput += Tmp;
    }

    // Output stage
    // For $D418 volume-register digi playback: an AC / DC separation for $D418 value at low (20Hz or so) cutoff-frequency,
    // sending AC (highpass) value to a 4th 'digi' channel mixed to the master output,
    // and set ONLY the DC (lowpass) value to the volume-control.
    // This solved 2 issues:
    //  Thanks to the lowpass filtering of the volume-control,
    //    SID tunes where digi is played together with normal SID channels
    //    won't sound distorted anymore,
    //  the volume-clicks disappear when setting SID-volume.
    //    (This is useful for fade-in/out tunes like Hades Nebula, where clicking ruins the intro.)
    int MainVolume;
    int Digi = 0;
    if (LIKELY(s->getRealSIDmode()))
    {
        int Tmp = (signed int)((VolumeBand & 0xF) << FRACTIONAL_SHIFTS);
        Digi = (Tmp - PrevVolume) * D418_DIGI_MUL; // highpass is digi, adding it to output must be before digifilter-code
        PrevVolume += (Tmp - PrevVolume) / 1024; // arithmetic shift amount determines digi lowpass-frequency
        MainVolume = PrevVolume >> FRACTIONAL_SHIFTS; // lowpass is main volume
    }
    else
        MainVolume = VolumeBand & 0xF;

    int Output = ((NonFiltered+FilterOutput) * MainVolume) + Digi;// / ((CHANNELS*VOLUME_MAX) + Attenuation);

    if (UNLIKELY(!++VUmeterUpdateCounter))
    {
        // average level (for VU-meter)
        Level += ((std::abs(Output)>>VUMETER_DIVSHIFTS) - Level ) / VUMETER_LOWPASS_DIV;
    }
    
    //YOYOFR
    sid_v4=Digi;
    //YOYOFR
    
    return Output / Attenuation; // master output
}

Filter::Filter(settings *s, unsigned char *regs) :
    regs(regs),
    s(s)
{
    reset();
}

void Filter::reset()
{
    PrevLowPass = PrevBandPass = PrevVolume = 0;
    Level = 0;
    VUmeterUpdateCounter = 0;
}

constexpr int CF_LEN = 0x800;
using co_tab_t = std::array<unsigned short, CF_LEN>;
using co_cache_t = std::map<unsigned short, co_tab_t>;

static co_cache_t CUTOFF_CACHE_8580;
static co_cache_t CUTOFF_CACHE_6581;
static std::mutex CUTOFF_CACHE_Lock;

void Filter::rebuildCutoffTables(unsigned short samplerate)
{
    constexpr int Magnitude = (1 << CRSID_FILTERTABLE_RESOLUTION);

    std::lock_guard<std::mutex> lock(CUTOFF_CACHE_Lock);

    //8580
    {
        co_cache_t::iterator lb = CUTOFF_CACHE_8580.lower_bound(samplerate);

        if (lb != CUTOFF_CACHE_8580.end() && !(CUTOFF_CACHE_8580.key_comp()(samplerate, lb->first)))
        {
            auto ct = &(lb->second);
            CutoffMul8580 = ct->data();
        }
        else
        {
            co_tab_t cutoff_tab;

            const double cutoff_ratio_8580 = -2 * 3.14159 * (12500 / 2048) / samplerate;

            //8580 Cutoff-curve (for samplerate)
            for (int i=0; i<CF_LEN; i++)
            {
                cutoff_tab[i] = (1. - std::exp((i+2) * cutoff_ratio_8580)) * Magnitude; //linear curve by resistor-ladder VCR (with a little leakage)
            }
            auto ct = &(CUTOFF_CACHE_8580.emplace_hint(lb, co_cache_t::value_type(samplerate, cutoff_tab))->second);
            CutoffMul8580 = ct->data();
        }
    }

    //6581
    {
        co_cache_t::iterator lb = CUTOFF_CACHE_6581.lower_bound(samplerate);

        if (lb != CUTOFF_CACHE_6581.end() && !(CUTOFF_CACHE_6581.key_comp()(samplerate, lb->first)))
        {
            auto ct = &(lb->second);
            CutoffMul6581 = ct->data();
        }
        else
        {
            co_tab_t cutoff_tab;

            constexpr double VCR_SHUNT_6581 = 1500.; //kOhm //cca 1.5 MOhm Rshunt across VCR FET drain and source (causing 220Hz bottom cutoff with 470pF integrator capacitors in old C64)
            constexpr int VCR_FET_TRESHOLD = 192; //Vth (on cutoff numeric range 0..2048) for the VCR cutoff-frequency control FET below which it doesn't conduct
            constexpr double CAP_6581 = 0.470; //nF //filter capacitor value for 6581
            constexpr double FILTER_DARKNESS_6581 = 22.0; //the bigger the value, the darker the filter control is (that is, cutoff frequency increases less with the same cutoff-value)
            //constexpr double FILTER_DISTORTION_6581 = 0.0016; //the bigger the value the more of resistance-modulation (filter distortion) is applied for 6581 cutoff-control

            constexpr double cap_6581_reciprocal = -1000000./CAP_6581;
            constexpr double cutoff_steepness_6581 = FILTER_DARKNESS_6581*(2048-VCR_FET_TRESHOLD); //pre-scale for 0...2048 cutoff-value range

            // 6581 Cutoff-curve: (for samplerate)
            for (int i=0; i<CF_LEN; ++i)
            {
                double rDS_VCR_FET = i<=VCR_FET_TRESHOLD ? 100000000.0 //below Vth treshold Vgs control-voltage FET presents an open circuit
                    : cutoff_steepness_6581/(i-VCR_FET_TRESHOLD);  // rDS ~ (-Vth*rDSon) / (Vgs-Vth)  //above Vth FET drain-source resistance is proportional to reciprocal of cutoff-control voltage

                cutoff_tab[i] = (1. - std::exp(cap_6581_reciprocal / (VCR_SHUNT_6581*rDS_VCR_FET/(VCR_SHUNT_6581+rDS_VCR_FET)) / samplerate)) * Magnitude; //curve with 1.5MOhm VCR parallel Rshunt emulation
            }
            auto ct = &(CUTOFF_CACHE_6581.emplace_hint(lb, co_cache_t::value_type(samplerate, cutoff_tab))->second);
            CutoffMul6581 = ct->data();
        }
    }
}

}
