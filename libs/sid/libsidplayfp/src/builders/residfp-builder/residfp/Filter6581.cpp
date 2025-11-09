/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2024 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2004,2010 Dag Lem <resid@nimrod.no>
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

#include "Filter6581.h"

#include "Integrator6581.h"

#include <cassert>

namespace reSIDfp
{

int Filter6581::solveIntegrators()
{
    Vbp = hpIntegrator.solve(Vhp);
    Vlp = bpIntegrator.solve(Vbp);

    int Vfilt = 0;
    if (lp) Vfilt += Vlp;
    if (bp) Vfilt += Vbp;
    if (hp) Vfilt += Vhp;

    // The filter input resistors are slightly bigger than the voice ones
    // Scale the values accordingly
    constexpr int filterGain = static_cast<int>(0.93 * (1 << 12));
    // Scaling unsigned values adds a DC offset
    constexpr int offset = 32767 * ((1 << 12) - filterGain);
    assert(Vfilt >= 0);
    return (Vfilt * filterGain + offset) >> 12;
}

Filter6581::~Filter6581()
{
    delete [] f0_dac;
}

void Filter6581::updateCenterFrequency()
{
    const unsigned short Vw = f0_dac[getFC()];
    hpIntegrator.setVw(Vw);
    bpIntegrator.setVw(Vw);
}

void Filter6581::setFilterCurve(double curvePosition)
{
    delete [] f0_dac;
    f0_dac = FilterModelConfig6581::getInstance()->getDAC(curvePosition);
    updateCenterFrequency();
}

void Filter6581::setFilterRange(double adjustment)
{
    FilterModelConfig6581::getInstance()->setFilterRange(adjustment);
}

} // namespace reSIDfp
