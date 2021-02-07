/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <errno.h>
#include <unistd.h>
#include <dmedia/audio.h>
#include "timidity.h"
#include "aenc.h"
#include "audriv.h"
#include "audio_cnv.h"
#include "timer.h"

#if !defined(SGI_OLDAL) && !defined(SGI_NEWAL)
#if defined(sgi) && __mips - 0 == 1
#define SGI_OLDAL
#endif
#endif /* SGI_OLDAL */

#ifdef SGI_OLDAL
#define alSetWidth ALsetwidth
#define alNewConfig ALnewconfig
#define alFreeConfig ALfreeconfig
#define alSetSampFmt ALsetsampfmt
#define alClosePort ALcloseport
#define alSetChannels ALsetchannels
#define alOpenPort ALopenport
#define alGetFrameTime ALgetframetime
#define alSetQueueSize ALsetqueuesize
#define alGetFilled ALgetfilled
#define alSetConfig ALsetconfig
#define alGetFillable ALgetfillable
#define alWriteFrames(p, s, n) ALwritesamps(p, s, (n) * play_nchannels)

#define stamp_t unsigned long long
#endif /* SGI_OLDAL */

#define OUT_AUDIO_QUEUE_SIZE 200000
void (* audriv_error_handler)(const char *errmsg) = NULL;
static ALconfig out_config;
static ALport   out = NULL;
static Bool audio_write_noblocking = False;
static int play_nchannels = 1;
static int play_frame_width = 2;
static int play_encoding   = AENC_SIGWORDB;
static int play_sample_rate = 8000;
static long reset_samples, play_counter;
static double play_start_time;

#ifndef SGI_OLDAL
static ALparamInfo out_ginfo;
#endif

char audriv_errmsg[BUFSIZ];

#ifndef SGI_OLDAL
#define ALERROR alGetErrorString(oserror())
#else
#define ALERROR strerror(oserror())
#endif /* SGI_OLDAL */

static void audriv_err(const char *msg)
{
    strncpy(audriv_errmsg, msg, sizeof(audriv_errmsg));
    if(audriv_error_handler != NULL)
	audriv_error_handler(audriv_errmsg);
}

static int audriv_al_set_width(ALconfig c, int encoding)
{
    if(encoding == AENC_G711_ULAW)
	return alSetWidth(c, 2);
    else
	return alSetWidth(c, AENC_SAMPW(encoding));
}

Bool audriv_setup_audio(void)
/* �����ǥ����ν������Ԥ��ޤ���
 * ������������ True �򡤼��Ԥ������� False ���֤��ޤ���
 */
{
    if(out_config)
	return True;

    if((out_config = alNewConfig()) == NULL)
    {
	audriv_err(ALERROR);
	return False;
    }

    out = NULL;

#ifndef SGI_OLDAL
    if(alGetParamInfo(AL_DEFAULT_OUTPUT, AL_GAIN, &out_ginfo) < 0)
    {
	audriv_err(ALERROR);
	alFreeConfig(out_config);
	out_config = NULL;
	return False;
    }
#endif /* SGI_OLDAL */

    if(alSetSampFmt(out_config, AL_SAMPFMT_TWOSCOMP) < 0)
    {
	audriv_err(ALERROR);
	return False;
    }

#if 0
    if(alSetQueueSize(out_config, OUT_AUDIO_QUEUE_SIZE) != 0)
    {
	fprintf(stderr, "Warning: Can't set queue size: %d\n",
		OUT_AUDIO_QUEUE_SIZE);
    }
#else
    alSetQueueSize(out_config, OUT_AUDIO_QUEUE_SIZE);
#endif

    return True;
}

void audriv_free_audio(void)
/* audio �θ������Ԥ��ޤ���
 */
{
    if(!out_config)
	return;
    if(out != NULL)
    {
	alClosePort(out);
	out = NULL;
    }

    alFreeConfig(out_config);
    out_config = NULL;

    if(out_config != NULL)
	alFreeConfig(out_config);
    out_config = NULL;
}

