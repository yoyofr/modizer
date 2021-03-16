/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2013 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2001 Simon White
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

#include "residfp.h"

#include <algorithm>
#include <new>

#include "residfp-emu.h"

ReSIDfpBuilder::~ReSIDfpBuilder()
{   // Remove all SID emulations
    remove();
}

// Create a new sid emulation.
unsigned int ReSIDfpBuilder::create(unsigned int sids)
{
    m_status = true;

    // Check available devices
    unsigned int count = availDevices();

    if (count && (count < sids))
        sids = count;

    for (count = 0; count < sids; count++)
    {
        try
        {
            sidobjs.insert(new libsidplayfp::ReSIDfp(this));
        }
        // Memory alloc failed?
        catch (std::bad_alloc const &)
        {
            m_errorBuffer.assign(name()).append(" ERROR: Unable to create ReSIDfp object");
            m_status = false;
            break;
        }
    }
    return count;

}

const char *ReSIDfpBuilder::credits() const
{
    return libsidplayfp::ReSIDfp::getCredits();
}

void ReSIDfpBuilder::filter(bool enable)
{
    std::for_each(sidobjs.begin(), sidobjs.end(), applyParameter<libsidplayfp::ReSIDfp, bool>(&libsidplayfp::ReSIDfp::filter, enable));
}

void ReSIDfpBuilder::filter6581Curve(double filterCurve)
{
    std::for_each(sidobjs.begin(), sidobjs.end(), applyParameter<libsidplayfp::ReSIDfp, double>(&libsidplayfp::ReSIDfp::filter6581Curve, filterCurve));
}

void ReSIDfpBuilder::filter8580Curve(double filterCurve)
{
    std::for_each(sidobjs.begin(), sidobjs.end(), applyParameter<libsidplayfp::ReSIDfp, double>(&libsidplayfp::ReSIDfp::filter8580Curve, filterCurve));
}
