/***************************************************************************
                          c64sid.h  -  ReSid Wrapper for redefining the
                                       filter
                             -------------------
    begin                : Fri Apr 4 2001
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

#include <cstring>
#include <string.h>
#include <stdio.h>

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif

#include "resid/siddefs.h"
#include "resid/spline.h"
#include "resid.h"
#include "resid-emu.h"

char ReSID::m_credit[];

ReSID::ReSID (sidbuilder *builder)
:sidemu(builder),
 m_context(NULL),
 m_phase(EVENT_CLOCK_PHI1),
#ifdef HAVE_EXCEPTIONS
 m_sid(*(new(std::nothrow) RESID_NS::SID)),
#else
 m_sid(*(new RESID_NS::SID)),
#endif
 m_status(true),
 m_locked(false)
{
    char *p = m_credit;
    m_error = "N/A";

    // Setup credits
    sprintf (p, "ReSID V%s Engine:", VERSION);
    p += strlen (p) + 1;
    strcpy  (p, "\t(C) 1999-2002 Simon White <sidplay2@yahoo.com>");
    p += strlen (p) + 1;
    sprintf (p, "MOS6581 (SID) Emulation (ReSID V%s):", resid_version_string);
    p += strlen (p) + 1;
    sprintf (p, "\t(C) 1999-2002 Dag Lem <resid@nimrod.no>");
    p += strlen (p) + 1;
    *p = '\0';

    if (!&m_sid)
    {
        m_error  = "RESID ERROR: Unable to create sid object";
        m_status = false;
        return;
    }

    m_buffer = new short[OUTPUTBUFFERSIZE];
    m_bufferpos = 0;
    reset (0);
}

ReSID::~ReSID ()
{
    if (&m_sid)
        delete &m_sid;
    delete[] m_buffer;
}
#if 0
bool ReSID::filter (const sid_filter_t *filter)
{
    /*RESID_NS::short_point fc[0x802];
    const RESID_NS::short_point *f0 = fc;
    int   points = 0;

    if (filter == NULL)
    {   // Select default filter
        //m_sid.fc_default (f0, points); //FIXME
        return false;
    }
    else
    {   // Make sure there are enough filter points and they are legal
        points = filter->points;
        if ((points < 2) || (points > 0x800))
            return false;

#ifdef DEBUG
    printf("points: %d\n", filter->points);
    for (int i=0; i<filter->points; ++i)
    {
        printf("point[%d,0]: %d\n", i, filter->cutoff[i][0]);
        printf("point[%d,1]: %d\n", i, filter->cutoff[i][1]);
    }
#endif

        {
            const sid_fc_t  fstart = {-1, 0};
            const sid_fc_t *fprev  = &fstart, *fin = filter->cutoff;
            RESID_NS::short_point *fout = fc;
            // Last check, make sure they are list in numerical order
            // for both axis
            while (points-- > 0)
            {
                if ((*fprev)[0] >= (*fin)[0])
                    return false;
                fout++;
                (*fout)[0] = (short) (*fin)[0];
                (*fout)[1] = (short) (*fin)[1];
                fprev      = fin++;
            }
            // Updated ReSID interpolate requires we
            // repeat the end points
            (*(fout+1))[0] = (*fout)[0];
            (*(fout+1))[1] = (*fout)[1];
            fc[0][0] = fc[1][0];
            fc[0][1] = fc[1][1];
            points   = filter->points + 2;
        }
    }

    // function from reSID
    points--;
    RESID_NS::interpolate (f0, f0 + points, RESID_NS::PointPlotter<short>(*fc), 1.0); //FIXME*/
    return true;
}
#endif
void ReSID::bias (const float dac_bias)
{
    m_sid.adjust_filter_bias(dac_bias);
}

// Standard component options
void ReSID::reset (uint8_t volume)
{
    m_accessClk = 0;
    m_sid.reset ();
    m_sid.write (0x18, volume);
}

uint8_t ReSID::read (uint_least8_t addr)
{
    clock();
    return m_sid.read (addr);
}

void ReSID::write (uint_least8_t addr, uint8_t data)
{
    clock();
    m_sid.write (addr, data);
}

void ReSID::clock()
{
    RESID_NS::cycle_count cycles = m_context->getTime(m_accessClk, m_phase);
    m_accessClk += cycles;
    m_bufferpos += m_sid.clock(cycles, (short *) m_buffer + m_bufferpos, OUTPUTBUFFERSIZE - m_bufferpos, 1);
}

void ReSID::filter (bool enable)
{
    m_sid.enable_filter (enable);
}

void ReSID::sampling (float systemclock, float freq,
        const sampling_method_t method, const bool fast)
{
    RESID_NS::sampling_method sampleMethod;
    switch (method)
    {
    case SID2_INTERPOLATE:
        sampleMethod = fast ? RESID_NS::SAMPLE_FAST : RESID_NS::SAMPLE_INTERPOLATE;
        break;
    case SID2_RESAMPLE_INTERPOLATE:
        sampleMethod = fast ? RESID_NS::SAMPLE_RESAMPLE_FASTMEM : RESID_NS::SAMPLE_RESAMPLE;
        break;
    default:
        m_status = false;
        m_error = "Invalid sampling method.";
        return;
    }

    if (! m_sid.set_sampling_parameters (systemclock, sampleMethod, freq)) {
        m_status = false;
        m_error = "Unable to set desired output frequency.";
    }
}

// Set execution environment and lock sid to it
bool ReSID::lock (c64env *env)
{
    if (env == NULL)
    {
        if (!m_locked)
            return false;
        m_locked  = false;
        m_context = NULL;
    }
    else
    {
        if (m_locked)
            return false;
        m_locked  = true;
        m_context = &env->context ();
    }
    return true;
}

// Set the emulated SID model
void ReSID::model (sid2_model_t model)
{
    if (model == SID2_MOS8580)
        m_sid.set_chip_model (RESID_NS::MOS8580);
    else
        m_sid.set_chip_model (RESID_NS::MOS6581);
/* MOS8580 + digi boost
        m_sid.set_chip_model (RESID_NS::MOS8580);
        m_sid.set_voice_mask(0x0f);
        m_sid.input(-32768);
*/
}