#ifndef SGI_OLDAL
static Bool audriv_al_set_rate(ALport port, unsigned long rate)
{
    ALpv pv;
    int r;

    r = alGetResource(port);
    pv.param    = AL_RATE;
    pv.value.ll = alIntToFixed(rate);
    if(alSetParams(r, &pv, 1) < 0)
	return False;
    return True;
}
#else
static Bool audriv_al_set_rate(ALport port, unsigned long rate)
{
    long pv[2];

    pv[0] = (port == out ? AL_OUTPUT_RATE : AL_INPUT_RATE);
    pv[1] = rate;
    if(ALsetparams(AL_DEFAULT_DEVICE, pv, 2) < 0)
	return False;
    return True;
}
#endif /* SGI_OLDAL */

Bool audriv_play_open(void)
/* audio ������Ѥ˳��������ĤǤ� audriv_write() �ˤ����ղ�ǽ��
 * ���֤ˤ��ޤ������˳����Ƥ�����Ϥʤˤ�Ԥ��ޤ���
 * ������������ True �򡤼��Ԥ������� False ���֤��ޤ���
 */
{
    if(out)
	return True;

    if(audriv_al_set_width(out_config, play_encoding) < 0)
    {
	audriv_err(ALERROR);
	return False;
    }

    alSetChannels(out_config, play_nchannels);
    out = alOpenPort("audriv", "w", out_config);
    if(!out)
    {
	audriv_err(ALERROR);
	return False;
    }

    if(audriv_al_set_rate(out, play_sample_rate) == False)
    {
	audriv_err(ALERROR);
	alClosePort(out);
	out = NULL;
	return False;
    }

    alSetConfig(out, out_config);
    reset_samples = 0;
    play_counter = 0;
    return True;
}

void audriv_play_close(void)
/* �����Ѥ˥����ץ󤵤줿 audio ���Ĥ��ޤ������Ǥ��Ĥ��Ƥ���
 * ���Ϥʤˤ�Ԥ��ޤ���
 */
{
    if(!out)
	return;
    while(audriv_play_active() == 1)
	audriv_wait_play();
    alClosePort(out);
    out = NULL;
}

long audriv_play_stop(void)
/* ���դ�¨�¤���ߤ������ľ���Υ���ץ�����֤��ޤ���
 * audriv_play_stop() �θƤӽФ��ˤ�äơ�audio ���Ĥ��ޤ���
 * audio �������Ĥ��Ƥ������ audriv_play_stop() ��ƤӽФ������� 0 ��
 * �֤��ޤ���
 * ���顼�ξ��� -1 ���֤��ޤ���
 */
{
    long samp;

    if(!out)
	return 0;
    samp = audriv_play_samples();
    alClosePort(out);
    reset_samples = play_counter = 0;
    out = NULL;
    return samp;
}

Bool audriv_is_play_open(void)
/* audio �����դǥ����ץ󤵤�Ƥ������ True,
 * �Ĥ��Ƥ������ False ���֤��ޤ���
 */
{
    return out != NULL;
}

