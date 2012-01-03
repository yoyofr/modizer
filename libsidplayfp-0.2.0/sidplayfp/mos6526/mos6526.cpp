/***************************************************************************
                          mos6526.cpp  -  CIA Timer
                             -------------------
    begin                : Wed Jun 7 2000
    copyright            : (C) 2000 by Simon White
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
 *  $Log: mos6526.cpp,v $
 *  Revision 1.28  2008/05/05 11:33:15  s_a_white
 *  Whilst re-programming and starting a timer causes a 2 cycle overhead,
 *  restarting a continuous timer from the interrupt has only a 1 cycle overhead.
 *  The 1 cycle overhead confirmed with NMIs, vice and Iisibiisi.sid.
 *
 *  Revision 1.27  2008/02/27 20:59:27  s_a_white
 *  Re-sync COM like interface and update to final names.
 *
 *  Revision 1.26  2006/10/28 08:39:55  s_a_white
 *  New, easier to use, COM style interface.
 *
 *  Revision 1.25  2006/04/09 16:50:49  s_a_white
 *  Confirmed timer re-programming overhead is 2 cycles.
 *
 *  Revision 1.24  2004/06/26 11:09:13  s_a_white
 *  Changes to support new calling convention for event scheduler.
 *
 *  Revision 1.23  2004/05/04 22:40:20  s_a_white
 *  Fix pulse mode output on the ports to work.
 *
 *  Revision 1.22  2004/04/23 00:58:22  s_a_white
 *  Reload counter on starting timer, not stopping.
 *
 *  Revision 1.21  2004/04/13 07:39:31  s_a_white
 *  Add lightpen support.
 *
 *  Revision 1.20  2004/03/20 16:17:29  s_a_white
 *  Clear all registers at reset.  Fix port B read bug.
 *
 *  Revision 1.19  2004/03/14 23:07:50  s_a_white
 *  Remove warning in Visual C about precendence order.
 *
 *  Revision 1.18  2004/03/09 20:52:30  s_a_white
 *  Added missing header.
 *
 *  Revision 1.17  2004/03/09 20:44:34  s_a_white
 *  Full serial and I/O port implementation.  No keyboard/joystick support as we
 *  are not that kind of emulator.
 *
 *  Revision 1.16  2004/02/29 14:29:44  s_a_white
 *  Serial port emulation.
 *
 *  Revision 1.15  2004/01/08 08:59:20  s_a_white
 *  Support the TOD frequency divider.
 *
 *  Revision 1.14  2004/01/06 21:28:27  s_a_white
 *  Initial TOD support (code taken from vice)
 *
 *  Revision 1.13  2003/10/28 00:22:53  s_a_white
 *  getTime now returns a time with respect to the clocks desired phase.
 *
 *  Revision 1.12  2003/02/24 19:44:30  s_a_white
 *  Make sure events are canceled on reset.
 *
 *  Revision 1.11  2003/01/17 08:39:04  s_a_white
 *  Event scheduler phase support.  Better default handling of keyboard lines.
 *
 *  Revision 1.10  2002/12/16 22:12:24  s_a_white
 *  Simulate serial input from data port A to prevent kernel lockups.
 *
 *  Revision 1.9  2002/11/20 22:50:27  s_a_white
 *  Reload count when timers are stopped
 *
 *  Revision 1.8  2002/10/02 19:49:21  s_a_white
 *  Revert previous change as was incorrect.
 *
 *  Revision 1.7  2002/09/11 22:30:47  s_a_white
 *  Counter interval writes now go to a new register call prescaler.  This is
 *  copied to the timer latch/counter as appropriate.
 *
 *  Revision 1.6  2002/09/09 22:49:06  s_a_white
 *  Proper idr clear if interrupt was only internally pending.
 *
 *  Revision 1.5  2002/07/20 08:34:52  s_a_white
 *  Remove unnecessary and pointless conts.
 *
 *  Revision 1.4  2002/03/03 22:04:08  s_a_white
 *  Tidy.
 *
 *  Revision 1.3  2001/07/14 13:03:33  s_a_white
 *  Now uses new component classes and event generation.
 *
 *  Revision 1.2  2001/03/23 23:21:38  s_a_white
 *  Removed redundant reset funtion.  Timer b now gets initialised properly.
 *  Switch case now allows write/read from timer b.
 *
 *  Revision 1.1  2001/03/21 22:41:45  s_a_white
 *  Non faked CIA emulation with NMI support.  Removal of Hacked VIC support
 *  off CIA timer.
 *
 *  Revision 1.8  2001/03/09 23:44:30  s_a_white
 *  Integrated more 6526 features.  All timer modes and interrupts correctly
 *  supported.
 *
 *  Revision 1.7  2001/02/21 22:07:10  s_a_white
 *  Prevent re-triggering of interrupt if it's already active.
 *
 *  Revision 1.6  2001/02/13 21:00:01  s_a_white
 *  Support for real interrupts.
 *
 *  Revision 1.4  2000/12/11 18:52:12  s_a_white
 *  Conversion to AC99
 *
 ***************************************************************************/

