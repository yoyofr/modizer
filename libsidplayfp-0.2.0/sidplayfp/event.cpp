/***************************************************************************
                          event.cpp  -  Event schdeduler (based on alarm
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
/***************************************************************************
 *  $Log: event.cpp,v $
 *  Revision 1.12  2004/06/26 10:52:29  s_a_white
 *  Support new event template class and add minor code improvements.
 *
 *  Revision 1.11  2003/02/20 19:10:16  s_a_white
 *  Code simplification.
 *
 *  Revision 1.10  2003/01/24 19:30:39  s_a_white
 *  Made code slightly more efficient.  Changes to this code greatly effect
 *  sidplay2s performance.  Somehow need to speed up the schedule routine.
 *
 *  Revision 1.9  2003/01/23 17:32:37  s_a_white
 *  Redundent code removal.
 *
 *  Revision 1.8  2003/01/17 08:35:46  s_a_white
 *  Event scheduler phase support.
 *
 *  Revision 1.7  2002/11/21 19:55:38  s_a_white
 *  We now jump to next event directly instead on clocking by a number of
 *  cycles.
 *
 *  Revision 1.6  2002/07/17 19:20:03  s_a_white
 *  More efficient event handling code.
 *
 *  Revision 1.5  2001/10/02 18:24:09  s_a_white
 *  Updated to support safe scheduler interface.
 *
 *  Revision 1.4  2001/09/17 19:00:28  s_a_white
 *  Constructor moved out of line.
 *
 *  Revision 1.3  2001/09/15 13:03:50  s_a_white
 *  timeWarp now zeros m_eventClk instead of m_pendingEventClk which
 *  fixes a inifinite loop problem when driving libsidplay1.
 *
 ***************************************************************************/

#include "event.h"

#define EVENT_TIMEWARP_COUNT 0x0FFFFF


EventScheduler::EventScheduler (const char * const name)
:Event(name),
 m_timeWarp("Time Warp", *this, &EventScheduler::event),
 m_events(0),
 m_events_future(0)
{
    m_next = this;
    m_prev = this;
    m_timeWarp.m_clk = 0;
    reset ();
}

// Used to prevent overflowing by timewarping
// the event clocks
void EventScheduler::event ()
{
    m_events = m_events_future;
    m_events_future = 0;
    m_timeWarp.m_next = this;
    m_timeWarp.m_prev = this->m_prev;
    this->m_prev->m_next = &m_timeWarp;
    this->m_prev = &m_timeWarp;
    m_timeWarp.m_pending = true;
}

void EventScheduler::reset (void)
{   // Remove all events
    Event *e = m_next;
    //uint   count = m_events;
    m_pending = false;
    while (e->m_pending)
    {
        e->m_pending = false;
        e = e->m_next;
    }
    m_next = this;
    m_prev = this;
    m_clk  = 0;
    m_events = 0;
    m_events_future = 0;
    event ();
}

// Add event to ordered pending queue
void EventScheduler::schedule (Event &event, event_clock_t cycles,
                               event_phase_t phase)
{
    for (;;)
    {
        if (!event.m_pending)
        {
            event_clock_t clk = m_clk + (cycles << 1);
            clk += ((clk & 1) ^ phase);

            // Now put in the correct place so we don't need to keep
            // searching the list later.
            Event *e;
            uint count;
            if (clk >= m_clk)
            {
                e = m_next;
                count = m_events++;
            }
            else
            {
                e = m_timeWarp.m_next;
                count = m_events_future++;
            }

            while (count-- && (e->m_clk <= clk))
                e = e->m_next;

            event.m_next      = e;
            event.m_prev      = e->m_prev;
            e->m_prev->m_next = &event;
            e->m_prev         = &event;
            event.m_pending   = true;
            event.m_clk       = clk;
            event.m_context   = this;
            break;
        }
        event.cancel ();
    }
}
