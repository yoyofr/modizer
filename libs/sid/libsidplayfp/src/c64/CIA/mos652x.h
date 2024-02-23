/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2021 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2009-2014 VICE Project
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000 Simon White
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef MOS652X_H
#define MOS652X_H

#include <memory>

#include <stdint.h>

#include "interrupt.h"
#include "timer.h"
#include "tod.h"
#include "SerialPort.h"
#include "EventScheduler.h"

#include "sidcxx11.h"

class EventContext;

namespace libsidplayfp
{

class MOS652X;

/**
 * This is the timer A of this CIA.
 *
 * @author Ken Händel
 *
 */
class TimerA final : public Timer
{
private:
    /**
     * Signal underflows of Timer A to Timer B.
     */
    void underFlow() override;

    void serialPort() override;

public:
    /**
     * Create timer A.
     */
    TimerA(EventScheduler &scheduler, MOS652X &parent) :
        Timer("CIA Timer A", scheduler, parent) {}
};

/**
 * This is the timer B of this CIA.
 *
 * @author Ken Händel
 *
 */
class TimerB final : public Timer
{
private:
    void underFlow() override;

public:
    /**
     * Create timer B.
     */
    TimerB(EventScheduler &scheduler, MOS652X &parent) :
        Timer("CIA Timer B", scheduler, parent) {}

    /**
     * Receive an underflow from Timer A.
     */
    void cascade()
    {
        // we pretend that we are CPU doing a write to ctrl register
        syncWithCpu();
        state |= CIAT_STEP;
        wakeUpAfterSyncWithCpu();
    }

    /**
     * Check if start flag is set.
     *
     * @return true if start flag is set, false otherwise
     */
    bool started() const { return (state & CIAT_CR_START) != 0; }
};

/**
 * InterruptSource that acts like new CIA
 */
class InterruptSource8521 final : public InterruptSource
{
protected:
    void triggerInterrupt() override
    {
        idr |= INTERRUPT_REQUEST;
        idrTemp |= INTERRUPT_REQUEST;

        if (ack0())
            scheduleIrq();
    }

public:
    InterruptSource8521(EventScheduler &scheduler, MOS652X &parent) :
        InterruptSource(scheduler, parent)
    {}

    void trigger(uint8_t interruptMask) override;
};

/**
 * InterruptSource that acts like old CIA
 */
class InterruptSource6526 final : public InterruptSource
{
protected:
    void triggerInterrupt() override { idr |= INTERRUPT_REQUEST; }

public:
    InterruptSource6526(EventScheduler &scheduler, MOS652X &parent) :
        InterruptSource(scheduler, parent)
    {}

    void trigger(uint8_t interruptMask) override;

    uint8_t clear() override;
};

/**
 * This class is heavily based on the ciacore/ciatimer source code from VICE.
 * The CIA state machine is lifted as-is. Big thanks to VICE project!
 * The Serial Port emulation is based on Denise emu code.
 */
class MOS652X
{
    friend class InterruptSource;
    friend class SerialPort;
    friend class TimerA;
    friend class TimerB;
    friend class Tod;

public:
    typedef enum
    {
        MOS6526 = 0     ///< Old CIA model, interrupts are delayed by 1 clock
        ,MOS8521        ///< New CIA model
        ,MOS6526W4485   ///< A batch of old CIA model with unique serial port behavior
    } model_t;

private:
    static const char *credit;

protected:
    /// Event context.
    EventScheduler &eventScheduler;

    /// Ports
    //@{
    uint8_t &pra, &prb, &ddra, &ddrb;
    //@}

    /// These are all CIA registers.
    uint8_t regs[0x10];

    /// Timers A and B.
    //@{
    TimerA timerA;
    TimerB timerB;
    //@}

    /// Interrupt Source
    std::unique_ptr<InterruptSource> interruptSource;

    /// TOD
    Tod tod;

    /// Serial Data Registers
    SerialPort serialPort;

    /// Events
    //@{
    EventCallback<MOS652X> bTickEvent;
    //@}

private:
    /**
     * Trigger an interrupt from TOD.
     */
    void todInterrupt();

    /**
     * Trigger an interrupt from Serial Port.
     */
    void spInterrupt();

    /**
     * This event exists solely to break the ambiguity of what scheduling on
     * top of PHI1 causes, because there is no ordering between events on
     * same phase. Thus it is scheduled in PHI2 to ensure the b.event() is
     * run once before the value changes.
     *
     * - PHI1 a.event() (which calls underFlow())
     * - PHI1 b.event()
     * - PHI2 bTick.event()
     * - PHI1 a.event()
     * - PHI1 b.event()
     */
    void bTick();

    /**
     * Timer A underflow.
     */
    void underflowA();

    /** Timer B underflow. */
    void underflowB();

    /**
     * Handle the serial port.
     */
    void handleSerialPort();

protected:
    /**
     * Create a new CIA.
     *
     * @param context the event context
     */
    MOS652X(EventScheduler &scheduler);

    /**
     * Signal interrupt.
     *
     * @param state
     *            interrupt state
     */
    virtual void interrupt(bool state) = 0;

    virtual void portA() {}
    virtual void portB() {}

     /**
      * Timers can appear on the port.
      */
     uint8_t adjustDataPort(uint8_t data);

    /**
     * Read CIA register.
     *
     * @param addr
     *            register address to read (lowest 4 bits)
     */
    uint8_t read(uint_least8_t addr);

    /**
     * Write CIA register.
     *
     * @param addr
     *            register address to write (lowest 4 bits)
     * @param data
     *            value to write
     */
    void write(uint_least8_t addr, uint8_t data);

public:
    /**
     * Select chip model.
     *
     * @param model
     */
    void setModel(model_t model);

    /**
     * Reset CIA.
     */
    virtual void reset();

    /**
     * Get the credits.
     *
     * @return the credits
     */
    static const char *credits();

    /**
     * Set day-of-time event occurence of rate.
     *
     * @param clock
     */
    void setDayOfTimeRate(unsigned int clock) { tod.setPeriod(clock); }
};

}

#endif // MOS6526_H
