/***************************************************************************
                          SidFilter.cpp  -  read filter
                             -------------------
    begin                : Sun Mar 11 2001
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdlib.h>
#include <math.h>

#include "SidFilter.h"
#include "iniParser.h"


SidFilter::SidFilter ()
:m_status(0),
m_errorString("SID Filter: No filter loaded")
{}

void SidFilter::read (const char *filename)
{
    read (filename, "Filter");
}


void SidFilter::read (const char *filename, const char* section)
{
    iniParser m_parser;
    m_status = 0;

    if (!m_parser.open (filename))
    {
        m_errorString = "SID Filter: Unable to open filter file";
        return;
    }

    if (!m_parser.setSection (section)) {
        m_errorString = "SID Filter: Unable to locate filter section in input file";
        return;
    }

    const char *type = m_parser.getValue ("type");
    long t;
    try
    {
        t = iniParser::parseLong (type);
    } catch (iniParser::parseError e) { t = 0; }

    switch (t)
    {
    case 1:
        readType1 (&m_parser);
    break;

    case 2:
        readType2 (&m_parser);
    break;

    default:
        readType3 (&m_parser);
        //m_status = false;
        //m_errorString = "SID Filter: Invalid filter type";
    break;
    }
}


bool SidFilter::parsePoint(const char* str, long &a, long &b) {

    char *end;
    a = strtol (str, &end, 10);
    if (*end != ',')
        return false;

    b = strtol (++end, &end, 10);

    return true;
}


void SidFilter::readType1 (iniParser *ini)
{
    long points;

    // Does Section exist in ini file
    const char* value = ini->getValue ("points");
    if (!value)
        goto SidFilter_readType1_errorDefinition;
    try
    {
        points = iniParser::parseLong (value);
    } catch (iniParser::parseError e) { points = 0; }

    // Make sure there are enough filter points
    if ((points < 2) || (points > 0x800))
        goto SidFilter_readType1_errorDefinition;
    m_filter.points = (uint_least16_t) points;

    {
        for (int i = 0; i < m_filter.points; i++)
        {   // First lets read the SID cutoff register value
            char key[12];
            sprintf (key, "point%d", i + 1);
            const char* value = ini->getValue (key);
            long  reg, fc;
            if (value && parsePoint (value, reg, fc))
            {
                // Got valid reg/fc
                m_filter.cutoff[i][0] = (uint) reg;
                m_filter.cutoff[i][1] = (uint) fc;
            }
            else
                goto SidFilter_readType1_errorDefinition;
        }
    }

    m_status = FILTER_RESID;
return;

SidFilter_readType1_errorDefinition:
    m_errorString = "SID Filter: Invalid Type 1 filter definition";
    m_status = 0;
return;
}

void SidFilter::readType2 (iniParser *ini)
{
    double fs, fm, ft;

    // Read filter parameters
    const char* value = ini->getValue ("fs");
    if (!value)
        goto SidFilter_readType2_errorDefinition;
    try
    {
        fs = iniParser::parseDouble (value);
    } catch (iniParser::parseError e) { fs = 0.; }

    value = ini->getValue ("fm");
    if (!value)
        goto SidFilter_readType2_errorDefinition;
    try
    {
        fm = iniParser::parseDouble (value);
    } catch (iniParser::parseError e) { fm = 0.; }

    value = ini->getValue ("ft");
    if (!value)
        goto SidFilter_readType2_errorDefinition;
    try
    {
        ft = iniParser::parseDouble (value);
    } catch (iniParser::parseError e) { ft = 0.; }

    // Calculate the filter
    calcType2 (fs, fm, ft);

    m_status = FILTER_RESID;
return;

SidFilter_readType2_errorDefinition:
    m_errorString = "SID Filter: Invalid Type 2 filter definition";
    m_status = 0;
}

// Calculate a Type 2 filter (Sidplay 1 Compatible)
void SidFilter::calcType2 (double fs, double fm, double ft)
{
    const double fcMax = 1.0;
    const double fcMin = 0.01;
    double fc;

    // Definition from reSID
    m_filter.points = 0x100;

    // Create filter
    for (uint i = 0; i < 0x100; i++)
    {
        uint rk = i << 3;
        m_filter.cutoff[i][0] = rk;
        fc = exp ((double) rk / 0x800 * log (fs)) / fm + ft;
        if (fc < fcMin)
            fc = fcMin;
        if (fc > fcMax)
            fc = fcMax;
        m_filter.cutoff[i][1] = (uint) (fc * 4100);
    }
}

void SidFilter::readType3 (iniParser *ini)
{
    struct parampair {
        const char* name;
        float*      address;
    };

    struct parampair sidparams[] = {
        { "DistortionAttenuation",	&m_filterfp.attenuation           },
        { "DistortionNonlinearity",	&m_filterfp.distortion_nonlinearity },
        { "VoiceNonlinearity",		&m_filterfp.voice_nonlinearity    },
        { "Type3BaseResistance",	&m_filterfp.baseresistance        },
        { "Type3Offset",		&m_filterfp.offset                },
        { "Type3Steepness",		&m_filterfp.steepness             },
        { "Type3MinimumFETResistance",	&m_filterfp.minimumfetresistance  },
        { "Type4K",			&m_filterfp.k                     },
        { "Type4B",			&m_filterfp.b                     },
        { NULL,				NULL                            }
    };

    for (int i = 0; sidparams[i].name != NULL; i ++) {
        /* Ensure that all parameters are zeroed, if missing. */
        double tmp = 0.;
        const char* value = ini->getValue (sidparams[i].name);
        if (value) {
            try
            {
                tmp = iniParser::parseDouble (value);
            } catch  (iniParser::parseError e) {}
        }

        *sidparams[i].address = (float)tmp;
    }

    m_status = FILTER_RESIDFP;
}

// Get filter
const sid_filter_t *SidFilter::provide () const
{
    return (m_status&FILTER_RESID)?&m_filter:NULL;
}

// Get filter
const sid_filterfp_t *SidFilter::providefp () const
{
    return (m_status&FILTER_RESIDFP)?&m_filterfp:NULL;
}

// Copy filter from another SidFilter class
const SidFilter &SidFilter::operator= (const SidFilter &filter)
{
    (void) operator= (filter.provide ());
    (void) operator= (filter.providefp ());
    return filter;
}

// Copy sidplay2 sid_filter_t object
const sid_filter_t &SidFilter::operator= (const sid_filter_t &filter)
{
    m_filter = filter;
    m_status = FILTER_RESID;
    return filter;
}

const sid_filter_t *SidFilter::operator= (const sid_filter_t *filter)
{
    m_status = 0;
    if (filter)
        (void) operator= (*filter);
    return filter;
}

const sid_filterfp_t &SidFilter::operator= (const sid_filterfp_t &filter)
{
    m_filterfp = filter;
    m_status = FILTER_RESIDFP;
    return filter;
}

const sid_filterfp_t *SidFilter::operator= (const sid_filterfp_t *filter)
{
    m_status = 0;
    if (filter)
        (void) operator= (*filter);
    return filter;
}
