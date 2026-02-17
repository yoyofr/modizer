
#include "usbsid.h"
#include "usbsid-emu.h"

#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

#include "driver/src/USBSID.h"

#include "sidcxx11.h"

namespace libsidplayfp
{

long USBSID::raster_rate = 19950; /* Start on PAL */
event_clock_t USBSID::m_delayClk = 0;

const char* USBSID::getCredits()
{
    return
        "USBSID V" VERSION " Engine:\n"
        "\t(C) 2024-2025 LouD\n";
}


USBSID::USBSID(sidbuilder *builder) :
    sidemu(builder),
    Event("USBSID Delay"),
    m_sid(*(new USBSID_NS::USBSID_Class)),
    m_handle(-1),
    sidno(0),
    m_status(false),
    busValue(0)
{

    /* Start the fucker */
    m_handle = m_sid.USBSID_Init(true, true);
    if (m_handle < 0)
    {
        m_error = "USBSID init failed";
        return;
    }

    sidno = m_sid.USBSID_GetInstanceID();

    m_status = m_sid.USBSID_isOpen();
    if (!m_status)
    {
        m_error = "out of memory";
        return;
    }

    if (sidno == 0) {
        raster_rate = m_sid.USBSID_GetRasterRate();    /* Get the USBSID internal raster rate */
    }
}

USBSID::~USBSID()
{
    if (sidno == 0) {
        m_sid.USBSID_Mute();
    }
    delete &m_sid;
}

void USBSID::reset(uint8_t)
{
    using namespace std;

    m_delayClk = 0;
    if (sidno == 0) {
        m_sid.USBSID_Reset();
        m_sid.USBSID_UnMute();
    }
    if (eventScheduler != nullptr) {
        eventScheduler->schedule(*this, raster_rate, EVENT_CLOCK_PHI1);
    }
}

event_clock_t USBSID::delay()
{
    event_clock_t cycles = eventScheduler->getTime(EVENT_CLOCK_PHI1) - m_delayClk;
    // event_clock_t cycles = eventScheduler->getTime(EVENT_CLOCK_PHI1) - (m_delayClk - 1);
    m_delayClk += cycles;
    while (cycles > 0xffff)
    {
        m_sid.USBSID_WaitForCycle(0xffff);
        cycles -= 0xffff;
    }
    m_sid.USBSID_WaitForCycle(cycles);
    return cycles;
}

uint8_t USBSID::read(uint_least8_t)
{
    return busValue;  /* Always return the busValue */
}

void USBSID::write(uint_least8_t addr, uint8_t data)
{
    busValue = data;
    if (addr > 0x18)
        return;

    const unsigned int cycles = delay();
    uint_least8_t address = ((0x20 * sidno) + addr);
    m_sid.USBSID_WriteRingCycled(address, data, cycles);
}

void USBSID::model(SidConfig::sid_model_t model, MAYBE_UNUSED bool digiboost)
{
    /* Not used for USBSID (yet) */
    runmodel = model;
}

void USBSID::sampling(float systemclock, float freq,
        SidConfig::sampling_method_t method)
{
    (void)freq; /* Audio frequency is not used for USBSID-Pico */
    (void)method; /* Interpolation method is not used for USBSID-Pico */
    if (sidno == 0) {
        m_sid.USBSID_SetClockRate((long)systemclock, true);  /* Set the USBSID internal oscillator speed */
        raster_rate = m_sid.USBSID_GetRasterRate();    /* Get the USBSID internal raster rate */
    }
}

void USBSID::event()
{
    event_clock_t cycles = eventScheduler->getTime(EVENT_CLOCK_PHI1) - m_delayClk;
    if (cycles < raster_rate)
    {
        eventScheduler->schedule(*this, raster_rate - cycles, EVENT_CLOCK_PHI1);
    }
    else
    {
        m_accessClk += cycles;
        m_delayClk += cycles;
        m_sid.USBSID_WaitForCycle(cycles);
        m_sid.USBSID_Flush();
        eventScheduler->schedule(*this, raster_rate, EVENT_CLOCK_PHI1);
    }
}

void USBSID::filter(bool enable)
{
 (void) enable;
}

void USBSID::flush() /* Only gets call on player exit!? */
{
    m_sid.USBSID_Flush();
}

} /* libsidplayfp */
