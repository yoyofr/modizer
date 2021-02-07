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

    bitset.h

    Author: Masanao Izumo <mo@goice.co.jp>
    Create: Sun Mar 02 1997
*/

#ifndef ___BITSET_H_
#define ___BITSET_H_

typedef struct _Bitset
{
    int nbits;
    unsigned int *bits;
} Bitset;
#define BIT_CHUNK_SIZE ((unsigned int)(8 * sizeof(unsigned int)))

/*
 * Bitset �ν����
 * ������塢���ƤΥӥåȤ� 0 �˽���������
 */
extern void init_bitset(Bitset *bitset, int nbits);

/*
 * start ���ܤΥӥåȤ��顢nbit ʬ��0 �˥��åȤ��롣
 */
extern void clear_bitset(Bitset *bitset, int start_bit, int nbits);

/*
 * start �ӥåȤ��顢nbits ʬ������
 */
extern void get_bitset(const Bitset *bitset, unsigned int *bits_return,
		       int start_bit, int nbits);
/* get_bitset �� 1 �ӥå��� */
extern int get_bitset1(Bitset *bitset, int n);

/*
 * start �ӥåȤ��顢nbits ʬ��bits �˥��åȤ���
 */
extern void set_bitset(Bitset *bitset, const unsigned int *bits,
		       int start_bit, int nbits);
/* set_bitset �� 1 �ӥå��� */
extern void set_bitset1(Bitset *bitset, int n, int bit);

/*
 * bitset ����� 1 �ӥåȤ�ޤޤ�Ƥ��ʤ���� 0 ���֤���
 * 1 �ӥåȤǤ�ޤޤ�Ƥ������ 0 �ʳ����ͤ��֤���
 */
extern unsigned int has_bitset(const Bitset *bitset);

/* bitset �����ɽ�� */
extern void print_bitset(Bitset *bitset);

#endif /* ___BITSET_H_ */
