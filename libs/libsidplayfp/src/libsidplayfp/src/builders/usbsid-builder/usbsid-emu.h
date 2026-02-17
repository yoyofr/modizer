
#ifndef USBSID_EMU_H
#define USBSID_EMU_H

#include <cstdint>

#include "sidplayfp/SidConfig.h"
#include "sidemu.h"
#include "Event.h"
#include "EventScheduler.h"
#include "sidplayfp/siddefs.h"
#include "driver/src/USBSID.h"

#include "sidcxx11.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

namespace libsidplayfp
{

/***************************************************************************
 * USBSID SID Specialisation
 ***************************************************************************/
class USBSID final : public sidemu, private Event//, public USBSIDBuilder
{
private:
    /* USBSID specific data */
    USBSID_NS::USBSID_Class &m_sid;
    int m_handle;
    int sidno;

    bool m_status;

    uint8_t busValue;  /* Return value on read */

    SidConfig::sid_model_t runmodel;  /* Read model type */

    event_clock_t delay();  /* Event */

public:
    static const char* getCredits();

public:
    USBSID(sidbuilder *builder);
    ~USBSID() override;

    /* static variables required due to
     * multiple class initializations */
    static long raster_rate;
    static event_clock_t m_delayClk;

    bool getStatus() const { return m_status; }

    uint8_t read(uint_least8_t addr) override;
    void write(uint_least8_t addr, uint8_t data) override;

    /* c64sid functions */
    void reset(uint8_t volume) override;

    /* Standard SID functions */
    void clock() override {};


    void sampling(float systemclock, float freq,
        SidConfig::sampling_method_t method) override;

    void model(SidConfig::sid_model_t model, MAYBE_UNUSED bool digiboost) override;

    /* USBSID specific */
    void flush(void);
    void filter(bool enable);

    /* ISSUE: Disabled, blocks playing */
    // Must lock the SID before using the standard functions.
    // bool lock(EventScheduler *env) override;
    // void unlock() override;

private:
    // Fixed interval timer delay to prevent sidplay2
    // shoot to 100% CPU usage when song no longer
    // writes to SID.
    void event() override;
};

}

#endif // USBSID_EMU_H
