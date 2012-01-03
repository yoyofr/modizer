/***************************************************************************
                          event.h  -  Event scheduler (based on alarm
                                      from Vice)
                             -------------------
    begin                : Wed May 9 2001
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

#ifndef _event_h_
#define _event_h_

#include "sidtypes.h"

typedef uint_fast32_t event_clock_t;
typedef enum {EVENT_CLOCK_PHI1 = 0, EVENT_CLOCK_PHI2 = 1} event_phase_t;
#define EVENT_CONTEXT_MAX_PENDING_EVENTS 0x100

class SID_EXTERN Event
{
    friend class EventScheduler;

private:
    const char * const m_name;
    class EventContext *m_context;
    event_clock_t m_clk;

    /* This variable is set by the event context
       when it is scheduled */
    bool m_pending;

    /* Link to the next and previous events in the
       list.  */
    Event *m_next, *m_prev;

public:
    Event(const char * const name)
        : m_name(name),
          m_pending(false) {}
    ~Event() {}

    virtual void event (void) = 0;
    bool    pending () { return m_pending; }
    void    cancel  ();
    void    schedule(EventContext &context, event_clock_t cycles,
                     event_phase_t phase);
};

template< class This >
class EventCallback: public Event
{
private:
    typedef void (This::*Callback) ();
    This          &m_this;
    Callback const m_callback;
    void event(void) { (m_this.*m_callback) (); }

public:
    EventCallback (const char * const name, This &_this, Callback callback)
      : Event(name), m_this(_this),
        m_callback(callback) {}
};

// Public Event Context
class EventContext
{
    friend class Event;

public:
    virtual void cancel   (Event &event) = 0;
    virtual void schedule (Event &event, event_clock_t cycles,
                           event_phase_t phase) = 0;
    virtual event_clock_t getTime (event_phase_t phase) const = 0;
    virtual event_clock_t getTime (event_clock_t clock, event_phase_t phase) const = 0;
    virtual event_phase_t phase () const = 0;
};

// Private Event Context Object (The scheduler)
class EventScheduler: public EventContext, public Event
{
private:
    EventCallback<EventScheduler> m_timeWarp;
    uint  m_events;
    uint  m_events_future;

private:
    void event (void);

protected:
    void schedule (Event &event, event_clock_t cycles,
                   event_phase_t phase);
    void cancel   (Event &event)
    {
        event.m_pending      = false;
        event.m_prev->m_next = event.m_next;
        event.m_next->m_prev = event.m_prev;
        m_events--;
    }

public:
    EventScheduler (const char * const name);
    void reset     (void);

    void clock (void)
    {
//        m_clk++;
//        while (m_events && (m_clk >= m_next->m_clk))
//            dispatch (*m_next);
        Event &e = *m_next;
        m_clk = e.m_clk;
        cancel (e);
        //printf ("Event \"%s\"\n", e.m_name);
        e.event();
    }

    // Get time with respect to a specific clock phase
    event_clock_t getTime (event_phase_t phase) const
    {   return (m_clk + (phase ^ 1)) >> 1; }
    event_clock_t getTime (event_clock_t clock, event_phase_t phase) const
    {   return ((getTime (phase) - clock) << 1) >> 1; } // 31 bit res.
    event_phase_t phase () const { return (event_phase_t) (m_clk & 1); }
};

inline void Event::schedule (EventContext &context, event_clock_t cycles,
                             event_phase_t phase)
{
    context.schedule (*this, cycles, phase);
}

inline void Event::cancel ()
{
    if (m_pending)
        m_context->cancel (*this);
}

#endif // _event_h_
