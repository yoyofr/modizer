// $Id: eupplayer_towns.h,v 1.4 1998/10/13 18:36:23 hayasaka Exp $


#ifndef TJH__EUP_TOWNS_H
#define TJH__EUP_TOWNS_H

#include "eupplayer.hpp"

class TownsAudioDevice : public PolyphonicAudioDevice {
public:
    TownsAudioDevice() {}
    virtual ~TownsAudioDevice() {}
    virtual void nextTick() {}
    virtual void enable(int channel, bool en=1) = 0;
    virtual void note(int channel, int n,
                      int onVelo, int offVelo, int gateTime) = 0;
    virtual void controlChange(int channel, int control, int value) = 0;
    virtual void programChange(int channel, int num) = 0;
    virtual void pitchBend(int channel, int value) = 0;
    virtual void assignFmDeviceToChannel(int channel, int fmChannel) = 0;
    virtual void assignPcmDeviceToChannel(int channel) = 0;
    virtual void setFmInstrumentParameter(int num, u_char const *instrument) = 0;
    virtual void setPcmInstrumentParameters(u_char const *instrument, size_t size) = 0;
};

#endif
