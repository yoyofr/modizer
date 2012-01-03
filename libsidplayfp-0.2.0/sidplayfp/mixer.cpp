/***************************************************************************
                          mixer.cpp  -  Sids Mixer Routines
                             -------------------
    begin                : Sun Jul 9 2000
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
 *  $Log: mixer.cpp,v $
 *  Revision 1.12  2004/06/26 10:55:34  s_a_white
 *  Changes to support new calling convention for event scheduler.
 *
 *  Revision 1.11  2003/01/17 08:35:46  s_a_white
 *  Event scheduler phase support.
 *
 *  Revision 1.10  2002/01/29 21:50:33  s_a_white
 *  Auto switching to a better emulation mode.  m_tuneInfo reloaded after a
 *  config.  Initial code added to support more than two sids.
 *
 *  Revision 1.9  2001/12/13 08:28:08  s_a_white
 *  Added namespace support to fix problems with xsidplay.
 *
 *  Revision 1.8  2001/11/16 19:25:33  s_a_white
 *  Removed m_context as where getting mixed with parent class.
 *
 *  Revision 1.7  2001/10/02 18:29:32  s_a_white
 *  Corrected fixed point maths overflow caused by fastforwarding.
 *
 *  Revision 1.6  2001/09/17 19:02:38  s_a_white
 *  Now uses fixed point maths for sample output and rtc.
 *
 *  Revision 1.5  2001/07/25 17:02:37  s_a_white
 *  Support for new configuration interface.
 *
 *  Revision 1.4  2001/07/14 12:47:39  s_a_white
 *  Mixer routines simplified.  Added new and more efficient method of
 *  determining when an output samples is required.
 *
 *  Revision 1.3  2001/03/01 23:46:37  s_a_white
 *  Support for sample mode to be selected at runtime.
 *
 *  Revision 1.2  2000/12/12 22:50:15  s_a_white
 *  Bug Fix #122033.
 *
 ***************************************************************************/

#include "player.h"

const int_least32_t VOLUME_MAX = 255;
/* Scheduling time for next sample event. 20000 is roughly 20 ms and
 * gives us about 1k samples per mixing event on typical settings. */
const int MIXER_EVENT_RATE = 20000;

SIDPLAY2_NAMESPACE_START

void Player::mixerReset (void)
{
    m_mixerEvent.schedule(context(), MIXER_EVENT_RATE, EVENT_CLOCK_PHI1);
}

void Player::mixer (void)
{
    short *buf = m_sampleBuffer + m_sampleIndex;
    sidemu *chip1 = sid[0];
    sidemu *chip2 = sid[1];

    /* this clocks the SID to the present moment, if it isn't already. */
    chip1->clock();
    chip2->clock();

    /* extract buffer info now that the SID is updated.
     * clock() may update bufferpos. */
    short *buf1 = chip1->buffer();
    short *buf2 = chip2->buffer();
    int samples = chip1->bufferpos();
    /* NB: if chip2 exists, its bufferpos is identical to chip1's. */

    int i = 0;
    while (i < samples) {
        /* Handle whatever output the sid has generated so far */
        if (m_sampleIndex >= m_sampleCount) {
            m_running = false;
            break;
        }
        /* Are there enough samples to generate the next one? */
        if (i + m_fastForwardFactor >= samples)
            break;

        /* This is a crude boxcar low-pass filter to
         * reduce aliasing during fast forward, something I commonly do. */
        int sample1 = 0;
        int sample2 = 0;
        int j;
        for (j = 0; j < m_fastForwardFactor; j += 1) {
            sample1 += buf1[i + j];
            if (buf2 != NULL)
                sample2 += buf2[i + j];
        }
        /* increment i to mark we ate some samples, finish the boxcar thing. */
        i += j;
        sample1 = sample1 * m_leftVolume / VOLUME_MAX;
        sample1 /= j;
        sample2 = sample2 * m_rightVolume / VOLUME_MAX;
        sample2 /= j;

        /* mono mix. */
        if (buf2 != NULL && m_cfg.playback != sid2_stereo)
            sample1 = (sample1 + sample2) / 2;
        /* stereo clone, for people who keep stereo on permanently. */
        if (buf2 == NULL && m_cfg.playback == sid2_stereo)
            sample2 = sample1;

        *buf++ = (short int)sample1;
        m_sampleIndex ++;
        if (m_cfg.playback == sid2_stereo) {
            *buf++ = (short int)sample2;
            m_sampleIndex ++;
        }
    }

    /* move the unhandled data to start of buffer, if any. */
    int j = 0;
    for (j = 0; j < samples - i; j += 1) {
        buf1[j] = buf1[i + j];
        if (buf2 != NULL)
            buf2[j] = buf2[i + j];
    }
    chip1->bufferpos(j);
    if (buf2 != NULL)
        chip2->bufferpos(j);

    /* Post a callback to ourselves. */
    m_mixerEvent.schedule(context(), MIXER_EVENT_RATE, EVENT_CLOCK_PHI1);
}

SIDPLAY2_NAMESPACE_STOP