Bool audriv_set_play_volume(int volume)
/* ���ղ��̤� 0 �� 255 ���ϰ�������ꤷ�ޤ���0 ��̵����255 �Ϻ��粻�̡�
 * 0 ̤���� 0��255 ��Ķ�����ͤ� 255 ��������
 * ������������ True �򡤼��Ԥ������� False ���֤��ޤ���
 */
{
#ifndef SGI_OLDAL
    double gain;
    ALfixed lrgain[2];
    ALpv pv;
    int resource;

    if(volume < 0)
	volume = 0;
    else if(volume > 255)
	volume = 255;

    if(volume == 0)
    {
	if(out_ginfo.specialVals & AL_NEG_INFINITY_BIT)
	    gain = alFixedToDouble(AL_NEG_INFINITY);
	else
	    gain = alFixedToDouble(out_ginfo.min.ll);
    }
    else if(volume == 255)
    {
	gain = alFixedToDouble(out_ginfo.max.ll);
    }
    else
    {
	double min, max;
	min = alFixedToDouble(out_ginfo.min.ll);
	max = alFixedToDouble(out_ginfo.max.ll);
	gain = min + (max - min) * (volume - 1) * (1.0/255);
	if(gain < min)
	    gain = min;
	else if(gain > max)
	    gain = max;
    }

    if(out == NULL)
	resource = AL_DEFAULT_OUTPUT;
    else
	resource = alGetResource(out);

    lrgain[0] = lrgain[1] = alDoubleToFixed(gain);
    pv.param = AL_GAIN;
    pv.value.ptr = lrgain;
    pv.sizeIn = 2;

    if(alSetParams(resource, &pv, 1) < 0)
    {
	audriv_err(ALERROR);
	return False;
    }
    return True;
#else
    long gain[4];

    if(volume < 0)
	volume = 0;
    else if(volume > 255)
	volume = 255;

    gain[0] = AL_LEFT_SPEAKER_GAIN;
    gain[1] = volume;
    gain[2] = AL_RIGHT_SPEAKER_GAIN;
    gain[3] = volume;
    if(ALsetparams(AL_DEFAULT_DEVICE, gain, 4) < 0)
    {
	audriv_err(ALERROR);
	return False;
    }
    return True;
#endif /* SGI_OLDAL */
}

int audriv_get_play_volume(void)
/* ���ղ��̤� 0 �� 255 ������ޤ���0 ��̵����255 �Ϻ��粻�̡�
 * ���Ԥ���� -1 ���֤��������Ǥʤ����� 0 �� 255 ��β��̤��֤��ޤ���
 */
{
#ifndef SGI_OLDAL
    ALfixed lrgain[2];
    ALpv pv;
    double gain, l, r, min, max;
    int volume;
    int resource;

    min = alFixedToDouble(out_ginfo.min.ll);
    max = alFixedToDouble(out_ginfo.max.ll);
    pv.param = AL_GAIN;
    pv.value.ptr = lrgain;
    pv.sizeIn = 2;
    if(out == NULL)
	resource = AL_DEFAULT_OUTPUT;
    else
	resource = alGetResource(out);

    if(alGetParams(resource, &pv, 1) < 0)
    {
	audriv_err(ALERROR);
	return -1;
    }
    l = alFixedToDouble(lrgain[0]);
    r = alFixedToDouble(lrgain[1]);
    if(l < min) l = min; else if(l > max) l = max;
    if(r < min) r = min; else if(r > max) r = max;
    gain = (l + r) / 2;
    volume = (gain - min) * 256 / (max - min);
    if(volume < 0)
	volume = 0;
    else if(volume > 255)
	volume = 255;
    return volume;
#else
    long gain[4];
    int volume;

    gain[0] = AL_LEFT_SPEAKER_GAIN;
    gain[2] = AL_RIGHT_SPEAKER_GAIN;
    if(ALgetparams(AL_DEFAULT_DEVICE, gain, 4) < 0)
    {
	audriv_err(ALERROR);
	return -1;
    }

    volume = (gain[1] + gain[3]) / 2;
    if(volume < 0)
	volume = 0;
    else if(volume > 255)
	volume = 255;
    return volume;
#endif /* SGI_OLDAL */
}

Bool audriv_set_play_output(int port)
/* audio �ν����� port ����ꤷ�ޤ���port �ˤϰʲ��Τɤ줫����ꤷ�ޤ���
 *
 *     AUDRIV_OUTPUT_SPEAKER	���ԡ����˽��ϡ�
 *     AUDRIV_OUTPUT_HEADPHONE	�إåɥۥ�˽��ϡ�
 *     AUDRIV_OUTPUT_LINE_OUT	�饤�󥢥��Ȥ˽��ϡ�
 *
 * ������������ True �򡤼��Ԥ������� False ���֤��ޤ���
 */
{
    if(port != AUDRIV_OUTPUT_SPEAKER)
    {
	audriv_err("���곰�ν��ϥݡ��Ȥ����ꤵ��ޤ�����");
	return False;
    }
    return True;
}

