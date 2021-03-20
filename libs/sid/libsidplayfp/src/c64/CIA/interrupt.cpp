/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2020 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
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

#include "interrupt.h"

#include "mos6526.h"


namespace libsidplayfp
{

void InterruptSource::event()
{
    triggerInterrupt();
    parent.interrupt(true);

    scheduled = false;
}

void InterruptSource::interrupt(bool state)
{
    parent.interrupt(state);
}

uint8_t InterruptSource::clear()
{
    last_clear = eventScheduler.getTime(EVENT_CLOCK_PHI2);

    if (scheduled)
    {
        eventScheduler.cancel(*this);
        scheduled = false;
    }

    uint8_t const old = idr;
    idr = 0;
    return old;
}

}
