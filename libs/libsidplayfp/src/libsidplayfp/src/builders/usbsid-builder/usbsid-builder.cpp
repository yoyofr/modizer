
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
#include <sstream>
#include <string>

#include "usbsid.h"
#include "usbsid-emu.h"


USBSIDBuilder::USBSIDBuilder(const char * const name) :
    sidbuilder(name)
{}

USBSIDBuilder::~USBSIDBuilder()
{
    /* Remove all SID objects */
    remove();
}

libsidplayfp::sidemu* USBSIDBuilder::create()
{
    /* Always init a new Object */
    try
    {
        std::unique_ptr<libsidplayfp::USBSID> sid(new libsidplayfp::USBSID(this));

        // SID init failed?
        if (!sid->getStatus())
        {
            m_errorBuffer = sid->error();
            return nullptr;
        }
        return sid.release();
    }
    /* Memory alloc failed? */
    catch (std::bad_alloc const &)
    {
        m_errorBuffer.assign(name()).append(" ERROR: Unable to create USBSID object");
        return nullptr;
    }
}

const char *USBSIDBuilder::getCredits() const
{
    return libsidplayfp::USBSID::getCredits();
}

void USBSIDBuilder::flush()
{
    for (libsidplayfp::sidemu* e: sidobjs)
        static_cast<libsidplayfp::USBSID*>(e)->flush();
}

void USBSIDBuilder::filter (bool enable)
{
    for (libsidplayfp::sidemu* e: sidobjs)
        static_cast<libsidplayfp::USBSID*>(e)->filter(enable);
}