int audriv_get_play_output(void)
/* audio �ν����� port �����ޤ���
 * ���Ԥ���� -1 ���֤�����������Ȱʲ��Τ����줫���ͤ��֤��ޤ���
 *
 *     AUDRIV_OUTPUT_SPEAKER	���ԡ����˽��ϡ�
 *     AUDRIV_OUTPUT_HEADPHONE	�إåɥۥ�˽��ϡ�
 *     AUDRIV_OUTPUT_LINE_OUT	�饤�󥢥��Ȥ˽��ϡ�
 *
 */
{
    return AUDRIV_OUTPUT_SPEAKER;
}

long audriv_play_samples(void)
/* ���߱�����Υ���ץ���֤��֤��ޤ���
 */
{
    double realtime, es;

    realtime = get_current_calender_time();
    if(play_counter == 0)
    {
	play_start_time = realtime;
	return reset_samples;
    }
    es = play_sample_rate * (realtime - play_start_time);
    if(es >= play_counter)
    {
	/* out of play counter */
	reset_samples += play_counter;
	play_counter = 0;
	play_start_time = realtime;
	return reset_samples;
    }
    if(es < 0)
	return 0; /* for safety */
    return (int32)es + reset_samples;
}

static void add_sample_counter(int32 count)
{
    audriv_play_samples(); /* update offset_samples */
    play_counter += count;
}

int audriv_write(char *buff, int n)
/* audio �� buff �� n �Х���ʬή�����ߤޤ���
 * audriv_set_noblock_write() ����֥�å����⡼�ɤ����ꤵ�줿
 * ���ϡ����δؿ��θƤӽФ���¨�¤˽������֤�ޤ���
 * �֤��ͤϼºݤ�ή�����ޤ줿�Х��ȿ��Ǥ��ꡤ��֥�å����⡼�ɤ�����
 * ����Ƥ�����ϡ����� n ��꾯�ʤ���礬����ޤ���
 * ���Ԥ���� -1 ���֤�����������ȡ��ºݤ�ή�����ޤ줿�Х��ȿ����֤��ޤ���
 */
{
    n /= play_frame_width;

    if(audio_write_noblocking)
    {
	int size;
	size = alGetFillable(out);
	if(size < n)
	    n = size;
    }

    add_sample_counter(n);

    if(play_encoding != AENC_G711_ULAW)
    {
	alWriteFrames(out, buff, n);
	return n * play_frame_width;
    }
    else
    {
	/* AENC_G711_ULAW */
	int i, m, ret;
	short samps[BUFSIZ];

	n *= play_frame_width;
	ret = n;
	while(n > 0)
	{
	    m = n;
	    if(m > BUFSIZ)
		m = BUFSIZ;
	    for(i = 0; i < m; i++)
		samps[i] = AUDIO_U2S(buff[i]);
	    alWriteFrames(out, samps, m / play_frame_width);
	    buff += m;
	    n    -= m;
	}
	return ret;
    }
}

Bool audriv_set_noblock_write(Bool noblock)
/* noblock �� True ����ꤹ��ȡ�audriv_write() �ƤӽФ��ǥ֥�å����ޤ���
 * False ����ꤹ��ȡ��ǥե���Ȥξ��֤��ᤷ�ޤ���
 * ��������������� True �򡤼��Ԥ���� False ���֤��ޤ���
 */
{
    audio_write_noblocking = noblock;
    return True;
}

int audriv_play_active(void)
/* ������ʤ� 1��������Ǥʤ��ʤ� 0, ���顼�ʤ� -1 ���֤��ޤ���
 */
{
    return out && alGetFilled(out) > 0;
}

long audriv_get_filled(void)
/* �����ǥ����Хåե���ΥХ��ȿ����֤��ޤ���
 * ���顼�ξ��� -1 ���֤��ޤ���
 */
{
    if(out == NULL)
	return 0;
    return (long)play_frame_width * alGetFilled(out);
}

