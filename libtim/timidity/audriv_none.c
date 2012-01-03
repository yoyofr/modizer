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

#include "timidity.h"
#include "aenc.h"
#include "audriv.h"
#include "timer.h"

#ifdef DEBUG
#include "audio_cnv.h"
#include <math.h>
#endif

void (* audriv_error_handler)(const char *errmsg) = NULL;

static int play_sample_rate = 8000;
static int play_sample_size = 1;
static double play_start_time;
static long play_counter, reset_samples;
static int play_encoding   = AENC_G711_ULAW;
static int play_channels = 1;
static int output_port = AUDRIV_OUTPUT_SPEAKER;
static int play_volume   = 0;
static Bool play_open_flag = False;

char audriv_errmsg[BUFSIZ];

static void audriv_err(const char *msg)
{
    strncpy(audriv_errmsg, msg, sizeof(audriv_errmsg) - 1);
    if(audriv_error_handler != NULL)
	audriv_error_handler(audriv_errmsg);
}

Bool audriv_setup_audio(void)
/* �����ǥ����ν������Ԥ��ޤ���
 * ������������ True �򡤼��Ԥ������� False ���֤��ޤ���
 */
{
    return True;
}

void audriv_free_audio(void)
/* audio �θ������Ԥ��ޤ���
 */
{
}

Bool audriv_play_open(void)
/* audio ������Ѥ˳��������ĤǤ� audriv_write() �ˤ����ղ�ǽ��
 * ���֤ˤ��ޤ������˳����Ƥ�����Ϥʤˤ�Ԥ��ޤ���
 * ������������ True �򡤼��Ԥ������� False ���֤��ޤ���
 */
{
    if(play_open_flag)
	return True;
    play_counter = 0;
    reset_samples = 0;
    play_open_flag = True;
    return True;
}

void audriv_play_close(void)
/* �����Ѥ˥����ץ󤵤줿 audio ���Ĥ��ޤ������Ǥ��Ĥ��Ƥ���
 * ���Ϥʤˤ�Ԥ��ޤ���
 */
{
    play_open_flag = False;
}

static long calculate_play_samples(void)
{
    long n, ret;

    if(play_counter <= 0)
	return 0;

    n = (long)((double)play_sample_rate
	       * (double)play_sample_size
	       * (get_current_calender_time() - play_start_time));
    if(n > play_counter)
	ret = play_counter / play_sample_size;
    else
	ret = n / play_sample_size;

    return ret;
}

long audriv_play_stop(void)
/* ���դ�¨�¤���ߤ������ľ���Υ���ץ�����֤��ޤ���
 * audriv_play_stop() �θƤӽФ��ˤ�äơ�audio ���Ĥ��ޤ���
 * audio �������Ĥ��Ƥ������ audriv_play_stop() ��ƤӽФ������� 0 ��
 * �֤��ޤ���
 * ���顼�ξ��� -1 ���֤��ޤ���
 */
{
    long n;

    n = audriv_play_samples();
    play_open_flag = False;
    return n;
}

Bool audriv_is_play_open(void)
/* audio �����դǥ����ץ󤵤�Ƥ������ True,
 * �Ĥ��Ƥ������ False ���֤��ޤ���
 */
{
    return play_open_flag;
}

Bool audriv_set_play_volume(int volume)
/* ���ղ��̤� 0 �� 255 ���ϰ�������ꤷ�ޤ���0 ��̵����255 �Ϻ��粻�̡�
 * 0 ̤���� 0��255 ��Ķ�����ͤ� 255 ��������
 * ������������ True �򡤼��Ԥ������� False ���֤��ޤ���
 */
{
    if(volume < 0)
	play_volume = 0;
    else if(volume > 255)
	play_volume = 255;
    else
	play_volume = volume;
    return True;
}