#include <string.h>
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "sidplayfp/sidendian.h"
#include "mos6526.h"

enum
{
    INTERRUPT_TA      = 1 << 0,
    INTERRUPT_TB      = 1 << 1,
    INTERRUPT_ALARM   = 1 << 2,
    INTERRUPT_SP      = 1 << 3,
    INTERRUPT_FLAG    = 1 << 4,
    INTERRUPT_REQUEST = 1 << 7
};

enum
{
    PRA     = 0,
    PRB     = 1,
    DDRA    = 2,
    DDRB    = 3,
    TAL     = 4,
    TAH     = 5,
    TBL     = 6,
    TBH     = 7,
    TOD_TEN = 8,
    TOD_SEC = 9,
    TOD_MIN = 10,
    TOD_HR  = 11,
    SDR     = 12,
    ICR     = 13,
    IDR     = 13,
    CRA     = 14,
    CRB     = 15
};

const char *MOS6526::credit =
{   // Optional information
    "*MOS6526 (CIA) Emulation:\0"
    "\tCopyright (C) 2001-2004 Simon White <sidplay2@yahoo.com>\0"
};


MOS6526::MOS6526 (EventContext *context)
:pra(regs[PRA]),
 prb(regs[PRB]),
 ddra(regs[DDRA]),
 ddrb(regs[DDRB]),
 idr(0),
 event_context(*context),
 m_phase(EVENT_CLOCK_PHI1),
 m_todPeriod(~0), // Dummy
 m_taEvent("CIA Timer A", *this, &MOS6526::ta_event),
 m_tbEvent("CIA Timer B", *this, &MOS6526::tb_event),
 m_tabEvent("CIA A underflow increments B", *this, &MOS6526::tab_event),
 m_todEvent("CIA Time of Day", *this, &MOS6526::tod_event),
 m_tapulse("CIA A pulse down", *this, &MOS6526::ta_pulse_down),
 m_tbpulse("CIA B pulse down", *this, &MOS6526::tb_pulse_down),
 m_trigger("Trigger Interrupt", *this, &MOS6526::trigger),
 m_taload("CIA Timer A Load", *this, &MOS6526::ta_load),
 m_tastart("CIA Timer A Start", *this, &MOS6526::ta_start),
 m_tacontinue("CIA Timer A Continue", *this, &MOS6526::ta_continue),
 m_tastop("CIA Timer A Stop", *this, &MOS6526::ta_stop),
 m_tbload("CIA Timer B Load", *this, &MOS6526::tb_load),
 m_tbstart("CIA Timer B Start", *this, &MOS6526::tb_start),
 m_tbcontinue("CIA Timer B Continue", *this, &MOS6526::tb_continue),
 m_tbstop("CIA Timer B Stop", *this, &MOS6526::tb_stop)
{
    reset ();
}

void MOS6526::ta_continue(void)
{
update_timers();
if (m_taEvent.pending())
	m_taEvent.cancel();
m_taEvent.schedule(event_context, (event_clock_t) ta, m_phase);
}

void MOS6526::ta_stop(void)
{
update_timers();
if (m_taEvent.pending())
	m_taEvent.cancel();
}

void MOS6526::ta_load(void)
{
update_timers();
if (m_taEvent.pending())
	m_taEvent.cancel();
ta = ta_latch;

// schedule start after load
if ((cra & 0x21) == 0x01)
	m_tastart.schedule(event_context, 1, m_phase);
}

void MOS6526::ta_start(void)
{
update_timers();
if (m_taEvent.pending())
	m_taEvent.cancel();
m_taEvent.schedule(event_context, (event_clock_t) ta, m_phase);
}

void MOS6526::tb_continue(void)
{
update_timers();
if (m_tbEvent.pending())
	m_tbEvent.cancel();
m_tbEvent.schedule(event_context, (event_clock_t) tb, m_phase);
}

void MOS6526::tb_stop(void)
{
update_timers();
if (m_tbEvent.pending())
	m_tbEvent.cancel();
}

