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

#include "residfp/Filter6581.h"
#include "residfp/Filter8580.h"


#include "residfp/siddefs-fp.h"
#include "residfp.h"
#include "residfp-emu.h"

char ReSIDfp::m_credit[];

ReSIDfp::ReSIDfp (sidbuilder *builder)
:sidemu(builder),
 m_context(NULL),
 m_phase(EVENT_CLOCK_PHI1),
#ifdef HAVE_EXCEPTIONS
 m_sid(*(new(std::nothrow) RESID_NAMESPACE::SID)),
#else
 m_sid(*(new RESID_NAMESPACE::SID)),
#endif
 m_status(true),
 m_locked(false)
{
    char *p = m_credit;
    m_error = "N/A";

    // Setup credits
    sprintf (p, "ReSIDfp V%s Engine:", VERSION);
    p += strlen (p) + 1;
    strcpy  (p, "\t(C) 1999-2002 Simon White <sidplay2@yahoo.com>");
    p += strlen (p) + 1;
    sprintf (p, "MOS6581 (SID) Emulation (ReSIDfp V%s):", residfp_version_string);
    p += strlen (p) + 1;
    sprintf (p, "\t(C) 1999-2002 Dag Lem <resid@nimrod.no>");
    p += strlen (p) + 1;
    sprintf (p, "\t(C) 2005-2011 Antti S. Lankila <alankila@bel.fi>");
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

ReSIDfp::~ReSIDfp ()
{
    if (&m_sid)
        delete &m_sid;
    delete[] m_buffer;
}
#if 0
bool ReSIDfp::filter (const sid_filterfp_t *filter)
{
    /* Set whatever data is provided in the filter def.
     * XXX: we should check that if one param in set is provided,
     * all are provided. */
    if (filter != NULL) {
      //FIXME
#ifdef DEBUG
    //printf("filterCurve6581: %f\n", filter->filterCurve6581);
    //printf("filterCurve8580: %f\n", filter->filterCurve8580);
#endif

      m_sid.getFilter6581()->setFilterCurve(0.5);
      m_sid.getFilter8580()->setFilterCurve(12500.);
    } else {
      /* Set sensible defaults. */
      m_sid.getFilter6581()->setFilterCurve(0.5);
      m_sid.getFilter8580()->setFilterCurve(12500.);
    }

    return true;
}
#endif
void ReSIDfp::filter6581Curve (const float filterCurve)
{
   m_sid.getFilter6581()->setFilterCurve(filterCurve);
}

void ReSIDfp::filter8580Curve (const float filterCurve)
{
   m_sid.getFilter8580()->setFilterCurve(filterCurve);
}

// Standard component options
void ReSIDfp::reset (uint8_t volume)
{
    m_accessClk = 0;
    m_sid.reset ();
    m_sid.write (0x18, volume);
}

uint8_t ReSIDfp::read (uint_least8_t addr)
{
    clock();
    return m_sid.read (addr);
}

void ReSIDfp::write (uint_least8_t addr, uint8_t data)
{
    clock();
    m_sid.write (addr, data);
}

void ReSIDfp::clock()
{
    const int cycles = m_context->getTime(m_accessClk, m_phase);
    m_accessClk += cycles;
    m_bufferpos += m_sid.clock(cycles, m_buffer+m_bufferpos);
}

void ReSIDfp::filter (bool enable)
{
      m_sid.getFilter6581()->enable(enable);
      m_sid.getFilter8580()->enable(enable);
}

void ReSIDfp::sampling (float systemclock, float freq,
        const sampling_method_t method, const bool fast)
{
    reSIDfp::SamplingMethod sampleMethod;
    switch (method)
    {
    case SID2_INTERPOLATE:
        sampleMethod = reSIDfp::DECIMATE;
        break;
    case SID2_RESAMPLE_INTERPOLATE:
        sampleMethod = reSIDfp::RESAMPLE;
        break;
    default:
        m_status = false;
        m_error = "Invalid sampling method.";
        return;
    }

    try
    {
      const int halfFreq = 5000*((int)freq/10000);
      m_sid.setSamplingParameters (systemclock, sampleMethod, freq, halfFreq>20000?20000:halfFreq);
    }
    catch (RESID_NAMESPACE::SIDError& e)
    {
        m_status = false;
        m_error = "Unable to set desired output frequency.";
    }
}

// Set execution environment and lock sid to it
bool ReSIDfp::lock (c64env *env)
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
void ReSIDfp::model (sid2_model_t model)
{
    if (model == SID2_MOS8580)
        m_sid.setChipModel (reSIDfp::MOS8580);
    else
        m_sid.setChipModel (reSIDfp::MOS6581);
}
