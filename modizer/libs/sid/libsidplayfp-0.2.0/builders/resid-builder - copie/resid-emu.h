/***************************************************************************
                          resid-emu.h  -  ReSid Emulation
                             -------------------
    begin                : Fri Apr 4 2001
    copyright            : (C) 2001 by Simon White
    email                : s_a_white@email.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "resid/sid.h"

//FIXME
#define RESID_NAMESPACE reSID

#ifdef RESID_NAMESPACE
#   define RESID_NS ::RESID_NAMESPACE
#else
#   define RESID_NS
#endif

enum {
    OUTPUTBUFFERSIZE = 5000 //32768
};

class ReSID: public sidemu
{
private:
    EventContext *m_context;
    event_phase_t m_phase;
    class RESID_NS::SID &m_sid;
    event_clock_t m_accessClk;
    static char   m_credit[180];
    const  char  *m_error;
    bool          m_status;
    bool          m_locked;

public:
    ReSID  (sidbuilder *builder);
    ~ReSID (void);

    // Standard component functions
    const char   *credits (void) {return m_credit;}
    void          reset   () { sidemu::reset (); }
    void          reset   (uint8_t volume);
    uint8_t       read    (uint_least8_t addr);
    void          write   (uint_least8_t addr, uint8_t data);
    const char   *error   (void) {return m_error;}

    // Standard SID functions
    void          clock   ();
    void          filter  (bool enable);
    void          voice   (uint_least8_t num, bool mute) {;}

    operator bool () { return m_status; }
    static   int  devices (char *error);

    // Specific to resid
    void sampling (float systemclock, float freq,
    const sampling_method_t method, const bool fast);
    //bool filter   (const sid_filter_t *filter); //FIXME remove?
    void bias     (const float dac_bias);
    void model    (sid2_model_t model);
    // Must lock the SID before using the standard functions.
    bool lock     (c64env *env);
};