void MOS6526::tb_load(void)
{
update_timers();
if (m_tbEvent.pending())
	m_tbEvent.cancel();
tb = tb_latch;

if ((crb & 0x61) == 0x01)
	m_tbstart.schedule(event_context, 1, m_phase);
}

void MOS6526::tb_start(void)
{
update_timers();
if (m_tbEvent.pending())
	m_tbEvent.cancel();
m_tbEvent.schedule(event_context, (event_clock_t) tb, m_phase);
}

void MOS6526::ta_pulse_down(void)
{
update_timers();
ta_pulse = false;
trigger();
if ((cra & 0x21) == 0x01)
	m_taEvent.schedule(event_context, (event_clock_t) ta, m_phase);
}

void MOS6526::tb_pulse_down(void)
{
update_timers();
tb_pulse = false;
trigger();
if ((crb & 0x61) == 0x01)
	m_tbEvent.schedule(event_context, (event_clock_t) tb, m_phase);
}

void MOS6526::update_timers(void)
{
const event_clock_t cycles = event_context.getTime (m_accessClk, event_context.phase ());
m_accessClk += cycles;

// Sync up timers
if (m_taEvent.pending())
	ta -= cycles;
if (m_tbEvent.pending())
	tb -= cycles;
}

void MOS6526::clear(void)
{
if (idr & INTERRUPT_REQUEST)
	interrupt(false);
idr = 0;
}


void MOS6526::clock (float64_t clock)
{    // Fixed point 25.7
m_todPeriod = (event_clock_t) (clock * (float64_t) (1 << 7));
}

void MOS6526::reset (void)
{
    ta  = ta_latch = 0xffff;
    tb  = tb_latch = 0xffff;
    ta_underflow = tb_underflow = false;
    cra = crb = sdr_out = 0;
    sdr_count = 0;
    sdr_buffered = false;
    // Clear off any IRQs
    clear();
    cnt_high  = true;
    icr = idr = 0;
    m_accessClk = 0;
    memset (regs, 0, sizeof (regs));

    // Reset tod
    memset(m_todclock, 0, sizeof(m_todclock));
    memset(m_todalarm, 0, sizeof(m_todalarm));
    memset(m_todlatch, 0, sizeof(m_todlatch));
    m_todlatched = false;
    m_todstopped = true;
    m_todclock[TOD_HR-TOD_TEN] = 1; // the most common value
    m_todCycles = 0;

    // Remove outstanding events
    m_taEvent.cancel ();
    m_tbEvent.cancel ();
    m_tabEvent.cancel();
    m_tapulse.cancel();
    m_tbpulse.cancel();
    m_trigger.cancel();
    m_taload.cancel();
    m_tastart.cancel();
    m_tacontinue.cancel();
    m_tastop.cancel();
    m_tbload.cancel();
    m_tbstart.cancel();
    m_tbcontinue.cancel();
    m_tbstop.cancel();
    m_todEvent.schedule (event_context, 0, m_phase);
}

uint8_t MOS6526::read (uint_least8_t addr)
{
addr &= 0x0f;

update_timers();

switch (addr)
	{
	case PRA: // Simulate a serial port
		return (regs[PRA] | ~regs[DDRA]);
	case PRB:{
		uint8_t data = regs[PRB] | ~regs[DDRB];
		// Timers can appear on the port
		if (cra & 0x02)
			{
			data &= 0xbf;
			if (cra & 0x04 ? ta_underflow : ta_pulse)
				data |= 0x40;
			}
		if (crb & 0x02)
			{
			data &= 0x7f;
			if (crb & 0x04 ? tb_underflow : tb_pulse)
				data |= 0x80;
			}
		return data;}
	case TAL:
		return endian_16lo8 (ta);
	case TAH:
		return endian_16hi8 (ta);
	case TBL:
		return endian_16lo8 (tb);
	case TBH:
		return endian_16hi8 (tb);

	// TOD implementation taken from Vice
	// TOD clock is latched by reading Hours, and released
	// upon reading Tenths of Seconds. The counter itself
	// keeps ticking all the time.
	// Also note that this latching is different from the input one.
	case TOD_TEN: // Time Of Day clock 1/10 s
	case TOD_SEC: // Time Of Day clock sec
	case TOD_MIN: // Time Of Day clock min
	case TOD_HR:  // Time Of Day clock hour
		if (!m_todlatched)
			memcpy(m_todlatch, m_todclock, sizeof(m_todlatch));
		if (addr == TOD_TEN)
			m_todlatched = false;
		if (addr == TOD_HR)
			m_todlatched = true;
		return m_todlatch[addr - TOD_TEN];
	case IDR:{
	// Clear IRQs, and return interrupt
	// data register
		const uint8_t ret = idr;
		clear();
		return ret;}
	case CRA:
		return cra;
	case CRB:
		return crb;
	default:
		return regs[addr];
	}
}