int audriv_get_play_volume(void)
/* ���ղ��̤� 0 �� 255 ������ޤ���0 ��̵����255 �Ϻ��粻�̡�
 * ���Ԥ���� -1 ���֤��������Ǥʤ����� 0 �� 255 ��β��̤��֤��ޤ���
 */
{
    return play_volume;
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
    switch(port)
    {
      case AUDRIV_OUTPUT_SPEAKER:
      case AUDRIV_OUTPUT_HEADPHONE:
      case AUDRIV_OUTPUT_LINE_OUT:
	output_port = port;
	break;
      default:
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
    return output_port;
}

#ifdef CFG_FOR_SF
static int record_volume = 0;
#endif
int audriv_get_record_volume(void)
/* Ͽ�����̤� 0 �� 255 ������ޤ���0 ��̵����255 �Ϻ��粻�̡�
 * ���Ԥ���� -1 ���֤��������Ǥʤ����� 0 �� 255 ��β��̤��֤��ޤ���
 */
{
    return record_volume;
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
    long qsz;

    qsz = audriv_get_filled();
    if(qsz == -1)
	return -1;
    if(qsz == 0)
    {
	reset_samples += play_counter / play_sample_size;
	play_counter = 0; /* Reset start time */
    }
    if(play_counter == 0)
	play_start_time = get_current_calender_time();
    play_counter += n;
    return n;
}

Bool audriv_set_noblock_write(Bool noblock)
/* noblock �� True ����ꤹ��ȡ�audriv_write() �ƤӽФ��ǥ֥�å����ޤ���
 * False ����ꤹ��ȡ��ǥե���Ȥξ��֤��ᤷ�ޤ���
 * ��������������� True �򡤼��Ԥ���� False ���֤��ޤ���
 */
{
    return True;
}

int audriv_play_active(void)
/* ������ʤ� 1��������Ǥʤ��ʤ� 0, ���顼�ʤ� -1 ���֤��ޤ���
 */
{
    long n;

    n = audriv_get_filled();
    if(n > 0)
	return 1;
    return n;
}

long audriv_play_samples(void)
/* ���߱�����Υ���ץ���֤��֤��ޤ���
 */
{
    return reset_samples + calculate_play_samples();
}

long audriv_get_filled(void)
/* �����ǥ����Хåե���ΥХ��ȿ����֤��ޤ���
 * ���顼�ξ��� -1 ���֤��ޤ���
 */
{
    long n;

    if(play_counter <= 0)
	return 0;
    n = (int)((double)play_sample_rate
	      * play_sample_size
	      * (get_current_calender_time() - play_start_time));
    if(n > play_counter)
	return 0;
    return play_counter - n;
}

const long *audriv_available_encodings(int *n_ret)
/* �ޥ��󤬥��ݡ��Ȥ��Ƥ��뤹�٤Ƥ���沽�ꥹ�Ȥ��֤��ޤ���n_ret �ˤ�
 * ���μ���ο������֤���ޤ�����沽�򤢤�魯����ͤ�
 * usffd.h ����������Ƥ��� enum usffd_data_encoding ���ͤǤ���
 * �֤��ͤ� free ���ƤϤʤ�ޤ���
 */
{
    static const long encoding_list[] =
    {
	AENC_SIGBYTE, AENC_UNSIGBYTE, AENC_G711_ULAW,
	AENC_G711_ALAW, AENC_SIGWORDB, AENC_UNSIGWORDB,
	AENC_SIGWORDL, AENC_UNSIGWORDL
    };

    *n_ret = 8;
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

    enc = audriv_available_encodings(&n);
    for(i = 0; i < n; i++)
	if(enc[i] == encoding)
	{
	    play_encoding = encoding;
	    play_sample_size = AENC_SAMPW(encoding) * play_channels;
	    return True;
	}
    return False;
}

Bool audriv_set_play_sample_rate(long sample_rate)
/* audio ���ջ��Υ���ץ�졼�Ȥ���ꤷ�ޤ���
 * ������������ True �򡤼��Ԥ������� False ���֤��ޤ���
 */
{
    int i, n;
    const long *r;

    r = audriv_available_sample_rates(&n);
    for(i = 0; i < n; i++)
	if(r[i] == sample_rate)
	{
	    play_sample_rate = sample_rate;
	    return True;
	}
    return False;
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
	{
	    play_channels = channels;
	    return True;
	}
    return False;
}

void audriv_wait_play(void)
/* CPU �ѥ��ϲ�񤷤ʤ��褦�ˤ��뤿��ˡ����Ū����ߤ��ޤ���*/
{
}
