/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2025 Leandro Nini <drfiemost@users.sourceforge.net>
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

#ifndef SIDPLAYFP_H
#define SIDPLAYFP_H

#include <stdint.h>
#include <stdio.h>

#include "sidplayfp/siddefs.h"
#include "sidplayfp/sidversion.h"

class  SidConfig;
class  SidTune;
class  SidInfo;
class  EventContext;

// Private Sidplayer
namespace libsidplayfp
{
    class Player;
}

/**
 * sidplayfp
 */
class SID_EXTERN sidplayfp
{
private:
    libsidplayfp::Player &sidplayer;

public:
    sidplayfp();
    ~sidplayfp();

    /**
     * Get the current engine configuration.
     *
     * @return a const reference to the current configuration.
     */
    const SidConfig &config() const;

    /**
     * Get the current player informations.
     *
     * @return a const reference to the current info.
     */
    const SidInfo &info() const;

    /**
     * Configure the engine.
     * Check #error for detailed message if something goes wrong.
     *
     * @param cfg the new configuration
     * @return true on success, false otherwise.
     */
    bool config(const SidConfig &cfg);

    /**
     * Error message.
     *
     * @return string error message.
     */
    const char *error() const;

    /**
     * Set the fast-forward factor.
     *
     * @param percent
     * @deprecated
     */
    SID_DEPRECATED
    bool fastForward(unsigned int percent);

    /**
     * Load a tune.
     * Check #error for detailed message if something goes wrong.
     *
     * @param tune the SidTune to load, 0 unloads current tune.
     * @return true on sucess, false otherwise.
     */
    bool load(SidTune *tune);

    /**
     * Run the emulation and produce samples to play if a buffer is given.
     *
     * @param buffer pointer to the buffer to fill with samples.
     * @param count the size of the buffer measured in 16 bit samples
     *              or 0 if no output is needed (e.g. Hardsid)
     * @return the number of produced samples. If less than requested
     *         or #isPlaying() is false an error occurred, use #error()
     *         to get a detailed message.
     * @deprecated use #play(unsigned int)
     */
    SID_DEPRECATED
    uint_least32_t play(short *buffer, uint_least32_t count);

    /**
     * Get the buffer pointers for each of the installed SID chip.
     *
     * @param buffers pointer to the array of buffer pointers.
     * @since 2.14
     */
    void buffers(short** buffers) const;

    /**
     * Run the emulation for selected number of cycles.
     * The value will be limited to a reasonable amount
     * if too large.
     *
     * @param cycles the number of cycles to run.
     * @return the number of produced samples or zero
     * for hardware devices. If negative an error occurred,
     * use #error() to get a detailed message.
     * @since 2.14
     */
    int play(unsigned int cycles);

    /**
     * Reinitialize the engine.
     *
     * @return false in case of error, use #error()
     * to get a detailed message.
     * @since 2.15
     */
    bool reset();

    /**
     * Get the number of installed SID chips.
     *
     * @return the number of SID chips.
     * @since 2.14
     */
    unsigned int installedSIDs() const;

    /**
     * Init mixer.
     *
     * @param stereo whether to mix in stereo or mono
     * @since 2.15
     */
    void initMixer(bool stereo);

    /**
     * Mix buffers.
     *
     * @param buffer the output buffer
     * @param samples number of samples to mix, returned from the #play(unsigned int) function
     * @return number of samples generated (samples for mono, samples*2 for stereo)
     * @since 2.15
     */
    unsigned int mix(short *buffer, unsigned int samples);

    /**
     * Check if the engine is playing or stopped.
     *
     * @return true if playing, false otherwise.
     * @deprecated
     */
    SID_DEPRECATED
    bool isPlaying() const;

    /**
     * Stop the engine.
     * @deprecated
     */
    SID_DEPRECATED
    void stop();

    /**
     * Control debugging.
     * Only has effect if library have been compiled
     * with the --enable-debug option.
     *
     * @param enable enable/disable debugging.
     * @param out the file where to redirect the debug info.
     */
    void debug(bool enable, FILE *out);

    /**
     * Mute/unmute a SID channel.
     *
     * @param sidNum the SID chip, 0 for the first one, 1 for the second or 2 for the third.
     * @param voice the channel to mute/unmute, 0 to 2 for the voices or 3 for samples.
     * @param enable true unmutes the channel, false mutes it.
     */
    void mute(unsigned int sidNum, unsigned int voice, bool enable);

    /**
     * Enable/disable SID filter.
     * Must be called after #config or it has no effect.
     *
     * @param sidNum the SID chip, 0 for the first one, 1 for the second or 2 for the third.
     * @param enable true enable the filter, false disable it.
     * @since 2.10
     */
    void filter(unsigned int sidNum, bool enable);

    /**
     * Get the current playing time.
     *
     * @return the current playing time measured in seconds.
     */
    uint_least32_t time() const;

    /**
     * Get the current playing time.
     *
     * @return the current playing time measured in milliseconds.
     * @since 2.0
     */
    uint_least32_t timeMs() const;

    /**
     * Set ROM images.
     *
     * @param kernal pointer to Kernal ROM.
     * @param basic pointer to Basic ROM, generally needed only for BASIC tunes.
     * @param character pointer to character generator ROM.
     */
    void setRoms(const uint8_t* kernal, const uint8_t* basic=0, const uint8_t* character=0);

    /**
     * Set the ROM banks.
     *
     * @param rom pointer to the ROM data.
     * @since 2.2
     */
    //@{
    void setKernal(const uint8_t* rom);
    void setBasic(const uint8_t* rom);
    void setChargen(const uint8_t* rom);
    //@}

    /**
     * Get the CIA 1 Timer A programmed value.
     */
    uint_least16_t getCia1TimerA() const;

    /**
     * Get the SID registers programmed value.
     *
     * @param sidNum the SID chip, 0 for the first one, 1 for the second and 2 for the third.
     * @param regs an array that will be filled with the last values written to the chip.
     * @return false if the requested chip doesn't exist.
     * @since 2.2
     */
    bool getSidStatus(unsigned int sidNum, uint8_t regs[32]);
};

#endif // SIDPLAYFP_H