void MOS6526::write (uint_least8_t addr, uint8_t data)
{
addr &= 0x0f;

update_timers();

regs[addr] = data;

switch (addr)
	{
	case PRA:
	case DDRA:
		portA();
		break;
	case PRB:
	case DDRB:
		portB();
		break;
	case TAL:
		endian_16lo8 (ta_latch, data);
		break;
	case TAH:
		endian_16hi8 (ta_latch, data);
		if (!(cra & 0x01)) // Reload timer if stopped
			ta = ta_latch;
		break;
	case TBL:
		endian_16lo8 (tb_latch, data);
		break;
	case TBH:
		endian_16hi8 (tb_latch, data);
		if (!(crb & 0x01)) // Reload timer if stopped
			tb = tb_latch;
		break;

	// TOD implementation taken from Vice
	case TOD_HR:  // Time Of Day clock hour
	// Flip AM/PM on hour 12
	//   (Andreas Boose <viceteam@t-online.de> 1997/10/11).
	// Flip AM/PM only when writing time, not when writing alarm
	// (Alexander Bluhm <mam96ehy@studserv.uni-leipzig.de> 2000/09/17).
		data &= 0x9f;
		if ((data & 0x1f) == 0x12 && !(crb & 0x80))
			data ^= 0x80;
	// deliberate run on
	case TOD_TEN: // Time Of Day clock 1/10 s
	case TOD_SEC: // Time Of Day clock sec
	case TOD_MIN: // Time Of Day clock min
		if (crb & 0x80) m_todalarm[addr - TOD_TEN] = data;
		else
			{
			if (addr == TOD_TEN)
				m_todstopped = false;
			if (addr == TOD_HR)
				m_todstopped = true;
			m_todclock[addr - TOD_TEN] = data;
			}
	// check alarm
		if (!m_todstopped && !memcmp(m_todalarm, m_todclock, sizeof(m_todalarm)))
			{
			idr |= INTERRUPT_ALARM;
			trigger();
			}
		break;
	case SDR:
		if (cra & 0x40)
			sdr_buffered = true;
		break;
	case ICR:
		if (data & 0x80)
			icr |= data & 0x7f;
		else
			icr &= ~(data & 0x7f);
		m_trigger.schedule(event_context, (event_clock_t) 1, m_phase);
		break;
	case CRA:{
	// Reset the underflow flipflop for the data port
		const bool start = (data & 0x01) && !(cra & 0x01);
		const bool clearOneShot = !(data & 8) && (cra & 8);
		const bool load = (data & 0x10);
		if (start)
			ta_underflow = true;
		cra = data & 0xef;

        // Check for forced load
		if (load)
			{
			m_taload.schedule(event_context, 1, m_phase);
			break;
			}
		/* Clearing ONESHOT at T-1 seems to arrive too late to prevent
		* counter getting stopped. It is likely that the CIA calculates
		* if ((ctrl & 0x8) != 0) { ctrl &= 0xfe; } at the underflow clock
		* and at the clock before the underflow clock. */
		if (clearOneShot && ta == 1)
			cra = data & 0xee;
		// schedule timer continue
		if ((cra & 0x21) == 0x01)
			{   // Active
			if (!m_taEvent.pending())
				m_tacontinue.schedule(event_context, 1, m_phase);
			}
		else
			m_tastop.schedule(event_context, 1, m_phase);
		break;}
	case CRB:{
	// Reset the underflow flipflop for the data port
		const bool start = (data & 0x01) && !(crb & 0x01);
		const bool clearOneShot = !(data & 8) && (crb & 8);
		const bool load = (data & 0x10);
		if (start)
			tb_underflow = true;
		crb = data & 0xef;

	// Check for forced load
		if (load)
			{
			m_tbload.schedule(event_context, 1, m_phase);
			break;
			}
		if (clearOneShot && tb == 1)
			crb = data & 0xee;
		if ((crb & 0x61) == 0x01)
			{   // Active
			if (!m_tbEvent.pending())
				m_tbcontinue.schedule(event_context, 1, m_phase);
			}
		else
			m_tbstop.schedule(event_context, 1, m_phase);
		break;}
	}
}

