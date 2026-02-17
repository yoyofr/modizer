/***************************************************************************
      exsid-builder.cpp - exSID builder class for creating/controlling
                               exSIDs.
                               -------------------
   Based on hardsid-builder.cpp (C) 2001 Simon White

    copyright            : (C) 2015 Thibaut VARENE
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 ***************************************************************************/

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
#include <sstream>
#include <string>

#include "properties.h"
#include "exsid.h"
#include "exsid-emu.h"


exSIDBuilder::exSIDBuilder(const char * const name) :
    sidbuilder(name)
{}

exSIDBuilder::~exSIDBuilder()
{
    // Remove all SID emulations
    remove();
}

// Create a new sid emulation. Called by sidbuilder only
libsidplayfp::sidemu* exSIDBuilder::create()
{
    try
    {
        std::unique_ptr<libsidplayfp::exSID> sid(new libsidplayfp::exSID(this));

        // SID init failed?
        if (!sid->getStatus())
        {
            m_errorBuffer = sid->error();
            return nullptr;
        }
        return sid.release();
    }
    // Memory alloc failed?
    catch (std::bad_alloc const &)
    {
        m_errorBuffer.assign(name()).append(" ERROR: Unable to create exSID object");
        return nullptr;
    }
}

const char *exSIDBuilder::getCredits() const
{
    return libsidplayfp::exSID::getCredits();
}

void exSIDBuilder::flush()
{
    for (libsidplayfp::sidemu* e: sidobjs)
        static_cast<libsidplayfp::exSID*>(e)->flush();
}
