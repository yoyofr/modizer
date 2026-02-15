/*
 * This file is part of sidplayfp, a console SID player.
 *
 * Copyright 2011-2021 Leandro Nini
 * Copyright 2000 Simon White
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

#ifndef INICONFIG_H
#define INICONFIG_H

#include "ini/types.h"
#include "ini/iniHandler.h"

#include "siddefines.h"
#include "sidlib_features.h"

#include <sidplayfp/sidplayfp.h>
#include <sidplayfp/SidConfig.h>

/*
 * Sidplayfp config file reader.
 */
class IniConfig
{
public:
    struct sidplay2_section
    {
        int            version;
        SID_STRING     database;
        uint_least32_t playLength;
        uint_least32_t recordLength;
        SID_STRING     kernalRom;
        SID_STRING     basicRom;
        SID_STRING     chargenRom;
        int            verboseLevel;
    };

    struct console_section
    {   // INI Section - [Console]
        bool ansi;
        char topLeft;
        char topRight;
        char bottomLeft;
        char bottomRight;
        char vertical;
        char horizontal;
        char junctionLeft;
        char junctionRight;
        color_t decorations;
        color_t title;
        color_t label_core;
        color_t text_core;
        color_t label_extra;
        color_t text_extra;
        color_t notes;
        color_t control_on;
        color_t control_off;
    };

    struct audio_section
    {   // INI Section - [Audio]
        int frequency; // sample rate
        int channels;  // number of channels
        int precision; // sample precision in bits
        int bufLength; // buffer length in milliseconds
        int getBufSize() const { return (bufLength * frequency) / 1000; }
    };

    struct emulation_section
    {   // INI Section - [Emulation]
        SID_STRING    engine;
        SidConfig::c64_model_t  modelDefault;
        bool          modelForced;
        SidConfig::sid_model_t  sidModel;
        bool          forceModel;
        SidConfig::cia_model_t  ciaModel;
        bool          digiboost;
        bool          filter;
        double        bias;
        double        filterCurve6581;
#ifdef FEAT_FILTER_RANGE
        double        filterRange6581;
#endif
        double        filterCurve8580;
#ifdef FEAT_CW_STRENGTH
        SidConfig::sid_cw_t combinedWaveformsStrength;
#endif
        int           powerOnDelay;
        SidConfig::sampling_method_t  samplingMethod;
        bool          fastSampling;
    };

protected:
    struct    sidplay2_section  sidplay2_s;
    struct    console_section   console_s;
    struct    audio_section     audio_s;
    struct    emulation_section emulation_s;

protected:
    void  clear ();

    void readSidplay2  (iniHandler &ini);
    void readConsole   (iniHandler &ini);
    void readAudio     (iniHandler &ini);
    void readEmulation (iniHandler &ini);

private:
    SID_STRING m_fileName;

public:
    IniConfig  ();
    ~IniConfig ();

    SID_STRING getFilename() const { return m_fileName; }

    void read ();

    // Sidplayfp Specific Section
    const sidplay2_section&  sidplay2  () { return sidplay2_s; }
    const console_section&   console   () { return console_s; }
    const audio_section&     audio     () { return audio_s; }
    const emulation_section& emulation () { return emulation_s; }
};

#endif // INICONFIG_H
