/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2025 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000-2001 Simon White
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

#ifndef SIDBUILDER_H
#define SIDBUILDER_H

#include <set>
#include <string>

#include "sidplayfp/SidConfig.h"

namespace libsidplayfp
{
class sidemu;
class EventScheduler;
}

/**
 * Base class for sid builders.
 */
class sidbuilder
{
protected:
    typedef std::set<libsidplayfp::sidemu*> emuset_t;

private:
    const char * const m_name;

protected:
    std::string m_errorBuffer;

    emuset_t sidobjs;

protected:
    virtual libsidplayfp::sidemu* create() = 0;

    virtual const char *getCredits() const = 0;

public:
    sidbuilder(const char * const name) :
        m_name(name),
        m_errorBuffer("N/A") {}
    virtual ~sidbuilder() {}

    /**
     * The number of used devices.
     *
     * @return number of used sids, 0 if none.
     */
    unsigned int usedDevices() const { return sidobjs.size(); }

    /**
     * @deprecated does nothing
     */
    SID_DEPRECATED unsigned int create(unsigned int sids) { return sids; }

    /**
     * Find a free SID of the required specs
     *
     * @param scheduler the event scheduler
     * @param model the required sid model
     * @param digiboost whether to enable digiboost for 8580
     * @return pointer to the locked sid, null if none is available
     */
    libsidplayfp::sidemu *lock(libsidplayfp::EventScheduler *scheduler, SidConfig::sid_model_t model, bool digiboost);

    /**
     * Release this SID.
     *
     * @param device the sid to unlock
     */
    void unlock(libsidplayfp::sidemu *device);

    /**
     * Remove all SID emulations.
     */
    void remove();

    /**
     * Get the builder's name.
     *
     * @return the name
     */
    const char *name() const { return m_name; }

    /**
     * Error message.
     *
     * @return string error message.
     */
    const char *error() const { return m_errorBuffer.c_str(); }

    /**
     * Get the builder's credits.
     *
     * @return credits
     */
    const char *credits() const;
};

#endif // SIDBUILDER_H
