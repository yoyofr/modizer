#ifndef  USBSID_H
#define  USBSID_H

#include "sidplayfp/sidbuilder.h"
#include "sidplayfp/siddefs.h"

#define USBSID_MAXSID 4

class SID_EXTERN USBSIDBuilder : public sidbuilder
{
protected:
    /**
     * Create the sid emu.
     */
    libsidplayfp::sidemu* create();

public:
    USBSIDBuilder(const char * const name);
    ~USBSIDBuilder();

    const char *getCredits() const;
    void flush();

    /**
     * enable/disable filter.
     */
    void filter(bool enable);

};

#endif // USBSID_H
