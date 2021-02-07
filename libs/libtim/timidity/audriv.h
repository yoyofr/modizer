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

#ifndef ___AUDRIV_H_
#define ___AUDRIV_H_

#ifndef Bool
#define Bool int
#endif

#ifndef False
#define False 0
#endif

#ifndef True
#define True 1
#endif

extern char audriv_errmsg[BUFSIZ];
/* ���顼��ȯ���������ϡ����顼��å����������ꤵ��ޤ���
 * ����ư��Ƥ�����ϡ��ѹ�����ޤ���
 */

/* �����ǥ�������������򼨤��� */
enum audriv_ports
{
    AUDRIV_OUTPUT_SPEAKER,
    AUDRIV_OUTPUT_HEADPHONE,
    AUDRIV_OUTPUT_LINE_OUT
};


extern Bool audriv_setup_audio(void);
/* �����ǥ����ν������Ԥ��ޤ���
 * ������������ True �򡤼��Ԥ������� False ���֤��ޤ���
 */

extern void audriv_free_audio(void);
/* audio �θ������Ԥ��ޤ���
 */

extern Bool audriv_play_open(void);
/* audio ������Ѥ˳��������ĤǤ� audriv_write() �ˤ����ղ�ǽ��
 * ���֤ˤ��ޤ������˳����Ƥ�����Ϥʤˤ�Ԥ��ޤ���
 * ������������ True �򡤼��Ԥ������� False ���֤��ޤ���
 */

extern void audriv_play_close(void);
/* �����Ѥ˥����ץ󤵤줿 audio ���Ĥ��ޤ������Ǥ��Ĥ��Ƥ���
 * ���Ϥʤˤ�Ԥ��ޤ���
 */

extern long audriv_play_stop(void);
/* ���դ�¨�¤���ߤ������ľ���Υ���ץ�����֤��ޤ���
 * audriv_play_stop() �θƤӽФ��ˤ�äơ�audio ���Ĥ��ޤ���
 * audio �������Ĥ��Ƥ������ audriv_play_stop() ��ƤӽФ������� 0 ��
 * �֤��ޤ���
 * ���顼�ξ��� -1 ���֤��ޤ���
 */

extern Bool audriv_is_play_open(void);
/* audio �����դǥ����ץ󤵤�Ƥ������ True,
 * �Ĥ��Ƥ������ False ���֤��ޤ���
 */

extern Bool audriv_set_play_volume(int volume);
/* ���ղ��̤� 0 �� 255 ���ϰ�������ꤷ�ޤ���0 ��̵����255 �Ϻ��粻�̡�
 * 0 ̤���� 0��255 ��Ķ�����ͤ� 255 ��������
 * ������������ True �򡤼��Ԥ������� False ���֤��ޤ���
 */

extern int audriv_get_play_volume(void);
/* ���ղ��̤� 0 �� 255 ������ޤ���0 ��̵����255 �Ϻ��粻�̡�
 * ���Ԥ���� -1 ���֤��������Ǥʤ����� 0 �� 255 ��β��̤��֤��ޤ���
 */

extern Bool audriv_set_play_output(int port);
/* audio �ν����� port ����ꤷ�ޤ���port �ˤϰʲ��Τɤ줫����ꤷ�ޤ���
 *
 *     AUDRIV_OUTPUT_SPEAKER	���ԡ����˽��ϡ�
 *     AUDRIV_OUTPUT_HEADPHONE	�إåɥۥ�˽��ϡ�
 *     AUDRIV_OUTPUT_LINE_OUT	�饤�󥢥��Ȥ˽��ϡ�
 *
 * ������������ True �򡤼��Ԥ������� False ���֤��ޤ���
 */

extern int audriv_get_play_output(void);
/* audio �ν����� port �����ޤ���
 * ���Ԥ���� -1 ���֤�����������Ȱʲ��Τ����줫���ͤ��֤��ޤ���
 *
 *     AUDRIV_OUTPUT_SPEAKER	���ԡ����˽��ϡ�
 *     AUDRIV_OUTPUT_HEADPHONE	�إåɥۥ�˽��ϡ�
 *     AUDRIV_OUTPUT_LINE_OUT	�饤�󥢥��Ȥ˽��ϡ�
 *
 */