void MOS6526::trigger (void)
{
if ((icr & idr & 0x7f) && !(idr & INTERRUPT_REQUEST))
	{
	idr |= INTERRUPT_REQUEST;
	interrupt (true);
	}
}

void MOS6526::tab_event (void)
{
tb--;
}

void MOS6526::ta_event (void)
{
update_timers();

// 1) signal interrupt to CPU & send the high pulse event.
// Pulse and IDR change occurs immediately, but IRQ trigger is delayed
// by a clock.
ta_pulse = true;
idr |= INTERRUPT_TA;

m_tapulse.schedule(event_context, (event_clock_t) 1, m_phase);

// 2) reload timer from latch.
// When the counter has reached zero, it is reloaded from the latch
// as soon as there is another clock waiting in the pipeline. In �2
// mode, this is always the case.
// This explains why you are reading zeros in cascaded mode only
// (2-2-2-1-1-1-0-0-2) but not in ø2 mode (2-1-2).
ta = ta_latch;

// By setting bits 2&3 of the control register,
// PB6/PB7 will be toggled between high and low at each underflow.
const bool toggle = (cra & 0x06) == 0x06;
ta_underflow = toggle && !ta_underflow;

// If the timer underflows and CRA bit 3 (one shot) is set or has
// been set in the clock before, then bit 0 of the CRA will be
// cleared. (Stops CIA.)
if (cra & 0x08)
	cra &= (~0x01);

// Handle serial port
if (cra & 0x40)
	{
	if (sdr_count)
		{
		if (!--sdr_count)
			{
			idr |= INTERRUPT_SP;
			trigger();
			}
		}
	if (!sdr_count && sdr_buffered)
		{
		sdr_out = regs[SDR];
		sdr_buffered = false;
		sdr_count = 16; // Output rate 8 bits at ta / 2
		}
	}

// Timer A signals underflows to Timer B
switch (crb & 0x61)
	{
	case 0x61:
		if (!cnt_high) break;
	case 0x41:
		if (tb == 0)
			m_tbEvent.schedule(event_context, (event_clock_t)1, m_phase);
		else
			m_tabEvent.schedule(event_context, (event_clock_t)2, m_phase);
	}
}

void MOS6526::tb_event (void)
{
update_timers();

tb_pulse = true;
idr |= INTERRUPT_TB;

m_tbpulse.schedule(event_context, 1, m_phase);

tb = tb_latch;

const bool toggle = (crb & 0x06) == 0x06;
tb_underflow = toggle && !tb_underflow;

if (crb & 0x08)
	crb &= (~0x01);
}

// TOD implementation taken from Vice
#define byte2bcd(byte) (((((byte) / 10) << 4) + ((byte) % 10)) & 0xff)
#define bcd2byte(bcd)  (((10*(((bcd) & 0xf0) >> 4)) + ((bcd) & 0xf)) & 0xff)

void MOS6526::tod_event(void)
{   // Reload divider according to 50/60 Hz flag
    // Only performed on expiry according to Frodo
    if (cra & 0x80)
        m_todCycles += (m_todPeriod * 5);
    else
        m_todCycles += (m_todPeriod * 6);

    // Fixed precision 25.7
    m_todEvent.schedule (event_context, m_todCycles >> 7, m_phase);
    m_todCycles &= 0x7F; // Just keep the decimal part

    if (!m_todstopped)
    {
        // inc timer
        uint8_t *tod = m_todclock;
        uint8_t t = bcd2byte(*tod) + 1;
        *tod++ = byte2bcd(t % 10);
        if (t >= 10)
        {
            t = bcd2byte(*tod) + 1;
            *tod++ = byte2bcd(t % 60);
            if (t >= 60)
            {
                t = bcd2byte(*tod) + 1;
                *tod++ = byte2bcd(t % 60);
                if (t >= 60)
                {
                    uint8_t pm = *tod & 0x80;
                    t = *tod & 0x1f;
                    if (t == 0x11)
                        pm ^= 0x80; // toggle am/pm on 0:59->1:00 hr
                    if (t == 0x12)
                        t = 1;
                    else if (++t == 10)
                        t = 0x10;   // increment, adjust bcd
                    t &= 0x1f;
                    *tod = t | pm;
                }
            }
        }
        // check alarm
        if (!memcmp(m_todalarm, m_todclock, sizeof(m_todalarm)))
		{
		idr |= INTERRUPT_ALARM;
		trigger();
		}
	}
}