const long *audriv_available_encodings(int *n_ret)
/* �ޥ��󤬥��ݡ��Ȥ��Ƥ��뤹�٤Ƥ���沽�ꥹ�Ȥ��֤��ޤ���n_ret �ˤ�
 * ���μ���ο������֤���ޤ�����沽�򤢤�魯����ͤ�
 * aenc.h ����������Ƥ��� enum aenc_data_encoding ���ͤǤ���
 * �֤��ͤ� free ���ƤϤʤ�ޤ���
 */
{
    static const long encoding_list[] =
    {
	AENC_SIGBYTE,
	AENC_G711_ULAW, /* Pseudo audio encoding */
	AENC_SIGWORDB
    };

    *n_ret = 3;
    return encoding_list;
}

const long *audriv_available_sample_rates(int *n_ret)
/* �ޥ��󤬥��ݡ��Ȥ��Ƥ��뤹�٤ƤΥ���ץ�졼�ȤΥꥹ�Ȥ��֤��ޤ���
 * n_ret �ˤϤ��μ���ο������֤���ޤ���
 * �֤��ͤ� free ���ƤϤʤ�ޤ���
 */
{
    static const long sample_rates[] =
    {
	5512, 6615,
	8000, 9600, 11025, 16000, 18900, 22050, 32000, 37800, 44100, 48000
    };
    *n_ret = 12;
    return sample_rates;
}

const long *audriv_available_channels(int *n_ret)
/* �ޥ��󤬥��ݡ��Ȥ��Ƥ��뤹�٤ƤΥ���ͥ���Υꥹ�Ȥ��֤��ޤ���
 * n_ret �ˤϤ��μ���ο������֤���ޤ���
 * �֤��ͤ� free ���ƤϤʤ�ޤ���
 */
{
    static const long channels[] = {1, 2};
    *n_ret = 2;
    return channels;
}

Bool audriv_set_play_encoding(long encoding)
/* audio ���ջ�����沽��������ꤷ�ޤ���
 * ������������ True �򡤼��Ԥ������� False ���֤��ޤ���
 */
{
    int i, n;
    const long *enc;

    if(encoding == play_encoding)
	return True;

    enc = audriv_available_encodings(&n);
    for(i = 0; i < n; i++)
	if(enc[i] == encoding)
	    break;
    if(i == n)
	return False;

    play_encoding = encoding;
    play_frame_width = AENC_SAMPW(encoding) * play_nchannels;
    if(out)
    {
	audriv_al_set_width(out_config, encoding);
	alSetConfig(out, out_config);
    }
    return True;
}

Bool audriv_set_play_sample_rate(long sample_rate)
/* audio ���ջ��Υ���ץ�졼�Ȥ���ꤷ�ޤ���
 * ������������ True �򡤼��Ԥ������� False ���֤��ޤ���
 */
{
#if 0
    int i, n;
    const long *r;
    r = audriv_available_sample_rates(&n);
    for(i = 0; i < n; i++)
	if(r[i] == sample_rate)
	    break;
    if(i == n)
	return False;
#endif

    if(sample_rate == play_sample_rate)
	return True;
    play_sample_rate = sample_rate;

    if(out)
    {
	if(audriv_al_set_rate(out, sample_rate) == False)
	{
	    audriv_err(ALERROR);
	    return False;
	}
    }
    return True;
}

Bool audriv_set_play_channels(long channels)
/* �����ѤΥ���ͥ�������ꤷ�ޤ���
 * ���Ԥ���� False ���֤������������ True ���֤��ޤ���
 */
{
    int i, n;
    const long *c = audriv_available_channels(&n);

    for(i = 0; i < n; i++)
	if(channels == c[i])
	    break;
    if(i == n)
	return False;

    if(play_nchannels == channels)
	return True;
    play_nchannels = channels;
    play_frame_width = AENC_SAMPW(play_encoding) * play_nchannels;

    if(out)
    {
	audriv_play_close();
	audriv_play_open();
    }

    return True;
}

void audriv_wait_play(void)
/* CPU �ѥ��ϲ�񤷤ʤ��褦�ˤ��뤿��ˡ����Ū����ߤ��ޤ���*/
{
    double spare;

    spare = (double)alGetFilled(out) / (double)play_sample_rate;
    if(spare < 0.1)
	return;
    usleep((unsigned long)(spare * 500000.0));
}