extern int audriv_write(char *buff, int n);
/* audio �� buff �� n �Х���ʬή�����ߤޤ���
 * audriv_set_noblock_write() ����֥�å����⡼�ɤ����ꤵ�줿
 * ���ϡ����δؿ��θƤӽФ���¨�¤˽������֤�ޤ���
 * �֤��ͤϼºݤ�ή�����ޤ줿�Х��ȿ��Ǥ��ꡤ��֥�å����⡼�ɤ�����
 * ����Ƥ�����ϡ����� n ��꾯�ʤ���礬����ޤ���
 * ���Ԥ���� -1 ���֤�����������ȡ��ºݤ�ή�����ޤ줿�Х��ȿ����֤��ޤ���
 */

extern Bool audriv_set_noblock_write(Bool noblock);
/* noblock �� True ����ꤹ��ȡ�audriv_write() �ƤӽФ��ǥ֥�å����ޤ���
 * False ����ꤹ��ȡ��ǥե���Ȥξ��֤��ᤷ�ޤ���
 * ��������������� True �򡤼��Ԥ���� False ���֤��ޤ���
 */

extern int audriv_play_active(void);
/* ������ʤ� 1��������Ǥʤ��ʤ� 0, ���顼�ʤ� -1 ���֤��ޤ���
 */

extern long audriv_play_samples(void);
/* ���߱�����Υ���ץ���֤��֤��ޤ���
 */

extern long audriv_get_filled(void);
/* �����ǥ����Хåե���ΥХ��ȿ����֤��ޤ���
 * ���顼�ξ��� -1 ���֤��ޤ���
 */

extern const long *audriv_available_encodings(int *n_ret);
/* �ޥ��󤬥��ݡ��Ȥ��Ƥ��뤹�٤Ƥ���沽�ꥹ�Ȥ��֤��ޤ���n_ret �ˤ�
 * ���μ���ο������֤���ޤ�����沽�򤢤�魯����ͤ�
 * aenc.h ����������Ƥ����ͤǤ���
 * �֤��ͤ� free ���ƤϤʤ�ޤ���
 */

extern const long *audriv_available_sample_rates(int *n_ret);
/* �ޥ��󤬥��ݡ��Ȥ��Ƥ��뤹�٤ƤΥ���ץ�졼�ȤΥꥹ�Ȥ��֤��ޤ���
 * �֤��ͤΥ���ץ�졼�Ȥ��㤤��ˤʤ��Ǥ��ޤ���
 * n_ret �ˤϤ��μ���ο������֤���ޤ���
 * �֤��ͤ� free ���ƤϤʤ�ޤ���
 */

extern const long *audriv_available_channels(int *n_ret);
/* �ޥ��󤬥��ݡ��Ȥ��Ƥ��뤹�٤ƤΥ���ͥ���Υꥹ�Ȥ��֤��ޤ���
 * n_ret �ˤϤ��μ���ο������֤���ޤ���
 * �֤��ͤ� free ���ƤϤʤ�ޤ���
 */

extern Bool audriv_set_play_encoding(long encoding);
/* audio ���ջ�����沽��������ꤷ�ޤ���
 * ������������ True �򡤼��Ԥ������� False ���֤��ޤ���
 */

extern Bool audriv_set_play_sample_rate(long sample_rate);
/* audio ���ջ��Υ���ץ�졼�Ȥ���ꤷ�ޤ���
 * ������������ True �򡤼��Ԥ������� False ���֤��ޤ���
 */

extern Bool audriv_set_play_channels(long channels);
/* �����ѤΥ���ͥ�������ꤷ�ޤ���
 * ���Ԥ���� False ���֤������������ True ���֤��ޤ���
 */

extern void (* audriv_error_handler)(const char *errmsg);
/* NULL �Ǥʤ���С����顼��ȯ���������ƤӽФ���ޤ���
 */

extern void audriv_wait_play(void);
/* CPU �ѥ��ϲ�񤷤ʤ��褦�ˤ��뤿��ˡ����Ū����ߤ��ޤ���*/

#endif /* ___AUDRIV_H_ */
