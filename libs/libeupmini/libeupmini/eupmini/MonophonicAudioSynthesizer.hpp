#ifndef TJH__EUP_MONOSYNTH_H
#define TJH__EUP_MONOSYNTH_H


#include <sys/types.h>



class EUP_TownsEmulator_MonophonicAudioSynthesizer {
    int _rate;
    int _velocity;
public:
    EUP_TownsEmulator_MonophonicAudioSynthesizer() {}
    virtual ~EUP_TownsEmulator_MonophonicAudioSynthesizer() {}
    virtual void setControlParameter(int control, int value) = 0;
    virtual void setInstrumentParameter(u_char const *fmInst,
                                        u_char const *pcmInst) = 0;
    virtual int velocity() const
    {
        return _velocity;
    }
    virtual void velocity(int velo)
    {
        _velocity = velo;
    }
    virtual void nextTick(int *outbuf, int buflen) = 0;
    virtual int rate() const
    {
        return _rate;
    }
    virtual void rate(int r)
    {
        _rate = r;
    }
    virtual void note(int n, int onVelo, int offVelo, int gateTime) = 0;
    virtual void pitchBend(int value) = 0;
};








#endif