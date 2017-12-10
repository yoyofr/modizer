//=============================================================================
//	Professional Music Driver [P.M.D.] version 4.8
//			Programmed By M.Kajihara
//			Windows Converted by C60
//=============================================================================

#ifdef WINDOWS
# include <windows.h>
# include <process.h>
# include <mbstring.h>
#endif
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "compat.h"
#include "misc.h"
#include "util.h"
#include "table.h"
#include "pmdwin.h"
#include "opnaw.h"
#include "ppz8l.h"
#include "ppsdrv.h"
#include "p86drv.h"

#ifndef WINDOWS
static void _splitpath (const char *path, char *drive, char *dir, char *fname,
						char *ext)
{
	const char *slash, *dot;

	if (*path && *(path + 1) == ':') {
		if (drive) *drive = toupper(*path);
		path += 2;
    }
	else
		if (drive) *drive = 0;

	slash = strrchr(path, '\\');
	if (!slash)
		slash = strrchr(path, '/');
	dot = strrchr(path, '.');
	if (dot && slash && dot < slash)
		// a:/home/kaoru-k/.xxx/foo �Τ褦�ʾ��
		dot = NULL;

	if (!slash) {
		if (drive) {
			if (*drive)
				if (dir) strcpy(dir, "\\");
		} else
			if (dir) strcpy(dir, "");
		if (fname) strcpy(fname, path);
		if (dot) {
			if (fname) *(fname + (dot - path)) = '\0';
			if (ext) strcpy(ext, dot + 1);
        } else
			if (ext) strcpy(ext, "");
    } else {
		if (*drive && *path != '\\' && *path != '/')
			if (dir) {
				strcpy(dir, "\\");
				strcat(dir, path);
				*(dir + (slash - path) + 1) = 0;
			}
			else
				if (dir) {
					strcpy (dir, path);
					if (slash - path == 0)
						*(dir + 1) = 0;
					else
						*(dir + (slash - path)) = 0;
				}

		if (fname) strcpy(fname, slash + 1);
		if (dot) {
			if (fname) *(fname + (dot - slash) - 1) = 0;
			if (ext) strcpy (ext, dot + 1);
		}
		else
			if (ext) strcpy (ext, "");
	}
}

static void _makepath (char *path, const char *drive, const char *dir,
				const char *fname, const char *ext)
{
  sprintf (path, "%s%s%s%s", drive, dir, fname, ext);
}
#endif

//-----------------------------------------------------------------------------
//	���󥹥ȥ饯�����ǥ��ȥ饯��
//-----------------------------------------------------------------------------
PMDWIN::PMDWIN()
{
	memset(wavbuf2, 0, sizeof(wavbuf2));
	memset(wavbuf, 0, sizeof(wavbuf2));
	memset(wavbuf_conv, 0, sizeof(wavbuf_conv));

	pos2 = (char *)wavbuf2;
	us2 = 0;
	upos = 0;

	uRefCount = 0;								// ���ȥ�����
}


PMDWIN::~PMDWIN()
{
}


//=============================================================================
//	TimerA�ν���[�ᥤ��]
//=============================================================================
void PMDWIN::TimerA_main(void)
{
	open_work.TimerAflag = 1;
	open_work.TimerAtime++;

	if((open_work.TimerAtime & 7) == 0) {
		fout();		//	Fadeout����
	}
	if(effwork.effon) {
		if(pmdwork.ppsdrv_flag == false || effwork.psgefcnum == 0x80) {
			effplay();		//		SSG���̲�����
		}
	}
	
	open_work.TimerAflag = 0;
}


//=============================================================================
//	TimerB�ν���[�ᥤ��]
//=============================================================================
void PMDWIN::TimerB_main(void)
{
	open_work.TimerBflag = 1;
	if(pmdwork.music_flag) {
		if(pmdwork.music_flag & 1) {
			mstart();	
		}

		if(pmdwork.music_flag & 2) {
			mstop();
		}
	}
	
	if(open_work.play_flag) {
		mmain();
		settempo_b();
		syousetu_count();
		pmdwork.lastTimerAtime = open_work.TimerAtime;
	}
	open_work.TimerBflag = 0;
}


//=============================================================================
//	MUSIC PLAYER MAIN [FROM TIMER-B]
//=============================================================================
void PMDWIN::mmain(void)
{
	int		i;

	pmdwork.loop_work = 3;

	if(open_work.x68_flg == 0) {
		for(i = 0; i < 3; i++) {	
			pmdwork.partb = i + 1;
			psgmain(&SSGPart[i]);
		}
	}
	
	pmdwork.fmsel = 0x100;
	for(i = 0; i < 3; i++) {
		pmdwork.partb = i + 1;
			fmmain(&FMPart[i+3]);
	}

	pmdwork.fmsel = 0;
	for(i = 0; i < 3; i++) {
		pmdwork.partb = i + 1;
		fmmain(&FMPart[i]);
	}

	for(i = 0; i < 3; i++) {
		pmdwork.partb = 3;
		fmmain(&ExtPart[i]);
	}
	
	if(open_work.x68_flg == 0) {
		rhythmmain(&RhythmPart);
		if(open_work.use_p86) {
			pcm86main(&ADPCMPart);
		} else {
			adpcmmain(&ADPCMPart);
		}
	}

	for(i = 0; i < 8; i++) {
		pmdwork.partb = i;
		ppz8main(&PPZ8Part[i]);
	}
	
	if(pmdwork.loop_work == 0) return;

	for(i = 0; i < 6; i++) {
		if(FMPart[i].loopcheck != 3) FMPart[i].loopcheck = 0;
	}

	for(i = 0; i < 3; i++) {
		if(SSGPart[i].loopcheck != 3) SSGPart[i].loopcheck = 0;
		if(ExtPart[i].loopcheck != 3) ExtPart[i].loopcheck = 0;
	}

	if(ADPCMPart.loopcheck != 3) ADPCMPart.loopcheck = 0;
	if(RhythmPart.loopcheck != 3) RhythmPart.loopcheck = 0;
	if(EffPart.loopcheck != 3) EffPart.loopcheck = 0;

	for(i = 0; i < PCM_CNL_MAX; i++) {
		if(PPZ8Part[i].loopcheck != 3) PPZ8Part[i].loopcheck = 0;
	}

	if(pmdwork.loop_work != 3) {
		open_work.status2++;
		if(open_work.status2 == 255) open_work.status2 = 1;
	} else {
		open_work.status2 = -1;
	}
}

//=============================================================================
//	�ƣͲ������եᥤ��
//=============================================================================
void PMDWIN::fmmain(QQ *qq)
{
	uchar	*si;
	
	if(qq->address == NULL) return;
	
	si = qq->address;
	
	qq->leng--;
	
	if(qq->partmask) {
		qq->keyoff_flag = -1;
	} else {
		// KEYOFF CHECK & Keyoff
		if((qq->keyoff_flag & 3) == 0) {		// ����keyoff��������
			if(qq->leng <= qq->qdat) {
				keyoff(qq);
				qq->keyoff_flag = -1;
			}
		}
	}
	
	// LENGTH CHECK
	if(qq->leng == 0) {
		if(qq->partmask == 0) qq->lfoswi &= 0xf7;
		
		while(1) {
			if(*si > 0x80 && *si != 0xda) {
				si = commands(qq, si);
			} else if(*si == 0x80) {
				qq->address = si;
				qq->loopcheck = 3;
				qq->onkai = 255;
				if(qq->partloop == NULL) {
					if(qq->partmask) {
						pmdwork.tieflag = 0;
						pmdwork.volpush_flag = 0;
						pmdwork.loop_work &= qq->loopcheck;
						return;
					} else {
						break;
					}
				}
				// "L"�����ä���
				si = qq->partloop;
				qq->loopcheck = 1;
			} else {
				if(*si == 0xda) {		// �ݥ륿����
					si = porta(qq, ++si);
					pmdwork.loop_work &= qq->loopcheck;
					return;
				} else if(qq->partmask == 0) {
					//  TONE SET
					lfoinit(qq, *si);
					fnumset(qq, oshift(qq, *si++));
					// printf("fnum = %d\n",qq->fnum);
					
					qq->leng = *si++;
					si = calc_q(qq, si);
					
					if(qq->volpush && qq->onkai != 255) {
						if(--pmdwork.volpush_flag) {
							pmdwork.volpush_flag = 0;
							qq->volpush = 0;
						}
					}
					
					volset(qq);
					otodasi(qq);
					keyon(qq);
					
					qq->keyon_flag++;
					qq->address = si;
					
					pmdwork.tieflag = 0;
					pmdwork.volpush_flag = 0;
					
					if(*si == 0xfb) {		// '&'��ľ��ˤ��ä���keyoff���ʤ�
						qq->keyoff_flag = 2;
					} else {
						qq->keyoff_flag = 0;
					}
					pmdwork.loop_work &= qq->loopcheck;
					return;
				} else {
					si++;
					qq->fnum = 0;		//���������
					qq->onkai = 255;
					qq->onkai_def = 255;
					qq->leng = *si++;
					qq->keyon_flag++;
					qq->address = si;
					
					if(--pmdwork.volpush_flag) {
						qq->volpush = 0;
					}
					
					pmdwork.tieflag = 0;
					pmdwork.volpush_flag = 0;
					break;
				}
			}
		}
	}
	
	if(qq->partmask == 0) {
	
		// LFO & Portament & Fadeout ���� �򤷤ƽ�λ
		if(qq->hldelay_c) {
			if(--qq->hldelay_c == 0) {
				opna.SetReg(pmdwork.fmsel +
						(pmdwork.partb - 1 + 0xb4), qq->fmpan);
			}
		}
		
		if(qq->sdelay_c) {
			if(--qq->sdelay_c == 0) {
				if((qq->keyoff_flag & 1) == 0) {	// ����keyoff��������
					keyon(qq);
				}
			}
		}
		
		if(qq->lfoswi) {
			pmdwork.lfo_switch = qq->lfoswi & 8;
			if(qq->lfoswi & 3) {
				if(lfo(qq)) {
					pmdwork.lfo_switch  |= (qq->lfoswi & 3);
				}
			}
			
			if(qq->lfoswi & 0x30) {
				lfo_change(qq);
				if(lfo(qq)) {
					lfo_change(qq);
					pmdwork.lfo_switch |= (qq->lfoswi & 0x30);
				} else {
					lfo_change(qq);
				}
			}
			
			if(pmdwork.lfo_switch & 0x19) {
				if(pmdwork.lfo_switch & 8) {
					porta_calc(qq);
					
				}
				otodasi(qq);
			}
			
			if(pmdwork.lfo_switch & 0x22) {
				volset(qq);
				pmdwork.loop_work &= qq->loopcheck;
				return;
			}
		}
		
		if(open_work.fadeout_speed) {
			volset(qq);
		}
	}
	
	pmdwork.loop_work &= qq->loopcheck;
}


//=============================================================================
//	KEY OFF
//=============================================================================
void PMDWIN::keyoff(QQ *qq)
{
	if(qq->onkai == 255) return;
	kof1(qq);
}


void PMDWIN::kof1(QQ *qq)
{
	if(pmdwork.fmsel == 0) {
		pmdwork.omote_key[pmdwork.partb-1]
				= (~qq->slotmask) & (pmdwork.omote_key[pmdwork.partb-1]);
		opna.SetReg(0x28, (pmdwork.partb-1) | pmdwork.omote_key[pmdwork.partb-1]);
	} else {
		pmdwork.ura_key[pmdwork.partb-1]
				= (~qq->slotmask) & (pmdwork.ura_key[pmdwork.partb-1]);
		opna.SetReg(0x28, ((pmdwork.partb-1) | pmdwork.ura_key[pmdwork.partb-1]) | 4);
	}
}


//=============================================================================
//	�ƣ͡��ˣţ٣ϣ�
//=============================================================================
void PMDWIN::keyon(QQ *qq)
{
	int	al;
	
	if(qq->onkai == 255) return;		// ���奦�� �� �ȥ�
	
	if(pmdwork.fmsel == 0) {
		al = pmdwork.omote_key[pmdwork.partb-1] | qq->slotmask;
		if(qq->sdelay_c) {
			al &= qq->sdelay_m;
		}
		
		pmdwork.omote_key[pmdwork.partb-1] = al;
		opna.SetReg(0x28, (pmdwork.partb-1) | al);
	} else {
		al = pmdwork.ura_key[pmdwork.partb-1] | qq->slotmask;
		if(qq->sdelay_c) {
			al &= qq->sdelay_m;
		}
		
		pmdwork.ura_key[pmdwork.partb-1] = al;
		opna.SetReg(0x28, ((pmdwork.partb-1) | al) | 4);
	}
}


//=============================================================================
//	Set [ FNUM/BLOCK + DETUNE + LFO ]
//=============================================================================
void PMDWIN::otodasi(QQ *qq)
{
	int		ax, cx;
	
	if(qq->fnum == 0) return;
	if(qq->slotmask == 0) return;
	
	cx = (qq->fnum & 0x3800);		// cx=BLOCK
	ax = (qq->fnum) & 0x7ff;		// ax=FNUM

	// Portament/LFO/Detune SET
	ax += qq->porta_num + qq->detune;
	
	if(pmdwork.partb == 3 && pmdwork.fmsel == 0 && open_work.ch3mode != 0x3f) {
		ch3_special(qq, ax, cx);
	} else {
		if(qq->lfoswi & 1) {
			ax += qq->lfodat;
		}
		
		if(qq->lfoswi & 0x10) {
			ax += qq->_lfodat;
		}
		
		fm_block_calc(&cx, &ax);
		
		// SET BLOCK/FNUM TO OPN
		
		ax |= cx;
		
		opna.SetReg(pmdwork.fmsel + pmdwork.partb+0xa4-1, ax >> 8);
		opna.SetReg(pmdwork.fmsel + pmdwork.partb+0xa4-5, ax & 0xff);
	}
}


//=============================================================================
//	FM������detune�ǥ��������֤��Ѥ����ν���
//		input	CX:block / AX:fnum+detune
//		output	CX:block / AX:fnum
//=============================================================================
void PMDWIN::fm_block_calc(int *cx, int *ax)
{
	while(*ax >= 0x26a) {
		if(*ax < (0x26a*2)) return;
		
		*cx += 0x800;			// oct.up
		if(*cx != 0x4000) {
			*ax -= 0x26a;		// 4d2h-26ah
		} else {				// �⥦ ���쥤���祦 ������ʥ����
			*cx = 0x3800;
			if(*ax >= 0x800) 
				*ax = 0x7ff;	// 4d2h
			return;
		}
	}
	
	while(*ax < 0x26a) {
		*cx -= 0x800;			// oct.down
		if(*cx >= 0) {
			*ax += 0x26a;		// 4d2h-26ah
		} else {				// �⥦ ���쥤���祦 ������ʥ����
			*cx = 0;
			if(*ax < 8) {
				*ax = 8;
			}
			return;
		}
	}
}


//=============================================================================
//	�ѡ��Ȥ�Ƚ�̤���ch3�ʤ�mode����
//=============================================================================
int PMDWIN::ch3_setting(QQ *qq)
{
	if(pmdwork.partb == 3 && pmdwork.fmsel == 0) {
		ch3mode_set(qq);
		return 1;
	} else {
		return 0;
	}
}


void PMDWIN::cm_clear(int *ah, int *al)
{
	*al ^= 0xff;
	if((pmdwork.slot3_flag &= *al) == 0) {
		if(pmdwork.slotdetune_flag != 1) {
			*ah = 0x3f;
		} else {
			*ah = 0x7f;
		}
	} else {
		*ah = 0x7f;
	}
}


//=============================================================================
//	FM3��mode�����ꤹ��
//=============================================================================
void PMDWIN::ch3mode_set(QQ *qq)
{
	int		ah, al;
	
	if(qq == &FMPart[3-1]) {
		al = 1;
	} else if(qq == &ExtPart[0]) {
		al = 2;
	} else if(qq == &ExtPart[1]) {
		al = 4;
	} else {
		al = 8;
	}
	
	if((qq->slotmask & 0xf0) == 0) {	// s0
		cm_clear(&ah, &al);
	} else if(qq->slotmask != 0xf0) {
		pmdwork.slot3_flag |= al;
		ah = 0x7f;
	} else if((qq->volmask & 0x0f) == 0) {
		cm_clear(&ah, &al);
	} else if((qq->lfoswi & 1) != 0) {
		pmdwork.slot3_flag |= al;
		ah = 0x7f;
	} else if((qq->_volmask & 0x0f) == 0) {
		cm_clear(&ah, &al);
	} else if(qq->lfoswi & 0x10) {
		pmdwork.slot3_flag |= al;
		ah = 0x7f;
	} else {
		cm_clear(&ah, &al);
	}
	
	if(ah == open_work.ch3mode) return;		// �������ѹ�̵���ʤ鲿�⤷�ʤ�
	open_work.ch3mode = ah;
	opna.SetReg(0x27, ah & 0xcf);			// Reset�Ϥ��ʤ�
	
	//	���̲��⡼�ɤ˰ܤä����Ϥ��������FM3�ѡ��Ȥǲ����񤭴���
	if(ah == 0x3f || qq == &FMPart[2]) return;
	
	if(FMPart[2].partmask == 0) otodasi(&FMPart[2]);
	if(qq == &ExtPart[0]) return;
	if(ExtPart[0].partmask == 0) otodasi(&ExtPart[0]);
	if(qq == &ExtPart[1]) return;
	if(ExtPart[1].partmask == 0) otodasi(&ExtPart[1]);
}


//=============================================================================
//	ch3=���̲��⡼�� ����Ѥ�����β�������
//			input CX:block AX:fnum
//=============================================================================
void PMDWIN::ch3_special(QQ *qq, int ax, int cx)
{
	int		ax_, bh, ch, si;
	int		shiftmask = 0x80;
	
	si = cx;
	
	if((qq->volmask & 0x0f) == 0) {
		bh = 0xf0;			// all
	} else {
		bh = qq->volmask;	// bh=lfo1 mask 4321xxxx
	}
	
	if((qq->_volmask & 0x0f) == 0) {
		ch = 0xf0;			// all
	} else {
		ch = qq->_volmask;	// ch=lfo2 mask 4321xxxx
	}
	
	//	slot	4
	if(qq->slotmask & 0x80) {
		ax_ = ax;
		ax += open_work.slot_detune4;
		if((bh & shiftmask) && (qq->lfoswi & 0x01))	ax += qq->lfodat;
		if((ch & shiftmask) && (qq->lfoswi & 0x10))	ax += qq->_lfodat;
		shiftmask >>= 1;
		
		cx = si;
		fm_block_calc(&cx, &ax);
		ax |= cx;
		
		opna.SetReg(0xa6, ax >> 8);
		opna.SetReg(0xa2, ax & 0xff);
		
		ax = ax_;
	}
	
	//	slot	3
	if(qq->slotmask & 0x40) {
		ax_ = ax;
		ax += open_work.slot_detune3;
		if((bh & shiftmask) && (qq->lfoswi & 0x01))	ax += qq->lfodat;
		if((ch & shiftmask) && (qq->lfoswi & 0x10))	ax += qq->_lfodat;
		shiftmask >>= 1;
		
		cx = si;
		fm_block_calc(&cx, &ax);
		ax |= cx;
		
		opna.SetReg(0xac, ax >> 8);
		opna.SetReg(0xa8, ax & 0xff);
		
		ax = ax_;
	}
	
	//	slot	2
	if(qq->slotmask & 0x20) {
		ax_ = ax;
		ax += open_work.slot_detune2;
		if((bh & shiftmask) && (qq->lfoswi & 0x01))	ax += qq->lfodat;
		if((ch & shiftmask) && (qq->lfoswi & 0x10))	ax += qq->_lfodat;
		shiftmask >>= 1;
		
		cx = si;
		fm_block_calc(&cx, &ax);
		ax |= cx;
		
		opna.SetReg(0xae, ax >> 8);
		opna.SetReg(0xaa, ax & 0xff);
		
		ax = ax_;
	}
	
	//	slot	1
	if(qq->slotmask & 0x10) {
		ax_ = ax;
		ax += open_work.slot_detune1;
		if((bh & shiftmask) && (qq->lfoswi & 0x01))	ax += qq->lfodat;
		if((ch & shiftmask) && (qq->lfoswi & 0x10))	ax += qq->_lfodat;
		cx = si;
		fm_block_calc(&cx, &ax);
		ax |= cx;
		
		opna.SetReg(0xad, ax >> 8);
		opna.SetReg(0xa9, ax & 0xff);
		
		ax = ax_;
	}
}


//=============================================================================
//	'p' COMMAND [FM PANNING SET]
//=============================================================================
uchar * PMDWIN::panset(QQ *qq, uchar *si)
{
	panset_main(qq, *si++);
	return si;
}


void PMDWIN::panset_main(QQ *qq, int al)
{
	qq->fmpan = (qq->fmpan & 0x3f) | (al << 6) & 0xc0;
	
	if(pmdwork.partb == 3 && pmdwork.fmsel == 0) {
		//	FM3�ξ��� 4�ĤΥѡ����������
		FMPart[2].fmpan = qq->fmpan;
		ExtPart[0].fmpan = qq->fmpan;
		ExtPart[1].fmpan = qq->fmpan;
		ExtPart[2].fmpan = qq->fmpan;
	}
	
	if(qq->partmask == 0) {		// �ѡ��ȥޥ�������Ƥ��뤫��
		// dl = al;
		opna.SetReg(pmdwork.fmsel + pmdwork.partb+0xb4-1,
													calc_panout(qq));
	}
}


//=============================================================================
//	0b4h�������ꤹ��ǡ�������� out.dl
//=============================================================================
uchar PMDWIN::calc_panout(QQ *qq)
{
	int	dl;
	
	dl = qq->fmpan;
	if(qq->hldelay_c) dl &= 0xc0;	// HLFO Delay���ĤäƤ���ϥѥ�Τ�����
	return dl;
}


//=============================================================================
//	Pan setting Extend
//=============================================================================
uchar * PMDWIN::panset_ex(QQ *qq, uchar *si)
{
	int		al;
	
	al = read_char(si++);
	si++;		// ����flag���ɤ����Ф�
	
	if(al > 0) {
		al = 2; 
		panset_main(qq, al);
	} else if(al == 0) {
		al = 3;
		panset_main(qq, al);
	} else {
		al = 1;
		panset_main(qq, al);
	}
	return si;
}


//=============================================================================
//	Pan setting Extend
//=============================================================================
uchar * PMDWIN::panset8_ex(QQ *qq, uchar *si)
{
	int		flag, data;
	
	qq->fmpan = (char)*si++;
	open_work.revpan = *si++;


	if(qq->fmpan == 0) {
		flag = 3;				// Center
		data = 0;
	} else if(qq->fmpan > 0) {
		flag = 2;				// Right
		data = 128 - qq->fmpan;
	} else {
		flag = 1;				// Left
		data = 128 + qq->fmpan;
	}

	if(open_work.revpan != 1) {
		flag |= 4;				// ����
	}	
	p86drv.SetPan(flag, data);
	
	return si;
}


//=============================================================================
//	�ƣͲ����ѡ�Entry
//=============================================================================
void PMDWIN::lfoinit(QQ *qq, int al)
{
	int		ah;
	
	ah = al & 0x0f;
	if(ah == 0x0c) {
		al = qq->onkai_def;
		ah = al & 0x0f;
	}

	qq->onkai_def = al;

	if(ah == 0x0f) {				// ���塼�� �� �ȥ� �� INIT ���ʥ���
		lfo_exit(qq);
		return;
	}
	
	qq->porta_num = 0;				// �ݥ륿���ȤϽ����
	
	if((pmdwork.tieflag & 1) == 0) {
		lfin1(qq);
	} else {
		lfo_exit(qq);
	}
}


//=============================================================================
//	�ƣ͡�BLOCK,F-NUMBER SET
//		INPUTS	-- AL [KEY#,0-7F]
//=============================================================================
void PMDWIN::fnumset(QQ *qq, int al)
{
	int		ax,bx;
	
	if((al & 0x0f) != 0x0f) {		// ����ξ��
		qq->onkai = al;
		
		// BLOCK/FNUM CALICULATE
		bx = al & 0x0f;		// bx=onkai
		ax = fnum_data[bx];
		
		// BLOCK SET
		ax |= (((al >> 1) & 0x38) << 8);
		qq->fnum = ax;
	} else {						// ����ξ��
		qq->onkai = 255;
		if((qq->lfoswi & 0x11) == 0) {
			qq->fnum = 0;			// ����LFO̤����
		}
	}
}


//=============================================================================
//	�ƣͲ�������ᥤ��
//=============================================================================
void PMDWIN::volset(QQ *qq)
{
	int bh, bl, cl, dh;
	uchar vol_tbl[4] = {0, 0, 0, 0};
	
	if(qq->slotmask == 0) return;
	if(qq->volpush) {
		cl = qq->volpush - 1;
	} else {
		cl = qq->volume;
	}
	
	if(qq != &EffPart) {	// ���̲��ξ���voldown/fadeout�ƶ�̵��
		//--------------------------------------------------------------------
		//	����down�׻�
		//--------------------------------------------------------------------
		if(open_work.fm_voldown) {
			cl = ((256-open_work.fm_voldown) * cl) >> 8;
		}
		
		//--------------------------------------------------------------------
		//	Fadeout�׻�
		//--------------------------------------------------------------------
		if(open_work.fadeout_volume >= 2) {
			cl = ((256-(open_work.fadeout_volume >> 1)) * cl) >> 8;
		}
	}
	
	//------------------------------------------------------------------------
	//	���̤�carrier������ & ����LFO����
	//		input	cl to Volume[0-127]
	//			bl to SlotMask
	//------------------------------------------------------------------------
	
	bh=0;					// Vol Slot Mask
	bl=qq->slotmask;		// ch=SlotMask Push

	vol_tbl[0] = 0x80;
	vol_tbl[1] = 0x80;
	vol_tbl[2] = 0x80;
	vol_tbl[3] = 0x80;
	
	cl = 255-cl;			// cl=carrier�����ꤹ�벻��+80H(add)
	bl &= qq->carrier;		// bl=���̤����ꤹ��SLOT xxxx0000b
	bh |= bl;
	
	if(bl & 0x80) vol_tbl[0] = cl;
	if(bl & 0x40) vol_tbl[1] = cl;
	if(bl & 0x20) vol_tbl[2] = cl;
	if(bl & 0x10) vol_tbl[3] = cl;
	
	if(cl != 255) {
		if(qq->lfoswi & 2) {
			bl = qq->volmask;
			bl &= qq->slotmask;		// bl=����LFO�����ꤹ��SLOT xxxx0000b
			bh |= bl;
			fmlfo_sub(qq, qq->lfodat, bl, vol_tbl);
		}
		
		if(qq->lfoswi & 0x20) {
			bl = qq->volmask;
			bl &= qq->slotmask;
			bh |= bl;
			fmlfo_sub(qq, qq->_lfodat, bl, vol_tbl);
		}
	}
	
	dh = 0x4c - 1 + pmdwork.partb;		// dh=FM Port Address
	
	if(bh & 0x80) volset_slot(dh,    qq->slot4, vol_tbl[0]);
	if(bh & 0x40) volset_slot(dh-8,  qq->slot3, vol_tbl[1]);
	if(bh & 0x20) volset_slot(dh-4,  qq->slot2, vol_tbl[2]);
	if(bh & 0x10) volset_slot(dh-12, qq->slot1, vol_tbl[3]);
}


//-----------------------------------------------------------------------------
//	����å���η׻� & ���� �ޥ���
//			in.	dl	����TL��
//				dh	Out����쥸����
//				al	������ư�� �濴=80h
//-----------------------------------------------------------------------------
void PMDWIN::volset_slot(int dh, int dl, int al)
{
	if((al += dl) > 255) al = 255;
	if((al -= 0x80) < 0) al = 0;
	opna.SetReg(pmdwork.fmsel + dh, al);
}


//-----------------------------------------------------------------------------
//	����LFO�ѥ���
//-----------------------------------------------------------------------------
void PMDWIN::fmlfo_sub(QQ *qq, int al, int bl, uchar *vol_tbl)
{
	if(bl & 0x80) vol_tbl[0] = Limit(vol_tbl[0] - al, 255, 0);
	if(bl & 0x40) vol_tbl[1] = Limit(vol_tbl[1] - al, 255, 0);
	if(bl & 0x20) vol_tbl[2] = Limit(vol_tbl[2] - al, 255, 0);
	if(bl & 0x10) vol_tbl[3] = Limit(vol_tbl[3] - al, 255, 0);
}


//=============================================================================
//	�ӣӣǲ��������ա��ᥤ��
//=============================================================================
void PMDWIN::psgmain(QQ *qq)
{
	uchar	*si;
	int		temp;

	if(qq->address == NULL) return;
	si = qq->address;
	
	qq->leng--;

	// KEYOFF CHECK & Keyoff

	if(qq == &SSGPart[2] && pmdwork.ppsdrv_flag &&
					open_work.kshot_dat && qq->leng <= qq->qdat) {
					// PPS ���ѻ� & SSG 3ch & SSG ���̲��Ĥ餷�Ƥ�����
		keyoffp(qq);
		opna.SetReg(pmdwork.partb+8-1, 0);		// ����Ū�˲���ߤ��
		qq->keyoff_flag = -1;			
	}
	
	if(qq->partmask) {
		qq->keyoff_flag = -1;
	} else {
		if((qq->keyoff_flag & 3) == 0) {		// ����keyoff��������
			if(qq->leng <= qq->qdat) {
				keyoffp(qq);
				qq->keyoff_flag = -1;
			}
		}
	}
	
	// LENGTH CHECK
	if(qq->leng == 0) {
		qq->lfoswi &= 0xf7;
		
		// DATA READ
		while(1) {
			if(*si == 0xda && ssgdrum_check(qq, *si) < 0) {
				si++;
			} else if(*si > 0x80 && *si != 0xda) {
				si = commandsp(qq, si);
			} else if(*si == 0x80) {
				qq->address = si;
				qq->loopcheck = 3;
				qq->onkai = 255;
				if(qq->partloop == NULL) {
					if(qq->partmask) {
						pmdwork.tieflag = 0;
						pmdwork.volpush_flag = 0;
						pmdwork.loop_work &= qq->loopcheck;
						return;
					} else {
						break;
					}
				}
				// "L"�����ä���
				si = qq->partloop;
				qq->loopcheck = 1;
			} else {
				if(*si == 0xda) {						// �ݥ륿����
					si = portap(qq, ++si);
					pmdwork.loop_work &= qq->loopcheck;
					return;
				} else if(qq->partmask) {
					if(ssgdrum_check(qq, *si) == 0) {
						si++;
						qq->fnum = 0;		//���������
						qq->onkai = 255;
						qq->leng = *si++;
						qq->keyon_flag++;
						qq->address = si;
						
						if(--pmdwork.volpush_flag) {
							qq->volpush = 0;
						}
						
						pmdwork.tieflag = 0;
						pmdwork.volpush_flag = 0;
						break;
					}
				}
				
				//  TONE SET
				lfoinitp(qq, *si);
				fnumsetp(qq, oshiftp(qq, *si++));
				
				qq->leng = *si++;
				si = calc_q(qq, si);
				
				if(qq->volpush && qq->onkai != 255) {
					if(--pmdwork.volpush_flag) {
						pmdwork.volpush_flag = 0;
						qq->volpush = 0;
					}
				}
				
				volsetp(qq);
				otodasip(qq);
				keyonp(qq);
				
				qq->keyon_flag++;
				qq->address = si;
				
				pmdwork.tieflag = 0;
				pmdwork.volpush_flag = 0;
				qq->keyoff_flag = 0;
				
				if(*si == 0xfb) {		// '&'��ľ��ˤ��ä���keyoff���ʤ�
					qq->keyoff_flag = 2;
				}
				pmdwork.loop_work &= qq->loopcheck;
				return;
			}
		}
	}
	
	pmdwork.lfo_switch = (qq->lfoswi & 8);
	if(qq->lfoswi) {
		if(qq->lfoswi & 3) {
			if(lfop(qq)) {
				pmdwork.lfo_switch |= (qq->lfoswi & 3);
			}
		}

		if(qq->lfoswi & 0x30) {
			lfo_change(qq);
			if(lfop(qq)) {
				lfo_change(qq);
				pmdwork.lfo_switch |= (qq->lfoswi & 0x30);
			} else {
				lfo_change(qq);
			}
		}
		
		if(pmdwork.lfo_switch & 0x19) {
			if(pmdwork.lfo_switch & 0x08) {
				porta_calc(qq);
			}
			otodasip(qq);
		}
	}
	
	temp = soft_env(qq);
	if(temp || pmdwork.lfo_switch & 0x22 || open_work.fadeout_speed) {
		volsetp(qq);
	}
	
	pmdwork.loop_work &= qq->loopcheck;
}


void PMDWIN::keyoffp(QQ *qq)
{
	if(qq->onkai == 255) return;		// ���奦�� �� �ȥ�
	if(qq->envf != -1) {
		qq->envf = 2;
	} else {
		qq->eenv_count = 4;
	}
}


//=============================================================================
//	�ꥺ��ѡ��ȡ����ա��ᥤ��
//=============================================================================
void PMDWIN::rhythmmain(QQ *qq)
{
	uchar	*si, *bx;
	int		al, result;
	ushort addr;
	
	if(qq->address == NULL) return;
	
	si = qq->address;
	
	if(--qq->leng == 0) {
		bx = open_work.rhyadr;
		
		rhyms00:
		do {
			result = 1;
			al = *bx++;
			if(al != 0xff) {
				if(al & 0x80) {
					bx = rhythmon(qq, bx, al, &result);
					if(result == 0) continue;
				} else {
					open_work.kshot_dat = 0;	//rest
				}
				
				al = *bx++;
				open_work.rhyadr = bx;
				qq->leng = al;
				qq->keyon_flag++;
				pmdwork.tieflag = 0;
				pmdwork.volpush_flag = 0;
				pmdwork.loop_work &= qq->loopcheck;
				return;
			}
		}while(result == 0);
		
		while(1) {
			while((al = *si++) != 0x80) {
				if(al < 0x80) {
					qq->address = si;

//					bx = (ushort *)&open_work.radtbl[al];
//					bx = open_work.rhyadr = &open_work.mmlbuf[*bx];
					addr = read_word(&open_work.radtbl[al]);
					bx = open_work.rhyadr = &open_work.mmlbuf[addr];
					goto rhyms00;
				}
				
				// al > 0x80
				si = commandsr(qq, si-1);
			}
			
			qq->address = --si;
			qq->loopcheck = 3;
			bx = qq->partloop;
			if(bx == NULL) {
				open_work.rhyadr = (uchar *)&pmdwork.rhydmy;
				pmdwork.tieflag = 0;
				pmdwork.volpush_flag = 0;
				pmdwork.loop_work &= qq->loopcheck;
				return;
			} else {		//  "L"�����ä���
				si = bx;
				qq->loopcheck = 1;
			}
		}
	}
	
	pmdwork.loop_work &= qq->loopcheck;
}


//=============================================================================
//	PSG�ꥺ�� ON
//=============================================================================
uchar * PMDWIN::rhythmon(QQ *qq, uchar *bx, int al, int *result)
{
	int		cl, dl, bx_;
	
	if(al & 0x40) {
		bx = commandsr(qq, bx-1);
		*result = 0;
		return bx;
	}
	
	*result = 1;
	
	if(qq->partmask) {		// mask����Ƥ�����
		open_work.kshot_dat = 0;
		return ++bx;
	}
	
	al = ((al << 8) + *bx++) & 0x3fff;
	open_work.kshot_dat = al;
	if(al == 0) return bx;
	open_work.rhyadr = bx;
	
	if(open_work.kp_rhythm_flag) {
		for(cl = 0; cl < 11; cl++) {
			if(al & (1 << cl)) {
				opna.SetReg(rhydat[cl][0], rhydat[cl][1]);
				if((dl = (rhydat[cl][2] & open_work.rhythmmask))) { // no check yet
					if(dl < 0x80) {
						opna.SetReg(0x10, dl);
					} else {
						opna.SetReg(0x10, 0x84);
						dl = open_work.rhythmmask & 8;
						if(dl) {
							opna.SetReg(0x10, dl);
						}
					}
				}
			}
		}
	}
	
	if(open_work.fadeout_volume) {
		if(open_work.kp_rhythm_flag) {
			dl = open_work.rhyvol;
			if(open_work.fadeout_volume) {
				dl = ((256 - open_work.fadeout_volume) * dl) >> 8;
			}
			opna.SetReg(0x11, dl);
		}
		if(pmdwork.ppsdrv_flag == false) {	// fadeout��ppsdrv�Ǥʤ�ȯ�����ʤ�
			bx = open_work.rhyadr;
			return bx;
		}
	}
	
	bx_ = al;
	al = 0;
	do {
		while((bx_ & 1) == 0) {
			bx_ >>= 1;
			al++;
		}
		effgo(qq, al);
		bx_ >>= 1;
	}while(pmdwork.ppsdrv_flag && bx_);	// PPSDRV�ʤ飲���ܰʾ���Ĥ餷�Ƥߤ�
	
	return open_work.rhyadr;
}


//=============================================================================
//	�Уӣǡ��ɥ�ॹ�����̲����롼����
//	�ƣ����ףԣ�����
//
//	AL �� ���̲��Σ������ơ��ã��̣̤���
//	ppsdrv������ʤ餽�ä����Ĥ餹
//=============================================================================
void PMDWIN::effgo(QQ *qq, int al)
{
	if(pmdwork.ppsdrv_flag) {		// PPS ���Ĥ餹
		al |= 0x80;
		if(effwork.last_shot_data == al) {
			ppsdrv.Stop();
		} else {
			effwork.last_shot_data = al;
		}
	}
	
	effwork.hosei_flag = 3;				//	����/������������ (K part) 
	eff_main(qq, al);
}


void PMDWIN::eff_on2(QQ *qq, int al)
{
	effwork.hosei_flag = 1;				//	�����Τ��������� (n command)
	eff_main(qq, al);
}


void PMDWIN::eff_main(QQ *qq, int al)
{
	int		ah, bh, bl;
	
	if(open_work.effflag) return;		//	���̲�����Ѥ��ʤ��⡼��

	if(pmdwork.ppsdrv_flag && (al & 0x80)) {	// PPS ���Ĥ餹
		if(effwork.effon >= 2) return;	// �̾���̲�ȯ������ȯ�������ʤ�
		
		SSGPart[2].partmask |= 2;		// Part Mask
		effwork.effon = 1;				// ͥ���٣�(ppsdrv)
		effwork.psgefcnum = al;			// �����ֹ����� (80H��)
		
		bh = 0;
		bl = 15;
		ah = effwork.hosei_flag;
		if(ah & 1) {
			bh = qq->detune % 256;		// BH = Detune�β��� 8bit
		}
		
		if(ah & 2) {
			if(qq->volume < 15) {
				bl = qq->volume;		// BL = volume�� (0��15)
			}
			
			if(open_work.fadeout_volume) {
				bl = (bl * (256 - open_work.fadeout_volume)) >> 8;
			}
		}
		
		if(bl) {
			bl ^= 0x0f;
			ah = 1;
			al &= 0x7f;
			ppsdrv.Play(al, bh, bl); 
		}
	} else {
		effwork.psgefcnum = al;
		
		if(effwork.effon <= efftbl[al].priority) {
			if(pmdwork.ppsdrv_flag) {
				ppsdrv.Stop();
			}
			
			SSGPart[2].partmask |= 2;		// Part Mask
			efffor(efftbl[al].table);		// ��ȯ�ܤ�ȯ��
			effwork.effon = efftbl[al].priority;
												//	ͥ���̤�����(ȯ������)
		}
	}
}


//=============================================================================
//	���������󡡤��󤽤����ᤤ��
// 	�ƣ����֣ңԣ�
//=============================================================================
void PMDWIN::effplay(void)
{
	if(--effwork.effcnt) {
		effsweep();			// ���������åȤ���ʤ�
	} else {
		efffor(effwork.effadr);
	}
}


void PMDWIN::efffor(const int *si)
{
	int		al, ch, cl;
	
	al = *si++;
	if(al == -1) {
		effend();
	} else {
		effwork.effcnt = al;		// ������ȿ�
		cl = *si;
		opna.SetReg(4, *si++);		// ���ȿ����å�
		ch = *si;
		opna.SetReg(5, *si++);		// ���ȿ����å�
		effwork.eswthz = (ch << 8) + cl;
		
		open_work.psnoi_last = effwork.eswnhz = *si;
		opna.SetReg(6, *si++);		// �Υ���
		
		opna.SetReg(7, ((*si++ << 2) & 0x24) | (opna.GetReg(0x07) & 0xdb));
		
		opna.SetReg(10, *si++);		// �ܥ�塼��
		opna.SetReg(11, *si++);		// ����٥��׼��ȿ�
		opna.SetReg(12, *si++);		// 
		opna.SetReg(13, *si++);		// ����٥���PATTERN
		
		effwork.eswtst = *si++;		// ����������ʬ (TONE)
		effwork.eswnst = *si++;		// ����������ʬ (NOISE)
		
		effwork.eswnct = effwork.eswnst & 15;		// �������ץ������ (NOISE)
		
		effwork.effadr = (int *)si;
	}
}


void PMDWIN::effend(void)
{
	if(pmdwork.ppsdrv_flag) {
		ppsdrv.Stop();
	}
	opna.SetReg(0x0a, 0x00);
	opna.SetReg(0x07,  ((opna.GetReg(0x07)) & 0xdb) | 0x24);
	effwork.effon = 0;
	effwork.psgefcnum = -1;
}


// ���ʤν���
void PMDWIN::effsweep(void)
{
	int		dl;
	
	effwork.eswthz += effwork.eswtst;
	opna.SetReg(4, effwork.eswthz & 0xff);
	opna.SetReg(5, effwork.eswthz >> 8);
	
	if(effwork.eswnst == 0) return;		// �Υ�����������̵��
	if(--effwork.eswnct) return;
	
	dl = effwork.eswnst;
	effwork.eswnct = dl & 15;
	
	effwork.eswnhz += dl / 16;
	
	opna.SetReg(6, effwork.eswnhz);
	open_work.psnoi_last = effwork.eswnhz;
}


//=============================================================================
//	PDR��switch
//=============================================================================
uchar * PMDWIN::pdrswitch(QQ *qq, uchar *si)
{
	if(pmdwork.ppsdrv_flag == false) return si+1;
	
//	ppsdrv.SetParam((*si & 1) << 1, *si & 1);		@����
	si++;
	return si;
}

//=============================================================================
//	�УãͲ��������ա��ᥤ��
//=============================================================================
void PMDWIN::adpcmmain(QQ *qq)
{
	uchar	*si;
	int		temp;
	
	if(qq->address == NULL) return;
	si = qq->address;
	
	qq->leng--;
	if(qq->partmask) {
		qq->keyoff_flag = -1;
	} else {
		// KEYOFF CHECK
		if((qq->keyoff_flag & 3) == 0) {		// ����keyoff��������
			if(qq->leng <= qq->qdat) {
				keyoffm(qq);
				qq->keyoff_flag = -1;
			}
		}
	}
	
	// LENGTH CHECK
	if(qq->leng == 0) {
		qq->lfoswi &= 0xf7;
		
		while(1) {
			if(*si > 0x80 && *si != 0xda) {
				si = commandsm(qq, si);
			} else if(*si == 0x80) {
				qq->address = si;
				qq->loopcheck = 3;
				qq->onkai = 255;
				if(qq->partloop == NULL) {
					if(qq->partmask) {
						pmdwork.tieflag = 0;
						pmdwork.volpush_flag = 0;
						pmdwork.loop_work &= qq->loopcheck;
						return;
					} else {
						break;
					}
				}
				// "L"�����ä���
				si = qq->partloop;
				qq->loopcheck = 1;
			} else {
				if(*si == 0xda) {				// �ݥ륿����
					si = portam(qq, ++si);
					pmdwork.loop_work &= qq->loopcheck;
					return;
				} else if(qq->partmask) {
					si++;
					qq->fnum = 0;		//���������
					qq->onkai = 255;
//					qq->onkai_def = 255;
					qq->leng = *si++;
					qq->keyon_flag++;
					qq->address = si;
					
					if(--pmdwork.volpush_flag) {
						qq->volpush = 0;
					}
					
					pmdwork.tieflag = 0;
					pmdwork.volpush_flag = 0;
					break;
				}

				//  TONE SET
				lfoinitp(qq, *si);
				fnumsetm(qq, oshift(qq, *si++));
				
				qq->leng = *si++;
				si = calc_q(qq, si);
				
				if(qq->volpush && qq->onkai != 255) {
					if(--pmdwork.volpush_flag) {
						pmdwork.volpush_flag = 0;
						qq->volpush = 0;
					}
				}
				
				volsetm(qq);
				otodasim(qq);
				if(qq->keyoff_flag & 1) {
					keyonm(qq);
				}
				
				qq->keyon_flag++;
				qq->address = si;
				
				pmdwork.tieflag = 0;
				pmdwork.volpush_flag = 0;
				
				if(*si == 0xfb) {		// '&'��ľ��ˤ��ä���keyoff���ʤ�
					qq->keyoff_flag = 2;
				} else {
					qq->keyoff_flag = 0;
				}
				pmdwork.loop_work &= qq->loopcheck;
				return;
				
			}
		}
	}
	
	pmdwork.lfo_switch = (qq->lfoswi & 8);
	if(qq->lfoswi) {
		if(qq->lfoswi & 3) {
			if(lfo(qq)) {
				pmdwork.lfo_switch |= (qq->lfoswi & 3);
			}
		}
		
		if(qq->lfoswi & 0x30) {
			lfo_change(qq);
			if(lfop(qq)) {
				lfo_change(qq);
				pmdwork.lfo_switch |= (qq->lfoswi & 0x30);
			} else {
				lfo_change(qq);
			}
		}
		
		if(pmdwork.lfo_switch & 0x19) {
			if(pmdwork.lfo_switch & 0x08) {
				porta_calc(qq);
			}
			otodasim(qq);
		}
	}
	
	temp = soft_env(qq);
	if(temp || pmdwork.lfo_switch & 0x22 || open_work.fadeout_speed) {
		volsetm(qq);
	}
	
	pmdwork.loop_work &= qq->loopcheck;
}


//=============================================================================
//	�УãͲ��������ա��ᥤ��(PMD86)
//=============================================================================
void PMDWIN::pcm86main(QQ *qq)
{
	uchar	*si;
	int		temp;
	
	if(qq->address == NULL) return;
	si = qq->address;
	
	qq->leng--;
	if(qq->partmask) {
		qq->keyoff_flag = -1;
	} else {
		// KEYOFF CHECK
		if((qq->keyoff_flag & 3) == 0) {		// ����keyoff��������
			if(qq->leng <= qq->qdat) {
				keyoff8(qq);
				qq->keyoff_flag = -1;
			}
		}
	}
	
	// LENGTH CHECK
	if(qq->leng == 0) {
		while(1) {
//			if(*si > 0x80 && *si != 0xda) {
			if(*si > 0x80) {
				si = commands8(qq, si);
			} else if(*si == 0x80) {
				qq->address = si;
				qq->loopcheck = 3;
				qq->onkai = 255;
				if(qq->partloop == NULL) {
					if(qq->partmask) {
						pmdwork.tieflag = 0;
						pmdwork.volpush_flag = 0;
						pmdwork.loop_work &= qq->loopcheck;
						return;
					} else {
						break;
					}
				}
				// "L"�����ä���
				si = qq->partloop;
				qq->loopcheck = 1;
			} else {
				if(qq->partmask) {
					si++;
					qq->fnum = 0;		//���������
					qq->onkai = 255;
//					qq->onkai_def = 255;
					qq->leng = *si++;
					qq->keyon_flag++;
					qq->address = si;
					
					if(--pmdwork.volpush_flag) {
						qq->volpush = 0;
					}
					
					pmdwork.tieflag = 0;
					pmdwork.volpush_flag = 0;
					break;
				}

				//  TONE SET
				lfoinitp(qq, *si);
				fnumset8(qq, oshift(qq, *si++));
				
				qq->leng = *si++;
				si = calc_q(qq, si);
				
				if(qq->volpush && qq->onkai != 255) {
					if(--pmdwork.volpush_flag) {
						pmdwork.volpush_flag = 0;
						qq->volpush = 0;
					}
				}
				
				volset8(qq);
				otodasi8(qq);
				if(qq->keyoff_flag & 1) {
					keyon8(qq);
				}
				
				qq->keyon_flag++;
				qq->address = si;
				
				pmdwork.tieflag = 0;
				pmdwork.volpush_flag = 0;
				
				if(*si == 0xfb) {		// '&'��ľ��ˤ��ä���keyoff���ʤ�
					qq->keyoff_flag = 2;
				} else {
					qq->keyoff_flag = 0;
				}
				pmdwork.loop_work &= qq->loopcheck;
				return;
				
			}
		}
	}
	
	if(qq->lfoswi & 0x22) {
		pmdwork.lfo_switch = 0;
		if(qq->lfoswi & 2) {
			lfo(qq);
			pmdwork.lfo_switch |= (qq->lfoswi & 2);
		}
		
		if(qq->lfoswi & 0x20) {
			lfo_change(qq);
			if(lfo(qq)) {
				lfo_change(qq);
				pmdwork.lfo_switch |= (qq->lfoswi & 0x20);
			} else {
				lfo_change(qq);
			}
		}
		
		temp = soft_env(qq);
		if(temp || pmdwork.lfo_switch & 0x22 || open_work.fadeout_speed) {
			volset8(qq);
		}
	} else {
		temp = soft_env(qq);
		if(temp || open_work.fadeout_speed) {
			volset8(qq);
		}
	}
	
	pmdwork.loop_work &= qq->loopcheck;
}


//=============================================================================
//	�УãͲ��������ա��ᥤ�� [PPZ8]
//=============================================================================
void PMDWIN::ppz8main(QQ *qq)
{
	uchar	*si;
	int		temp;
	
	if(qq->address == NULL) return;
	si = qq->address;
	
	qq->leng--;
	if(qq->partmask) {
		qq->keyoff_flag = -1;
	} else {
		// KEYOFF CHECK
		if((qq->keyoff_flag & 3) == 0) {		// ����keyoff��������
			if(qq->leng <= qq->qdat) {
				keyoffz(qq);
				qq->keyoff_flag = -1;
			}
		}
	}
	
	// LENGTH CHECK
	if(qq->leng == 0) {
		qq->lfoswi &= 0xf7;
		
		while(1) {
			if(*si > 0x80 && *si != 0xda) {
				si = commandsz(qq, si);
			} else if(*si == 0x80) {
				qq->address = si;
				qq->loopcheck = 3;
				qq->onkai = 255;
				if(qq->partloop == NULL) {
					if(qq->partmask) {
						pmdwork.tieflag = 0;
						pmdwork.volpush_flag = 0;
						pmdwork.loop_work &= qq->loopcheck;
						return;
					} else {
						break;
					}
				}
				// "L"�����ä���
				si = qq->partloop;
				qq->loopcheck = 1;
			} else {
				if(*si == 0xda) {				// �ݥ륿����
					si = portaz(qq, ++si);
					pmdwork.loop_work &= qq->loopcheck;
					return;
				} else if(qq->partmask) {
					si++;
					qq->fnum = 0;		//���������
					qq->onkai = 255;
//					qq->onkai_def = 255;
					qq->leng = *si++;
					qq->keyon_flag++;
					qq->address = si;
					
					if(--pmdwork.volpush_flag) {
						qq->volpush = 0;
					}
					
					pmdwork.tieflag = 0;
					pmdwork.volpush_flag = 0;
					break;
				}

				//  TONE SET
				lfoinitp(qq, *si);
				fnumsetz(qq, oshift(qq, *si++));
				
				qq->leng = *si++;
				si = calc_q(qq, si);
				
				if(qq->volpush && qq->onkai != 255) {
					if(--pmdwork.volpush_flag) {
						pmdwork.volpush_flag = 0;
						qq->volpush = 0;
					}
				}
				
				volsetz(qq);
				otodasiz(qq);
				if(qq->keyoff_flag & 1) {
					keyonz(qq);
				}
				
				qq->keyon_flag++;
				qq->address = si;
				
				pmdwork.tieflag = 0;
				pmdwork.volpush_flag = 0;
				
				if(*si == 0xfb) {		// '&'��ľ��ˤ��ä���keyoff���ʤ�
					qq->keyoff_flag = 2;
				} else {
					qq->keyoff_flag = 0;
				}
				pmdwork.loop_work &= qq->loopcheck;
				return;
				
			}
		}
	}
	
	pmdwork.lfo_switch = (qq->lfoswi & 8);
	if(qq->lfoswi) {
		if(qq->lfoswi & 3) {
			if(lfo(qq)) {
				pmdwork.lfo_switch |= (qq->lfoswi & 3);
			}
		}
		
		if(qq->lfoswi & 0x30) {
			lfo_change(qq);
			if(lfop(qq)) {
				lfo_change(qq);
				pmdwork.lfo_switch |= (qq->lfoswi & 0x30);
			} else {
				lfo_change(qq);
			}
		}
		
		if(pmdwork.lfo_switch & 0x19) {
			if(pmdwork.lfo_switch & 0x08) {
				porta_calc(qq);
			}
			otodasiz(qq);
		}
	}
	
	temp = soft_env(qq);
	if(temp || pmdwork.lfo_switch & 0x22 || open_work.fadeout_speed) {
		volsetz(qq);
	}
	
	pmdwork.loop_work &= qq->loopcheck;
}


//=============================================================================
//	PCM KEYON
//=============================================================================
void PMDWIN::keyonm(QQ *qq)
{
	if(qq->onkai == 255) return;
	
	opna.SetReg(0x101, 0x02);	// PAN=0 / x8 bit mode
	opna.SetReg(0x100, 0x21);	// PCM RESET
	
	opna.SetReg(0x102, open_work.pcmstart & 0xff);
	opna.SetReg(0x103, open_work.pcmstart >> 8);
	opna.SetReg(0x104, open_work.pcmstop & 0xff);
	opna.SetReg(0x105, open_work.pcmstop >> 8);
	
	if((pmdwork.pcmrepeat1 | pmdwork.pcmrepeat2) == 0) {
		opna.SetReg(0x100, 0xa0);	// PCM PLAY(non_repeat)
		opna.SetReg(0x101, qq->fmpan | 2);	// PAN SET / x8 bit mode
	} else {
		opna.SetReg(0x100, 0xb0);	// PCM PLAY(repeat)
		opna.SetReg(0x101, qq->fmpan | 2);	// PAN SET / x8 bit mode
		opna.SetReg(0x102, pmdwork.pcmrepeat1 & 0xff);
		opna.SetReg(0x103, pmdwork.pcmrepeat1 >> 8);
		opna.SetReg(0x104, pmdwork.pcmrepeat2 & 0xff);
		opna.SetReg(0x105, pmdwork.pcmrepeat2 >> 8);
	}
}


//=============================================================================
//	PCM KEYON(PMD86)
//=============================================================================
void PMDWIN::keyon8(QQ *qq)
{
	if(qq->onkai == 255) return;
	p86drv.Play();
}


//=============================================================================
//	PPZ KEYON
//=============================================================================
void PMDWIN::keyonz(QQ *qq)
{
	if(qq->onkai == 255) return;
	
	if((qq->voicenum & 0x80) == 0) {
		ppz8.Play(pmdwork.partb, 0, qq->voicenum);
	} else {
		ppz8.Play(pmdwork.partb, 1, qq->voicenum & 0x7f);
	}
}


//=============================================================================
//	PCM OTODASI
//=============================================================================
void PMDWIN::otodasim(QQ *qq)
{
	int		bx, dx;
	
	if((bx = qq->fnum) == 0) return;
	
	// Portament/LFO/Detune SET
	
	bx += qq->porta_num;
	
	if((qq->lfoswi & 0x11) && (qq->lfoswi & 1)) {
		dx = qq->lfodat;
	} else {
		dx = 0;
	}
	
	if(qq->lfoswi & 0x10) {
		dx += qq->_lfodat;
	}
	dx *= 4;	// PCM �� LFO �� ������˥��� �Υ� depth �� 4�Х� ����
	
	dx += qq->detune;
	if(dx >= 0) {
		bx += dx;
		if(bx > 0xffff) bx = 0xffff;
	} else {
		bx += dx;
		if(bx < 0) bx = 0;
	}
	
	// TONE SET
	
	opna.SetReg(0x109, bx & 0xff);
	opna.SetReg(0x10a, bx >> 8);
}


//=============================================================================
//	PCM OTODASI(PMD86)
//=============================================================================
void PMDWIN::otodasi8(QQ *qq)
{
	int		bl, cx;

	if(qq->fnum == 0) return;
	
	bl = (qq->fnum & 0x0e00000) >> (16+5);		// ������ȿ�
	cx = qq->fnum & 0x01fffff;					// fnum

	if(open_work.pcm86_vol == 0 && qq->detune) {
		cx = Limit((cx >> 5) + qq->detune, 65535, 1) << 5;
	}

	p86drv.SetOntei(bl, cx);
}


//=============================================================================
//	PPZ OTODASI
//=============================================================================
void PMDWIN::otodasiz(QQ *qq)
{
	uint	cx;
	_int64	cx2;
	int		ax;
	
	if((cx = qq->fnum) == 0) return;
	cx += qq->porta_num * 16;
	
	if(qq->lfoswi & 1) {
		ax = qq->lfodat;
	} else {
		ax = 0;
	}
	
	if(qq->lfoswi & 0x10) {
		ax += qq->_lfodat;
	}
	
	ax += qq->detune;

	cx2 = cx + ((_int64)cx) / 256 * ax;
	if(cx2 > 0xffffffff) cx = 0xffffffff;
	else if(cx2 < 0) cx = 0;
	else cx = (uint)cx2;
	
	// TONE SET
	ppz8.SetOntei(pmdwork.partb, cx);
}


//=============================================================================
//	PCM VOLUME SET
//=============================================================================
void PMDWIN::volsetm(QQ *qq)
{
	int		ah, al, dx;
	
	if(qq->volpush) {
		al = qq->volpush;
	} else {
		al = qq->volume;
	}
	
	//------------------------------------------------------------------------
	//	����down�׻�
	//------------------------------------------------------------------------
	al = ((256-open_work.pcm_voldown) * al) >> 8;
	
	//------------------------------------------------------------------------
	//	Fadeout�׻�
	//------------------------------------------------------------------------
	if(open_work.fadeout_volume) {
		al = (((256 - open_work.fadeout_volume) * (256 - open_work.fadeout_volume) >> 8) * al) >> 8;
	}

	//------------------------------------------------------------------------
	//	ENVELOPE �׻�
	//------------------------------------------------------------------------
	if(al == 0) {
		opna.SetReg(0x10b, 0);
		return;
	}
	
	if(qq->envf == -1) {
		//	��ĥ�� ����=al*(eenv_vol+1)/16
		if(qq->eenv_volume == 0) {
			opna.SetReg(0x10b, 0);
			return;
		}
		
		al = ((((al * (qq->eenv_volume + 1))) >> 3) + 1) >> 1;
	} else {
		if(qq->eenv_volume < 0) {
			ah = -qq->eenv_volume * 16;
			if(al < ah) {
				opna.SetReg(0x10b, 0);
				return;
			} else {
				al -= ah;
			}
		} else {
			ah = qq->eenv_volume * 16;
			if(al + ah > 255) {
				al = 255;
			} else {
				al += ah;
			}
		}
	}
	
	//--------------------------------------------------------------------
	//	����LFO�׻�
	//--------------------------------------------------------------------
	
	if((qq->lfoswi & 0x22) == 0) {
		opna.SetReg(0x10b, al);
		return;
	}
	
	if(qq->lfoswi & 2) {
		dx = qq->lfodat;
	} else {
		dx = 0;
	}
	
	if(qq->lfoswi & 0x20) {
		dx += qq->_lfodat;
	}
	
	if(dx >= 0) {
		al += dx;
		if(al & 0xff00) {
			opna.SetReg(0x10b, 255);
		} else {
			opna.SetReg(0x10b, al);
		}
	} else {
		al += dx;
		if(al < 0) {
			opna.SetReg(0x10b, 0);
		} else {
			opna.SetReg(0x10b, al);
		}
	}
}


//=============================================================================
//	PCM VOLUME SET(PMD86)
//=============================================================================
void PMDWIN::volset8(QQ *qq)
{
	int		ah, al, dx;
	
	if(qq->volpush) {
		al = qq->volpush;
	} else {
		al = qq->volume;
	}
	
	//------------------------------------------------------------------------
	//	����down�׻�
	//------------------------------------------------------------------------
	al = ((256-open_work.pcm_voldown) * al) >> 8;
	
	//------------------------------------------------------------------------
	//	Fadeout�׻�
	//------------------------------------------------------------------------
	if(open_work.fadeout_volume) {
		al = ((256 - open_work.fadeout_volume) * al) >> 8;
	}

	//------------------------------------------------------------------------
	//	ENVELOPE �׻�
	//------------------------------------------------------------------------
	if(al == 0) {
		opna.SetReg(0x10b, 0);
		return;
	}
	
	if(qq->envf == -1) {
		//	��ĥ�� ����=al*(eenv_vol+1)/16
		if(qq->eenv_volume == 0) {
			opna.SetReg(0x10b, 0);
			return;
		}
		
		al = ((((al * (qq->eenv_volume + 1))) >> 3) + 1) >> 1;
	} else {
		if(qq->eenv_volume < 0) {
			ah = -qq->eenv_volume * 16;
			if(al < ah) {
				opna.SetReg(0x10b, 0);
				return;
			} else {
				al -= ah;
			}
		} else {
			ah = qq->eenv_volume * 16;
			if(al + ah > 255) {
				al = 255;
			} else {
				al += ah;
			}
		}
	}
	
	//--------------------------------------------------------------------
	//	����LFO�׻�
	//--------------------------------------------------------------------
	
	if(qq->lfoswi & 2) {
		dx = qq->lfodat;
	} else {
		dx = 0;
	}
	
	if(qq->lfoswi & 0x20) {
		dx += qq->_lfodat;
	}
	
	if(dx >= 0) {
		if((al += dx) > 255) al = 255;
	} else {
		if((al += dx) < 0) al = 0;
	}
	
	if(open_work.pcm86_vol) {
		//	SPB��Ʊ�ͤβ�������
		al = (int)sqrt(al);
	} else {
		al >>= 4;
	}
	
	p86drv.SetVol(al);
}


//=============================================================================
//	PPZ VOLUME SET
//=============================================================================
void PMDWIN::volsetz(QQ *qq)
{
	int		ah, al, dx;
	
	if(qq->volpush) {
		al = qq->volpush;
	} else {
		al = qq->volume;
	}
	
	//------------------------------------------------------------------------
	//	����down�׻�
	//------------------------------------------------------------------------
	al = ((256-open_work.ppz_voldown) * al) >> 8;
	
	//------------------------------------------------------------------------
	//	Fadeout�׻�
	//------------------------------------------------------------------------
	if(open_work.fadeout_volume) {
		al = ((256 - open_work.fadeout_volume) * al) >> 8;
	}

	//------------------------------------------------------------------------
	//	ENVELOPE �׻�
	//------------------------------------------------------------------------
	if(al == 0) {
//*@
		ppz8.SetVol(pmdwork.partb, 0);
		ppz8.Stop(pmdwork.partb);
		return;
	}
	
	if(qq->envf == -1) {
		//	��ĥ�� ����=al*(eenv_vol+1)/16
		if(qq->eenv_volume == 0) {
//*@		ppz8.SetVol(pmdwork.partb, 0);
			ppz8.Stop(pmdwork.partb);
			return;
		}
		
		al = ((((al * (qq->eenv_volume + 1))) >> 3) + 1) >> 1;
	} else {
		if(qq->eenv_volume < 0) {
			ah = -qq->eenv_volume * 16;
			if(al < ah) {
//*@			ppz8.SetVol(pmdwork.partb, 0);
				ppz8.Stop(pmdwork.partb);
				return;
			} else {
				al -= ah;
			}
		} else {
			ah = qq->eenv_volume * 16;
			if(al + ah > 255) {
				al = 255;
			} else {
				al += ah;
			}
		}
	}
	
	//--------------------------------------------------------------------
	//	����LFO�׻�
	//--------------------------------------------------------------------
	
	if((qq->lfoswi & 0x22)) {
		if(qq->lfoswi & 2) {
			dx = qq->lfodat;
		} else {
			dx = 0;
		}
		
		if(qq->lfoswi & 0x20) {
			dx += qq->_lfodat;
		}
		
		al += dx;
		if(dx >= 0) {
			if(al & 0xff00) {
				al = 255;
			}
		} else {
			if(al < 0) {
				al = 0;
			}
		}
	}
	
	if(al) {
		ppz8.SetVol(pmdwork.partb, al >> 4);
	} else {
		ppz8.Stop(pmdwork.partb);
	}
//*@
/*
	ppz8.SetVol(pmdwork.partb, al >> 4);

	if(al <= 0) {
		ppz8.Stop(pmdwork.partb);
	}
*/	
}


//=============================================================================
//	ADPCM FNUM SET
//=============================================================================
void PMDWIN::fnumsetm(QQ *qq, int al)
{
	int		ax, bx, ch, cl;
	
	if((al & 0x0f) != 0x0f) {			// ����ξ��
		qq->onkai = al;
		
		bx = al & 0x0f;					// bx=onkai
		ch = cl = (al >> 4) & 0x0f;		// cl = octarb
		
		if(cl > 5) {
			cl = 0;
		} else {
			cl = 5 - cl;				// cl=5-octarb
		}
		
		ax = pcm_tune_data[bx];
		if(ch >= 6) {					// o7�ʾ�?
			ch = 0x50;
			if(ax < 0x8000) {
				ax *= 2;				// o7�ʾ��2�ܤǤ������2��
				ch = 0x60;
			}
			qq->onkai = (qq->onkai & 0x0f) | ch;	// onkai�ͽ���
		} else {
			ax >>= cl;					// ax=ax/[2^OCTARB]
		}
		qq->fnum = ax;
	} else {						// ����ξ��
		qq->onkai = 255;
		if((qq->lfoswi & 0x11) == 0) {
			qq->fnum = 0;			// ����LFO̤����
		}
	}
}


//=============================================================================
//	PCM FNUM SET(PMD86)
//=============================================================================
void PMDWIN::fnumset8(QQ *qq, int al)
{
	int		ah, bl;
	
	ah = al & 0x0f;
	if(ah != 0x0f) {			// ����ξ��
		if(open_work.pcm86_vol && al >= 0x65) {		// o7e?
			if(ah < 5) {
				al = 0x60;		// o7
			} else {
				al = 0x50;		// o6
			}
			al |= ah;
		}

		qq->onkai = al;
		bl = ((al & 0xf0) >> 4) * 12 + ah;
		qq->fnum = p86_tune_data[bl];
	} else {						// ����ξ��
		qq->onkai = 255;
		if((qq->lfoswi & 0x11) == 0) {
			qq->fnum = 0;			// ����LFO̤����
		}
	}
}


//=============================================================================
//	PPZ FNUM SET
//=============================================================================
void PMDWIN::fnumsetz(QQ *qq, int al)
{
	uint	ax;
	int		bx, cl;
	
	
	if((al & 0x0f) != 0x0f) {			// ����ξ��
		qq->onkai = al;
		
		bx = al & 0x0f;					// bx=onkai
		cl = (al >> 4) & 0x0f;		// cl = octarb
		
		ax = ppz_tune_data[bx];
		
		if((cl -= 4) < 0) {
			cl = -cl;
			ax >>= cl;
		} else {
			ax <<= cl;
		}
		qq->fnum = ax;
	} else {						// ����ξ��
		qq->onkai = 255;
		if((qq->lfoswi & 0x11) == 0) {
			qq->fnum = 0;			// ����LFO̤����
		}
	}
}

//=============================================================================
//	�ݥ륿����(PCM)
//=============================================================================
uchar * PMDWIN::portam(QQ *qq, uchar *si)
{
	int		ax, al_, bx_;
	
	if(qq->partmask) {
		qq->fnum = 0;		//���������
		qq->onkai = 255;
		qq->leng = *(si+2);
		qq->keyon_flag++;
		qq->address = si+3;
				
		if(--pmdwork.volpush_flag) {
			qq->volpush = 0;
		}
				
		pmdwork.tieflag = 0;
		pmdwork.volpush_flag = 0;
		pmdwork.loop_work &= qq->loopcheck;
		return si+3;		// �ɤ����Ф�	(Mask��)
	}
	
	lfoinitp(qq, *si);
	fnumsetm(qq, oshift(qq, *si++));
	
	bx_ = qq->fnum;
	al_ = qq->onkai;
	fnumsetm(qq, oshift(qq, *si++));
	ax = qq->fnum; 			// ax = �ݥ륿�������delta_n��
	
	qq->onkai = al_;
	qq->fnum = bx_;			// bx = �ݥ륿���ȸ���dekta_n��
	ax -= bx_;				// ax = delta_n��
	
	qq->leng = *si++;
	si = calc_q(qq, si);
	
	qq->porta_num2 = ax / qq->leng;		// ��
	qq->porta_num3 = ax % qq->leng;		// ;��
	qq->lfoswi |= 8;				// Porta ON
	
	if(qq->volpush && qq->onkai != 255) {
		if(--pmdwork.volpush_flag) {
			pmdwork.volpush_flag = 0;
			qq->volpush = 0;
		}
	}
	
	volsetm(qq);
	otodasim(qq);
	if(qq->keyoff_flag & 1) {
		keyonm(qq);
	}
	
	qq->keyon_flag++;
	qq->address = si;
	
	pmdwork.tieflag = 0;
	pmdwork.volpush_flag = 0;
	qq->keyoff_flag = 0;
	
	if(*si == 0xfb) {			// '&'��ľ��ˤ��ä���keyoff���ʤ�
		qq->keyoff_flag = 2;
	}
	
	pmdwork.loop_work &= qq->loopcheck;
	return si;
}


//=============================================================================
//	�ݥ륿����(PPZ)
//=============================================================================
uchar * PMDWIN::portaz(QQ *qq, uchar *si)
{
	int		ax, al_, bx_;
	
	if(qq->partmask) {
		qq->fnum = 0;		//���������
		qq->onkai = 255;
		qq->leng = *(si+2);
		qq->keyon_flag++;
		qq->address = si+3;
				
		if(--pmdwork.volpush_flag) {
			qq->volpush = 0;
		}
				
		pmdwork.tieflag = 0;
		pmdwork.volpush_flag = 0;
		pmdwork.loop_work &= qq->loopcheck;
		return si+3;		// �ɤ����Ф�	(Mask��)
	}
	
	lfoinitp(qq, *si);
	fnumsetz(qq, oshift(qq, *si++));
	
	bx_ = qq->fnum;
	al_ = qq->onkai;
	
	fnumsetz(qq, oshift(qq, *si++));
	ax = qq->fnum; 			// ax = �ݥ륿�������delta_n��
	
	qq->onkai = al_;
	qq->fnum = bx_;			// bx = �ݥ륿���ȸ���dekta_n��
	ax -= bx_;				// ax = delta_n��
	ax /= 16;
	
	qq->leng = *si++;
	si = calc_q(qq, si);
	
	qq->porta_num2 = ax / qq->leng;		// ��
	qq->porta_num3 = ax % qq->leng;		// ;��
	qq->lfoswi |= 8;				// Porta ON
	
	if(qq->volpush && qq->onkai != 255) {
		if(--pmdwork.volpush_flag) {
			pmdwork.volpush_flag = 0;
			qq->volpush = 0;
		}
	}
	
	volsetz(qq);
	otodasiz(qq);
	if(qq->keyoff_flag & 1) {
		keyonz(qq);
	}
	
	qq->keyon_flag++;
	qq->address = si;
	
	pmdwork.tieflag = 0;
	pmdwork.volpush_flag = 0;
	qq->keyoff_flag = 0;
	
	if(*si == 0xfb) {			// '&'��ľ��ˤ��ä���keyoff���ʤ�
		qq->keyoff_flag = 2;
	}
	
	pmdwork.loop_work &= qq->loopcheck;
	return si;
}


void PMDWIN::keyoffm(QQ *qq)
{
	if(qq->envf != -1)  {
		if(qq->envf == 2) return;
	} else {
		if(qq->eenv_count == 4) return;
	}
	
	if(pmdwork.pcmrelease != 0x8000) {
		// PCM RESET
		opna.SetReg(0x100, 0x21);
		
		opna.SetReg(0x102, pmdwork.pcmrelease & 0xff);
		opna.SetReg(0x103, pmdwork.pcmrelease >> 8);
		
		// Stop ADDRESS for Release
		opna.SetReg(0x104, open_work.pcmstop & 0xff);	
		opna.SetReg(0x105, open_work.pcmstop >> 8);
		
		// PCM PLAY(non_repeat)
		opna.SetReg(0x100, 0xa0);
	}
	
	keyoffp(qq);
	return;
}


void PMDWIN::keyoff8(QQ *qq)
{
	p86drv.Keyoff();

	if(qq->envf != -1) {
		if(qq->envf != 2) {
			keyoffp(qq);
		}
		return;
	}

	if(qq->eenv_count != 4) {
		keyoffp(qq);
	}
	return;
}


//=============================================================================
//	ppz KEYOFF
//=============================================================================
void PMDWIN::keyoffz(QQ *qq)
{
	if(qq->envf != -1)  {
		if(qq->envf == 2) return;
	} else {
		if(qq->eenv_count == 4) return;
	}
	
	keyoffp(qq);
	return;
}


//=============================================================================
//	Pan setting Extend
//=============================================================================
uchar * PMDWIN::pansetm_ex(QQ *qq, uchar * si)
{
	if(*si == 0) {
		qq->fmpan = 0xc0;
	} else if(*si < 0x80) {
		qq->fmpan = 0x80;
	} else {
		qq->fmpan = 0x40;
	}
	
	return si+2;	// ����flag���ɤ����Ф�
}


//=============================================================================
//	��ԡ�������
//=============================================================================
uchar * PMDWIN::pcmrepeat_set(QQ *qq, uchar * si)
{
	int		ax;
	
	ax = read_short(si);
	si += 2;
	
	if(ax >= 0) {
		ax += open_work.pcmstart;
	} else {
		ax += open_work.pcmstop;
	}
	
	pmdwork.pcmrepeat1 = ax;
	
	ax = read_short(si);
	si += 2;
	if(ax > 0) {
		ax += open_work.pcmstart;
	} else {
		ax += open_work.pcmstop;
	}
	
	pmdwork.pcmrepeat2 = ax;
	
	ax = read_word(si);
	si += 2;
	if(ax < 0x8000) {
		ax += open_work.pcmstart;
	} else if(ax > 0x8000) {
		ax += open_work.pcmstop;
	}

	pmdwork.pcmrelease = ax;
	return si;
}


//=============================================================================
//	��ԡ�������(PMD86)
//=============================================================================
uchar * PMDWIN::pcmrepeat_set8(QQ *qq, uchar * si)
{
	short loop_start, loop_end, release_start;

	loop_start = read_short(si);
	si += 2;
	loop_end = read_short(si);
	si += 2;
	release_start = read_short(si);

	if(open_work.pcm86_vol) {
		p86drv.SetLoop(loop_start, loop_end, release_start, true);
	} else {
		p86drv.SetLoop(loop_start, loop_end, release_start, false);
	}

	return si + 2;
}


//=============================================================================
//	��ԡ�������
//=============================================================================
uchar * PMDWIN::ppzrepeat_set(QQ *qq, uchar * si)
{
	int		ax, ax2;
	
	if((qq->voicenum & 0x80) == 0) {
		ax = read_short(si);
		si += 2;
		if(ax < 0) {
			ax = (int)ppz8.PCME_WORK[0].pcmnum[qq->voicenum].size - ax;
		}
		
		ax2 = read_short(si);
		si += 2;
		if(ax2 < 0) {
			ax2 = (int)ppz8.PCME_WORK[0].pcmnum[qq->voicenum].size - ax;
		}
	} else {
		ax = read_short(si);
		si += 2;
		if(ax < 0) {
			ax = (int)ppz8.PCME_WORK[1].pcmnum[qq->voicenum&0x7f].size - ax;
		}
		
		ax2 = read_short(si);
		si += 2;
		if(ax2 < 0) {
			ax2 = (int)ppz8.PCME_WORK[1].pcmnum[qq->voicenum&0x7f].size - ax2;
		}
	}
	
	ppz8.SetLoop(pmdwork.partb, ax, ax2);
	return si+2;
}


uchar * PMDWIN::vol_one_up_pcm(QQ *qq, uchar *si)
{
	int		al;
	
	al = (int)*si++ + qq->volume;
	if(al > 254) al = 254;
	al++;
	qq->volpush = al;
	pmdwork.volpush_flag = 1;
	return si;
}


//=============================================================================
//	COMMAND 'p' [Panning Set]
//=============================================================================
uchar * PMDWIN::pansetm(QQ *qq, uchar *si)
{
		qq->fmpan = (*si << 6) & 0xc0;
	return si+1;
}


//=============================================================================
//	COMMAND 'p' [Panning Set]
//	p0		����
//	p1		��
//	p2		��
//	p3		��
//=============================================================================
uchar * PMDWIN::panset8(QQ *qq, uchar *si)
{
	int		flag, data;

	data = 0;
		
	switch(*si++) {
		case 1 :					// Right
			flag = 2;
			data = 1;
			break;

		case 2 :					// Left
			flag = 1;
			data = 0;
			break;

		case 3 :					// Center
			flag = 3;
			data = 0;
			break;

		default :					// ����
			flag = 3 | 4;
			data = 0;

	}
	p86drv.SetPan(flag, data);
	return si;
}


//=============================================================================
//	COMMAND 'p' [Panning Set]
//		0=0	̵��
//		1=9	��
//		2=1	��
//		3=5	���
//=============================================================================
uchar * PMDWIN::pansetz(QQ *qq, uchar *si)
{
	qq->fmpan = ppzpandata[*si++];
	ppz8.SetPan(pmdwork.partb, qq->fmpan);
	return si;
}


//=============================================================================
//	Pan setting Extend
//		px -4��+4
//=============================================================================
uchar * PMDWIN::pansetz_ex(QQ *qq, uchar *si)
{
	int		al;
	
	al = read_char(si++);
	si++;		// ����flag���ɤ����Ф�
	
	if(al >= 5) {
		al = 4;
	} else if(al < -4) {
		al = -4;
	}
	
	qq->fmpan = al + 5;
	ppz8.SetPan(pmdwork.partb, qq->fmpan);
	return si;
}


uchar * PMDWIN::comatm(QQ *qq, uchar *si)
{
	qq->voicenum = *si++;
	open_work.pcmstart = pcmends.pcmadrs[qq->voicenum][0];
	open_work.pcmstop  = pcmends.pcmadrs[qq->voicenum][1];
	pmdwork.pcmrepeat1 = 0;
	pmdwork.pcmrepeat2 = 0;
	pmdwork.pcmrelease = 0x8000;
	return si;
}


uchar * PMDWIN::comat8(QQ *qq, uchar *si)
{
	qq->voicenum = *si++;
	p86drv.SetNeiro(qq->voicenum);
	return si;
}


uchar *PMDWIN::comatz(QQ *qq, uchar *si)
{
	qq->voicenum = *si++;
	
	if((qq->voicenum & 0x80) == 0) {
		ppz8.SetLoop(pmdwork.partb, 
				(uint)ppz8.PCME_WORK[0].pcmnum[qq->voicenum].loop_start,
				(uint)ppz8.PCME_WORK[0].pcmnum[qq->voicenum].loop_end);
		ppz8.SetSourceRate(pmdwork.partb, 
				ppz8.PCME_WORK[0].pcmnum[qq->voicenum].rate);
	} else {
		ppz8.SetLoop(pmdwork.partb, 
				(uint)ppz8.PCME_WORK[1].pcmnum[qq->voicenum & 0x7f].loop_start,
				(uint)ppz8.PCME_WORK[1].pcmnum[qq->voicenum & 0x7f].loop_end);
		ppz8.SetSourceRate(pmdwork.partb, 
				ppz8.PCME_WORK[1].pcmnum[qq->voicenum & 0x7f].rate);
	}
	return si;
}


//=============================================================================
//	SSG�ɥ���ä���SSG�����褵���뤫�ɤ���check
//		input	AL <- Command
//		output	cy=1 : ���褵����
//=============================================================================
int PMDWIN::ssgdrum_check(QQ *qq, int al)
{
	// SSG�ޥ�����ϥɥ���ߤ�ʤ�
	// SSG�ɥ����ĤäƤʤ�
	if((qq->partmask & 1) || ((qq->partmask & 2) == 0)) return 0;
	
	// ���̤θ��̲��Ͼä��ʤ�
	if(effwork.effon >= 2) return 0;
	
	al = (al & 0x0f);
	
	// ����λ��ϥɥ��ϻߤ�ʤ�
	if(al == 0x0f) return 0;
	
	// SSG�ɥ��Ϥޤ������椫��
	if(effwork.effon == 1) {
		effend();			// SSG�ɥ���ä�
	}
	
	if((qq->partmask &= 0xfd) == 0) return -1;
	return 0;
}


//=============================================================================
//	�Ƽ��ü쥳�ޥ�ɽ���
//=============================================================================
uchar * PMDWIN::commands(QQ *qq, uchar *si)
{
	int		al;
	
	al = *si++;
	switch(al) {
		case 0xff : si = comat(qq, si); break;
		case 0xfe : qq->qdata = *si++; qq->qdat3 = 0; break;
		case 0xfd : qq->volume = *si++; break;
		case 0xfc : si = comt(si); break;
		
		case 0xfb : pmdwork.tieflag |= 1; break;
		case 0xfa : qq->detune = read_short(si); si+=2; break;
		case 0xf9 : si = comstloop(qq, si); break;
		case 0xf8 : si = comedloop(qq, si); break;
		case 0xf7 : si = comexloop(qq, si); break;
		case 0xf6 : qq->partloop = si; break;
		case 0xf5 : qq->shift = read_char(si++); break;
		case 0xf4 : if((qq->volume+=4) > 127) qq->volume = 127; break;
		case 0xf3 : if(qq->volume < 4) qq->volume=0; else qq->volume-=4; break;
		case 0xf2 : si = lfoset(qq, si); break;
		case 0xf1 : si = lfoswitch(qq, si); ch3_setting(qq); break;
		case 0xf0 : si+=4; break;
		
		case 0xef : opna.SetReg(pmdwork.fmsel + *si, *(si+1)); si+=2; break;
		case 0xee : si++; break;
		case 0xed : si++; break;
		case 0xec : si = panset(qq, si); break;				// FOR SB2
		case 0xeb : si = rhykey(si); break;
		case 0xea : si = rhyvs(si); break;
		case 0xe9 : si = rpnset(si); break;
		case 0xe8 : si = rmsvs(si); break;
		//
		case 0xe7 : qq->shift += read_char(si++); break;
		case 0xe6 : si = rmsvs_sft(si); break;
		case 0xe5 : si = rhyvs_sft(si); break;
		//
		case 0xe4 : qq->hldelay = *si++; break;
		//�ɲ� for V2.3
		case 0xe3 : if((qq->volume += *si++) > 127) qq->volume = 127; break;
		case 0xe2 : 
			if(qq->volume < *si) qq->volume = 0; else qq->volume -= *si;
			si++;
			break;
		//
		case 0xe1 : si = hlfo_set(qq, si); break;
		case 0xe0 : open_work.port22h = *si; opna.SetReg(0x22, *si++); break;
		//
		case 0xdf : open_work.syousetu_lng = *si++; break;
		//
		case 0xde : si = vol_one_up_fm(qq, si); break;
		case 0xdd : si = vol_one_down(qq, si); break;
		//
		case 0xdc : open_work.status = *si++; break;
		case 0xdb : open_work.status += *si++; break;
		//
		case 0xda : si = porta(qq, si); break;
		//
		case 0xd9 : si++; break;
		case 0xd8 : si++; break;
		case 0xd7 : si++; break;
		//
		case 0xd6 : qq->mdspd = qq->mdspd2 = *si++; qq->mdepth = read_char(si++); break;
		case 0xd5 : qq->detune += read_short(si); si+=2; break;
		//
		case 0xd4 : si = ssg_efct_set(qq, si); break;
		case 0xd3 : si = fm_efct_set(qq, si); break;
		case 0xd2 :
			open_work.fadeout_flag = 1;
			open_work.fadeout_speed = *si++;
			break;
		//
		case 0xd1 : si++; break;
		case 0xd0 : si++; break;
		//
		case 0xcf : si = slotmask_set(qq, si); break;
		case 0xce : si+=6; break;
		case 0xcd : si+=5; break;
		case 0xcc : si++; break;
		case 0xcb : qq->lfo_wave = *si++; break;
		case 0xca :
			qq->extendmode = (qq->extendmode & 0xfd) | ((*si++ & 1) << 1);
			break;
		
		case 0xc9 : si++; break;
		case 0xc8 : si = slotdetune_set(qq, si); break;
		case 0xc7 : si = slotdetune_set2(qq, si); break;
		case 0xc6 : si = fm3_extpartset(qq, si); break;
		case 0xc5 : si = volmask_set(qq, si); break;
		case 0xc4 : qq->qdatb = *si++; break;
		case 0xc3 : si = panset_ex(qq, si); break;
		case 0xc2 : qq->delay = qq->delay2 = *si++; lfoinit_main(qq); break;
		case 0xc1 : break;
		case 0xc0 : si = fm_mml_part_mask(qq, si); break;
		case 0xbf : lfo_change(qq); si = lfoset(qq, si); lfo_change(qq);break;
		case 0xbe : si = _lfoswitch(qq, si); ch3_setting(qq); break;
		case 0xbd :
			lfo_change(qq);
			qq->mdspd = qq->mdspd2 = *si++;
			qq->mdepth = read_char(si++);
			lfo_change(qq);
			break;
		
		case 0xbc : lfo_change(qq); qq->lfo_wave=*si++; lfo_change(qq); break;
		case 0xbb :
			lfo_change(qq);
			qq->extendmode = (qq->extendmode & 0xfd) | ((*si++ & 1) << 1);
			lfo_change(qq);
			break;
		
		case 0xba : si = _volmask_set(qq, si); break;
		case 0xb9 :
			lfo_change(qq);
			qq->delay = qq->delay2 = *si++; lfoinit_main(qq);
			lfo_change(qq);
			break;
			
		case 0xb8 : si = tl_set(qq, si); break;
		case 0xb7 : si = mdepth_count(qq, si); break;
		case 0xb6 : si = fb_set(qq, si); break;
		case 0xb5 : 
			qq->sdelay_m = (~(*si++) << 4) & 0xf0;
			qq->sdelay_c = qq->sdelay = *si++;
			break;
		
		case 0xb4 : si+=16; break;
		case 0xb3 : qq->qdat2 = *si++; break;
		case 0xb2 : qq->shift_def = read_char(si++); break;
		case 0xb1 : qq->qdat3 = *si++; break;
		
		default : 
			si--;
			*si = 0x80;
	}
	
	return si;
}


uchar * PMDWIN::commandsp(QQ *qq, uchar *si)
{
	int		al;
	
	al = *si++;
	switch(al) {
		case 0xff : si++; break;
		case 0xfe : qq->qdata = *si++; qq->qdat3 = 0; break;
		case 0xfd : qq->volume = *si++; break;
		case 0xfc : si = comt(si); break;
		
		case 0xfb : pmdwork.tieflag |= 1; break;
		case 0xfa : qq->detune = read_short(si); si+=2; break;
		case 0xf9 : si = comstloop(qq, si); break;
		case 0xf8 : si = comedloop(qq, si); break;
		case 0xf7 : si = comexloop(qq, si); break;
		case 0xf6 : qq->partloop = si; break;
		case 0xf5 : qq->shift = read_char(si++); break;
		case 0xf4 : if(qq->volume < 15) qq->volume++; break;
		case 0xf3 : if(qq->volume > 0) qq->volume--; break;
		case 0xf2 : si = lfoset(qq, si); break;
		case 0xf1 : si = lfoswitch(qq, si); break;
		case 0xf0 : si = psgenvset(qq, si); break;
		
		case 0xef : opna.SetReg(*si, *(si+1)); si+=2; break;
		case 0xee : open_work.psnoi = *si++; break;
		case 0xed : qq->psgpat = *si++; break;
		//
		case 0xec : si++; break;
		case 0xeb : si = rhykey(si); break;
		case 0xea : si = rhyvs(si); break;
		case 0xe9 : si = rpnset(si); break;
		case 0xe8 : si = rmsvs(si); break;
		//
		case 0xe7 : qq->shift += read_char(si++); break;
		case 0xe6 : si = rmsvs_sft(si); break;
		case 0xe5 : si = rhyvs_sft(si); break;
		//
		case 0xe4 : si++; break;
		//�ɲ� for V2.3
		case 0xe3 : if((qq->volume + *si) < 16) qq->volume += *si; si++;break;
		case 0xe2 : if((qq->volume - *si) >= 0) qq->volume -= *si; si++;break;
		//
		case 0xe1 : si++; break;
		case 0xe0 : si++; break;
		//
		case 0xdf : open_work.syousetu_lng = *si++; break;
		//
		case 0xde : si = vol_one_up_psg(qq, si); break;
		case 0xdd : si = vol_one_down(qq, si); break;
		//
		case 0xdc : open_work.status = *si++; break;
		case 0xdb : open_work.status += *si++; break;
		//
		case 0xda : si = portap(qq, si); break;
		//
		case 0xd9 : si++; break;
		case 0xd8 : si++; break;
		case 0xd7 : si++; break;
		//
		case 0xd6 : qq->mdspd = qq->mdspd2 = *si++; qq->mdepth = read_char(si++); break;
		case 0xd5 : qq->detune += read_short(si); si+=2; break;
		//
		case 0xd4 : si = ssg_efct_set(qq, si); break;
		case 0xd3 : si = fm_efct_set(qq, si); break;
		case 0xd2 :
			open_work.fadeout_flag = 1;
			open_work.fadeout_speed = *si++;
			break;
		//
		case 0xd1 : si++; break;
		case 0xd0 : si = psgnoise_move(si); break;
		//
		case 0xcf : si++; break;
		case 0xce : si+=6; break;
		case 0xcd : si = extend_psgenvset(qq, si); break;
		case 0xcc :
			qq->extendmode = (qq->extendmode & 0xfe) | (*si++ & 1);
			break;
		
		case 0xcb : qq->lfo_wave = *si++; break;
		case 0xca :
			qq->extendmode = (qq->extendmode & 0xfd) | ((*si++ & 1) << 1);
			break;
		
		case 0xc9 :
			qq->extendmode = (qq->extendmode & 0xfb) | ((*si++ & 1) << 2);
			break;
		
		case 0xc8 : si+=3; break;
		case 0xc7 : si+=3; break;
		case 0xc6 : si+=6; break;
		case 0xc5 : si++; break;
		case 0xc4 : qq->qdatb = *si++; break;
		case 0xc3 : si+=2; break;
		case 0xc2 : qq->delay = qq->delay2 = *si++; lfoinit_main(qq); break;
		case 0xc1 : break;
		case 0xc0 : si = ssg_mml_part_mask(qq, si); break;
		case 0xbf : lfo_change(qq); si = lfoset(qq, si); lfo_change(qq);break;
		case 0xbe : 
			qq->lfoswi = (qq->lfoswi & 0x8f) | ((*si++ & 7) << 4);
			lfo_change(qq); lfoinit_main(qq); lfo_change(qq);
			break;
		
		case 0xbd :
			lfo_change(qq);
			qq->mdspd = qq->mdspd2 = *si++;
			qq->mdepth = read_char(si++);
			lfo_change(qq);
			break;
		
		case 0xbc : lfo_change(qq); qq->lfo_wave=*si++; lfo_change(qq); break;
		case 0xbb :
			lfo_change(qq);
			qq->extendmode = (qq->extendmode & 0xfd) | ((*si++ & 1) << 1);
			lfo_change(qq);
			break;
		
		case 0xba : si++; break;
		case 0xb9 :
			lfo_change(qq);
			qq->delay = qq->delay2 = *si++; lfoinit_main(qq); break;
			lfo_change(qq);
			break;
			
		case 0xb8 : si+=2; break;
		case 0xb7 : si = mdepth_count(qq, si); break;
		case 0xb6 : si++; break;
		case 0xb5 : si+= 2; break;
		case 0xb4 : si+=16; break;
		case 0xb3 : qq->qdat2 = *si++; break;
		case 0xb2 : qq->shift_def = read_char(si++); break;
		case 0xb1 : qq->qdat3 = *si++; break;
	
		default : 
			si--;
			*si = 0x80;
	}
	
	return si;
}


uchar * PMDWIN::commandsr(QQ *qq, uchar *si)
{
	int		al;
	
	al = *si++;
	switch(al) {
		case 0xff : si++; break;
		case 0xfe : si++; break;
		case 0xfd : qq->volume = *si++; break;
		case 0xfc : si = comt(si); break;
		
		case 0xfb : pmdwork.tieflag |= 1; break;
		case 0xfa : qq->detune = read_short(si); si+=2; break;
		case 0xf9 : si = comstloop(qq, si); break;
		case 0xf8 : si = comedloop(qq, si); break;
		case 0xf7 : si = comexloop(qq, si); break;
		case 0xf6 : qq->partloop = si; break;
		case 0xf5 : si++; break;
		case 0xf4 : if(qq->volume < 15) qq->volume++; break;
		case 0xf3 : if(qq->volume > 0) qq->volume--; break;
		case 0xf2 : si+=4; break;
		case 0xf1 : si = pdrswitch(qq, si); break;
		case 0xf0 : si+=4; break;
		
		case 0xef : opna.SetReg(*si, *(si+1)); si+=2; break;
		case 0xee : si++; break;
		case 0xed : si++; break;
		//
		case 0xec : si++; break;
		case 0xeb : si = rhykey(si); break;
		case 0xea : si = rhyvs(si); break;
		case 0xe9 : si = rpnset(si); break;
		case 0xe8 : si = rmsvs(si); break;
		//
		case 0xe7 : si++; break;
		case 0xe6 : si = rmsvs_sft(si); break;
		case 0xe5 : si = rhyvs_sft(si); break;
		//
		case 0xe4 : si++; break;
		//�ɲ� for V2.3
		case 0xe3 : if((qq->volume + *si) < 16) qq->volume += *si; si++;break;
		case 0xe2 : if((qq->volume - *si) >= 0) qq->volume -= *si; si++;break;
		//
		case 0xe1 : si++; break;
		case 0xe0 : si++; break;
		//
		case 0xdf : open_work.syousetu_lng = *si++; break;
		//
		case 0xde : si = vol_one_up_psg(qq, si); break;
		case 0xdd : si = vol_one_down(qq, si); break;
		//
		case 0xdc : open_work.status = *si++; break;
		case 0xdb : open_work.status += *si++; break;
		//
		case 0xda : si++; break;
		//
		case 0xd9 : si++; break;
		case 0xd8 : si++; break;
		case 0xd7 : si++; break;
		//
		case 0xd6 : si+=2; break;
		case 0xd5 : qq->detune += read_short(si); si+=2; break;
		//
		case 0xd4 : si = ssg_efct_set(qq, si); break;
		case 0xd3 : si = fm_efct_set(qq, si); break;
		case 0xd2 :
			open_work.fadeout_flag = 1;
			open_work.fadeout_speed = *si++;
			break;
		//
		case 0xd1 : si++; break;
		case 0xd0 : si++; break;
		//
		case 0xcf : si++; break;
		case 0xce : si+=6; break;
		case 0xcd : si+=5; break;
		case 0xcc : si++; break;
		case 0xcb : si++; break;
		case 0xca : si++; break;
		case 0xc9 : si++; break;
		case 0xc8 : si+=3; break;
		case 0xc7 : si+=3; break;
		case 0xc6 : si+=6; break;
		case 0xc5 : si++; break;
		case 0xc4 : si++; break;
		case 0xc3 : si+=2; break;
		case 0xc2 : si++; break;
		case 0xc1 : break;
		case 0xc0 : si = rhythm_mml_part_mask(qq, si); break;
		case 0xbf : si+=4; break;
		case 0xbe : si++; break;
		case 0xbd : si+=2; break;
		case 0xbc : si++; break;
		case 0xbb : si++; break;
		case 0xba : si++; break;
		case 0xb9 : si++; break;
		case 0xb8 : si+=2; break;
		case 0xb7 : si++; break;
		case 0xb6 : si++; break;
		case 0xb5 : si+= 2; break;
		case 0xb4 : si+=16; break;
		case 0xb3 : si++; break;
		case 0xb2 : si++; break;
		case 0xb1 : si++; break;

		default : 
			si--;
			*si = 0x80;
	}
	
	return si;
}


uchar * PMDWIN::commandsm(QQ *qq, uchar *si)
{
	int		al;
	
	al = *si++;
	switch(al) {
		case 0xff : si = comatm(qq, si); break;
		case 0xfe : qq->qdata = *si++; break;
		case 0xfd : qq->volume = *si++; break;
		case 0xfc : si = comt(si); break;
		case 0xfb : pmdwork.tieflag |= 1; break;
		case 0xfa : qq->detune = read_short(si); si+=2; break;
		case 0xf9 : si = comstloop(qq, si); break;
		case 0xf8 : si = comedloop(qq, si); break;
		case 0xf7 : si = comexloop(qq, si); break;
		case 0xf6 : qq->partloop = si; break;
		case 0xf5 : qq->shift = read_char(si++); break;
		case 0xf4 : if((qq->volume+=16) < qq->volume) qq->volume = 255; break;
		case 0xf3 : if(qq->volume < 16) qq->volume=0; else qq->volume-=16; break;
		case 0xf2 : si = lfoset(qq, si); break;
		case 0xf1 : si = lfoswitch(qq, si); break;
		case 0xf0 : si = psgenvset(qq, si); break;
		
		case 0xef : opna.SetReg(0x100 + *si, *(si+1)); si+=2; break;
		case 0xee : si++; break;
		case 0xed : si++; break;
		case 0xec : si = pansetm(qq, si); break;				// FOR SB2
		case 0xeb : si = rhykey(si); break;
		case 0xea : si = rhyvs(si); break;
		case 0xe9 : si = rpnset(si); break;
		case 0xe8 : si = rmsvs(si); break;
		//
		case 0xe7 : qq->shift += read_char(si++); break;
		case 0xe6 : si = rmsvs_sft(si); break;
		case 0xe5 : si = rhyvs_sft(si); break;
		//
		case 0xe4 : si++; break;
		//�ɲ� for V2.3
		case 0xe3 : if((qq->volume += *si++) < qq->volume) qq->volume = 255; break;
		case 0xe2 :
			if(qq->volume < *si) qq->volume = 0; else qq->volume -= *si;
			si++;
			break;
		//
		case 0xe1 : si++; break;
		case 0xe0 : si++; break;
		//
		case 0xdf : open_work.syousetu_lng = *si++; break;
		//
		case 0xde : si = vol_one_up_pcm(qq, si); break;
		case 0xdd : si = vol_one_down(qq, si); break;
		//
		case 0xdc : open_work.status = *si++; break;
		case 0xdb : open_work.status += *si++; break;
		//
		case 0xda : si = portam(qq, si); break;
		//
		case 0xd9 : si++; break;
		case 0xd8 : si++; break;
		case 0xd7 : si++; break;
		//
		case 0xd6 : qq->mdspd = qq->mdspd2 = *si++; qq->mdepth = read_char(si++); break;
		case 0xd5 : qq->detune += read_short(si); si+=2; break;
		//
		case 0xd4 : si = ssg_efct_set(qq, si); break;
		case 0xd3 : si = fm_efct_set(qq, si); break;
		case 0xd2 :
			open_work.fadeout_flag = 1;
			open_work.fadeout_speed = *si++;
			break;
		//
		case 0xd1 : si++; break;
		case 0xd0 : si++; break;
		//
		case 0xcf : si++; break;
		case 0xce : si = pcmrepeat_set(qq, si); break;
		case 0xcd : si = extend_psgenvset(qq, si); break;
		case 0xcc : si++; break;
		case 0xcb : qq->lfo_wave = *si++; break;
		case 0xca :
			qq->extendmode = (qq->extendmode & 0xfd) | ((*si++ & 1) << 1);
			break;
		
		case 0xc9 :
			qq->extendmode = (qq->extendmode & 0xfb) | ((*si++ & 1) << 2);
			break;
		
		case 0xc8 : si+=3; break;
		case 0xc7 : si+=3; break;
		case 0xc6 : si+=6; break;
		case 0xc5 : si++; break;
		case 0xc4 : qq->qdatb = *si++; break;
		case 0xc3 : si = pansetm_ex(qq, si); break;
		case 0xc2 : qq->delay = qq->delay2 = *si++; lfoinit_main(qq); break;
		case 0xc1 : break;
		case 0xc0 : si = pcm_mml_part_mask(qq, si); break;
		case 0xbf : lfo_change(qq); si = lfoset(qq, si); lfo_change(qq);break;
		case 0xbe : si = _lfoswitch(qq, si); break;
		case 0xbd :
			lfo_change(qq);
			qq->mdspd = qq->mdspd2 = *si++;
			qq->mdepth = read_char(si++);
			lfo_change(qq);
			break;
		
		case 0xbc : lfo_change(qq); qq->lfo_wave=*si++; lfo_change(qq); break;
		case 0xbb :
			lfo_change(qq);
			qq->extendmode = (qq->extendmode & 0xfd) | ((*si++ & 1) << 1);
			lfo_change(qq);
			break;
		
		case 0xba : si = _volmask_set(qq, si); break;
		case 0xb9 :
			lfo_change(qq);
			qq->delay = qq->delay2 = *si++; lfoinit_main(qq); break;
			lfo_change(qq);
			break;
			
		case 0xb8 : si += 2; break;
		case 0xb7 : si = mdepth_count(qq, si); break;
		case 0xb6 : si++; break;
		case 0xb5 : si += 2; break;
		case 0xb4 : si = ppz_extpartset(qq, si); break;
		case 0xb3 : qq->qdat2 = *si++; break;
		case 0xb2 : qq->shift_def = read_char(si++); break;
		case 0xb1 : qq->qdat3 = *si++; break;
	
		default : 
			si--;
			*si = 0x80;
	}
	
	return si;
}


uchar * PMDWIN::commands8(QQ *qq, uchar *si)
{
	int		al;
	
	al = *si++;
	switch(al) {
		case 0xff : si = comat8(qq, si); break;
		case 0xfe : qq->qdata = *si++; break;
		case 0xfd : qq->volume = *si++; break;
		case 0xfc : si = comt(si); break;
		case 0xfb : pmdwork.tieflag |= 1; break;
		case 0xfa : qq->detune = read_short(si); si+=2; break;
		case 0xf9 : si = comstloop(qq, si); break;
		case 0xf8 : si = comedloop(qq, si); break;
		case 0xf7 : si = comexloop(qq, si); break;
		case 0xf6 : qq->partloop = si; break;
		case 0xf5 : qq->shift = read_char(si++); break;
		case 0xf4 : if((qq->volume+=16) < qq->volume) qq->volume = 255; break;
		case 0xf3 : if(qq->volume < 16) qq->volume=0; else qq->volume-=16; break;
		case 0xf2 : si = lfoset(qq, si); break;
		case 0xf1 : si = lfoswitch(qq, si); break;
		case 0xf0 : si = psgenvset(qq, si); break;
		
		case 0xef : opna.SetReg(0x100 + *si, *(si+1)); si+=2; break;
		case 0xee : si++; break;
		case 0xed : si++; break;
		case 0xec : si = panset8(qq, si); break;				// FOR SB2
		case 0xeb : si = rhykey(si); break;
		case 0xea : si = rhyvs(si); break;
		case 0xe9 : si = rpnset(si); break;
		case 0xe8 : si = rmsvs(si); break;
		//
		case 0xe7 : qq->shift += read_char(si++); break;
		case 0xe6 : si = rmsvs_sft(si); break;
		case 0xe5 : si = rhyvs_sft(si); break;
		//
		case 0xe4 : si++; break;
		//�ɲ� for V2.3
		case 0xe3 : if((qq->volume += *si++) < qq->volume) qq->volume = 255; break;
		case 0xe2 :
			if(qq->volume < *si) qq->volume = 0; else qq->volume -= *si;
			si++;
			break;
		//
		case 0xe1 : si++; break;
		case 0xe0 : si++; break;
		//
		case 0xdf : open_work.syousetu_lng = *si++; break;
		//
		case 0xde : si = vol_one_up_pcm(qq, si); break;
		case 0xdd : si = vol_one_down(qq, si); break;
		//
		case 0xdc : open_work.status = *si++; break;
		case 0xdb : open_work.status += *si++; break;
		//
		case 0xda : si++; break;
		//
		case 0xd9 : si++; break;
		case 0xd8 : si++; break;
		case 0xd7 : si++; break;
		//
		case 0xd6 : qq->mdspd = qq->mdspd2 = *si++; qq->mdepth = read_char(si++); break;
		case 0xd5 : qq->detune += read_short(si); si+=2; break;
		//
		case 0xd4 : si = ssg_efct_set(qq, si); break;
		case 0xd3 : si = fm_efct_set(qq, si); break;
		case 0xd2 :
			open_work.fadeout_flag = 1;
			open_work.fadeout_speed = *si++;
			break;
		//
		case 0xd1 : si++; break;
		case 0xd0 : si++; break;
		//
		case 0xcf : si++; break;
		case 0xce : si = pcmrepeat_set8(qq, si); break;
		case 0xcd : si = extend_psgenvset(qq, si); break;
		case 0xcc : si++; break;
		case 0xcb : qq->lfo_wave = *si++; break;
		case 0xca :
			qq->extendmode = (qq->extendmode & 0xfd) | ((*si++ & 1) << 1);
			break;
		
		case 0xc9 :
			qq->extendmode = (qq->extendmode & 0xfb) | ((*si++ & 1) << 2);
			break;
		
		case 0xc8 : si+=3; break;
		case 0xc7 : si+=3; break;
		case 0xc6 : si+=6; break;
		case 0xc5 : si++; break;
		case 0xc4 : qq->qdatb = *si++; break;
		case 0xc3 : si = panset8_ex(qq, si); break;
		case 0xc2 : qq->delay = qq->delay2 = *si++; lfoinit_main(qq); break;
		case 0xc1 : break;
		case 0xc0 : si = pcm_mml_part_mask8(qq, si); break;
		case 0xbf : si+=4; break;
		case 0xbe : si++; break;
		case 0xbd : si+=2; break;
		case 0xbc : si++; break;
		case 0xbb : si++; break;
		case 0xba : si++; break;
		case 0xb9 : si++; break;
		case 0xb8 : si+=2; break;
		case 0xb7 : si = mdepth_count(qq, si); break;
		case 0xb6 : si++; break;
		case 0xb5 : si+=2; break;
		case 0xb4 : si = ppz_extpartset(qq, si); break;
		case 0xb3 : qq->qdat2 = *si++; break;
		case 0xb2 : qq->shift_def = read_char(si++); break;
		case 0xb1 : qq->qdat3 = *si++; break;
	
		default : 
			si--;
			*si = 0x80;
	}
	
	return si;
}


uchar * PMDWIN::commandsz(QQ *qq, uchar *si)
{
	int		al;
	
	al = *si++;
	switch(al) {
		case 0xff : si = comatz(qq, si); break;
		case 0xfe : qq->qdata = *si++; break;
		case 0xfd : qq->volume = *si++; break;
		case 0xfc : si = comt(si); break;
		
		case 0xfb : pmdwork.tieflag |= 1; break;
		case 0xfa : qq->detune = read_short(si); si+=2; break;
		case 0xf9 : si = comstloop(qq, si); break;
		case 0xf8 : si = comedloop(qq, si); break;
		case 0xf7 : si = comexloop(qq, si); break;
		case 0xf6 : qq->partloop = si; break;
		case 0xf5 : qq->shift = read_char(si++); break;
		case 0xf4 : if((qq->volume+=16) < qq->volume) qq->volume = 255; break;
		case 0xf3 : if(qq->volume < 16) qq->volume=0; else qq->volume-=16; break;
		case 0xf2 : si = lfoset(qq, si); break;
		case 0xf1 : si = lfoswitch(qq, si); break;
		case 0xf0 : si = psgenvset(qq, si); break;
		
		case 0xef : opna.SetReg(pmdwork.fmsel + *si, *(si+1)); si+=2; break;
		case 0xee : si++; break;
		case 0xed : si++; break;
		case 0xec : si = pansetz(qq, si); break;				// FOR SB2
		case 0xeb : si = rhykey(si); break;
		case 0xea : si = rhyvs(si); break;
		case 0xe9 : si = rpnset(si); break;
		case 0xe8 : si = rmsvs(si); break;
		//
		case 0xe7 : qq->shift += read_char(si++); break;
		case 0xe6 : si = rmsvs_sft(si); break;
		case 0xe5 : si = rhyvs_sft(si); break;
		//
		case 0xe4 : si++; break;
		//�ɲ� for V2.3
		case 0xe3 : if((qq->volume += *si++) < qq->volume) qq->volume = 255; break;
		case 0xe2 :
			if(qq->volume < *si) qq->volume = 0; else qq->volume -= *si;
			si++;
			break;
		//
		case 0xe1 : si++; break;
		case 0xe0 : si++; break;
		//
		case 0xdf : open_work.syousetu_lng = *si++; break;
		//
		case 0xde : si = vol_one_up_pcm(qq, si); break;
		case 0xdd : si = vol_one_down(qq, si); break;
		//
		case 0xdc : open_work.status = *si++; break;
		case 0xdb : open_work.status += *si++; break;
		//
		case 0xda : si = portaz(qq, si); break;
		//
		case 0xd9 : si++; break;
		case 0xd8 : si++; break;
		case 0xd7 : si++; break;
		//
		case 0xd6 : qq->mdspd = qq->mdspd2 = *si++; qq->mdepth = read_char(si++); break;
		case 0xd5 : qq->detune += read_short(si); si+=2; break;
		//
		case 0xd4 : si = ssg_efct_set(qq, si); break;
		case 0xd3 : si = fm_efct_set(qq, si); break;
		case 0xd2 :
			open_work.fadeout_flag = 1;
			open_work.fadeout_speed = *si++;
			break;
		//
		case 0xd1 : si++; break;
		case 0xd0 : si++; break;
		//
		case 0xcf : si++; break;
		case 0xce : si = ppzrepeat_set(qq, si); break;
		case 0xcd : si = extend_psgenvset(qq, si); break;
		case 0xcc : si++; break;
		case 0xcb : qq->lfo_wave = *si++; break;
		case 0xca :
			qq->extendmode = (qq->extendmode & 0xfd) | ((*si++ & 1) << 1);
			break;
		
		case 0xc9 :
			qq->extendmode = (qq->extendmode & 0xfb) | ((*si++ & 1) << 2);
			break;
		
		case 0xc8 : si+=3; break;
		case 0xc7 : si+=3; break;
		case 0xc6 : si+=6; break;
		case 0xc5 : si++; break;
		case 0xc4 : qq->qdatb = *si++; break;
		case 0xc3 : si = pansetz_ex(qq, si); break;
		case 0xc2 : qq->delay = qq->delay2 = *si++; lfoinit_main(qq); break;
		case 0xc1 : break;
		case 0xc0 : si = ppz_mml_part_mask(qq, si); break;
		case 0xbf : lfo_change(qq); si = lfoset(qq, si); lfo_change(qq);break;
		case 0xbe : si = _lfoswitch(qq, si); break;
		case 0xbd :
			lfo_change(qq);
			qq->mdspd = qq->mdspd2 = *si++;
			qq->mdepth = read_char(si++);
			lfo_change(qq);
			break;
		
		case 0xbc : lfo_change(qq); qq->lfo_wave=*si++; lfo_change(qq); break;
		case 0xbb :
			lfo_change(qq);
			qq->extendmode = (qq->extendmode & 0xfd) | ((*si++ & 1) << 1);
			lfo_change(qq);
			break;
		
		case 0xba : si = _volmask_set(qq, si); break;
		case 0xb9 :
			lfo_change(qq);
			qq->delay = qq->delay2 = *si++; lfoinit_main(qq); break;
			lfo_change(qq);
			break;
			
		case 0xb8 : si += 2; break;
		case 0xb7 : si = mdepth_count(qq, si); break;
		case 0xb6 : si++; break;
		case 0xb5 : si += 2; break;
		case 0xb4 : si += 16; break;
		case 0xb3 : qq->qdat2 = *si++; break;
		case 0xb2 : qq->shift_def = read_char(si++); break;
		case 0xb1 : qq->qdat3 = *si++; break;
	
		default : 
			si--;
			*si = 0x80;
	}
	
	return si;
}


//=============================================================================
//	COMMAND '@' [PROGRAM CHANGE]
//=============================================================================
uchar * PMDWIN::comat(QQ *qq, uchar *si)
{
	uchar	*bx;
	int		al, dl;

	qq->voicenum = al = *si++;
	dl = qq->voicenum;
	
	if(qq->partmask == 0) {	// �ѡ��ȥޥ�������Ƥ��뤫��
		neiroset(qq, dl);
		return si;
	} else {
		bx = toneadr_calc(qq, dl);
		qq->alg_fb = dl = bx[24];
		bx += 4;
		
		// tl���� 
		qq->slot1 = bx[0];
		qq->slot3 = bx[1];
		qq->slot2 = bx[2];
		qq->slot4 = bx[3];
		
		//	FM3ch�ǡ��ޥ�������Ƥ�����硢fm3_alg_fb������
		if(pmdwork.partb == 3 && qq->neiromask) {
			if(pmdwork.fmsel == 0) {
				// in. dl = alg/fb
				if((qq->slotmask & 0x10) == 0) {
					al = pmdwork.fm3_alg_fb & 0x38;		// fb�������ͤ����
					dl = (dl & 7) | al;
				}
				
				pmdwork.fm3_alg_fb = dl;
				qq->alg_fb = al;
			}
		}
		
	}
	return si;
}

//=============================================================================
//	����������
//		INPUTS	-- [PARTB]
//			-- dl [TONE_NUMBER]
//			-- di [PART_DATA_ADDRESS]
//=============================================================================
void PMDWIN::neiroset(QQ *qq, int dl)
{
	uchar	*bx;
	int		ah, al, dh;
	
	bx = toneadr_calc(qq, dl);
	if(silence_fmpart(qq)) {
		// neiromask=0�λ� (TL��work�Τ�����)
		bx += 4;
		
		// tl����
		qq->slot1 = bx[0];
		qq->slot3 = bx[1];
		qq->slot2 = bx[2];
		qq->slot4 = bx[3];
		return;
	}
	
	//=========================================================================
	//	��������ᥤ��
	//=========================================================================
	//-------------------------------------------------------------------------
	//	AL/FB������
	//-------------------------------------------------------------------------
	
	dh = 0xb0 - 1 + pmdwork.partb;
	
	if(pmdwork.af_check) {		// ALG/FB�����ꤷ�ʤ�mode����
		dl = qq->alg_fb;
	} else {
		dl = bx[24];
	}
	
	if(pmdwork.partb == 3 && pmdwork.fmsel == 0) {
		if(pmdwork.af_check != 0) {	// ALG/FB�����ꤷ�ʤ�mode����
			dl = pmdwork.fm3_alg_fb;
		} else {
			if((qq->slotmask & 0x10) == 0) {	// slot1����Ѥ��Ƥ��뤫��
				dl = (pmdwork.fm3_alg_fb & 0x38) | (dl & 7);
			}
			pmdwork.fm3_alg_fb = dl;
		}
	}
	
	opna.SetReg(pmdwork.fmsel + dh, dl);
	qq->alg_fb = dl;
	dl &= 7;		// dl = algo
	
	//-------------------------------------------------------------------------
	//	Carrier�ΰ��֤�Ĵ�٤� (VolMask�ˤ�����)
	//-------------------------------------------------------------------------
	
	if((qq->volmask & 0x0f) == 0) {
		qq->volmask = carrier_table[dl];
	}
	
	if((qq->_volmask & 0x0f) == 0) {
		qq->_volmask = carrier_table[dl];
	}
	
	qq->carrier = carrier_table[dl];
	ah = carrier_table[dl+8];	// slot2/3�ε�ž�ǡ���(not�Ѥ�)
	al = qq->neiromask;
	ah &= al;				// AH=TL�Ѥ�mask / AL=����¾�Ѥ�mask
	
	//-------------------------------------------------------------------------
	//	�Ʋ����ѥ�᡼�������� (TL�ϥ⥸��졼���Τ�)
	//-------------------------------------------------------------------------
	
	dh = 0x30 - 1 + pmdwork.partb;
	dl = *bx++;				// DT/ML
	if(al & 0x80) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;
	
	dl = *bx++;
	if(al & 0x40) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;
	
	dl = *bx++;
	if(al & 0x20) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;
	
	dl = *bx++;
	if(al & 0x10) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;
	
	dl = *bx++;				// TL
	if(ah & 0x80) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;
	
	dl = *bx++;
	if(ah & 0x40) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;
	
	dl = *bx++;
	if(ah & 0x20) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;
	
	dl = *bx++;
	if(ah & 0x10) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;
	
	dl = *bx++;				// KS/AR
	if(al & 0x08) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;
	
	dl = *bx++;
	if(al & 0x04) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;
	
	dl = *bx++;
	if(al & 0x02) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;
	
	dl = *bx++;
	if(al & 0x01) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;
	
	dl = *bx++;				// AM/DR
	if(al & 0x80) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;
	
	dl = *bx++;
	if(al & 0x40) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;
	
	dl = *bx++;
	if(al & 0x20) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;
	
	dl = *bx++;
	if(al & 0x10) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;
	
	dl = *bx++;				// SR
	if(al & 0x08) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;
	
	dl = *bx++;
	if(al & 0x04) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;
	
	dl = *bx++;
	if(al & 0x02) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;
	
	dl = *bx++;
	if(al & 0x01) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;

	dl = *bx++;				// SL/RR
	if(al & 0x80) {
		opna.SetReg(pmdwork.fmsel + dh, dl);
	}
	dh+=4;
	
	dl = *bx++;
	if(al & 0x40) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;
	
	dl = *bx++;
	if(al & 0x20) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;
	
	dl = *bx++;
	if(al & 0x10) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;

/*
	dl = *bx++;				// SL/RR
	if(al & 0x80) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;
	
	dl = *bx++;
	if(al & 0x40) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;
	
	dl = *bx++;
	if(al & 0x20) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;
	
	dl = *bx++;
	if(al & 0x10) opna.SetReg(pmdwork.fmsel + dh, dl);
	dh+=4;
*/

	//-------------------------------------------------------------------------
	//	SLOT���TL��������¸
	//-------------------------------------------------------------------------
	bx -= 20;
	qq->slot1 = bx[0];
	qq->slot3 = bx[1];
	qq->slot2 = bx[2];
	qq->slot4 = bx[3];
}


//=============================================================================
//	[PartB]�Υѡ��Ȥβ������˾ä� (TL=127 and RR=15 and KEY-OFF)
//		cy=1 ������ ������å�neiromask����Ƥ���
//=============================================================================
int PMDWIN::silence_fmpart(QQ *qq)
{
	int		dh;
	
	if(qq->neiromask == 0) {
		return 1;
	}
	
	dh = pmdwork.partb + 0x40 - 1;
	
	if(qq->neiromask & 0x80) {
		opna.SetReg(pmdwork.fmsel + dh, 127);
		opna.SetReg((pmdwork.fmsel + 0x40) + dh, 127);
	}
	dh += 4;
	
	if(qq->neiromask & 0x40) {
		opna.SetReg(pmdwork.fmsel + dh, 127);
		opna.SetReg(pmdwork.fmsel + 0x40 + dh, 127);
	}
	dh += 4;
	
	if(qq->neiromask & 0x20) {
		opna.SetReg(pmdwork.fmsel + dh, 127);
		opna.SetReg(pmdwork.fmsel + 0x40 + dh, 127);
	}
	dh += 4;
	
	if(qq->neiromask & 0x10) {
		opna.SetReg(pmdwork.fmsel + dh, 127);
		opna.SetReg(pmdwork.fmsel + 0x40 + dh, 127);
	}
	
	kof1(qq);
	return 0;
}


//=============================================================================
//	TONE DATA START ADDRESS ��׻�
//		input	dl	tone_number
//		output	bx	address
//=============================================================================
uchar * PMDWIN::toneadr_calc(QQ *qq, int dl)
{
	uchar *bx;
	
	if(open_work.prg_flg == 0 && qq != &EffPart) {
		return open_work.tondat + (dl << 5);
	} else {
		bx = open_work.prgdat_adr;
		
		while(*bx != dl) {
			bx += 26;
		}
		
		return bx+1;
	}
}


//=============================================================================
//	�ƣͲ����ϡ��ɣ̣ƣϤ�����ʣ֣�������ĥʬ��
//=============================================================================
uchar * PMDWIN::hlfo_set(QQ *qq, uchar *si)
{
	qq->fmpan = (qq->fmpan & 0xc0) | *si++;
	
	if(pmdwork.partb == 3 && pmdwork.fmsel == 0) {
		// 2608�λ��ΤߤʤΤ� part_e�Ϥ��ꤨ�ʤ�
		//	FM3�ξ��� 4�ĤΥѡ����������
		FMPart[2].fmpan = qq->fmpan;
		ExtPart[0].fmpan = qq->fmpan;
		ExtPart[1].fmpan = qq->fmpan;
		ExtPart[2].fmpan = qq->fmpan;
	}
	
	if(qq->partmask == 0) {		// �ѡ��ȥޥ�������Ƥ��뤫��
		opna.SetReg(pmdwork.fmsel + pmdwork.partb + 0xb4 - 1,
			calc_panout(qq));
	}
	return si;
}


//=============================================================================
//	�ܥ�塼��򼡤ΰ�Ĥ����ѹ��ʣ֣�������ĥʬ��
//=============================================================================
uchar * PMDWIN::vol_one_up_fm(QQ *qq, uchar *si)
{
	int		al;
	
	al = (int)qq->volume + 1 + *si++;
	if(al > 128) al = 128;
	
	qq->volpush = al;
	pmdwork.volpush_flag = 1;
	return si;
}


//=============================================================================
//	�ݥ륿����(FM)
//=============================================================================
uchar * PMDWIN::porta(QQ *qq, uchar *si)
{
	int		ax, cx, cl, bx, bh;
	
	if(qq->partmask) {
		qq->fnum = 0;		//���������
		qq->onkai = 255;
		qq->leng = *(si+2);
		qq->keyon_flag++;
		qq->address = si+3;
		
		if(--pmdwork.volpush_flag) {
			qq->volpush = 0;
		}
		
		pmdwork.tieflag = 0;
		pmdwork.volpush_flag = 0;
		pmdwork.loop_work &= qq->loopcheck;
		
		return si+3;		// �ɤ����Ф�	(Mask��)
	}
	
	lfoinit(qq, *si);
	fnumset(qq, oshift(qq, *si++));
	
	cx = qq->fnum;
	cl = qq->onkai;
	fnumset(qq, oshift(qq, *si++));
	bx = qq->fnum;			// bx=�ݥ륿�������fnum��
	qq->onkai = cl;
	qq->fnum = cx;			// cx=�ݥ륿���ȸ���fnum��
	
	bh = (int)((bx / 256) & 0x38) - ((cx / 256) & 0x38);	// ���octarb - ����octarb
	if(bh) {
		bh /= 8;
		ax = bh * 0x26a;			// ax = 26ah * octarb��
	} else {
		ax = 0;
	}
		
	bx  = (bx & 0x7ff) - (cx & 0x7ff);
	ax += bx;				// ax=26ah*octarb�� + ������
	
	qq->leng = *si++;
	si = calc_q(qq, si);
	
	qq->porta_num2 = ax / qq->leng;	// ��
	qq->porta_num3 = ax % qq->leng;	// ;��
	qq->lfoswi |= 8;				// Porta ON
	
	if(qq->volpush && qq->onkai != 255) {
		if(--pmdwork.volpush_flag) {
			pmdwork.volpush_flag = 0;
			qq->volpush = 0;
		}
	}
	
	volset(qq);
	otodasi(qq);
	keyon(qq);
	
	qq->keyon_flag++;
	qq->address = si;
	
	pmdwork.tieflag = 0;
	pmdwork.volpush_flag = 0;
	
	if(*si == 0xfb) {		// '&'��ľ��ˤ��ä���keyoff���ʤ�
		qq->keyoff_flag = 2;
	} else {
		qq->keyoff_flag = 0;
	}
	pmdwork.loop_work &= qq->loopcheck;
	return si;
}


//=============================================================================
//	FM slotmask set
//=============================================================================
uchar * PMDWIN::slotmask_set(QQ *qq, uchar *si)
{
	uchar	*bx;
	int		ah, al, bl;
	
	ah = al = *si++;

	if(al &= 0x0f) {
		qq->carrier = al << 4;
	} else {
		if(pmdwork.partb == 3 && pmdwork.fmsel == 0) {
			bl = pmdwork.fm3_alg_fb;
		} else {
			bx = toneadr_calc(qq, qq->voicenum);
			bl = bx[24];
		}
		qq->carrier = carrier_table[bl & 7];
	}
	
	ah &= 0xf0;
	if(qq->slotmask != ah) {
		qq->slotmask = ah;
		if((ah & 0xf0) == 0) {
			qq->partmask |= 0x20;	// s0�λ��ѡ��ȥޥ���
		} else {
			qq->partmask &= 0xdf;	// s0�ʳ��λ��ѡ��ȥޥ������
		}
		
		if(ch3_setting(qq)) {		// FM3ch�ξ��Τ� ch3mode���ѹ�����
			// ch3�ʤ顢���������FM3�ѡ��Ȥ�keyon����
			if(qq != &FMPart[2]) {
				if(FMPart[2].partmask == 0 &&
						(FMPart[2].keyoff_flag & 1) == 0) {
					keyon(&FMPart[2]);
				}
				
				if(qq != &ExtPart[0]) {
					if(ExtPart[0].partmask == 0 &&
							(ExtPart[0].keyoff_flag & 1) == 0) {
						keyon(&ExtPart[0]);
					}
					
					if(qq != &ExtPart[1]) {
						if(ExtPart[1].partmask == 0 &&
								(ExtPart[1].keyoff_flag & 1) == 0) {
							keyon(&ExtPart[1]);
						}
					}
				}
			}
		}
		
		ah = 0;
		if(qq->slotmask & 0x80) ah += 0x11;		// slot4
		if(qq->slotmask & 0x40) ah += 0x44;		// slot3
		if(qq->slotmask & 0x20) ah += 0x22;		// slot2
		if(qq->slotmask & 0x10) ah += 0x88;		// slot1
		qq->neiromask = ah;
	}
	return si;
}


//=============================================================================
//	Slot Detune Set
//=============================================================================
uchar * PMDWIN::slotdetune_set(QQ *qq, uchar *si)
{
	int		ax, bl;
	
	if(pmdwork.partb != 3 || pmdwork.fmsel) {
		return si+3;
	}
	
	bl = *si++;
	ax = read_short(si);
	si+=2;
	
	if(bl & 1) {
		open_work.slot_detune1 = ax;
	}
	
	if(bl & 2) {
		open_work.slot_detune2 = ax;
	}
	
	if(bl & 4) {
		open_work.slot_detune3 = ax;
	}
	
	if(bl & 8) {
		open_work.slot_detune4 = ax;
	}
	
	if(open_work.slot_detune1 || open_work.slot_detune2 ||
			open_work.slot_detune3 || open_work.slot_detune4) {
			pmdwork.slotdetune_flag = 1;
	} else {
		pmdwork.slotdetune_flag = 0;
		open_work.slot_detune1 = 0;
	}
	ch3mode_set(qq);
	return si;
}


//=============================================================================
//	Slot Detune Set(����)
//=============================================================================
uchar * PMDWIN::slotdetune_set2(QQ *qq, uchar *si)
{
	int		ax, bl;
	
	if(pmdwork.partb != 3 || pmdwork.fmsel) {
		return si+3;
	}
	
	bl = *si++;
	ax = read_short(si);
	si+=2;
	
	if(bl & 1) {
		open_work.slot_detune1 += ax;
	}
	
	if(bl & 2) {
		open_work.slot_detune2 += ax;
	}
	
	if(bl & 4) {
		open_work.slot_detune3 += ax;
	}
	
	if(bl & 8) {
		open_work.slot_detune4 += ax;
	}
	
	if(open_work.slot_detune1 || open_work.slot_detune2 ||
			open_work.slot_detune3 || open_work.slot_detune4) {
		pmdwork.slotdetune_flag = 1;
	} else {
		pmdwork.slotdetune_flag = 0;
		open_work.slot_detune1 = 0;
	}
	ch3mode_set(qq);
	return si;
}


void PMDWIN::fm3_partinit(QQ *qq, uchar *ax)
{
	qq->address = ax;
	qq->leng = 1;					// ���� 1������� �� ���󥽥� ������
	qq->keyoff_flag = -1;			// ����keyoff��
	qq->mdc = -1;					// MDepth Counter (̵��)
	qq->mdc2 = -1;					//
	qq->_mdc = -1;					//
	qq->_mdc2 = -1;					//
	qq->onkai = 255;				// rest
	qq->onkai_def = 255;			// rest
	qq->volume = 108;				// FM  VOLUME DEFAULT= 108
	qq->fmpan = FMPart[2].fmpan;	// FM PAN = CH3��Ʊ��
	qq->partmask |= 0x20;			// s0�� partmask
}


//=============================================================================
//	FM3ch ��ĥ�ѡ��ȥ��å�
//=============================================================================
uchar * PMDWIN::fm3_extpartset(QQ *qq, uchar *si)
{
	short ax;
	
	ax = read_short(si);
	si+=2;
	if(ax) fm3_partinit(&ExtPart[0], &open_work.mmlbuf[ax]);
	
	ax = read_short(si);
	si+=2;
	if(ax) fm3_partinit(&ExtPart[1], &open_work.mmlbuf[ax]);
	
	ax = read_short(si);
	si+=2;
	if(ax) fm3_partinit(&ExtPart[2], &open_work.mmlbuf[ax]);
	return si;
}


//=============================================================================
//	ppz ��ĥ�ѡ��ȥ��å�
//=============================================================================
uchar * PMDWIN::ppz_extpartset(QQ *qq, uchar *si)
{
	short	ax;
	int		i;

	for(i = 0; i < 8; i++) {
		ax = read_short(si);
		si+=2;
		if(ax) {
			PPZ8Part[i].address = &open_work.mmlbuf[ax];
			PPZ8Part[i].leng = 1;					// ���� 1������� �� ���󥽥� ������
			PPZ8Part[i].keyoff_flag = -1;			// ����keyoff��
			PPZ8Part[i].mdc = -1;					// MDepth Counter (̵��)
			PPZ8Part[i].mdc2 = -1;					//
			PPZ8Part[i]._mdc = -1;					//
			PPZ8Part[i]._mdc2 = -1;					//
			PPZ8Part[i].onkai = 255;				// rest
			PPZ8Part[i].onkai_def = 255;			// rest
			PPZ8Part[i].volume = 128;				// PCM VOLUME DEFAULT= 128
			PPZ8Part[i].fmpan = 5;					// PAN=Middle
		}
	}
	return si;
}


//=============================================================================
//	���̥ޥ���slot������
//=============================================================================
uchar * PMDWIN::volmask_set(QQ *qq, uchar *si)
{
	int		al;
	
	al = *si++ & 0x0f;
	if(al) {
		al = (al << 4) | 0x0f;
		qq->volmask = al;
	} else {
		qq->volmask = qq->carrier;
	}
	ch3_setting(qq);
	return si;
}


//=============================================================================
//	0c0h���ɲ�special̿��
//=============================================================================
uchar * PMDWIN::special_0c0h(QQ *qq, uchar *si, uchar al)
{
	switch(al) {
		case 0xff : open_work.fm_voldown = *si++; break;
		case 0xfe : si = _vd_fm(qq, si); break;
		case 0xfd : open_work.ssg_voldown = *si++; break;
		case 0xfc : si = _vd_ssg(qq, si); break;
		case 0xfb : open_work.pcm_voldown = *si++; break;
		case 0xfa : si = _vd_pcm(qq, si); break;
		case 0xf9 : open_work.rhythm_voldown = *si++; break;
		case 0xf8 : si = _vd_rhythm(qq, si); break;
		case 0xf7 : open_work.pcm86_vol = (*si++ & 1); break;
		case 0xf6 : open_work.ppz_voldown = *si++; break;
		case 0xf5 : si = _vd_ppz(qq, si); break;
		default : 
			si--;
			*si = 0x80;
	}
	return si;
}


uchar * PMDWIN::_vd_fm(QQ *qq, uchar *si)
{
	int		al;

	al = read_char(si++);
	if(al) {
		open_work.fm_voldown = Limit(al + open_work.fm_voldown, 255, 0);
	} else {
		open_work.fm_voldown = open_work._fm_voldown;
	}
	return si;
}


uchar * PMDWIN::_vd_ssg(QQ *qq, uchar *si)
{
	int		al;

	al = read_char(si++);
	if(al) {
		open_work.ssg_voldown = Limit(al + open_work.ssg_voldown, 255, 0);
	} else {
		open_work.ssg_voldown = open_work._ssg_voldown;
	}
	return si;
}


uchar * PMDWIN::_vd_pcm(QQ *qq, uchar *si)
{
	int		al;

	al = read_char(si++);
	if(al) {
		open_work.pcm_voldown = Limit(al + open_work.pcm_voldown, 255, 0);
	} else {
		open_work.pcm_voldown = open_work._pcm_voldown;
	}
	return si;
}


uchar * PMDWIN::_vd_rhythm(QQ *qq, uchar *si)
{
	int		al;

	al = read_char(si++);
	if(al) {
		open_work.rhythm_voldown = Limit(al + open_work.rhythm_voldown, 255, 0);
	} else {
		open_work.rhythm_voldown = open_work._rhythm_voldown;
	}
	return si;
}


uchar * PMDWIN::_vd_ppz(QQ *qq, uchar *si)
{
	int		al;

	al = read_char(si++);
	if(al) {
		open_work.ppz_voldown = Limit(al + open_work.ppz_voldown, 255, 0);
	} else {
		open_work.ppz_voldown = open_work._ppz_voldown;
	}
	return si;
}

//=============================================================================
//	������ѡ��ȤΥޥ���on/off
//=============================================================================
uchar * PMDWIN::fm_mml_part_mask(QQ *qq, uchar *si)
{
	int		al;
	
	al = *si++;
	if(al >= 2) {
		si = special_0c0h(qq, si, al);
	} else if(al) {
		qq->partmask |= 0x40;
		if(qq->partmask == 0x40) {
			silence_fmpart(qq);	// ���õ�
		}
	} else {
		if((qq->partmask &= 0xbf) == 0) {
			neiro_reset(qq);		// ����������
		}
	}
	return si;
}


uchar * PMDWIN::ssg_mml_part_mask(QQ *qq, uchar *si)
{
	int		ah, al;
	
	al = *si++;
	if(al >= 2) {
		si = special_0c0h(qq, si, al);
	} else if(al) {
		qq->partmask |= 0x40;
		if(qq->partmask == 0x40) {
			ah = ((1 << (pmdwork.partb-1)) | (4 << pmdwork.partb));
			al = opna.GetReg(0x07);
			opna.SetReg(0x07, ah | al);		// PSG keyoff
		}
	} else {
		qq->partmask &= 0xbf;
	}
	return si;
}


uchar * PMDWIN::rhythm_mml_part_mask(QQ *qq, uchar *si)
{
	int		al;
	
	al = *si++;
	if(al >= 2) {
		si = special_0c0h(qq, si, al);
	} else if(al) {
		qq->partmask |= 0x40;
	} else {
		qq->partmask &= 0xbf;
	}
	return si;
}


uchar * PMDWIN::pcm_mml_part_mask(QQ *qq, uchar * si)
{
	int		al;
	
	al = *si++;
	if(al >= 2) {
		si = special_0c0h(qq, si, al);
	} else if(al) {
		qq->partmask |= 0x40;
		if(qq->partmask == 0x40) {
			opna.SetReg(0x101, 0x02);		// PAN=0 / x8 bit mode
			opna.SetReg(0x100, 0x01);		// PCM RESET
		}
	} else {
		qq->partmask &= 0xbf;
	}
	return si;
}


uchar * PMDWIN::pcm_mml_part_mask8(QQ *qq, uchar * si)
{
	int		al;
	
	al = *si++;
	if(al >= 2) {
		si = special_0c0h(qq, si, al);
	} else if(al) {
		qq->partmask |= 0x40;
		if(qq->partmask == 0x40) {
			p86drv.Stop();
		}
	} else {
		qq->partmask &= 0xbf;
	}
	return si;
}


uchar * PMDWIN::ppz_mml_part_mask(QQ *qq, uchar * si)
{
	int		al;
	
	al = *si++;
	if(al >= 2) {
		si = special_0c0h(qq, si, al);
	} else if(al) {
		qq->partmask |= 0x40;
		if(qq->partmask == 0x40) {
			ppz8.Stop(pmdwork.partb);
		}
	} else {
		qq->partmask &= 0xbf;
	}
	return si;
}


//=============================================================================
//	FM�����β����������
//=============================================================================
void PMDWIN::neiro_reset(QQ *qq)
{
	int		dh, al, s1, s2, s3, s4;
	
	if(qq->neiromask == 0) return;
	
	s1 = qq->slot1;
	s2 = qq->slot2;
	s3 = qq->slot3;
	s4 = qq->slot4;
	pmdwork.af_check = 1;
	neiroset(qq, qq->voicenum);		// ��������
	pmdwork.af_check = 0;
	qq->slot1 = s1;
	qq->slot2 = s2;
	qq->slot3 = s3;
	qq->slot4 = s4;
	
	al = ((~qq->carrier) & qq->slotmask) & 0xf0;
		// al<- TL������ꤷ�Ƥ���slot 4321xxxx
	if(al) {
		dh = 0x4c - 1 + pmdwork.partb;	// dh=TL FM Port Address
		if(al & 0x80) opna.SetReg(pmdwork.fmsel + dh, s4);
		
		dh -= 8;
		if(al & 0x40) opna.SetReg(pmdwork.fmsel + dh, s3);
		
		dh += 4;
		if(al & 0x20) opna.SetReg(pmdwork.fmsel + dh, s2);
		
		dh -= 8;
		if(al & 0x10) opna.SetReg(pmdwork.fmsel + dh, s1);
	}
	
	dh = pmdwork.partb + 0xb4 - 1;
	opna.SetReg(pmdwork.fmsel + dh, calc_panout(qq));
}


uchar * PMDWIN::_lfoswitch(QQ *qq, uchar *si)
{
	qq->lfoswi = (qq->lfoswi & 0x8f) | ((*si++ & 7) << 4);
	lfo_change(qq);
	lfoinit_main(qq);
	lfo_change(qq);
	return si;
}


uchar * PMDWIN::_volmask_set(QQ *qq, uchar *si)
{
	int		al;
	
	al = *si++ & 0x0f;
	if(al) {
		al = (al << 4) | 0x0f;
		qq->_volmask = al;
	} else {
		qq->_volmask = qq->carrier;
	}
	ch3_setting(qq);
	return si;
}


//=============================================================================
//	TL�Ѳ�
//=============================================================================
uchar * PMDWIN::tl_set(QQ *qq, uchar *si)
{
	int		ah, al, ch, dh, dl;
	
	dh = 0x40 - 1 + pmdwork.partb;		// dh=TL FM Port Address
	al = read_char(si++);
	ah = al & 0x0f;
	ch = (qq->slotmask >> 4) | ((qq->slotmask << 4) & 0xf0);
	ah &= ch;							// ah=�Ѳ�������slot 00004321
	dl = *si++;
	
	if(al >= 0) {
		dl &= 127;
		if(ah & 1) {
			qq->slot1 = dl;
			if(qq->partmask == 0) {
				opna.SetReg(pmdwork.fmsel + dh, dl);
			}
		}
		
		dh += 8;
		if(ah & 2) {
			qq->slot2 = dl;
			if(qq->partmask == 0) {
				opna.SetReg(pmdwork.fmsel + dh, dl);
			}
		}
		
		dh -= 4;
		if(ah & 4) {
			qq->slot3 = dl;
			if(qq->partmask == 0) {
				opna.SetReg(pmdwork.fmsel + dh, dl);
			}
		}
		
		dh += 8;
		if(ah & 8) {
			qq->slot4 = dl;
			if(qq->partmask == 0) {
				opna.SetReg(pmdwork.fmsel + dh, dl);
			}
		}
	} else {
		//	�����Ѳ�
		al = dl;
		if(ah & 1) {
			if((dl = (int)qq->slot1 + al) < 0) {
				dl = 0;
				if(al >= 0) dl = 127;
			}
			if(qq->partmask == 0) {
				opna.SetReg(pmdwork.fmsel + dh, dl);
			}
			qq->slot1 = dl;
		}
		
		dh += 8;
		if(ah & 2) {
			if((dl = (int)qq->slot2 + al) < 0) {
				dl = 0;
				if(al >= 0) dl = 127;
			}
			if(qq->partmask == 0) {
				opna.SetReg(pmdwork.fmsel + dh, dl);
			}
			qq->slot2 = dl;
		}
		
		dh -= 4;
		if(ah & 4) {
			if((dl = (int)qq->slot3 + al) < 0) {
				dl = 0;
				if(al >= 0) dl = 127;
			}
			if(qq->partmask == 0) {
				opna.SetReg(pmdwork.fmsel + dh, dl);
			}
			qq->slot3 = dl;
		}
		
		dh += 8;
		if(ah & 8) {
			if((dl = (int)qq->slot4 + al) < 0) {
				dl = 0;
				if(al >= 0) dl = 127;
			}
			if(qq->partmask == 0) {
				opna.SetReg(pmdwork.fmsel + dh, dl);
			}
			qq->slot4 = dl;
		}
	}
	return si;
}


//=============================================================================
//	FB�Ѳ�
//=============================================================================
uchar * PMDWIN::fb_set(QQ *qq, uchar *si)
{
	int		al, dh, dl;
	
	
	dh = pmdwork.partb + 0xb0 - 1;	// dh=ALG/FB port address
	al = read_char(si++);
	if(al >= 0) {
		// in	al 00000xxx ���ꤹ��FB
		al = ((al << 3) & 0xff) | (al >> 5);
		
		// in	al 00xxx000 ���ꤹ��FB
		if(pmdwork.partb == 3 && pmdwork.fmsel == 0) {
			if((qq->slotmask & 0x10) == 0) return si;
			dl = (pmdwork.fm3_alg_fb & 7) | al;
			pmdwork.fm3_alg_fb = dl;
		} else {
			dl = (qq->alg_fb & 7) | al;
		}
		opna.SetReg(pmdwork.fmsel + dh, dl);
		qq->alg_fb = dl;
		return si;
	} else {
		if((al & 0x40) == 0) al &= 7;
		if(pmdwork.partb == 3 && pmdwork.fmsel == 0) {
			dl = pmdwork.fm3_alg_fb;
		} else {
			dl = qq->alg_fb;
		
		}
		
		dl = (dl >> 3) & 7;
		if((al += dl) >= 0) {
			if(al >= 8) {
				al = 0x38;
				if(pmdwork.partb == 3 && pmdwork.fmsel == 0) {
					if((qq->slotmask & 0x10) == 0) return si;
					
					dl = (pmdwork.fm3_alg_fb & 7) | al;
					pmdwork.fm3_alg_fb = dl;
				} else {
					dl = (qq->alg_fb & 7) | al;
				}
				opna.SetReg(pmdwork.fmsel + dh, dl);
				qq->alg_fb = dl;
				return si;
			} else {
				// in	al 00000xxx ���ꤹ��FB
				al = ((al << 3) & 0xff) | (al >> 5);
				
				// in	al 00xxx000 ���ꤹ��FB
				if(pmdwork.partb == 3 && pmdwork.fmsel == 0) {
					if((qq->slotmask & 0x10) == 0) return si;
					dl = (pmdwork.fm3_alg_fb & 7) | al;
					pmdwork.fm3_alg_fb = dl;
				} else {
					dl = (qq->alg_fb & 7) | al;
				}
				opna.SetReg(pmdwork.fmsel + dh, dl);
				qq->alg_fb = dl;
				return si;
			}
		} else {
			al = 0;
			if(pmdwork.partb == 3 && pmdwork.fmsel == 0) {
				if((qq->slotmask & 0x10) == 0) return si;
				
				dl = (pmdwork.fm3_alg_fb & 7) | al;
				pmdwork.fm3_alg_fb = dl;
			} else {
				dl = (qq->alg_fb & 7) | al;
			}
			opna.SetReg(pmdwork.fmsel + dh, dl);
			qq->alg_fb = dl;
			return si;
		}
	}
}


//=============================================================================
//	COMMAND 't' [TEMPO CHANGE1]
//	COMMAND 'T' [TEMPO CHANGE2]
//	COMMAND 't��' [TEMPO CHANGE ����1]
//	COMMAND 'T��' [TEMPO CHANGE ����2]
//=============================================================================
uchar * PMDWIN::comt(uchar *si)
{
	int		al;
	
	al = *si++;
	if(al < 251) {
		open_work.tempo_d = al;		// T (FC)
		open_work.tempo_d_push = al;
		calc_tb_tempo();
	
	} else if(al == 0xff) {
		al = *si++;					// t (FC FF)
		if(al < 18) al = 18;
		open_work.tempo_48 = al;
		open_work.tempo_48_push = al;
		calc_tempo_tb();
	
	} else if(al == 0xfe) {
		al = char(*si++);			// T�� (FC FE)
		if(al >= 0) {
			al += open_work.tempo_d_push;
		} else {
			al += open_work.tempo_d_push;
			if(al < 0) {
				al = 0;
			}
		}

		if(al > 250) al = 250;
		
		open_work.tempo_d = al;
		open_work.tempo_d_push = al;
		calc_tb_tempo();
	
	} else {
		al = char(*si++);			// t�� (FC FD)
		if(al >= 0) {
			al += open_work.tempo_48_push;
			if(al > 255) {
				al = 255;
			}
		} else {
			al += open_work.tempo_48_push;
			if(al < 0) al = 18;
		}
		open_work.tempo_48 = al;
		open_work.tempo_48_push = al;
		calc_tempo_tb();
	}
	return si;
}


//=============================================================================
//	COMMAND '[' [�롼�� ��������]
//=============================================================================
uchar * PMDWIN::comstloop(QQ *qq, uchar *si)
{
	uchar *ax;
	
	if(qq == &EffPart) {
		ax = open_work.efcdat;
	} else {
		ax = open_work.mmlbuf;
	}
	ax[read_word(si)+1] = 0;
	si+=2;
	return si;
}


//=============================================================================
//	COMMAND	']' [�롼�� �����]
//=============================================================================
uchar * PMDWIN::comedloop(QQ *qq, uchar *si)
{
	int		ah, al, ax;
	ah = *si++;
	
	if(ah) {
		(*si)++;
		al = *si++;
		if(ah == al) {
			si+=2;
			return si;
		}
	} else {			// 0 �ʥ� �ॸ�祦���� �롼��
		si++;
		qq->loopcheck = 1;
	}
	
	ax = read_word(si) + 2;
	
	if(qq == &EffPart) {
		si = open_work.efcdat + ax;
	} else {
		si = open_work.mmlbuf + ax;
	}
	return si;
}


//=============================================================================
//	COMMAND	':' [�롼�� ���å����]
//=============================================================================
uchar * PMDWIN::comexloop(QQ *qq, uchar *si)
{
	uchar	*bx;
	int		dl;


	if(qq == &EffPart) {
		bx = open_work.efcdat;
	} else {
		bx = open_work.mmlbuf;
	}
	
	
	bx += read_word(si);
	si+=2;
	
	dl = *bx++ - 1;
	if(dl != *bx) return si;
	si = bx + 3;
	return si;
}


//=============================================================================
//	LFO �ѥ�᡼�� ���å�
//=============================================================================
uchar * PMDWIN::lfoset(QQ *qq, uchar *si)
{
	qq->delay = *si;
	qq->delay2 = *si++;
	qq->speed = *si;
	qq->speed2 = *si++;
	qq->step = read_char(si);
	qq->step2 = read_char(si++);
	qq->time = *si;
	qq->time2 = *si++;
	lfoinit_main(qq);
	return si;
}


//=============================================================================
//	LFO SWITCH
//=============================================================================
uchar * PMDWIN::lfoswitch(QQ *qq, uchar *si)
{
	int al;
	
	al = *si++;
	
	if(al & 0xf8) {
		al = 1;
	}
	al &= 7;
	qq->lfoswi = (qq->lfoswi & 0xf8) | al;
	lfoinit_main(qq);
	return si;
}


//=============================================================================
//	PSG ENVELOPE SET
//=============================================================================
uchar * PMDWIN::psgenvset(QQ *qq, uchar *si)
{
	qq->eenv_ar = *si; qq->eenv_arc = *si++;
	qq->eenv_dr = read_char(si++);
	qq->eenv_sr = *si; qq->eenv_src = *si++;
	qq->eenv_rr = *si; qq->eenv_rrc = *si++;
	if(qq->envf == -1) {
		qq->envf = 2;		// RR
		qq->eenv_volume = -15;		// volume
	}
	return si;
}


//=============================================================================
//	"\?" COMMAND [ OPNA Rhythm Keyon/Dump ]
//=============================================================================
uchar * PMDWIN::rhykey(uchar *si)
{
	int		al, dl;
	
	if((dl = (*si++ & open_work.rhythmmask)) == 0) {
		return si;
	}

	if(open_work.fadeout_volume) {
		al = ((256 - open_work.fadeout_volume) * open_work.rhyvol) >> 8;
		opna.SetReg(0x11, al);
	}
	
	if(dl < 0x80) {
		if(dl & 0x01) opna.SetReg(0x18, open_work.rdat[0]);
		if(dl & 0x02) opna.SetReg(0x19, open_work.rdat[1]);
		if(dl & 0x04) opna.SetReg(0x1a, open_work.rdat[2]);
		if(dl & 0x08) opna.SetReg(0x1b, open_work.rdat[3]);
		if(dl & 0x10) opna.SetReg(0x1c, open_work.rdat[4]);
		if(dl & 0x20) opna.SetReg(0x1d, open_work.rdat[5]);
	}
	
	opna.SetReg(0x10, dl);
	if(dl >= 0x80) {
		if(dl & 0x01) open_work.rdump_bd++;
		if(dl & 0x02) open_work.rdump_sd++;
		if(dl & 0x04) open_work.rdump_sym++;
		if(dl & 0x08) open_work.rdump_hh++;
		if(dl & 0x10) open_work.rdump_tom++;
		if(dl & 0x20) open_work.rdump_rim++;
		open_work.rshot_dat &= (~dl);
	} else {
		if(dl & 0x01) open_work.rshot_bd++;
		if(dl & 0x02) open_work.rshot_sd++;
		if(dl & 0x04) open_work.rshot_sym++;
		if(dl & 0x08) open_work.rshot_hh++;
		if(dl & 0x10) open_work.rshot_tom++;
		if(dl & 0x20) open_work.rshot_rim++;
		open_work.rshot_dat |= dl;
	}
	
	return si;
}


//=============================================================================
//	"\v?n" COMMAND
//=============================================================================
uchar * PMDWIN::rhyvs(uchar *si)
{
	int		*bx;
	int		dh, dl;
	
	dl = *si & 0x1f;
	dh = *si++ >> 5;
	bx = &open_work.rdat[dh-1];
	dh = 0x18 - 1 + dh;
	dl |= (*bx & 0xc0);
	*bx = dl;
	
	opna.SetReg(dh, dl);
	
	return si;
}


uchar * PMDWIN::rhyvs_sft(uchar *si)
{
	int		*bx;
	int		al, dh, dl;
	
	bx = &open_work.rdat[*si-1];
	dh = *si++ + 0x18 - 1;
	dl = *bx & 0x1f;
	
	al = (read_char(si++) + dl);
	if(al >= 32) {
		al = 31;
	} else if(al < 0) {
		al = 0;
	}
	
	dl = (al &= 0x1f);
	dl = *bx = ((*bx & 0xe0) | dl);
	opna.SetReg(dh, dl);
	
	return si;
}


//=============================================================================
//	"\p?" COMMAND
//=============================================================================
uchar * PMDWIN::rpnset(uchar *si)
{
	int		*bx;
	int		dh, dl;
	
	dl = (*si & 3) << 6;
	
	dh = (*si++ >> 5) & 0x07;
	bx = &open_work.rdat[dh-1];
	
	dh += 0x18 - 1;
	dl |= (*bx & 0x1f);
	*bx = dl;
	opna.SetReg(dh, dl);
	
	return si;
}


//=============================================================================
//	"\Vn" COMMAND
//=============================================================================
uchar * PMDWIN::rmsvs(uchar *si)
{
	int		dl;
	
	dl = *si++;
	if(open_work.rhythm_voldown) {
		dl = ((256 - open_work.rhythm_voldown) * dl) >> 8;
	}
	
	open_work.rhyvol = dl;
	
	if(open_work.fadeout_volume) {
		dl = ((256 - open_work.fadeout_volume) * dl) >> 8;
	}
	
	opna.SetReg(0x11, dl);
	
	return si;
}


uchar *PMDWIN::rmsvs_sft(uchar *si)
{
	int		dl;
	
	dl = open_work.rhyvol + read_char(si++);
	
	if(dl >= 64) {
		if(dl & 0x80) {
			dl = 0;
		} else {
			dl = 63;
		}
	}
	open_work.rhyvol = dl;
	
	if(open_work.fadeout_volume) {
		dl = ((256 - open_work.fadeout_volume) * dl) >> 8;
	}
	
	opna.SetReg(0x11, dl);
	
	return si;
}


//=============================================================================
//	�ܥ�塼��򼡤ΰ�Ĥ����ѹ��ʣ֣�������ĥʬ��
//=============================================================================
uchar * PMDWIN::vol_one_up_psg(QQ *qq, uchar *si)
{
	int		al;
	
	al = qq->volume + *si++;
	if(al > 15) al = 15;
	qq->volpush = ++al;
	pmdwork.volpush_flag = 1;
	return si;
}


uchar * PMDWIN::vol_one_down(QQ *qq, uchar *si)
{
	int		al;
	
	al = qq->volume - *si++;
	if(al < 0) {
		al = 0;
	} else {
		if(al >= 255) al = 254;
	}
	
	qq->volpush = ++al;
	pmdwork.volpush_flag = 1;
	return si;
}


//=============================================================================
//	�ݥ륿����(PSG)
//=============================================================================
uchar * PMDWIN::portap(QQ *qq, uchar *si)
{
	int		ax, al_, bx_;

	if(qq->partmask) {
		qq->fnum = 0;		//���������
		qq->onkai = 255;
		qq->leng = *(si+2);
		qq->keyon_flag++;
		qq->address = si+3;
		
		if(--pmdwork.volpush_flag) {
			qq->volpush = 0;
		}
				
		pmdwork.tieflag = 0;
		pmdwork.volpush_flag = 0;
		pmdwork.loop_work &= qq->loopcheck;
		return si+3;		// �ɤ����Ф�	(Mask��)
	}
	
	lfoinitp(qq, *si);
	fnumsetp(qq, oshiftp(qq, *si++));
	
	bx_ = qq->fnum;
	al_ = qq->onkai;
	fnumsetp(qq, oshiftp(qq, *si++));
	ax = qq->fnum; 			// ax = �ݥ륿�������psg_tune��
	
	qq->onkai = al_;
	qq->fnum = bx_;			// bx = �ݥ륿���ȸ���psg_tune��
	ax -= bx_;
	
	qq->leng = *si++;
	si = calc_q(qq, si);
	
	qq->porta_num2 = ax / qq->leng;		// ��
	qq->porta_num3 = ax % qq->leng;		// ;��
	qq->lfoswi |= 8;				// Porta ON
	
	if(qq->volpush && qq->onkai != 255) {
		if(--pmdwork.volpush_flag) {
			pmdwork.volpush_flag = 0;
			qq->volpush = 0;
		}
	}
	
	volsetp(qq);
	otodasip(qq);
	keyonp(qq);
	
	qq->keyon_flag++;
	qq->address = si;
	
	pmdwork.tieflag = 0;
	pmdwork.volpush_flag = 0;
	qq->keyoff_flag = 0;
	
	if(*si == 0xfb) {			// '&'��ľ��ˤ��ä���keyoff���ʤ�
		qq->keyoff_flag = 2;
	}
	
	pmdwork.loop_work &= qq->loopcheck;
	return si;
}


//=============================================================================
//	'w' COMMAND [PSG NOISE �إ����� ���奦�ϥ���]
//=============================================================================
uchar * PMDWIN::psgnoise_move(uchar *si)
{
	open_work.psnoi += read_char(si++);
	if(open_work.psnoi < 0) open_work.psnoi = 0;
	if(open_work.psnoi > 31) open_work.psnoi = 31;
	return si;
}


//=============================================================================
//	PSG Envelope set (Extend)
//=============================================================================
uchar * PMDWIN::extend_psgenvset(QQ *qq, uchar *si)
{
	qq->eenv_ar = *si++ & 0x1f;
	qq->eenv_dr = *si++ & 0x1f;
	qq->eenv_sr = *si++ & 0x1f;
	qq->eenv_rr = *si & 0x0f;
	qq->eenv_sl = ((*si++ >> 4) & 0x0f) ^ 0x0f;
	qq->eenv_al = *si++ & 0x0f;
	
	if(qq->envf != -1) {	// �Ρ��ޥ���ĥ�˰ܹԤ�������
		qq->envf = -1;
		qq->eenv_count = 4;		// RR
		qq->eenv_volume = 0;	// Volume
	}
	return si;
}


uchar * PMDWIN::mdepth_count(QQ *qq, uchar *si)
{
	int		al;
	
	al = *si++;
	
	if(al >= 0x80) {
		if((al &= 0x7f) == 0) al = 255;
		qq->_mdc = al;
		qq->_mdc2 = al;
		return si;
	}
	
	if(al == 0) al = 255;
	qq->mdc = al;
	qq->mdc2 = al;
	return si;
}


//=============================================================================
//	�̣ƣϤȣУӣǡ��УãͤΥ��եȥ���������٥��פν����
//=============================================================================
//=============================================================================
//	�Уӣǡ��УãͲ����ѡ�Entry
//=============================================================================
void PMDWIN::lfoinitp(QQ *qq, int al)
{
	int		ah;

	ah = al & 0x0f;
	
	if(ah == 0x0c) {
		al = qq->onkai_def;
		ah = al & 0x0f;
	}

	qq->onkai_def = al;

	if(ah == 0x0f) {		// ���塼�� �� �ȥ� �� INIT ���ʥ���
		lfo_exit(qq);
		return;
	}
	
	qq->porta_num = 0;				// �ݥ륿���ȤϽ����
	
	if(pmdwork.tieflag & 1) {
		lfo_exit(qq);
		return;
	}
	
	//------------------------------------------------------------------------
	//	���եȥ���������٥��׽����
	//------------------------------------------------------------------------
	if(qq->envf != -1) {
		qq->envf = 0;
		qq->eenv_volume = 0;
		qq->eenv_ar = qq->eenv_arc;
		
		if(qq->eenv_ar == 0) {
			qq->envf = 1;	// ATTACK=0 ... ���� Decay ��
			qq->eenv_volume = qq->eenv_dr;
		}
		
		qq->eenv_sr = qq->eenv_src;
		qq->eenv_rr = qq->eenv_rrc;
		lfin1(qq);
		
	} else {
		//	��ĥssg_envelope��
		
		qq->eenv_arc = qq->eenv_ar - 16;
		if(qq->eenv_dr < 16) {
			qq->eenv_drc = (qq->eenv_dr-16)*2;
		} else {
			qq->eenv_drc = qq->eenv_dr-16;
		}
		
		if(qq->eenv_sr < 16) {
			qq->eenv_src = (qq->eenv_sr-16)*2;
		} else {
			qq->eenv_src = qq->eenv_sr-16;
		}
		
		qq->eenv_rrc = (qq->eenv_rr) * 2 - 16;
		qq->eenv_volume = qq->eenv_al;
		qq->eenv_count = 1;
		ext_ssgenv_main(qq);
		lfin1(qq);
	}
}


void PMDWIN::lfo_exit(QQ *qq)
{
	if((qq->lfoswi & 3) != 0) {		// ���� & �ξ�� -> 1�� LFO����
		lfo(qq);
	}
	
	if((qq->lfoswi & 0x30) != 0) {	// ���� & �ξ�� -> 1�� LFO����
		lfo_change(qq);
		lfo(qq);
		lfo_change(qq);
	}
}


//=============================================================================
//	�̣ƣϽ����
//=============================================================================
void PMDWIN::lfin1(QQ *qq)
{
	qq->hldelay_c = qq->hldelay;

	if(qq->hldelay) {
		opna.SetReg(pmdwork.fmsel + pmdwork.partb+0xb4-1, (qq->fmpan) & 0xc0);
	}
	
	qq->sdelay_c = qq->sdelay;
	
	if(qq->lfoswi & 3) {	// LFO��̤����
		if((qq->lfoswi & 4) == 0) {	//keyon��Ʊ����?
			lfoinit_main(qq);
		}
		lfo(qq);
	}
	
	if(qq->lfoswi & 0x30) {	// LFO��̤����
		if((qq->lfoswi & 0x40) == 0) {	//keyon��Ʊ����?
			lfo_change(qq);
			lfoinit_main(qq);
			lfo_change(qq);
		}
		
		lfo_change(qq);
		lfo(qq);
		lfo_change(qq);
	}
}


void PMDWIN::lfoinit_main(QQ *qq)
{
	qq->lfodat = 0;
	qq->delay = qq->delay2;
	qq->speed = qq->speed2;
	qq->step = qq->step2;
	qq->time = qq->time2;
	qq->mdc = qq->mdc2;
	
	if(qq->lfo_wave == 2 || qq->lfo_wave == 3) {	// ����� or �������ȡ�
		qq->speed = 1;	// delayľ���LFO���ݤ���褦�ˤ���
	} else {
		qq->speed++;	// ����ʳ��ξ���delayľ���speed�ͤ� +1
	}
}


//=============================================================================
//	SHIFT[di] ʬ��Ĵ����
//=============================================================================
int PMDWIN::oshiftp(QQ *qq, int al)
{
	return oshift(qq, al);
}


int PMDWIN::oshift(QQ *qq, int al)
{
	int	bl, bh, dl;
	
	if(al == 0x0f) return al;

	if(al == 0x0c) {
		if((al = qq->onkai) >= 0x80) {
			return 0x0f;
		} else {
			return al;	//@�����б�
		}
	}

	dl = qq->shift + qq->shift_def;
	if(dl == 0) return al;
	
	bl = (al & 0x0f);		// bl = ONKAI
	bh = (al & 0xf0) >> 4;	// bh = OCT

	if(dl < 0) {
		// - �ۥ����� ���ե�
		if((bl += dl) < 0) {
			do {
				bh--;
			} while((bl+=12) < 0);
		}
		if(bh < 0) bh = 0;
		return (bh << 4) | bl;

	} else {
		// + �ۥ����� ���ե�
		bl += dl;
		while(bl >= 0x0c) {
			bh++;
			bl -= 12;
		}
		
		if(bh > 7) bh = 7;
		return (bh << 4) | bl;
	}
}


//=============================================================================
//	PSG TUNE SET
//=============================================================================
void PMDWIN::fnumsetp(QQ *qq, int al)
{
	int	ax, bx, cl;
	
	if((al & 0x0f) == 0x0f) {		// ���奦�� �ʥ� FNUM �� 0 �� ���å�
		qq->onkai = 255;
		if(qq->lfoswi & 0x11) return;
		qq->fnum = 0;	// ����LFO̤����
		return;
	}
	
	qq->onkai = al;
	
	cl = (al >> 4) & 0x0f;	// cl=oct
	bx = al & 0x0f;			// bx=onkai
	
	ax = psg_tune_data[bx];
	if(cl > 0) {
		ax = (ax + 1) >> cl;
	}
	
	qq->fnum = ax;
}


//=============================================================================
//	Q�ͤη׻�
//		break	dx
//=============================================================================
uchar * PMDWIN::calc_q(QQ *qq, uchar *si)
{
	int		ax, dh, dl;
	
	if(*si == 0xc1) {		// &&
		si++;
		qq->qdat = 0;
		return si;
	}
	
	dl = qq->qdata;
	if(qq->qdatb) {
		dl += (qq->leng * qq->qdatb) >> 8;
	}
	
	if(qq->qdat3) {		//	Random-Q
		ax = rand() % ((qq->qdat3 & 0x7f) + 1);
		if((qq->qdat3 & 0x80) == 0) {
			dl += ax;
		} else {
			dl -= ax;
			if(dl < 0) dl = 0;
		}
	}

	if(qq->qdat2) {
		if((dh = qq->leng - qq->qdat2) < 0) {
			qq->qdat = 0;
			return si;
		}
		
		if(dl < dh) {
			qq->qdat = dl;
		} else {
			qq->qdat = dh;
		}
	} else {
		qq->qdat = dl;
	}

	return si;
}


//=============================================================================
//	�Уӣǡ��֣ϣ̣գͣš��ӣţ�
//=============================================================================
void PMDWIN::volsetp(QQ *qq)
{
	int		ax, dl;
	
	if(qq->envf == 3 || (qq->envf == -1 && qq->eenv_count == 0)) return;

	if(qq->volpush) {
		dl = qq->volpush-1;
	} else {
		dl = qq->volume;
	}
	
	//------------------------------------------------------------------------
	//	����down�׻�
	//------------------------------------------------------------------------
	dl = ((256-open_work.ssg_voldown) * dl) >> 8;
	
	//------------------------------------------------------------------------
	//	Fadeout�׻�
	//------------------------------------------------------------------------
	dl = ((256 - open_work.fadeout_volume) * dl) >> 8;
	
	//------------------------------------------------------------------------
	//	ENVELOPE �׻�
	//------------------------------------------------------------------------
	if(dl <= 0) {
		opna.SetReg(pmdwork.partb+8-1, 0);
		return;
	}
	
	if(qq->envf == -1) {
		if(qq->eenv_volume == 0) {
			opna.SetReg(pmdwork.partb+8-1, 0);
			return;
		}
		dl = ((((dl * (qq->eenv_volume + 1))) >> 3) + 1) >> 1;
	} else {
		dl += qq->eenv_volume;
		if(dl <= 0) {
			opna.SetReg(pmdwork.partb+8-1, 0);
			return;
		}
		if(dl > 15) dl = 15;
	}

	//--------------------------------------------------------------------
	//	����LFO�׻�
	//--------------------------------------------------------------------
	if((qq->lfoswi & 0x22) == 0) {
		opna.SetReg(pmdwork.partb+8-1, dl);
		return;
	}
	
	if(qq->lfoswi & 2) {
		ax = qq->lfodat;
	} else {
		ax = 0;
	}
	
	if(qq->lfoswi & 0x20) {
		ax += qq->_lfodat;
	}
	
	dl = dl + ax;
	if(dl <  0) {
		opna.SetReg(pmdwork.partb+8-1, 0);
		return;
	}
	if(dl > 15) dl = 15;
	
	//------------------------------------------------------------------------
	//	����
	//------------------------------------------------------------------------
	opna.SetReg(pmdwork.partb+8-1, dl);
}


//=============================================================================
//	�Уӣǡ���������
//=============================================================================
void PMDWIN::otodasip(QQ *qq)
{
	int		ax, dx;
	
	if(qq->fnum == 0) return;

	// PSG Portament set
	
	ax = qq->fnum + qq->porta_num;
	dx = 0;

	// PSG Detune/LFO set
	if((qq->extendmode & 1) == 0) {
		ax -= qq->detune;
		if(qq->lfoswi & 1) {
			ax -= qq->lfodat;
		}
		
		if(qq->lfoswi & 0x10) {
			ax -= qq->_lfodat;
		}
	} else {
		// ��ĥDETUNE(DETUNE)�η׻�
		if(qq->detune) {
			dx = (ax * qq->detune) >> 12;		// dx:ax=ax * qq->detune
			if(dx >= 0) {
				dx++;
			} else {
				dx--;
			}
			ax -= dx;
		}
		// ��ĥDETUNE(LFO)�η׻�
		if(qq->lfoswi & 0x11) {
			if(qq->lfoswi & 1) {
				dx = qq->lfodat;
			} else {
				dx = 0;
			}
			
			if(qq->lfoswi & 0x10) {
				dx += qq->_lfodat;
			}
			
			if(dx) {
				dx = (ax * dx) >> 12;
				if(dx >= 0) {
					dx++;
				} else {
					dx--;
				}
			}
			ax -= dx;
		}
	}
	
	// TONE SET
	
	if(ax >= 0x1000) {
		if(ax >= 0) {
			ax = 0xfff;
		} else {
			ax = 0;
		}
	}
	
	opna.SetReg((pmdwork.partb-1) * 2, ax & 0xff);
	opna.SetReg((pmdwork.partb-1) * 2 + 1, ax >> 8);
}


//=============================================================================
//	�Уӣǡ��ˣţ٣ϣ�
//=============================================================================
void PMDWIN::keyonp(QQ *qq)
{
	int		ah, al;
	
	if(qq->onkai == 255) return;		// ���奦�� �� �ȥ�

	ah=(1 << (pmdwork.partb -1)) | (1 << (pmdwork.partb +2));
	al = opna.GetReg(0x07) | ah;
	ah = ~(ah & qq->psgpat);
	al &= ah;
	opna.SetReg(7, al);
	
	// PSG �Υ��� ���奦�ϥ��� �� ���å�
	
	if(open_work.psnoi != open_work.psnoi_last && effwork.effon == 0) {
		opna.SetReg(6, open_work.psnoi);
		open_work.psnoi_last = open_work.psnoi;
	}
}


//=============================================================================
//	�̣ƣϽ���
//		Don't Break cl
//		output		cy=1	�Ѳ������ä�
//=============================================================================
int PMDWIN::lfo(QQ *qq)
{
	return lfop(qq);
}

int PMDWIN::lfop(QQ *qq)
{
	int		ax, ch;
	
	if(qq->delay) {
		qq->delay--;
		return 0;
	}
	
	if(qq->extendmode & 2) {	// TimerA�ȹ�碌�뤫��
								// ��������ʤ��ʤ�̵����lfo����
		ch = open_work.TimerAtime - pmdwork.lastTimerAtime;
		if(ch == 0) return 0;
		ax = qq->lfodat;
		
		for(; ch > 0; ch--) {
			lfo_main(qq);
		}
	} else {
		
		ax = qq->lfodat;
		lfo_main(qq);
	}
	
	if(ax == qq->lfodat) {
		return 0;
	}
	return 1;
}


void PMDWIN::lfo_main(QQ *qq)
{
	int		al, ax;
	
	if(qq->speed != 1) {
		if(qq->speed != 255) qq->speed--;
		return;
	}
	
	qq->speed = qq->speed2;
	
	if(qq->lfo_wave == 0 || qq->lfo_wave == 4 || qq->lfo_wave == 5) {
		//	������		lfowave = 0,4,5
		if(qq->lfo_wave == 5) {
			ax = abs(qq->step) * qq->step;
		} else {
			ax = qq->step;
		}

		if((qq->lfodat += ax) == 0) {
			md_inc(qq);
		}
	
		al = qq->time;
		if(al != 255) {
			if(--al == 0) {
				al = qq->time2;
				if(qq->lfo_wave != 4) {
					al += al;	// lfowave=0,5�ξ�� time��ȿž�����ܤˤ���
				}
				qq->time = al;
				qq->step = -qq->step;
				return;
			}
		}
		qq->time = al;

	} else if(qq->lfo_wave == 2) {
		//	�����		lfowave = 2
		qq->lfodat = (qq->step * qq->time);
		md_inc(qq);
		qq->step = -qq->step;

	} else if(qq->lfo_wave == 6) {
		//	��󥷥�å�	lfowave = 6
		if(qq->time) {
			if(qq->time != 255) {
				qq->time--;
			}
			qq->lfodat += qq->step;
		}
	} else if(qq->lfo_wave == 1) {
		//�Υ�������	lfowave = 1
		qq->lfodat += qq->step;
		al = qq->time;
		if(al != -1) {		
			al--;
			if(al == 0)  {
				qq->lfodat = -qq->lfodat;
				md_inc(qq);
				al = (qq->time2) * 2;
			}
		}
		qq->time = al;

	} else {
		//	��������	lfowave = 3
		ax = abs(qq->step) * qq->time;
		qq->lfodat = ax - (rand() % (ax * 2));
		md_inc(qq);
	}
}


//=============================================================================
//	MD���ޥ�ɤ��ͤˤ�ä�STEP�ͤ��ѹ�
//=============================================================================
void PMDWIN::md_inc(QQ *qq)
{
	int		al;
		
	if(--qq->mdspd) return;
	
	qq->mdspd = qq->mdspd2;
	
	if(qq->mdc == 0) return;		// count = 0
	if(qq->mdc <= 127) {
		qq->mdc--;
	}

	if(qq->step < 0) {
		al = qq->mdepth - qq->step;
		if(al >= 0) {
			qq->step = -al;
		} else {
			if(qq->mdepth < 0) {
				qq->step = 0;
			} else {
				qq->step = -127;
			}
		}
	} else {
		al = qq->step + qq->mdepth;
		if(al >= 0) {
			qq->step = al;
		} else {
			if(qq->mdepth < 0) {
				qq->step = 0;
			} else {
				qq->step = 127;
			}
		}
	}
}


void PMDWIN::swap(int *a, int *b)
{
	int		temp;
	
	temp = *a;
	*a = *b;
	*b = temp;
}


//=============================================================================
//	LFO1<->LFO2 change
//=============================================================================
void PMDWIN::lfo_change(QQ *qq)
{
	swap(&qq->lfodat, &qq->_lfodat);
	qq->lfoswi = ((qq->lfoswi & 0x0f) << 4) + (qq->lfoswi >> 4);
	qq->extendmode = ((qq->extendmode & 0x0f) << 4) + (qq->extendmode >> 4);
	
	swap(&qq->delay, &qq->_delay);
	swap(&qq->speed, &qq->_speed);
	swap(&qq->step, &qq->_step);
	swap(&qq->time, &qq->_time);
	swap(&qq->delay2, &qq->_delay2);
	swap(&qq->speed2, &qq->_speed2);
	swap(&qq->step2, &qq->_step2);
	swap(&qq->time2, &qq->_time2);
	swap(&qq->mdepth, &qq->_mdepth);
	swap(&qq->mdspd, &qq->_mdspd);
	swap(&qq->mdspd2, &qq->_mdspd2);
	swap(&qq->lfo_wave, &qq->_lfo_wave);
	swap(&qq->mdc, &qq->_mdc);
	swap(&qq->mdc2, &qq->_mdc2);
}


//=============================================================================
//	�ݥ륿���ȷ׻��ʤΤ�
//=============================================================================
void PMDWIN::porta_calc(QQ *qq)
{
	qq->porta_num += qq->porta_num2;
	if(qq->porta_num3 == 0) return;
	if(qq->porta_num3 > 0) {
		qq->porta_num3--;
		qq->porta_num++;
	} else {
		qq->porta_num3++;
		qq->porta_num--;
	}
}


//=============================================================================
//	�Уӣǡ��УãͤΥ��եȥ���������٥���
//=============================================================================
int PMDWIN::soft_env(QQ *qq)
{
	int		i, cl;
	
	if(qq->extendmode & 4) {
		if(open_work.TimerAtime == pmdwork.lastTimerAtime) return 0;
		
		cl = 0;
		for(i = 0; i < open_work.TimerAtime - pmdwork.lastTimerAtime; i++) {
			if(soft_env_main(qq)) {
				cl = 1;
			}
		}
		return cl;
	} else {
		return soft_env_main(qq);
	}
}


int PMDWIN::soft_env_main(QQ *qq)
{
	int		dl;
	
	if(qq->envf == -1) {
		return ext_ssgenv_main(qq);
	}
	
	dl = qq->eenv_volume;
	soft_env_sub(qq);
	if(dl == qq->eenv_volume) {
		return 0;
	} else {
		return -1;
	}
}


int PMDWIN::soft_env_sub(QQ *qq)
{
	if(qq->envf == 0) {
		// Attack
		if(--qq->eenv_ar != 0) {
			return 0;
		}
		
		qq->envf = 1;
		qq->eenv_volume = qq->eenv_dr;
		return 1;
	}
	
	if(qq->envf != 2) {
		// Decay
		if(qq->eenv_sr == 0) return 0;	// �ģҡᣰ�λ��ϸ��ꤷ�ʤ�
		if(--qq->eenv_sr != 0) return 0;
		qq->eenv_sr = qq->eenv_src;
		qq->eenv_volume--;
		
		if(qq->eenv_volume >= -15 || qq->eenv_volume < 15) return 0;
		qq->eenv_volume = -15;
		return 0;
	}
	
	
	// Release
	if(qq->eenv_rr == 0) {				// �ңҡᣰ�λ��Ϥ����˲��ä�
		qq->eenv_volume = -15;
		return 0;
	}
	
	if(--qq->eenv_rr != 0) return 0;
	qq->eenv_rr = qq->eenv_rrc;
	qq->eenv_volume--;
	
	if(qq->eenv_volume >= -15 && qq->eenv_volume < 15) return 0;
	qq->eenv_volume = -15;
	return 0;
}


//	��ĥ��
int PMDWIN::ext_ssgenv_main(QQ *qq)
{
	int		dl;

	if(qq->eenv_count == 0) return 0;
	
	dl = qq->eenv_volume;
	esm_sub(qq, qq->eenv_count);
	
	if(dl == qq->eenv_volume) return 0;
	return -1;
}


void PMDWIN::esm_sub(QQ *qq, int ah)
{
	if(--ah == 0) {
		//	[[[ Attack Rate ]]]
		if(qq->eenv_arc > 0) {
			qq->eenv_volume += qq->eenv_arc;
			if(qq->eenv_volume < 15) {
				qq->eenv_arc = qq->eenv_ar-16;
				return;
			}
			
			qq->eenv_volume = 15;
			qq->eenv_count++;
			if(qq->eenv_sl != 15) return;		// SL=0�ξ��Ϥ���SR��
			qq->eenv_count++;
			return;
		} else {
			if(qq->eenv_ar == 0) return;
			qq->eenv_arc++;
			return;
		}
	}
	
	if(--ah == 0) {
		//	[[[ Decay Rate ]]]
		if(qq->eenv_drc > 0) {	// 0�ʲ��ξ��ϥ������CHECK
			qq->eenv_volume -= qq->eenv_drc;
			if(qq->eenv_volume < 0 || qq->eenv_volume < qq->eenv_sl) {
				qq->eenv_volume = qq->eenv_sl;
				qq->eenv_count++;
				return;
			}
			
			if(qq->eenv_dr < 16) {
				qq->eenv_drc = (qq->eenv_dr - 16) * 2;
			} else {
				qq->eenv_drc = qq->eenv_dr - 16;
			}
			return;
		}
		
		if(qq->eenv_dr == 0) return;
		qq->eenv_drc++;
		return;
	}
	
	if(--ah == 0) {
		//	[[[ Sustain Rate ]]]
		if(qq->eenv_src > 0) {	// 0�ʲ��ξ��ϥ������CHECK
			if((qq->eenv_volume -= qq->eenv_src) < 0) {
				qq->eenv_volume = 0;
			}
			
			if(qq->eenv_sr < 16) {
				qq->eenv_src = (qq->eenv_sr - 16) * 2;
			} else {
				qq->eenv_src = qq->eenv_sr - 16;
			}
			return;
		}
		
		if(qq->eenv_sr == 0) return;	// SR=0?
		qq->eenv_src++;
		return;
	}

	//	[[[ Release Rate ]]]
	if(qq->eenv_rrc > 0) {	// 0�ʲ��ξ��ϥ������CHECK
		if((qq->eenv_volume -= qq->eenv_rrc) < 0) {
			qq->eenv_volume = 0;
		}
		
		qq->eenv_rrc = (qq->eenv_rr) * 2 - 16;
		return;
	}
	
	if(qq->eenv_rr == 0) return;
	qq->eenv_rrc++;
}


//=============================================================================
//	�ƥ������
//=============================================================================
void PMDWIN::settempo_b(void)
{
	if(open_work.tempo_d != open_work.TimerB_speed) {
		open_work.TimerB_speed = open_work.tempo_d;
		opna.SetReg(0x26, open_work.TimerB_speed);
	}
}


//=============================================================================
//	����Υ������
//=============================================================================
void PMDWIN::syousetu_count(void)
{
	if(open_work.opncount + 1 == open_work.syousetu_lng) {
		open_work.syousetu++;
		open_work.opncount = 0;
	} else {
		open_work.opncount++;
	}
}


//=============================================================================
//	�ϣУγ����ߵ��Ľ���
//=============================================================================
void PMDWIN::opnint_start(void)
{
	memset(FMPart, 0, sizeof(FMPart));
	memset(SSGPart, 0, sizeof(SSGPart));
	memset(&ADPCMPart, 0, sizeof(ADPCMPart));
	memset(&RhythmPart, 0, sizeof(RhythmPart));
	memset(&DummyPart, 0, sizeof(DummyPart));
	memset(ExtPart, 0, sizeof(ExtPart));
	memset(&EffPart, 0, sizeof(EffPart));
	memset(PPZ8Part, 0, sizeof(PPZ8Part));
	
	open_work.rhythmmask = 255;
	pmdwork.rhydmy = 255;
	data_init();
	opn_init();
	
	opna.SetReg(0x07, 0xbf);
	mstop();
	setint();
	opna.SetReg(0x29, 0x83);
}


//=============================================================================
//	DATA AREA �� ���˥���饤��
//=============================================================================
void PMDWIN::data_init(void)
{
	int		i;
	int		partmask, keyon_flag;
	
	open_work.fadeout_volume = 0;
	open_work.fadeout_speed = 0;
	open_work.fadeout_flag = 0;
	open_work.fadeout2_speed = 0;

	for(i = 0; i < 6; i++) {
		partmask = FMPart[i].partmask;
		keyon_flag = FMPart[i].keyon_flag;
		memset(&FMPart[i], 0, sizeof(QQ));
		FMPart[i].partmask = partmask & 0x0f;
		FMPart[i].keyon_flag = keyon_flag;
		FMPart[i].onkai = 255;
		FMPart[i].onkai_def = 255;
	}
	
	for(i = 0; i < 3; i++) {
		partmask = SSGPart[i].partmask;
		keyon_flag = SSGPart[i].keyon_flag;
		memset(&SSGPart[i], 0, sizeof(QQ));
		SSGPart[i].partmask = partmask & 0x0f;
		SSGPart[i].keyon_flag = keyon_flag;
		SSGPart[i].onkai = 255;
		SSGPart[i].onkai_def = 255;
	}
	
	partmask = ADPCMPart.partmask;
	keyon_flag = ADPCMPart.keyon_flag;
	memset(&ADPCMPart, 0, sizeof(QQ));
	ADPCMPart.partmask = partmask & 0x0f;
	ADPCMPart.keyon_flag = keyon_flag;
	ADPCMPart.onkai = 255;
	ADPCMPart.onkai_def = 255;
	
	partmask = RhythmPart.partmask;
	keyon_flag = RhythmPart.keyon_flag;
	memset(&RhythmPart, 0, sizeof(QQ));
	RhythmPart.partmask = partmask & 0x0f;
	RhythmPart.keyon_flag = keyon_flag;
	RhythmPart.onkai = 255;
	RhythmPart.onkai_def = 255;

	for(i = 0; i < 3; i++) {
		partmask = ExtPart[i].partmask;
		keyon_flag = ExtPart[i].keyon_flag;
		memset(&ExtPart[i], 0, sizeof(QQ));
		ExtPart[i].partmask = partmask & 0x0f;
		ExtPart[i].keyon_flag = keyon_flag;
		ExtPart[i].onkai = 255;
		ExtPart[i].onkai_def = 255;
	}
	
	for(i = 0; i < 8; i++) {
		partmask = PPZ8Part[i].partmask;
		keyon_flag = PPZ8Part[i].keyon_flag;
		memset(&PPZ8Part[i], 0, sizeof(QQ));
		PPZ8Part[i].partmask = partmask & 0x0f;
		PPZ8Part[i].keyon_flag = keyon_flag;
		PPZ8Part[i].onkai = 255;
		PPZ8Part[i].onkai_def = 255;
	}

	pmdwork.tieflag = 0;
	open_work.status = 0;
	open_work.status2 = 0;
	open_work.syousetu = 0;
	open_work.opncount = 0;
	open_work.TimerAtime = 0;
	pmdwork.lastTimerAtime = 0;

	pmdwork.omote_key[0] = 0;
	pmdwork.omote_key[1] = 0;
	pmdwork.omote_key[2] = 0;
	pmdwork.ura_key[0] = 0;
	pmdwork.ura_key[1] = 0;
	pmdwork.ura_key[2] = 0;

	pmdwork.fm3_alg_fb = 0;
	pmdwork.af_check = 0;

	open_work.pcmstart = 0;
	open_work.pcmstop = 0;
	pmdwork.pcmrepeat1 = 0;
	pmdwork.pcmrepeat2 = 0;
	pmdwork.pcmrelease = 0x8000;

	open_work.kshot_dat = 0;
	open_work.rshot_dat = 0;
	effwork.last_shot_data = 0;

	pmdwork.slotdetune_flag = 0;
	open_work.slot_detune1 = 0;
	open_work.slot_detune2 = 0;
	open_work.slot_detune3 = 0;
	open_work.slot_detune4 = 0;
	
	pmdwork.slot3_flag = 0;
	open_work.ch3mode = 0x3f;

	pmdwork.fmsel = 0;

	open_work.syousetu_lng = 96;

	open_work.fm_voldown = open_work._fm_voldown;
	open_work.ssg_voldown = open_work._ssg_voldown;
	open_work.pcm_voldown = open_work._pcm_voldown;
	open_work.ppz_voldown = open_work._ppz_voldown; 
	open_work.rhythm_voldown = open_work._rhythm_voldown;
	open_work.pcm86_vol = open_work._pcm86_vol;
}


//=============================================================================
//	OPN INIT
//=============================================================================
void PMDWIN::opn_init(void)
{
	int		i;
	
	opna.ClearBuffer();
	opna.SetReg(0x29, 0x83);
	
	open_work.psnoi = 0;
//@	if(effwork.effon == 0) {
		opna.SetReg(0x06, 0x00);
		open_work.psnoi_last = 0;
//@	}
	
	//------------------------------------------------------------------------
	//	PAN/HARDLFO DEFAULT
	//------------------------------------------------------------------------
	opna.SetReg(0xb4, 0xc0);
	opna.SetReg(0xb5, 0xc0);
	opna.SetReg(0xb6, 0xc0);
	opna.SetReg(0x1b4, 0xc0);
	opna.SetReg(0x1b5, 0xc0);
	opna.SetReg(0x1b6, 0xc0);
	
	open_work.port22h = 0x00;
	opna.SetReg(0x22, 0x00);
	
	//------------------------------------------------------------------------
	//	Rhythm Default = Pan : Mid , Vol : 15
	//------------------------------------------------------------------------
	for(i = 0; i < 6; i++) {
		open_work.rdat[i] = 0xcf;
	}
	opna.SetReg(0x10, 0xff);
	
	//------------------------------------------------------------------------
	//	�ꥺ��ȡ������٥롡���å�
	//------------------------------------------------------------------------
	open_work.rhyvol = 48*4*(256-open_work.rhythm_voldown)/1024;
	opna.SetReg(0x11, open_work.rhyvol);
	
	//------------------------------------------------------------------------
	//	�Уã͡�reset & �̣ɣͣɣԡ��ӣţ�
	//------------------------------------------------------------------------
	opna.SetReg(0x10c, 0xff);
	opna.SetReg(0x10d, 0xff);

	for(i = 0; i < PCM_CNL_MAX; i++) {
		ppz8.SetPan(i, 5);
	}
}


//=============================================================================
//	�ͣգӣɣá��ӣԣϣ�
//=============================================================================
void PMDWIN::mstop_f(void)
{
	if(open_work.TimerAflag || open_work.TimerBflag) {
		pmdwork.music_flag |= 2;
	} else {
		open_work.fadeout_flag = 0;
		mstop();
	}

	memset(wavbuf2, 0, sizeof(wavbuf2));
	upos = 0;
}


void PMDWIN::mstop(void)
{
	pmdwork.music_flag &= 0xfd;
	open_work.play_flag = 0;
	open_work.fadeout_speed = 0;
	open_work.status2 = -1;
	open_work.fadeout_volume = 0xff;
	silence();
}


//=============================================================================
//	ALL SILENCE
//=============================================================================
void PMDWIN::silence(void)
{
	int		i;
	
	opna.SetReg(0x80, 0xff);			// FM Release = 15
	opna.SetReg(0x81, 0xff);
	opna.SetReg(0x82, 0xff);
	opna.SetReg(0x84, 0xff);
	opna.SetReg(0x85, 0xff);
	opna.SetReg(0x86, 0xff);
	opna.SetReg(0x88, 0xff);
	opna.SetReg(0x89, 0xff);
	opna.SetReg(0x8a, 0xff);
	opna.SetReg(0x8c, 0xff);
	opna.SetReg(0x8d, 0xff);
	opna.SetReg(0x8e, 0xff);
	
	opna.SetReg(0x180, 0xff);
	opna.SetReg(0x181, 0xff);
	opna.SetReg(0x184, 0xff);
	opna.SetReg(0x185, 0xff);
	opna.SetReg(0x188, 0xff);
	opna.SetReg(0x189, 0xff);
	opna.SetReg(0x18c, 0xff);
	opna.SetReg(0x18d, 0xff);
	
	opna.SetReg(0x182, 0xff);
	opna.SetReg(0x186, 0xff);
	opna.SetReg(0x18a, 0xff);
	opna.SetReg(0x18e, 0xff);
	
	opna.SetReg(0x28, 0x00);			// FM KEYOFF
	opna.SetReg(0x28, 0x01);
	opna.SetReg(0x28, 0x02);
	opna.SetReg(0x28, 0x04);			// FM KEYOFF [URA]
	opna.SetReg(0x28, 0x05);
	opna.SetReg(0x28, 0x06);
	
	ppsdrv.Stop();
	p86drv.Stop();

	if(effwork.effon == 0) {
		opna.SetReg(0x07, 0xbf);
	} else {
		opna.SetReg(0x07, (opna.GetReg(0x07) & 0x3f) | 0x9b);
	}
	
	opna.SetReg(0x10, 0xff);			// Rhythm dump

	opna.SetReg(0x101, 0x02);			// PAN=0 / x8 bit mode
	opna.SetReg(0x100, 0x01);			// PCM RESET
	opna.SetReg(0x110, 0x80);			// TA/TB/EOS �� RESET
	opna.SetReg(0x110, 0x18);			// TIMERB/A/EOS�Τ�bit�Ѳ�����

	for(i = 0; i < PCM_CNL_MAX; i++) {
		ppz8.Stop(i);
	}
}


//=============================================================================
//	���ճ���(function)
//=============================================================================
void PMDWIN::mstart_f(void)
{
	if(open_work.TimerAflag || open_work.TimerBflag) {
		pmdwork.music_flag |= 1;			//	TA/TB������� �¹Ԥ��ʤ�
		return;
	}

	mstart();
}


//=============================================================================
//	���ճ���
//=============================================================================
void PMDWIN::mstart(void)
{
	// TimerB = 0 �����ꤷ��Timer Reset(�ʤ�Ĺ������󤽤��뤿��)
	open_work.tempo_d = 0;
	settempo_b();	
	opna.SetReg(0x27, 0);	// TIMER RESET(timerA,B�Ȥ�)

	//------------------------------------------------------------------------
	//	�������
	//------------------------------------------------------------------------
	pmdwork.music_flag &= 0xfe;
	mstop();

	//------------------------------------------------------------------------
	//	�Хåե������
	//------------------------------------------------------------------------
	pos2 = (char *)wavbuf2;	// buf ��;�äƤ��륵��ץ����Ƭ����
	us2	= 0;				// buf ��;�äƤ��륵��ץ��
	upos = 0;				// ���ճ��Ϥ���λ���(��sec)
	
	//------------------------------------------------------------------------
	//	���ս���
	//------------------------------------------------------------------------
	data_init();
	play_init();

	//------------------------------------------------------------------------
	//	OPN�����
	//------------------------------------------------------------------------
	opn_init();

	//------------------------------------------------------------------------
	//	���ڤα��դ򳫻�
	//------------------------------------------------------------------------
	setint();
	open_work.play_flag = 1;
}


ushort PMDWIN::read_word(void *value)
{
	ushort temp;
	
	memcpy(&temp,value,sizeof(ushort));
		
	return temp;
}
short PMDWIN::read_short(void *value)
{
	short temp;
	
	memcpy(&temp,value,sizeof(short));
		
	return temp;
}

long PMDWIN::read_long(void *value)
{
	long temp;
	
	memcpy(&temp,value,sizeof(long));
		
	return temp;
}

int PMDWIN::read_char(void *value)
{
	int temp;
	
	if ((*(uchar *)value) & 0x80)
		temp = -1;
	else
		temp = 0;

	memcpy(&temp,value,sizeof(char));
		
	return temp;
}


//=============================================================================
//	�ƥѡ��ȤΥ������ȥ��ɥ쥹�ڤӽ���ͤ򥻥å�
//=============================================================================
void PMDWIN::play_init(void)
{
	int		i;
	ushort	addr,*p;
	uchar tmp;
	
	open_work.x68_flg = *(open_work.mmlbuf-1);
	
	//�������ɲ�ʬ
	if(*open_work.mmlbuf != 2*(max_part2+1)) {
		addr =  read_word(&open_work.mmlbuf[2*(max_part2+1)]);
		open_work.prgdat_adr = open_work.mmlbuf + addr;
		open_work.prg_flg = 1;
	} else {
		open_work.prg_flg = 0;
	}
	
	p = (ushort *)open_work.mmlbuf;

	//	Part 0,1,2,3,4,5(FM1��6)�λ�
	for(i = 0; i < 6; i++) { // word�����˥Х�
		addr = read_word(p);
		tmp = open_work.mmlbuf[addr];
		if (tmp == 0x80) {		//��Ƭ��80h�ʤ���դ��ʤ�
			FMPart[i].address = NULL;
		} else {
			FMPart[i].address = &open_work.mmlbuf[read_word(p)];
		}
		
		FMPart[i].leng = 1;
		FMPart[i].keyoff_flag = -1;		// ����keyoff��
		FMPart[i].mdc = -1;				// MDepth Counter (̵��)
		FMPart[i].mdc2 = -1;			// Ʊ��
		FMPart[i]._mdc = -1;			// Ʊ��
		FMPart[i]._mdc2 = -1;			// Ʊ��
		FMPart[i].onkai = 255;			// rest
		FMPart[i].onkai_def = 255;		// rest
		FMPart[i].volume = 108;			// FM  VOLUME DEFAULT= 108
		FMPart[i].fmpan = 0xc0;			// FM PAN = Middle
		FMPart[i].slotmask = 0xf0;		// FM SLOT MASK
		FMPart[i].neiromask = 0xff;		// FM Neiro MASK
		p++;
	}
	
	//	Part 6,7,8(PSG1��3)�λ�
	for(i = 0; i < 3; i++) {
		if(open_work.mmlbuf[read_word(p)] == 0x80) {		//��Ƭ��80h�ʤ���դ��ʤ�
			SSGPart[i].address = NULL;
		} else {
			SSGPart[i].address = &open_work.mmlbuf[read_word(p)];
		}
		
		SSGPart[i].leng = 1;
		SSGPart[i].keyoff_flag = -1;	// ����keyoff��
		SSGPart[i].mdc = -1;			// MDepth Counter (̵��)
		SSGPart[i].mdc2 = -1;			// Ʊ��
		SSGPart[i]._mdc = -1;			// Ʊ��
		SSGPart[i]._mdc2 = -1;			// Ʊ��
		SSGPart[i].onkai = 255;			// rest
		SSGPart[i].onkai_def = 255;		// rest
		SSGPart[i].volume = 8;			// PSG VOLUME DEFAULT= 8
		SSGPart[i].psgpat = 7;			// PSG = TONE
		SSGPart[i].envf = 3;			// PSG ENV = NONE/normal
		p++;

}
	
	//	Part 9(OPNA/ADPCM)�λ�
	if(open_work.mmlbuf[read_word(p)] == 0x80) {		//��Ƭ��80h�ʤ���դ��ʤ�
		ADPCMPart.address = NULL;
	} else {
		ADPCMPart.address = &open_work.mmlbuf[read_word(p)];
	}
	
	ADPCMPart.leng = 1;
	ADPCMPart.keyoff_flag = -1;		// ����keyoff��
	ADPCMPart.mdc = -1;				// MDepth Counter (̵��)
	ADPCMPart.mdc2 = -1;			// Ʊ��
	ADPCMPart._mdc = -1;			// Ʊ��
	ADPCMPart._mdc2 = -1;			// Ʊ��
	ADPCMPart.onkai = 255;			// rest
	ADPCMPart.onkai_def = 255;		// rest
	ADPCMPart.volume = 128;			// PCM VOLUME DEFAULT= 128
	ADPCMPart.fmpan = 0xc0;			// PCM PAN = Middle
	p++;

	//	Part 10(Rhythm)�λ�
	if(open_work.mmlbuf[read_word(p)] == 0x80) {		//��Ƭ��80h�ʤ���դ��ʤ�
		RhythmPart.address = NULL;
	} else {
		RhythmPart.address = &open_work.mmlbuf[read_word(p)];
	}
	
	RhythmPart.leng = 1;
	RhythmPart.keyoff_flag = -1;	// ����keyoff��
	RhythmPart.mdc = -1;			// MDepth Counter (̵��)
	RhythmPart.mdc2 = -1;			// Ʊ��
	RhythmPart._mdc = -1;			// Ʊ��
	RhythmPart._mdc2 = -1;			// Ʊ��
	RhythmPart.onkai = 255;			// rest
	RhythmPart.onkai_def = 255;		// rest
	RhythmPart.volume = 15;			// PPSDRV volume
	p++;
	
	//------------------------------------------------------------------------
	//	Rhythm �Υ��ɥ쥹�ơ��֥�򥻥å�
	//------------------------------------------------------------------------
	
	open_work.radtbl = (ushort *)&open_work.mmlbuf[read_word(p)];
	open_work.rhyadr = (uchar *)&pmdwork.rhydmy;
}


//=============================================================================
//	���󥿥�ץȡ�����
//	FM��������
//=============================================================================
void PMDWIN::setint(void)
{
	//
	// �ϣУγ����߽������
	//
	
	open_work.tempo_d = 200;
	open_work.tempo_d_push = 200;
	
	calc_tb_tempo();
	settempo_b();
	
	opna.SetReg(0x25, 0x00);			// TIMER A SET (9216��s����)
	opna.SetReg(0x24, 0x00);			// �����٤������٤���
	opna.SetReg(0x27, 0x3f);			// TIMER ENABLE
	
	//
	//�����ᥫ���󥿥ꥻ�å�
	//
	
	open_work.opncount = 0;
	open_work.syousetu = 0;
	open_work.syousetu_lng = 96;
}


//=============================================================================
//	T->t �Ѵ�
//		input	[tempo_d]
//		output	[tempo_48]
//=============================================================================
void PMDWIN::calc_tb_tempo(void)
{
//	TEMPO = 0x112C / [ 256 - TB ]	timerB -> tempo
	int temp;
	
	if(256 - open_work.tempo_d == 0) {
		temp = 255;
	} else {
		temp = (0x112c * 2 / (256 - open_work.tempo_d) + 1) / 2;
		if(temp > 255) temp = 255;
	}
	
	open_work.tempo_48 = temp;
	open_work.tempo_48_push = temp;
}


//=============================================================================
//	t->T �Ѵ�
//		input	[tempo_48]
//		output	[tempo_d]
//=============================================================================
void PMDWIN::calc_tempo_tb(void)
{
	int		al;

	//	TB = 256 - [ 112CH / TEMPO ]	tempo -> timerB
	
	if(open_work.tempo_48 >= 18) {
		al = 256 - 0x112c / open_work.tempo_48;
		if(0x112c % open_work.tempo_48 >= 128) {
			al--;
		}
		//al = 256 - (0x112c * 2 / open_work.tempo_48 + 1) / 2;
	} else {
		al = 0;
	}
	open_work.tempo_d = al;
	open_work.tempo_d_push = al;
}


//=============================================================================
//	�Уãͥ��꤫��ᥤ�����ؤΥǡ���������
//
//	INPUTS 	.. pcmstart   	to Start Address
//			.. pcmstop    	to Stop  Address
//			.. buf			to PCMDATA_Buffer
//=============================================================================
void PMDWIN::pcmread(ushort pcmstart, ushort pcmstop, uchar *buf)
{
	int		i;
	
	opna.SetReg(0x100, 0x01);
	opna.SetReg(0x110, 0x00);
	opna.SetReg(0x110, 0x80);
	opna.SetReg(0x100, 0x20);
	opna.SetReg(0x101, 0x02);		// x8
	opna.SetReg(0x10c, 0xff);
	opna.SetReg(0x10d, 0xff);
	opna.SetReg(0x102, pcmstart & 0xff);
	opna.SetReg(0x103, pcmstart >> 8);
	opna.SetReg(0x104, 0xff);
	opna.SetReg(0x105, 0xff);

	*buf = opna.GetReg(0x108);		// ̵���ɤ�
	*buf = opna.GetReg(0x108);		// ̵���ɤ�

	for(i = 0; i < (pcmstop - pcmstart) * 32; i++) {
		*buf++ = opna.GetReg(0x108);
		opna.SetReg(0x110, 0x80);
	}
}


//=============================================================================
//	�Уãͥ���إᥤ����꤫��ǡ��������� (x8,��®��)
//
//	INPUTS 	.. pcmstart   	to Start Address
//			.. pcmstop    	to Stop  Address
//			.. buf			to PCMDATA_Buffer
//=============================================================================
void PMDWIN::pcmstore(ushort pcmstart, ushort pcmstop, uchar *buf)
{
	int		i;
	
	opna.SetReg(0x100, 0x01);
//	opna.SetReg(0x110, 0x17);	// brdy�ʳ��ϥޥ���(=timer�����ߤϳݤ���ʤ�)
	opna.SetReg(0x110, 0x80);
	opna.SetReg(0x100, 0x60);
	opna.SetReg(0x101, 0x02);	// x8
	opna.SetReg(0x10c, 0xff);
	opna.SetReg(0x10d, 0xff);
	opna.SetReg(0x102, pcmstart & 0xff);
	opna.SetReg(0x103, pcmstart >> 8);
	opna.SetReg(0x104, 0xff);
	opna.SetReg(0x105, 0xff);
	
	for(i = 0; i < (pcmstop - pcmstart) * 32; i++) {
		opna.SetReg(0x108, *buf++);
	}
}


//=============================================================================
//	PCM �򸡺��ǥ��쥯�ȥ꤫�鸡��
//		input
//			filename	: ��������ե�����(�ե�����̾��ʬ�Τ�)
//			currentdir	: �ǽ�˸��������꤬����ʤ����
//						  open_work.pcmdir ���ͥ�褹��
//						  NULL �ʤ�̵�� 
//		output			: ������̡ʥե�ѥ�)
//						  ���Ĥ���ʤ��ä��� NULL
//=============================================================================
char * PMDWIN::search_pcm(char *dest, const char *filename, const char *current_dir)
{
	int		i;
	char	fullpcmfilename[_MAX_PATH];

	if(current_dir != NULL) {
		strcpy(fullpcmfilename, current_dir);
		if((char *)_mbsrchr((uchar *)current_dir, PATH_DELIMIT_CHR) != &current_dir[strlen(current_dir)-1]) {
			strcat(fullpcmfilename, PATH_DELIMIT_STR);
		}
		strcat(fullpcmfilename, filename);
		
		if(GetFileSize_s(fullpcmfilename) >= 0) {
			strcpy(dest, fullpcmfilename);
			return dest;
		}
	}

	i = -1;
	do {
		i++;
		if(open_work.pcmdir[i][0] == '\0') {
			*dest = '\0';
			return dest;
		}

		strcpy(fullpcmfilename, open_work.pcmdir[i]);
		strcat(fullpcmfilename, filename);
	}while(GetFileSize_s(fullpcmfilename) < 0);

	strcpy(dest, fullpcmfilename);
	return dest;
}


//=============================================================================
//	�ʤ��ɤ߹��ߤ��Σ��ʥ��꤫�顢�����ȥǥ��쥯�ȥꤢ���
//=============================================================================
int PMDWIN::music_load3(uchar *musdata, int size, char *current_dir)
{
	int		resultppc, resultpps, resultz1, resultz2;
	char	pcmfilename[_MAX_PATH];
	char	ppzfilename1[_MAX_PATH];
	char	ppzfilename2[_MAX_PATH];
	char	fullfilename[_MAX_PATH];
	char	drive[_MAX_DRIVE];
	char	dir[_MAX_DIR];
	char	fname[_MAX_FNAME];
	char	ext[_MAX_EXT];
	char	*p;

	if(size > sizeof(mdataarea)) {
		return ERR_WRONG_MUSIC_FILE;
	}
	
	if(musdata[0] > 0x0f || (musdata[1] != 0x18 && musdata[1] != 0x1a) || musdata[2]) {
		return ERR_WRONG_MUSIC_FILE;		// not PMD data
	}
	
	mstop_f();

	// ����� 0x80(��λ������)�ǽ��������
	memset(mdataarea, 0x80, sizeof(mdataarea));

	memcpy(mdataarea, musdata, size);
	memset(open_work.mus_filename, 0x0, sizeof(open_work.mus_filename));

	// PPC/P86 �ɤ߹���
	getmemo(pcmfilename, musdata, size, 0);
	resultppc = PMDWIN_OK;

	if(*pcmfilename != '\0') {
		_splitpath(pcmfilename, drive, dir, fname, ext);
		if(strcmp(ext, ".P86") == 0 || strcmp(ext, ".p86") == 0) {
																// P86 -> PPC
			search_pcm(fullfilename, pcmfilename, current_dir);
			resultppc = p86drv.Load(fullfilename);
			if(resultppc == _P86DRV_OK || resultppc == _WARNING_P86_ALREADY_LOAD) {
				open_work.use_p86 = true;
			} else {
				strcpy(ext, ".PPC");
				_makepath(pcmfilename, drive, dir, fname, ext);
				search_pcm(fullfilename, pcmfilename, current_dir);
				resultppc = ppc_load2(fullfilename);
				if(resultppc == PMDWIN_OK || resultppc == WARNING_PPC_ALREADY_LOAD) {
					open_work.use_p86 = false;
				} else {
					if(resultppc == ERR_OPEN_PPC_FILE) {
						return ERR_OPEN_P86_FILE;
					} else {
						return resultppc;
					}
				}
			}
		} else {
																// PPC -> P86
			search_pcm(fullfilename, pcmfilename, current_dir);
			resultppc = ppc_load2(fullfilename);
			if(resultppc == PMDWIN_OK || resultppc == WARNING_PPC_ALREADY_LOAD) {
				open_work.use_p86 = false;
			} else {
				strcpy(ext, ".P86");
				_makepath(pcmfilename, drive, dir, fname, ext);
				search_pcm(fullfilename, pcmfilename, current_dir);
				resultppc = p86drv.Load(fullfilename);
				if(resultppc == _P86DRV_OK || resultppc == _WARNING_P86_ALREADY_LOAD) {
					open_work.use_p86 = true;
				} else {
					switch(resultppc) {
						case _P86DRV_OK:
						case _WARNING_P86_ALREADY_LOAD:	break;
						case _ERR_OPEN_P86_FILE:		return ERR_OPEN_PPC_FILE;
						case _ERR_WRONG_P86_FILE:		return ERR_WRONG_P86_FILE;
						case _ERR_OUT_OF_MEMORY:		return ERR_OUT_OF_MEMORY;
						default:						return ERR_OTHER;
					}
				}
			}
		}
	}
	
	// PPS �ɤ߹���
	getmemo(pcmfilename, musdata, size, -1);
	resultpps = PMDWIN_OK;
	if(*pcmfilename != '\0') {
		search_pcm(fullfilename, pcmfilename, current_dir);
		resultpps = ppsdrv.Load(fullfilename);
		switch(resultpps) {
			case _PPSDRV_OK:
			case _WARNING_PPS_ALREADY_LOAD:	break;
			case _ERR_OPEN_PPS_FILE:		return ERR_OPEN_PPS_FILE;
			case _ERR_WRONG_PPS_FILE:		return ERR_WRONG_PPS_FILE;
			case _ERR_OUT_OF_MEMORY:		return ERR_OUT_OF_MEMORY;
			default:						return ERR_OTHER;
		}
	}

	// PPZ �ɤ߹���
	getmemo(ppzfilename1, musdata, size, -2);
	if(*ppzfilename1 != '\0') {
		if((p = strchr(ppzfilename1, ',')) == NULL) {	// ���ĤΤ�
			if((p = strchr(ppzfilename1, '.')) == NULL) {
				strcat(ppzfilename1, ".PZI");			// �Ȥꤢ���� pzi �ˤ���
			}
			
			search_pcm(fullfilename, ppzfilename1, current_dir); 
			resultz1 = ppz8.Load(fullfilename, 0);
			switch(resultz1) {
				case _PPZ8_OK:					return resultppc;
				case _ERR_OPEN_PPZ_FILE:		return ERR_OPEN_PPZ1_FILE;
				case _ERR_WRONG_PPZ_FILE: 		return ERR_WRONG_PPZ1_FILE;
				case _WARNING_PPZ_ALREADY_LOAD:	return WARNING_PPZ1_ALREADY_LOAD;
				case _ERR_OUT_OF_MEMORY:		return ERR_OUT_OF_MEMORY;
				default:						return ERR_OTHER;
			}
		} else {
			*p = '\0';
			strcpy(ppzfilename2, p + 1);

			if((p = strchr(ppzfilename1, '.')) == NULL)
			strcat(ppzfilename1, ".PZI");				// �Ȥꤢ���� pzi �ˤ���

			if((p = strchr(ppzfilename2, '.')) == NULL)
			strcat(ppzfilename2, ".PZI");				// �Ȥꤢ���� pzi �ˤ���

			search_pcm(fullfilename, ppzfilename1, current_dir); 
			resultz1 = ppz8.Load(fullfilename, 0);

			search_pcm(fullfilename, ppzfilename2, current_dir); 
			resultz2 = ppz8.Load(fullfilename, 1);
			if(resultz1 != PMDWIN_OK) {
				switch(resultz1) {
					case _ERR_OPEN_PPZ_FILE:		return ERR_OPEN_PPZ1_FILE;
					case _ERR_OUT_OF_MEMORY:		return ERR_OUT_OF_MEMORY;
					case _ERR_WRONG_PPZ_FILE: 		return ERR_WRONG_PPZ1_FILE;
					case _WARNING_PPZ_ALREADY_LOAD:	return WARNING_PPZ1_ALREADY_LOAD;
					default:						return ERR_OTHER;
				}
			}
			
			switch(resultz2) {
				case _PPZ8_OK:					return PMDWIN_OK;
				case _ERR_OPEN_PPZ_FILE:		return ERR_OPEN_PPZ2_FILE;
				case _ERR_OUT_OF_MEMORY:		return ERR_OUT_OF_MEMORY;
				case _ERR_WRONG_PPZ_FILE: 		return ERR_WRONG_PPZ2_FILE;
				case _WARNING_PPZ_ALREADY_LOAD:	return WARNING_PPZ2_ALREADY_LOAD;
				default:						return ERR_OTHER;
			}
		}
	}

	return PMDWIN_OK;
}


//=============================================================================
//	ppc_load������������
//=============================================================================
int PMDWIN::ppc_load2(char *filename)
{
	uchar	*pcmbuf;
	FILE	*fp;
	int		size, result;

	if((fp = fopen(filename, "rb")) == NULL) {
		return ERR_OPEN_PPC_FILE;
	}

	size = (int)GetFileSize_s(filename);		// �ե����륵����
	
	if((pcmbuf = (uchar *)malloc(size)) == NULL) {
		return ERR_OUT_OF_MEMORY;					// ���꤬���ݤǤ��ʤ�
	}

	fread(pcmbuf, 1, size, fp);
	fclose(fp);
	result = ppc_load3(pcmbuf, size);
	strcpy(open_work.ppcfilename, filename);
	free(pcmbuf);
	return result;
}


//=============================================================================
//	 ppc_load ���Σ��ʥ��꤫���
//=============================================================================
int PMDWIN::ppc_load3(uchar *pcmdata, int size)
{
	int		i;
	uchar	tempbuf[0x26*32];
	uchar	tempbuf2[30+4*256+128+2];
	ushort	*pcmdata2;
	ushort	pcmstart, pcmstop;
	int		bx = 0;
	bool	pvi_flag;

	if(size < 0x10) {
		memset(open_work.ppcfilename, 0x0, sizeof(open_work.ppcfilename));
		return ERR_WRONG_PPC_FILE;	// PPC / PVI �ǤϤʤ�
	}

	if(strncmp((char *)pcmdata, PVIHeader, sizeof(PVIHeader)-1) == 0 &&
		pcmdata[10] == 2) {	// PVI, x8
		pvi_flag = true;

		//--------------------------------------------------------------------
		//	pvi�β������󤫤�pmd��ž��
		//--------------------------------------------------------------------
	
		 for(i = 0; i < 128; i++) {
			if(read_word(&pcmdata[18+i*4]) == 0) {
				pcmends.pcmadrs[i][0] = pcmdata[16+i*4];
				pcmends.pcmadrs[i][1] = 0;
			} else {
				pcmends.pcmadrs[i][0] = read_word(&pcmdata[16+i*4])+0x26;
				pcmends.pcmadrs[i][1] = read_word(&pcmdata[18+i*4])+0x26;
			}

			if(bx < pcmends.pcmadrs[i][1]) {
				bx = pcmends.pcmadrs[i][1] + 1;
			}
		}

		for(i = 128; i < 256; i++) {		// �Ĥ�128�Ĥ�̤���
			pcmends.pcmadrs[i][0] = 0;
			pcmends.pcmadrs[i][1] = 0;
		}
		pcmends.pcmends = bx;
	
	} else if(strncmp((char *)pcmdata, PPCHeader, sizeof(PPCHeader)-1) == 0) {	// PPC
		pvi_flag = false;
		pcmdata2 = (ushort *)pcmdata + 30 / 2;
		if(size < 30 + 4*256+2) {						// PPC �ǤϤʤ�
			memset(open_work.ppcfilename, 0x0, sizeof(open_work.ppcfilename));
			return ERR_WRONG_PPC_FILE;	// PPC / PVI �ǤϤʤ�
		}
		
		pcmends.pcmends = *pcmdata2++;
		for(i = 0; i < 256; i++) {
			pcmends.pcmadrs[i][0] = *pcmdata2++;
			pcmends.pcmadrs[i][1] = *pcmdata2++;
		}
	} else {											// PPC / PVI �ǤϤʤ�
		memset(open_work.ppcfilename, 0x0, sizeof(open_work.ppcfilename));
		return ERR_WRONG_PPC_FILE;	// PPC / PVI �ǤϤʤ�
	}
	
	//------------------------------------------------------------------------
	//	PMD�Υ����PCMRAM�Υإå������
	//------------------------------------------------------------------------
		
	pcmread(0, 0x25, tempbuf);
	// "ADPCM��"�إå������Ф�
	//	�ե�����̵̾��(PMDWin ����)
	if(memcmp(&tempbuf[30], &pcmends, sizeof(pcmends)) == 0) {
		return WARNING_PPC_ALREADY_LOAD;		// ���פ���
	}
	
	//------------------------------------------------------------------------
	//	PMD�Υ����PCMRAMƬ�˽񤭹���
	//------------------------------------------------------------------------
	
	memcpy(tempbuf2, PPCHeader, sizeof(PPCHeader)-1);
	memcpy(&tempbuf2[sizeof(PPCHeader)-1], &pcmends.pcmends, 
								sizeof(tempbuf2)-(sizeof(PPCHeader)-1));
	pcmstore(0, 0x25, tempbuf2);
	
	//------------------------------------------------------------------------
	//	PCMDATA��PCMRAM�˽񤭹���
	//------------------------------------------------------------------------
	
	if(pvi_flag) {
		pcmdata2 = (ushort *)(pcmdata + 0x10 + sizeof(ushort) * 2 * 128);
		if(size < (int)(pcmends.pcmends - (0x10 + sizeof(ushort) * 2 * 128)) * 32) {
			// PVI �ǤϤʤ�
			memset(open_work.ppcfilename, 0x0, sizeof(open_work.ppcfilename));
			return ERR_WRONG_PPC_FILE;
		}
	} else {
		pcmdata2 = (ushort *)pcmdata + (30 + 4*256 + 2) / 2;
		if(size < (pcmends.pcmends - ((30 + 4*256 + 2) / 2)) * 32) {
			// PPC �ǤϤʤ�
			memset(open_work.ppcfilename, 0x0, sizeof(open_work.ppcfilename));
			return ERR_WRONG_PPC_FILE;
		}
	}

	pcmstart =  0x26;
	pcmstop  = pcmends.pcmends;
	pcmstore(pcmstart, pcmstop, (uchar *)pcmdata2);

	return PMDWIN_OK;
}


//=============================================================================
//	fm effect
//=============================================================================
uchar * PMDWIN::fm_efct_set(QQ *qq, uchar *si)
{
	return si+1;
}


uchar * PMDWIN::ssg_efct_set(QQ *qq, uchar *si)
{
	int		al;

	al = *si++;

	if(qq->partmask) return si;

	if(al) {
		eff_on2(qq, al);
	} else {
		effend();
	}
	return si;
}


//=============================================================================
//	FADE IN / OUT ROUTINE
//
//		FROM Timer-A
//=============================================================================
void PMDWIN::fout(void)
{
	if(open_work.fadeout_speed == 0) return;

	if(open_work.fadeout_speed > 0) {
		if(open_work.fadeout_speed + open_work.fadeout_volume < 256) {
			open_work.fadeout_volume += open_work.fadeout_speed;
		} else {
			open_work.fadeout_volume = 255;
			open_work.fadeout_speed = 0;
			if(open_work.fade_stop_flag == 1) {
				pmdwork.music_flag |= 2;
			}
		}
	} else {								// fade in
		if(open_work.fadeout_speed + open_work.fadeout_volume > 255) {
			open_work.fadeout_volume += open_work.fadeout_speed;
		} else {
			open_work.fadeout_volume = 0;
			open_work.fadeout_speed = 0;
			opna.SetReg(0x11, open_work.rhyvol);
		}
	}
}


//=============================================================================
//	�����
//=============================================================================
bool WINAPI PMDWIN::init(char *path)
{
	int		i;
	char	path2[_MAX_PATH];

	if(path == NULL) {
		*path2 = '\0';
	} else {
		strcpy(path2, path);
	}

	if((char *)_mbsrchr((uchar *)path2, PATH_DELIMIT_CHR) != &path2[strlen(path2)-1]) {
		strcat(path2, PATH_DELIMIT_STR);
	}

	open_work.rate = SOUND_44K;
	ppz8.Init(open_work.rate, false);
	ppsdrv.Init(open_work.rate, false);
	p86drv.Init(open_work.rate, false);

	// OPEN_WORK ���Фν����
	open_work.rhyvol = 0x3c;
	open_work.fade_stop_flag = 0;
	open_work.TimerBflag = 0;
	open_work.TimerAflag = 0;
	open_work.TimerB_speed = 0x100;
	open_work.port22h = 0;
	memset(open_work.mus_filename, 0x0, sizeof(open_work.mus_filename));
	open_work.pcmdir[0][0] = '\0';
	open_work.use_p86 = false;
	
	if(opna.Init(OPNAClock, SOUND_44K, false, path2) == false) return false;
	
	opna.SetVolumeFM(0);
	opna.SetVolumePSG(-10);
	opna.SetVolumeADPCM(0);
	opna.SetVolumeRhythmTotal(0);
	ppz8.SetVolume(0);
	ppsdrv.SetVolume(0);
	p86drv.SetVolume(0);

	opna.SetFMWait(DEFAULT_REG_WAIT);
	opna.SetSSGWait(DEFAULT_REG_WAIT);
	opna.SetRhythmWait(DEFAULT_REG_WAIT);
	opna.SetADPCMWait(DEFAULT_REG_WAIT);
	
	open_work.ppz8ip = false;
	open_work.p86ip = false;
	open_work.ppsip = false;
	
	//----------------------------------------------------------------
	//	�ѿ��ν����
	//----------------------------------------------------------------
	
	open_work.MusPart[ 0] = &FMPart[0];
	open_work.MusPart[ 1] = &FMPart[1];
	open_work.MusPart[ 2] = &FMPart[2];
	open_work.MusPart[ 3] = &FMPart[3];
	open_work.MusPart[ 4] = &FMPart[4];
	open_work.MusPart[ 5] = &FMPart[5];
	open_work.MusPart[ 6] = &SSGPart[0];
	open_work.MusPart[ 7] = &SSGPart[1];
	open_work.MusPart[ 8] = &SSGPart[2];
	open_work.MusPart[ 9] = &ADPCMPart;
	open_work.MusPart[10] = &RhythmPart;
	open_work.MusPart[11] = &ExtPart[0];
	open_work.MusPart[12] = &ExtPart[1];
	open_work.MusPart[13] = &ExtPart[2];
	open_work.MusPart[14] = &DummyPart;
	open_work.MusPart[15] = &EffPart;
	open_work.MusPart[16] = &PPZ8Part[0];
	open_work.MusPart[17] = &PPZ8Part[1];
	open_work.MusPart[18] = &PPZ8Part[2];
	open_work.MusPart[19] = &PPZ8Part[3];
	open_work.MusPart[20] = &PPZ8Part[4];
	open_work.MusPart[21] = &PPZ8Part[5];
	open_work.MusPart[22] = &PPZ8Part[6];
	open_work.MusPart[23] = &PPZ8Part[7];
	
	pcmends.pcmends = 0x26;
	for(i = 0; i < 256; i++) {
		pcmends.pcmadrs[i][0] = 0;
		pcmends.pcmadrs[i][1] = 0;
	}
	
	memset(open_work.ppcfilename, 0x0, sizeof(open_work.ppcfilename));
	memset(open_work.pcmdir, 0, sizeof(open_work.pcmdir));
				
	mdataarea[0] = 0;
	for(i = 0; i < 12; i++) {
		mdataarea[i*2+1] = 0x18;
		mdataarea[i*2+2] = 0;
	}
	mdataarea[25] = 0x80;
	
	open_work.fm_voldown = fmvd_init;		// FM_VOLDOWN
	open_work._fm_voldown = fmvd_init;		// FM_VOLDOWN
	open_work.ssg_voldown = 0;				// SSG_VOLDOWN
	open_work._ssg_voldown = 0;				// SSG_VOLDOWN
	open_work.pcm_voldown = 0;				// PCM_VOLDOWN
	open_work._pcm_voldown = 0;				// PCM_VOLDOWN
	open_work.ppz_voldown = 0;				// PPZ_VOLDOWN
	open_work._ppz_voldown = 0;				// PPZ_VOLDOWN
	open_work.rhythm_voldown = 0;			// RHYTHM_VOLDOWN
	open_work._rhythm_voldown = 0;			// RHYTHM_VOLDOWN
	open_work.kp_rhythm_flag =  false;		// SSGDRUM��RHYTHM�������Ĥ餹�� 
	open_work.rshot_bd = 0;					// �ꥺ�಻�� shot inc flag (BD)
	open_work.rshot_sd = 0;					// �ꥺ�಻�� shot inc flag (SD)
	open_work.rshot_sym = 0;				// �ꥺ�಻�� shot inc flag (CYM)
	open_work.rshot_hh = 0;					// �ꥺ�಻�� shot inc flag (HH)
	open_work.rshot_tom = 0;				// �ꥺ�಻�� shot inc flag (TOM)
	open_work.rshot_rim = 0;				// �ꥺ�಻�� shot inc flag (RIM)
	open_work.rdump_bd = 0;					// �ꥺ�಻�� dump inc flag (BD)
	open_work.rdump_sd = 0;					// �ꥺ�಻�� dump inc flag (SD)
	open_work.rdump_sym = 0;				// �ꥺ�಻�� dump inc flag (CYM)
	open_work.rdump_hh = 0;					// �ꥺ�಻�� dump inc flag (HH)
	open_work.rdump_tom = 0;				// �ꥺ�಻�� dump inc flag (TOM)
	open_work.rdump_rim = 0;				// �ꥺ�಻�� dump inc flag (RIM)
	
	memset(FMPart, 0, sizeof(FMPart));
	memset(SSGPart, 0, sizeof(SSGPart));
	memset(&ADPCMPart, 0, sizeof(ADPCMPart));
	memset(&RhythmPart, 0, sizeof(RhythmPart));
	memset(&ExtPart, 0, sizeof(ExtPart));
	memset(&EffPart, 0, sizeof(EffPart));
	memset(&PPZ8Part, 0, sizeof(PPZ8Part));
	
	open_work.pcm86_vol = 0;		// PCM���̹�碌
	open_work._pcm86_vol = 0;		// PCM���̹�碌
	open_work.fade_stop_flag = 1;	// FADEOUT��MSTOP���뤫 FLAG
	
	pmdwork.ppsdrv_flag = false;	// PPSDRV FLAG
	pmdwork.music_flag =  0;
	
	//----------------------------------------------------------------
	//	�ʥǡ����������ǡ�����Ǽ���Ϥ�����
	//----------------------------------------------------------------
	open_work.mmlbuf = &mdataarea[1];
	open_work.tondat = vdataarea;
	open_work.efcdat = edataarea;
	
	//----------------------------------------------------------------
	//	���̲�/FMINT/EFCINT������
	//----------------------------------------------------------------
	effwork.effon = 0;
	effwork.psgefcnum = 0xff;
	
	//----------------------------------------------------------------
	//	088/188/288/388 (ƱINT�ֹ�Τ�) ��������
	//----------------------------------------------------------------
	opna.SetReg(0x29, 0x00);
	opna.SetReg(0x24, 0x00);
	opna.SetReg(0x25, 0x00);
	opna.SetReg(0x26, 0x00);
	opna.SetReg(0x27, 0x3f);
	
	//----------------------------------------------------------------
	//	�ϣУγ����߳���
	//----------------------------------------------------------------
	opnint_start();	

	return true;
}


//=============================================================================
//	�ꥺ�಻�κ��ɤ߹���
//=============================================================================
bool WINAPI PMDWIN::loadrhythmsample(char *path)
{
	char	path2[_MAX_PATH];

	strcpy(path2, path);
	if((char *)_mbsrchr((uchar *)path2, PATH_DELIMIT_CHR) != &path2[strlen(path2)-1]) {
		strcat(path2, PATH_DELIMIT_STR);
	}
	
	mstop_f();
	return opna.LoadRhythmSample(path2);
}


//=============================================================================
//	PCM �����ǥ��쥯�ȥ������
//=============================================================================
bool WINAPI PMDWIN::setpcmdir(char **path)
{
	int		i = 0;

	while(path[i] && i < MAX_PCMDIR) {
        if (*path[i] != '\0') break;
        printf("i:%d path:%s\n",i,path[i]);
		if(*path[i] == '\0') break;
		strcpy(open_work.pcmdir[i], path[i]);

		if((char *)_mbsrchr((uchar *)open_work.pcmdir[i], PATH_DELIMIT_CHR) !=
										&open_work.pcmdir[i][strlen(open_work.pcmdir[i])-1]) {
			strcat(open_work.pcmdir[i], PATH_DELIMIT_STR);
		}

		if(++i == MAX_PCMDIR) {
			open_work.pcmdir[i][0]= '\0';
			return false;
		}
	}

	open_work.pcmdir[i][0]= '\0';
	return true;
}


//=============================================================================
//	�������ȿ�������
//=============================================================================
void WINAPI PMDWIN::setpcmrate(int rate)
{
	if(rate == SOUND_55K) {
		open_work.rate = open_work.ppzrate = SOUND_44K;
		open_work.fmcalc55k = true;
	} else {
		open_work.rate = open_work.ppzrate = rate;
		open_work.fmcalc55k = false;
	}
	
	opna.SetRate(OPNAClock, open_work.rate, open_work.fmcalc55k);
	ppz8.SetRate(open_work.ppzrate, open_work.ppz8ip);
	ppsdrv.SetRate(open_work.rate, open_work.ppsip);
	p86drv.SetRate(open_work.rate, open_work.p86ip);
}


//=============================================================================
//	PPZ �������ȿ�������
//=============================================================================
void WINAPI PMDWIN::setppzrate(int rate)
{
	open_work.ppzrate = rate;
	ppz8.SetRate(rate, open_work.ppz8ip);
}


//=============================================================================
//	PPS ���Ĥ餹����
//=============================================================================
void WINAPI PMDWIN::setppsuse(bool value)
{
	pmdwork.ppsdrv_flag = value;
}


//=============================================================================
//	SSG ���̲��� OPNA Rhythm ���Ĥ餹����
//=============================================================================
void WINAPI PMDWIN::setrhythmwithssgeffect(bool value)
{
	open_work.kp_rhythm_flag = value;
}


//=============================================================================
//	PMD86 �� PCM �� PMDB2 �ߴ��ˤ��뤫��
//=============================================================================
void WINAPI PMDWIN::setpmd86pcmmode(bool value)
{
	if(value) {
		open_work.pcm86_vol = open_work._pcm86_vol = 1;
	} else {
		open_work.pcm86_vol = open_work._pcm86_vol = 0;
	}
}


//=============================================================================
//	PMD86 �� PCM �� PMDB2 �ߴ����ɤ������������
//=============================================================================
bool WINAPI PMDWIN::getpmd86pcmmode(void)
{
	if(open_work.pcm86_vol) return true;
	return false;
}


//=============================================================================
//	�ʤ��ɤ߹��ߤ��Σ��ʥե����뤫���
//=============================================================================
int WINAPI PMDWIN::music_load(char *filename)
{
	FILE	*fp;
	int		size, result;
	char	drive[_MAX_DRIVE];
	char	dir[_MAX_DIR];
	char	current_dir[_MAX_DRIVE+_MAX_DIR];
	uchar	musbuf[mdata_def*1024];

	if((fp = fopen(filename, "rb")) == NULL) {
		return ERR_OPEN_MUSIC_FILE;
	}
	
	mstop_f();
	_splitpath(filename, drive, dir, NULL, NULL);
	strcpy(current_dir, drive);
	strcat(current_dir, dir);
	
	size = (int)fread(musbuf, 1, sizeof(musbuf), fp);
	result = music_load3(musbuf, size, current_dir);
	fclose(fp);
	if(result == PMDWIN_OK || result == WARNING_PPC_ALREADY_LOAD ||
			result == WARNING_PPS_ALREADY_LOAD ||
			result == WARNING_PPZ1_ALREADY_LOAD ||
			result == WARNING_PPZ2_ALREADY_LOAD) {
		strcpy(open_work.mus_filename, filename);
	} else {
		open_work.mus_filename[0] = '\0';
	}
	return result;
}


//=============================================================================
//	�ʤ��ɤ߹��ߤ��Σ��ʥ��꤫���
//=============================================================================
int WINAPI PMDWIN::music_load2(uchar *musdata, int size)
{
	return music_load3(musdata, size, NULL);
}


//=============================================================================
//	���ճ���
//=============================================================================
void WINAPI PMDWIN::music_start(void)
{
	mstart_f();
}


//=============================================================================
//	�������
//=============================================================================
void WINAPI PMDWIN::music_stop(void)
{
	mstop_f();
}


//=============================================================================
//	�ե����ɥ�����(PMD�ߴ�)
//=============================================================================
void WINAPI PMDWIN::fadeout(int speed)
{
	open_work.fadeout_speed = speed;
}


//=============================================================================
//	�ե����ɥ�����(�ⲻ��)
//=============================================================================
void WINAPI PMDWIN::fadeout2(int speed)
{
	if(speed > 0) {
		if(open_work.fadeout2_speed == 0) {
			fpos = upos;
		}
		open_work.fadeout2_speed = speed;
	} else {
		open_work.fadeout2_speed = 0;	// fadeout �������
	}
}


//=============================================================================
//	PCM �ǡ�����wave �ǡ����ˤμ���
//=============================================================================
void WINAPI PMDWIN::getpcmdata(short *buf, int nsamples)
{
	int	copysamples = 0;				// ���ԡ��ѤߤΥ���ץ��
	int				i, us, ftemp;
	int	ppzsample, delta, carry;
	
	do {
		if(nsamples - copysamples <= us2) {
			memcpy(buf, pos2, (nsamples - copysamples)*4);
			us2 -= (nsamples - copysamples);
			pos2 += (nsamples - copysamples)*4;
			copysamples = nsamples;
		} else {
			memcpy(buf, pos2, us2 * 4);
			buf += (us2 * 2);
			copysamples += us2;
			pos2 = (char *)wavbuf2;

			if(opna.ReadStatus() & 0x01) {
				TimerA_main();
			}

			if(opna.ReadStatus() & 0x02) {
				TimerB_main();
			}

			opna.SetReg(0x27, open_work.ch3mode | 0x30);	// TIMER RESET(timerA,B�Ȥ�)


			us = opna.GetNextEvent();
			us2 = (int)((double)us * open_work.rate / 1000000.0);
			opna.Count(us);

			memset(wavbuf, 0x0, us2 * sizeof(Sample) * 2);
			
			if(open_work.rate == open_work.ppzrate) {
				ppz8.Mix((Sample *)wavbuf, us2);
			} else {			
				// ppz8 �� pcm �μ��ȿ��Ѵ�(�䴰�ʤ�)
				ppzsample = us2 * open_work.ppzrate / open_work.rate + 1;
				delta = 8192 * open_work.ppzrate / open_work.rate;
				memset(wavbuf_conv, 0x0, ppzsample * sizeof(Sample) * 2);
				ppz8.Mix((Sample *)wavbuf_conv, ppzsample);
				
				carry = 0;
				for(i = 0; i < us2; i++) {		// ���ȿ��Ѵ�(1 << 13 = 8192)
					wavbuf[i].left  = wavbuf_conv[(carry >> 13)].left;
					wavbuf[i].right = wavbuf_conv[(carry >> 13)].right;
					carry += delta;
				}
			}

			opna.Mix((Sample *)wavbuf, us2);
			if(pmdwork.ppsdrv_flag) {
				ppsdrv.Mix((Sample *)wavbuf, us2);
			}
			if(open_work.use_p86) {
				p86drv.Mix((Sample *)wavbuf, us2);
			}

			upos += us;
			
			if(open_work.fadeout2_speed > 0) {
				if(open_work.status2 == -1) {
					ftemp = 0;
				} else {
					ftemp = (int)((1 << 10) * pow(512, -(double)(upos - fpos) / 1000 / open_work.fadeout2_speed));
				}

				for(i = 0; i < us2; i++) {
					wavbuf2[i].left = Limit(wavbuf[i].left * ftemp >> 10, 32767, -32768);
					wavbuf2[i].right = Limit(wavbuf[i].right * ftemp >> 10, 32767, -32768);
				}
				if(upos - fpos > open_work.fadeout2_speed * 1000 &&
									open_work.fade_stop_flag == 1) {	// fadeout ��λ
					pmdwork.music_flag |= 2;
				}
			} else {
				for(i = 0; i < us2; i++) {
					wavbuf2[i].left = Limit(wavbuf[i].left , 32767, -32768);
					wavbuf2[i].right = Limit(wavbuf[i].right, 32767, -32768);
				}
			}
		}
	} while(copysamples < nsamples);
}


//=============================================================================
//	FM �� 55kHz�������켡�䴰���뤫�ɤ���������
//=============================================================================
void WINAPI PMDWIN::setfmcalc55k(bool flag)
{
	open_work.fmcalc55k = flag;
	opna.SetRate(OPNAClock, open_work.rate, open_work.fmcalc55k);
}


//=============================================================================
//	PPS �ǰ켡�䴰���뤫�ɤ���������
//=============================================================================
void WINAPI PMDWIN::setppsinterpolation(bool ip)
{
	open_work.ppsip = ip;
	ppsdrv.SetRate(open_work.rate, ip);
}


//=============================================================================
//	P86 �ǰ켡�䴰���뤫�ɤ���������
//=============================================================================
void WINAPI PMDWIN::setp86interpolation(bool ip)
{
	open_work.p86ip = ip;
	p86drv.SetRate(open_work.rate, ip);
}


//=============================================================================
//	PPZ8 �ǰ켡�䴰���뤫�ɤ���������
//=============================================================================
void WINAPI PMDWIN::setppzinterpolation(bool ip)
{
	open_work.ppz8ip = ip;
	ppz8.SetRate(open_work.ppzrate, ip);
}


//=============================================================================
//	���μ���
//=============================================================================
char* WINAPI PMDWIN::getmemo(char *dest, uchar *musdata, int size, int al)
{
	uchar	*si, *mmlbuf;
	int		i, dx;
	int		maxsize;

	if(musdata == NULL || size == 0) {
		mmlbuf = open_work.mmlbuf;
		maxsize = sizeof(mdataarea) - 1;
	} else {
		mmlbuf = &musdata[1];
		maxsize = size - 1;
	}
	if(maxsize < 2) {
		*dest = '\0';			// 	�ʥǡ���������
		return NULL;
	}
	
	if(mmlbuf[0] != 0x1a || mmlbuf[1] != 0x00) {
		*dest = '\0';			// 	�������ʤ�file=���Υ��ɥ쥹������ǽ
		return dest;
	}
	
	if(maxsize < 0x18+1) {
		*dest = '\0';			// 	�ʥǡ���������
		return NULL;
	}
	
	if(maxsize < read_word(&mmlbuf[0x18]) - 4 + 3) {
		*dest = '\0';			// 	�ʥǡ���������
		return NULL;
	}
	
	si = &mmlbuf[read_word(&mmlbuf[0x18]) - 4];
	if(*(si + 2) != 0x40) {
		if(*(si + 3) != 0xfe || *(si + 2) < 0x41) {
			*dest = '\0';			// 	�������ʤ�file=���Υ��ɥ쥹������ǽ
			return dest;
		}
	}
	
	if(*(si + 2) >= 0x42) {
		al++;
	}
	
	if(*(si + 2) >= 0x48) {
		al++;
	}
	
	if(al < 0) {
		*dest = '\0';				//	��Ͽ�ʤ�
		return dest;
	}
	
	si = &mmlbuf[read_word(si)];
	
	for(i = 0; i <= al; i++) {
		if(maxsize < si - mmlbuf + 1) {
			*dest = '\0';			// 	�ʥǡ���������
			return NULL;
		}
		
		dx = read_word(si);
		if(dx == 0) {
			*dest = '\0';
			return dest;
		}
		
		if(maxsize < dx) {
			*dest = '\0';			// 	�ʥǡ���������
			return NULL;
		}
		
		if(mmlbuf[dx] == '/') {
			*dest = '\0';
			return dest;
		}
		
		si += 2;
	}
	
	for(i = dx; i < maxsize; i++) {
		if(mmlbuf[i] == '\0') break;
	}

	// ��ü�� \0 ���ʤ����
	if(i >= maxsize) {
		memcpy(dest, &mmlbuf[dx], maxsize - dx);
		dest[maxsize - dx - 1] = '\0';
	} else {
		strcpy(dest, (char *)&mmlbuf[dx]);
	}
	return dest;
}


//=============================================================================
//	���μ����ʣ��Х���Ⱦ�Ѣ�Ⱦ��ʸ�����Ѵ���
//=============================================================================
char* WINAPI PMDWIN::getmemo2(char *dest, uchar *musdata, int size, int al)
{
	char	*buf;
	char	*rslt;

	if((buf = (char *)malloc(mdata_def*1024)) == NULL) {
		*dest = '\0';		// ������­
		return NULL;
	}
	getmemo(buf, musdata, size, al);
	rslt = zen2tohan(dest, buf);
	free(buf);
	return rslt;
}


//=============================================================================
//	���μ����ʣ��Х���Ⱦ�Ѣ�Ⱦ��ʸ�����Ѵ���ESC�������󥹤ν����
//=============================================================================
char* WINAPI PMDWIN::getmemo3(char *dest, uchar *musdata, int size, int al)
{
	char	*buf;
	char	*rslt;
	
	if((buf = (char *)malloc(mdata_def*1024)) == NULL) {
		*dest = '\0';		// ������­
		return NULL;
	}
	getmemo2((char *)buf, musdata, size, al);
	rslt = delesc(dest, buf);
	free(buf);
	return rslt;
}


//=============================================================================
//	���μ����ʥե�����̾�����
//=============================================================================
int WINAPI PMDWIN::fgetmemo(char *dest, char *filename, int al)
{
	FILE	*fp;
	uchar	*mmlbuf;
	int		size;

	if(filename != NULL) {
		if(*filename != '\0') {
			// �ե����뤫��
			if((fp = fopen(filename, "rb")) == NULL) {
				return ERR_OPEN_MUSIC_FILE;
			}
			
			if((mmlbuf = (uchar *)malloc(mdata_def*1024)) == NULL) {
				*dest = '\0';
				return ERR_OUT_OF_MEMORY;
			}

			size = (int)fread(mmlbuf, 1, mdata_def*1024, fp);
			getmemo(dest, mmlbuf, size, al);
			free(mmlbuf);
			fclose(fp);
			return PMDWIN_OK;
		}
	}

	// �����ǡ�������
	getmemo(dest, NULL, 0, al);
	return PMDWIN_OK;
}


//=============================================================================
//	���μ����ʥե�����̾���顿���Х���Ⱦ�Ѣ�Ⱦ��ʸ�����Ѵ���
//=============================================================================
int WINAPI PMDWIN::fgetmemo2(char *dest, char *filename, int al)
{
	char	*buf;
	int		rslt;

	if((buf = (char *)malloc(mdata_def*1024)) == NULL) {
		*dest = '\0';		// ������­
		return ERR_OUT_OF_MEMORY;
	}
	if((rslt = fgetmemo(buf, filename, al)) == PMDWIN_OK) {
		zen2tohan(dest, buf);
	}  else {
		*dest = '\0';
	}
	free(buf);
	return rslt;
}


//=============================================================================
//	���μ����ʥե�����̾���顿Ⱦ��ʸ�����Ѵ���ESC�������󥹤ν����
//=============================================================================
int WINAPI PMDWIN::fgetmemo3(char *dest, char *filename, int al)
{
	char	*buf;
	int		rslt;

	if((buf = (char *)malloc(mdata_def*1024)) == NULL) {
		*dest = '\0';		// ������­
		return ERR_OUT_OF_MEMORY;
	}
	if((rslt = fgetmemo2(buf, filename, al)) == PMDWIN_OK) {
		delesc(dest, buf);
	} else {
		*dest = '\0';
	}
	free(buf);
	return rslt;
}


//=============================================================================
//	�ʤ� filename �μ���
//=============================================================================
char* WINAPI PMDWIN::getmusicfilename(char *dest)
{
	strcpy(dest, open_work.mus_filename);
	return dest;
}


//=============================================================================
//	PPC / P86 filename �μ���
//=============================================================================
char* WINAPI PMDWIN::getpcmfilename(char *dest)
{
	if(open_work.use_p86) {
		strcpy(dest, p86drv.p86_file);
	} else {
		strcpy(dest, open_work.ppcfilename);
	}
	return dest;
}


//=============================================================================
//	PPC filename �μ���
//=============================================================================
char* WINAPI PMDWIN::getppcfilename(char *dest)
{
	strcpy(dest, open_work.ppcfilename);
	return dest;
}


//=============================================================================
//	PPS filename �μ���
//=============================================================================
char* WINAPI PMDWIN::getppsfilename(char *dest)
{
	strcpy(dest, ppsdrv.pps_file);
	return dest;
}


//=============================================================================
//	P86 filename �μ���
//=============================================================================
char* WINAPI PMDWIN::getp86filename(char *dest)
{
	strcpy(dest, p86drv.p86_file);
	return dest;
}


//=============================================================================
//	PPZ filename �μ���
//=============================================================================
char* WINAPI PMDWIN::getppzfilename(char *dest, int bufnum)
{
	strcpy(dest, ppz8.PVI_FILE[bufnum]);
	return dest;
}


//=============================================================================
//	.PPC ���ɤ߹��ߡʥե����뤫���
//=============================================================================
int WINAPI PMDWIN::ppc_load(char *filename)
{
	int		resultppc;
	
	music_stop();
	resultppc = ppc_load2(filename);
	if(resultppc == PMDWIN_OK || resultppc == WARNING_PPC_ALREADY_LOAD) {
		open_work.use_p86 = false;
	}

	return resultppc;
}


//=============================================================================
//	.PPS ���ɤ߹��ߡʥե����뤫���
//=============================================================================
int WINAPI PMDWIN::pps_load(char *filename)
{
	int		result;

	music_stop();

	result = ppsdrv.Load(filename);
	switch(result) {
		case _PPSDRV_OK:				return PMDWIN_OK;
		case _ERR_OPEN_PPS_FILE:		return ERR_OPEN_PPS_FILE;
		case _WARNING_PPS_ALREADY_LOAD:	return WARNING_PPS_ALREADY_LOAD;
		case _ERR_OUT_OF_MEMORY:		return ERR_OUT_OF_MEMORY;
		default:						return ERR_OTHER;
	}
}


//=============================================================================
//	.P86 ���ɤ߹��ߡʥե����뤫���
//=============================================================================
int WINAPI PMDWIN::p86_load(char *filename)
{
	int		result;

	music_stop();

	result = p86drv.Load(filename);

	if(result == _P86DRV_OK || result == _WARNING_P86_ALREADY_LOAD) {
		open_work.use_p86 = true;
	}

	switch(result) {
		case _P86DRV_OK:				return PMDWIN_OK;
		case _ERR_OPEN_P86_FILE:		return ERR_OPEN_P86_FILE;
		case _ERR_WRONG_P86_FILE:		return ERR_WRONG_P86_FILE;
		case _WARNING_P86_ALREADY_LOAD:	return WARNING_P86_ALREADY_LOAD;
		case _ERR_OUT_OF_MEMORY:		return ERR_OUT_OF_MEMORY;
		default:						return ERR_OTHER;
	}
}


//=============================================================================
//	.PZI, .PVI ���ɤ߹��ߡʥե����뤫���
//=============================================================================
int WINAPI PMDWIN::ppz_load(char *filename, int bufnum)
{
	int		result;

	music_stop();

	result = ppz8.Load(filename, bufnum);
	switch(result) {
		case _PPZ8_OK:					return PMDWIN_OK;
		case _ERR_OPEN_PPZ_FILE:		return bufnum ? ERR_OPEN_PPZ2_FILE : ERR_OPEN_PPZ1_FILE;
		case _ERR_WRONG_PPZ_FILE: 		return bufnum ? ERR_WRONG_PPZ2_FILE : ERR_WRONG_PPZ1_FILE;
		case _WARNING_PPZ_ALREADY_LOAD:	return bufnum ? WARNING_PPZ2_ALREADY_LOAD : WARNING_PPZ1_ALREADY_LOAD;
		case _ERR_OUT_OF_MEMORY:		return ERR_OUT_OF_MEMORY;
		default:						return ERR_OTHER;
	}
}


//=============================================================================
//	�ѡ��ȤΥޥ���
//=============================================================================
int WINAPI PMDWIN::maskon(int ch)
{
	int		ah, fmseltmp;
	
	
	if(ch >= sizeof(open_work.MusPart) / sizeof(QQ *)) {
		return ERR_WRONG_PARTNO;		// part number error
	}
	
	if(part_table[ch][0] < 0) {
		open_work.rhythmmask = 0;	// Rhythm������Mask
		opna.SetReg(0x10, 0xff);	// Rhythm����������Dump
	} else {
		fmseltmp = pmdwork.fmsel;
		if(open_work.MusPart[ch]->partmask == 0 && open_work.play_flag) {
			if(part_table[ch][2] == 0) {
				pmdwork.partb = part_table[ch][1];
				pmdwork.fmsel = 0;
				silence_fmpart(open_work.MusPart[ch]);	// �������˾ä�
			} else if(part_table[ch][2] == 1) {
				pmdwork.partb = part_table[ch][1];
				pmdwork.fmsel = 0x100;
				silence_fmpart(open_work.MusPart[ch]);	// �������˾ä�
			} else if(part_table[ch][2] == 2) {
				pmdwork.partb = part_table[ch][1];
				ah =  1 << (pmdwork.partb - 1);
				ah |= (ah << 3);
												// PSG keyoff
				opna.SetReg(0x07, ah | opna.GetReg(0x07));
			} else if(part_table[ch][2] == 3) {
				opna.SetReg(0x101, 0x02);		// PAN=0 / x8 bit mode
				opna.SetReg(0x100, 0x01);		// PCM RESET
			} else if(part_table[ch][2] == 4) {
				if(effwork.psgefcnum < 11) {
					effend();
				}
			} else if(part_table[ch][2] == 5) {
				ppz8.Stop(part_table[ch][1]);
			}
		}
		open_work.MusPart[ch]->partmask |= 1;
		pmdwork.fmsel = fmseltmp;
	}
	return PMDWIN_OK;
}


//=============================================================================
//	�ѡ��ȤΥޥ������
//=============================================================================
int WINAPI PMDWIN::maskoff(int ch)
{
	int		fmseltmp;
	
	if(ch >= sizeof(open_work.MusPart) / sizeof(QQ *)) {
		return ERR_WRONG_PARTNO;		// part number error
	}

	if(part_table[ch][0] < 0) {
		open_work.rhythmmask = 0xff;
	} else {
		if(open_work.MusPart[ch]->partmask == 0) return ERR_NOT_MASKED;	// �ޥ�������Ƥ��ʤ�
											// ���̲��Ǥޤ��ޥ�������Ƥ���
		if((open_work.MusPart[ch]->partmask &= 0xfe) != 0) return ERR_EFFECT_USED;
											// �ʤ��ߤޤäƤ���
		if(open_work.play_flag == 0) return ERR_MUSIC_STOPPED;

		fmseltmp = pmdwork.fmsel;
		if(open_work.MusPart[ch]->address != NULL) {
			if(part_table[ch][2] == 0) {		// FM����(ɽ)
				pmdwork.fmsel = 0;
				pmdwork.partb = part_table[ch][1];
				neiro_reset(open_work.MusPart[ch]);
			} else if(part_table[ch][2] == 1) {	// FM����(΢)
				pmdwork.fmsel = 0x100;
				pmdwork.partb = part_table[ch][1];
				neiro_reset(open_work.MusPart[ch]);
			}
		}
		pmdwork.fmsel = fmseltmp;

	}
	return PMDWIN_OK;
}


//=============================================================================
//	FM Volume Down ������
//=============================================================================
void WINAPI PMDWIN::setfmvoldown(int voldown)
{
	open_work.fm_voldown = open_work._fm_voldown = voldown;
}


//=============================================================================
//	SSG Volume Down ������
//=============================================================================
void WINAPI PMDWIN::setssgvoldown(int voldown)
{
	open_work.ssg_voldown = open_work._ssg_voldown = voldown;
}


//=============================================================================
//	Rhythm Volume Down ������
//=============================================================================
void WINAPI PMDWIN::setrhythmvoldown(int voldown)
{
	open_work.rhythm_voldown = open_work._rhythm_voldown = voldown;
	open_work.rhyvol = 48*4*(256-open_work.rhythm_voldown)/1024;
	opna.SetReg(0x11, open_work.rhyvol);

}


//=============================================================================
//	ADPCM Volume Down ������
//=============================================================================
void WINAPI PMDWIN::setadpcmvoldown(int voldown)
{
	open_work.pcm_voldown = open_work._pcm_voldown = voldown;
}


//=============================================================================
//	PPZ8 Volume Down ������
//=============================================================================
void WINAPI PMDWIN::setppzvoldown(int voldown)
{
	open_work.ppz_voldown = open_work._ppz_voldown = voldown;
}


//=============================================================================
//	FM Volume Down �μ���
//=============================================================================
int WINAPI PMDWIN::getfmvoldown(void)
{
	return open_work.fm_voldown;
}


//=============================================================================
//	FM Volume Down �μ����ʤ��Σ���
//=============================================================================
int WINAPI PMDWIN::getfmvoldown2(void)
{
	return open_work._fm_voldown;
}


//=============================================================================
//	SSG Volume Down �μ���
//=============================================================================
int WINAPI PMDWIN::getssgvoldown(void)
{
	return open_work.ssg_voldown;
}


//=============================================================================
//	SSG Volume Down �μ����ʤ��Σ���
//=============================================================================
int WINAPI PMDWIN::getssgvoldown2(void)
{
	return open_work._ssg_voldown;
}


//=============================================================================
//	Rhythm Volume Down �μ���
//=============================================================================
int WINAPI PMDWIN::getrhythmvoldown(void)
{
	return open_work.rhythm_voldown;
}


//=============================================================================
//	Rhythm Volume Down �μ����ʤ��Σ���
//=============================================================================
int WINAPI PMDWIN::getrhythmvoldown2(void)
{
	return open_work._rhythm_voldown;
}


//=============================================================================
//	ADPCM Volume Down �μ���
//=============================================================================
int WINAPI PMDWIN::getadpcmvoldown(void)
{
	return open_work.pcm_voldown;
}


//=============================================================================
//	ADPCM Volume Down �μ����ʤ��Σ���
//=============================================================================
int WINAPI PMDWIN::getadpcmvoldown2(void)
{
	return open_work._pcm_voldown;
}


//=============================================================================
//	PPZ8 Volume Down �μ���
//=============================================================================
int WINAPI PMDWIN::getppzvoldown(void)
{
	return open_work.ppz_voldown;
}


//=============================================================================
//	PPZ8 Volume Down �μ����ʤ��Σ���
//=============================================================================
int WINAPI PMDWIN::getppzvoldown2(void)
{
	return open_work._ppz_voldown;
}


//=============================================================================
//	�������֤ΰ�ư(pos : ms)
//=============================================================================
void WINAPI PMDWIN::setpos(int pos)
{
	_int64	_pos;
	int		us;

	_pos = (_int64)pos * 1000;			// (ms -> usec �ؤ��Ѵ�)

	if(upos > _pos) {
		mstart();
		pos2 = (char *)wavbuf2;	// buf ��;�äƤ��륵��ץ����Ƭ����
		us2	= 0;				// buf ��;�äƤ��륵��ץ��
		upos = 0;				// ���ճ��Ϥ���λ���(��sec)
	}

	while(upos < _pos) {
		if(opna.ReadStatus() & 0x01) {
			TimerA_main();
		}

		if(opna.ReadStatus() & 0x02) {
			TimerB_main();
		}

		opna.SetReg(0x27, open_work.ch3mode | 0x30);	// TIMER RESET(timerA,B�Ȥ�)

		us = opna.GetNextEvent();
		opna.Count(us);
		upos += us;			
	}

	if(open_work.status2 == -1) {
		silence();
	}
	opna.ClearBuffer();
}


//=============================================================================
//	�������֤ΰ�ư(pos : count ñ��)
//=============================================================================
void WINAPI PMDWIN::setpos2(int pos)
{
	int		us;

	if(open_work.syousetu_lng * open_work.syousetu + open_work.opncount > pos) {
		mstart();
		pos2 = (char *)wavbuf2;	// buf ��;�äƤ��륵��ץ����Ƭ����
		us2	= 0;				// buf ��;�äƤ��륵��ץ��
	}

	while(open_work.syousetu_lng * open_work.syousetu + open_work.opncount < pos) {
		if(opna.ReadStatus() & 0x01) {
			TimerA_main();
		}

		if(opna.ReadStatus() & 0x02) {
			TimerB_main();
		}

		opna.SetReg(0x27, open_work.ch3mode | 0x30);	// TIMER RESET(timerA,B�Ȥ�)

		us = opna.GetNextEvent();
		opna.Count(us);
	}

	if(open_work.status2 == -1) {
		silence();
	}
	opna.ClearBuffer();
}


//=============================================================================
//	�������֤μ���(pos : ms)
//=============================================================================
int WINAPI PMDWIN::getpos(void)
{
	return (int)(upos / 1000);
}


//=============================================================================
//	�������֤μ���(pos : count ñ��)
//=============================================================================
int WINAPI PMDWIN::getpos2(void)
{
	return open_work.syousetu_lng * open_work.syousetu + open_work.opncount;
}


//=============================================================================
//	�ʤ�Ĺ���μ���(pos : ms)
//=============================================================================
bool WINAPI PMDWIN::getlength(char *filename, int *length, int *loop)
{
	int		us;
	int		result;
	int		fmwaittemp;
	int		ssgwaittemp;
	int		rhythmwaittemp;
	int		adpcmwaittemp;

	result = music_load(filename);
	if(result == ERR_OPEN_MUSIC_FILE || result == ERR_WRONG_MUSIC_FILE ||
									result == ERR_OUT_OF_MEMORY) return false;

	mstart();
	upos = 0;				// ���ճ��Ϥ���λ���(��sec)
	*length = 0;

	fmwaittemp		= opna.GetFMWait();
	ssgwaittemp		= opna.GetSSGWait();
	rhythmwaittemp	= opna.GetRhythmWait();
	adpcmwaittemp	= opna.GetADPCMWait();

	opna.SetFMWait(0);
	opna.SetSSGWait(0);
	opna.SetRhythmWait(0);
	opna.SetADPCMWait(0);

	do {
		if(opna.ReadStatus() & 0x01) {
			TimerA_main();
		}

		if(opna.ReadStatus() & 0x02) {
			TimerB_main();
		}

		opna.SetReg(0x27, open_work.ch3mode | 0x30);	// TIMER RESET(timerA,B�Ȥ�)

		us = opna.GetNextEvent();
		opna.Count(us);
		upos += us;
		if(open_work.status2 == 1 && *length == 0) {	// �롼�׻�
			*length = (int)(upos / 1000);
		} else if(open_work.status2 == -1) {			// �롼�פʤ���λ��
			*length = (int)(upos / 1000);
			*loop = 0;
			mstop();
			opna.SetFMWait(fmwaittemp);
			opna.SetSSGWait(ssgwaittemp);
			opna.SetRhythmWait(rhythmwaittemp);
			opna.SetADPCMWait(adpcmwaittemp);
			return true;
		} else if(getpos2() >= 65536) {		// 65536����å��ʾ�ʤ鶯����λ
			*length = (int)(upos / 1000);
			*loop = *length;
			return true;
		}
	} while(open_work.status2 < 2);

	*loop = (int)(upos / 1000) - *length;
	mstop();
	opna.SetFMWait(fmwaittemp);
	opna.SetSSGWait(ssgwaittemp);
	opna.SetRhythmWait(rhythmwaittemp);
	opna.SetADPCMWait(adpcmwaittemp);
	return true;
}


//=============================================================================
//	�ʤ�Ĺ���μ���(pos : count ñ��)
//=============================================================================
bool WINAPI PMDWIN::getlength2(char *filename, int *length, int *loop)
{
	int		us;
	int		result;
	int		fmwaittemp;
	int		ssgwaittemp;
	int		rhythmwaittemp;
	int		adpcmwaittemp;
	
	result = music_load(filename);
	if(result == ERR_OPEN_MUSIC_FILE || result == ERR_WRONG_MUSIC_FILE ||
									result == ERR_OUT_OF_MEMORY) return false;

	mstart();
	upos = 0;				// ���ճ��Ϥ���λ���(��sec)
	*length = 0;

	fmwaittemp		= opna.GetFMWait();
	ssgwaittemp		= opna.GetSSGWait();
	rhythmwaittemp	= opna.GetRhythmWait();
	adpcmwaittemp	= opna.GetADPCMWait();

	opna.SetFMWait(0);
	opna.SetSSGWait(0);
	opna.SetRhythmWait(0);
	opna.SetADPCMWait(0);

	do {
		if(opna.ReadStatus() & 0x01) {
			TimerA_main();
		}

		if(opna.ReadStatus() & 0x02) {
			TimerB_main();
		}

		opna.SetReg(0x27, open_work.ch3mode | 0x30);	// TIMER RESET(timerA,B�Ȥ�)

		us = opna.GetNextEvent();
		opna.Count(us);
		upos += us;			
		if(open_work.status2 == 1 && *length == 0) {	// �롼�׻�
			*length = getpos2();
		} else if(open_work.status2 == -1) {			// �롼�פʤ���λ��
			*length = getpos2();
			*loop = 0;
			mstop();
			opna.SetFMWait(fmwaittemp);
			opna.SetSSGWait(ssgwaittemp);
			opna.SetRhythmWait(rhythmwaittemp);
			opna.SetADPCMWait(adpcmwaittemp);
			return true;
		} else if(getpos2() >= 65536) {		// 65536����å��ʾ�ʤ鶯����λ
			*length = getpos2();
			*loop = *length;
			return true;
		}
	} while(open_work.status2 < 2);

	*loop = getpos2() - *length;
	mstop();
	opna.SetFMWait(fmwaittemp);
	opna.SetSSGWait(ssgwaittemp);
	opna.SetRhythmWait(rhythmwaittemp);
	opna.SetADPCMWait(adpcmwaittemp);
	return true;
}


//=============================================================================
//	�롼�ײ���μ���
//=============================================================================
int WINAPI PMDWIN::getloopcount(void)
{
	return open_work.status2;
}


//=============================================================================
//	FM �� Register ���ϸ�� wait ����
//=============================================================================
void WINAPI PMDWIN::setfmwait(int nsec)
{
	opna.SetFMWait(nsec);
}


//=============================================================================
//	SSG �� Register ���ϸ�� wait ����
//=============================================================================
void WINAPI PMDWIN::setssgwait(int nsec)
{
	opna.SetSSGWait(nsec);
}


//=============================================================================
//	rhythm �� Register ���ϸ�� wait ����
//=============================================================================
void WINAPI PMDWIN::setrhythmwait(int nsec)
{
	opna.SetRhythmWait(nsec);
}


//=============================================================================
//	ADPCM �� Register ���ϸ�� wait ����
//=============================================================================
void WINAPI PMDWIN::setadpcmwait(int nsec)
{
	opna.SetADPCMWait(nsec);
}


//=============================================================================
//	OPEN_WORK�Υݥ��󥿤μ���
//=============================================================================
OPEN_WORK* WINAPI PMDWIN::getopenwork(void)
{
	return &open_work;
}


//=============================================================================
//	�ѡ��ȥ���Υݥ��󥿤μ���
//=============================================================================
QQ* WINAPI PMDWIN::getpartwork(int ch)
{
	if(ch >= sizeof(open_work.MusPart) / sizeof(QQ *)) {
		return NULL;
	}
	return open_work.MusPart[ch];
}


//=============================================================================
//	IUnknown Interface(QueryInterface)
//=============================================================================
#ifdef ENABLE_COM_INTERFACE
HRESULT WINAPI PMDWIN::QueryInterface(
           /* [in] */ REFIID riid,
           /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
{

	if(IsEqualIID(riid, IID_IPCMMUSICDRIVER)) {
		*ppvObject = (IPCMMUSICDRIVER *)this;
	} else if(IsEqualIID(riid, IID_IFMPMD)) {
		*ppvObject = (IFMPMD *)this;
	} else if(IsEqualIID(riid, IID_IPMDWIN)) {
		*ppvObject = (IPMDWIN *)this;
	} else {
		*ppvObject = NULL;
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}


//=============================================================================
//	IUnknown Interface(AddRef)
//=============================================================================
ULONG WINAPI PMDWIN::AddRef(void)
{
	return ++uRefCount;
}


//=============================================================================
//	IUnknown Interface(Release)
//=============================================================================
ULONG WINAPI PMDWIN::Release(void)
{
	ULONG ref = --uRefCount;
	if(ref == 0) {
		delete this;
	}
	return ref;
}
#endif // ENABLE_COM_INTERFACE

//=============================================================================
//	DLL Export Functions
//=============================================================================
PMDWIN* pmdwin;
PMDWIN* pmdwin2;


#ifdef WINDOWS
BOOL WINAPI DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			pmdwin = pmdwin2 = NULL;
			break;

		case DLL_THREAD_ATTACH:
			break;

		case DLL_THREAD_DETACH:
			break;

		case DLL_PROCESS_DETACH:
			delete pmdwin;
			delete pmdwin2;
			break;
    }
    return TRUE;
}
#endif

#ifdef __cplusplus
extern "C" {
#endif


//=============================================================================
//	�С���������
//=============================================================================
__declspec(dllexport) int WINAPI getversion(void)
{
	return	DLLVersion;
}


//=============================================================================
//	���󥿡��ե������ΥС���������
//=============================================================================
__declspec(dllexport) int WINAPI getinterfaceversion(void)
{
	return	InterfaceVersion;
}


//=============================================================================
//	COM �����󥿡��ե�����(PMDWIN ���󥹥��󥹤μ���)	
//=============================================================================
#ifdef ENABLE_COM_INTERFACE
__declspec(dllexport) HRESULT WINAPI pmd_CoCreateInstance(
  REFCLSID rclsid,     //Class identifier (CLSID) of the object
  LPUNKNOWN pUnkOuter, //Pointer to whether object is or isn't part 
                       // of an aggregate
  DWORD dwClsContext,  //Context for running executable code
  REFIID riid,         //Reference to the identifier of the interface
  LPVOID * ppv         //Address of output variable that receives 
                       // the interface pointer requested in riid
)
{
	IUnknown* pUnknown;

	if(rclsid != CLSID_PMDWIN) {
		return REGDB_E_CLASSNOTREG;		// CLSID ���㤦
	} else if(riid != IID_IPCMMUSICDRIVER && riid != IID_IFMPMD && riid != IID_IPMDWIN) {
		return REGDB_E_CLASSNOTREG;		// IID ���㤦
	} else {
		pUnknown = (IUnknown*)new(PMDWIN);
		*ppv = (void **)pUnknown;
		pUnknown->AddRef();
		return S_OK;
	}
}
#endif

//=============================================================================
//	�����
//=============================================================================
__declspec(dllexport) bool WINAPI pmdwininit(char *path)
{
	bool result;

	if(pmdwin == NULL) {
		pmdwin = new PMDWIN;
		pmdwin2 = new PMDWIN;
	}
	
	pmdwin2->init(path);
	result = pmdwin->init(path);
/*
	pmdwin2->setfmwait(0);			// ��Ĺ�׻���®���Τ���
	pmdwin2->setssgwait(0);			// ��Ĺ�׻���®���Τ���
	pmdwin2->setrhythmwait(0);		// ��Ĺ�׻���®���Τ���
	pmdwin2->setadpcmwait(0);		// ��Ĺ�׻���®���Τ���
*/
	return result;
}


//=============================================================================
//	�ꥺ�಻�κ��ɤ߹���
//=============================================================================
__declspec(dllexport) bool WINAPI loadrhythmsample(char *path)
{
	return pmdwin->loadrhythmsample(path);
}


//=============================================================================
//	PCM �����ǥ��쥯�ȥ������
//=============================================================================
__declspec(dllexport) bool WINAPI setpcmdir(char **path)
{
	return pmdwin->setpcmdir(path);
}


//=============================================================================
//	�������ȿ�������
//=============================================================================
__declspec(dllexport) void WINAPI setpcmrate(int rate)
{
	pmdwin->setpcmrate(rate);
}


//=============================================================================
//	PPZ �������ȿ�������
//=============================================================================
__declspec(dllexport) void WINAPI setppzrate(int rate)
{
	pmdwin->setppzrate(rate);
}


//=============================================================================
//	PPS ���Ĥ餹����
//=============================================================================
__declspec(dllexport) void WINAPI setppsuse(bool value)
{
	pmdwin->setppsuse(value);
}


//=============================================================================
//	SSG ���̲��� OPNA Rhythm ���Ĥ餹����
//=============================================================================
__declspec(dllexport) void WINAPI setrhythmwithssgeffect(bool value)
{
	pmdwin->setrhythmwithssgeffect(value);
}


//=============================================================================
//	PMD86 �� PCM �� PMDB2 �ߴ��ˤ��뤫��
//=============================================================================
__declspec(dllexport) void WINAPI setpmd86pcmmode(bool value)
{
	pmdwin->setpmd86pcmmode(value);
}


//=============================================================================
//	PMD86 �� PCM �� PMDB2 �ߴ����ɤ������������
//=============================================================================
__declspec(dllexport) bool WINAPI getpmd86pcmmode(void)
{
	return pmdwin->getpmd86pcmmode();
}


//=============================================================================
//	�ʤ��ɤ߹��ߤ��Σ��ʥե����뤫���
//=============================================================================
__declspec(dllexport) int WINAPI music_load(char *filename)
{
	return pmdwin->music_load(filename);
}


//=============================================================================
//	�ʤ��ɤ߹��ߤ��Σ��ʥ��꤫���
//=============================================================================
__declspec(dllexport) int WINAPI music_load2(uchar *musdata, int size)
{
	return pmdwin->music_load2(musdata, size);
}


//=============================================================================
//	���ճ���
//=============================================================================
__declspec(dllexport) void WINAPI music_start(void)
{
	pmdwin->music_start();
}


//=============================================================================
//	�������
//=============================================================================
__declspec(dllexport) void WINAPI music_stop(void)
{
	pmdwin->music_stop();
}


//=============================================================================
//	�ե����ɥ�����(PMD�ߴ�)
//=============================================================================
__declspec(dllexport) void WINAPI fadeout(int speed)
{
	pmdwin->fadeout(speed);
}


//=============================================================================
//	�ե����ɥ�����(�ⲻ��)
//=============================================================================
__declspec(dllexport) void WINAPI fadeout2(int speed)
{
	pmdwin->fadeout2(speed);
}


//=============================================================================
//	PCM �ǡ�����wave �ǡ����ˤμ���
//=============================================================================
__declspec(dllexport) void WINAPI getpcmdata(short *buf, int nsamples)
{
	pmdwin->getpcmdata(buf, nsamples);
}


//=============================================================================
//	FM �� 55kHz�������켡�䴰���뤫�ɤ���������
//=============================================================================
__declspec(dllexport) void WINAPI setfmcalc55k(bool flag)
{
	pmdwin->setfmcalc55k(flag);
}


//=============================================================================
//	PPS �ǰ켡�䴰���뤫�ɤ���������
//=============================================================================
__declspec(dllexport) void WINAPI setppsinterpolation(bool ip)
{
	pmdwin->setppsinterpolation(ip);
}


//=============================================================================
//	P86 �ǰ켡�䴰���뤫�ɤ���������
//=============================================================================
__declspec(dllexport) void WINAPI setp86interpolation(bool ip)
{
	pmdwin->setp86interpolation(ip);
}


//=============================================================================
//	PPZ8 �ǰ켡�䴰���뤫�ɤ���������
//=============================================================================
__declspec(dllexport) void WINAPI setppzinterpolation(bool ip)
{
	pmdwin->setppzinterpolation(ip);
}


//=============================================================================
//	���μ���
//=============================================================================
__declspec(dllexport) char * WINAPI getmemo(char *dest, uchar *musdata, int size, int al)
{
	return pmdwin->getmemo(dest, musdata, size, al);
}


//=============================================================================
//	���μ����ʣ��Х���Ⱦ�Ѣ�Ⱦ��ʸ�����Ѵ���
//=============================================================================
__declspec(dllexport) char * WINAPI getmemo2(char *dest, uchar *musdata, int size, int al)
{
	return pmdwin->getmemo2(dest, musdata, size, al);
}


//=============================================================================
//	���μ����ʣ��Х���Ⱦ�Ѣ�Ⱦ��ʸ�����Ѵ���ESC�������󥹤ν����
//=============================================================================
__declspec(dllexport) char * WINAPI getmemo3(char *dest, uchar *musdata, int size, int al)
{
	return pmdwin->getmemo3(dest, musdata, size, al);
}


//=============================================================================
//	���μ����ʥե�����̾�����
//=============================================================================
__declspec(dllexport) int WINAPI fgetmemo(char *dest, char *filename, int al)
{
	return pmdwin->fgetmemo(dest, filename, al);
}


//=============================================================================
//	���μ����ʥե�����̾���顿���Х���Ⱦ�Ѣ�Ⱦ��ʸ�����Ѵ���
//=============================================================================
__declspec(dllexport) int WINAPI fgetmemo2(char *dest, char *filename, int al)
{
	return pmdwin->fgetmemo2(dest, filename, al);
}


//=============================================================================
//	���μ����ʥե�����̾���顿Ⱦ��ʸ�����Ѵ���ESC�������󥹤ν����
//=============================================================================
__declspec(dllexport) int WINAPI fgetmemo3(char *dest, char *filename, int al)
{
	return pmdwin->fgetmemo3(dest, filename, al);
}


//=============================================================================
//	�ʤ� filename �μ���
//=============================================================================
__declspec(dllexport) char * WINAPI getmusicfilename(char *dest)
{
	return pmdwin->getmusicfilename(dest);
}


//=============================================================================
//	PPC / P86 filename �μ���
//=============================================================================
__declspec(dllexport) char * WINAPI getpcmfilename(char *dest)
{
	return pmdwin->getpcmfilename(dest);
}


//=============================================================================
//	PPC filename �μ���
//=============================================================================
__declspec(dllexport) char * WINAPI getppcfilename(char *dest)
{
	return pmdwin->getppcfilename(dest);
}


//=============================================================================
//	PPS filename �μ���
//=============================================================================
__declspec(dllexport) char * WINAPI getppsfilename(char *dest)
{
	return pmdwin->getppsfilename(dest);
}


//=============================================================================
//	P86 filename �μ���
//=============================================================================
__declspec(dllexport) char * WINAPI getp86filename(char *dest)
{
	return pmdwin->getp86filename(dest);
}


//=============================================================================
//	PPZ filename �μ���
//=============================================================================
__declspec(dllexport) char * WINAPI getppzfilename(char *dest, int bufnum)
{
	return pmdwin->getppzfilename(dest, bufnum);
}


//=============================================================================
//	.PPC ���ɤ߹��ߡʥե����뤫���
//=============================================================================
__declspec(dllexport) int WINAPI ppc_load(char *filename)
{
	return pmdwin->ppc_load(filename);
}


//=============================================================================
//	.PPS ���ɤ߹��ߡʥե����뤫���
//=============================================================================
__declspec(dllexport) int WINAPI pps_load(char *filename)
{
	return pmdwin->pps_load(filename);
}


//=============================================================================
//	.P86 ���ɤ߹��ߡʥե����뤫���
//=============================================================================
__declspec(dllexport) int WINAPI p86_load(char *filename)
{
	return pmdwin->p86_load(filename);
}


//=============================================================================
//	.PZI, .PVI ���ɤ߹��ߡʥե����뤫���
//=============================================================================
__declspec(dllexport) int WINAPI ppz_load(char *filename, int bufnum)
{
	return pmdwin->ppz_load(filename, bufnum);
}


//=============================================================================
//	�ѡ��ȤΥޥ���
//=============================================================================
__declspec(dllexport) int WINAPI maskon(int ch)
{
	return pmdwin->maskon(ch);
}


//=============================================================================
//	�ѡ��ȤΥޥ������
//=============================================================================
__declspec(dllexport) int WINAPI maskoff(int ch)
{
	return pmdwin->maskoff(ch);
}


//=============================================================================
//	FM Volume Down ������
//=============================================================================
__declspec(dllexport) void WINAPI setfmvoldown(int voldown)
{
	pmdwin->setfmvoldown(voldown);
}


//=============================================================================
//	SSG Volume Down ������
//=============================================================================
__declspec(dllexport) void WINAPI setssgvoldown(int voldown)
{
	pmdwin->setssgvoldown(voldown);
}


//=============================================================================
//	Rhythm Volume Down ������
//=============================================================================
__declspec(dllexport) void WINAPI setrhythmvoldown(int voldown)
{
	pmdwin->setrhythmvoldown(voldown);
}


//=============================================================================
//	ADPCM Volume Down ������
//=============================================================================
__declspec(dllexport) void WINAPI setadpcmvoldown(int voldown)
{
	pmdwin->setadpcmvoldown(voldown);
}


//=============================================================================
//	PPZ8 Volume Down ������
//=============================================================================
__declspec(dllexport) void WINAPI setppzvoldown(int voldown)
{
	pmdwin->setppzvoldown(voldown);
}


//=============================================================================
//	FM Volume Down �μ���
//=============================================================================
__declspec(dllexport) int WINAPI getfmvoldown(void)
{
	return pmdwin->getfmvoldown();
}


//=============================================================================
//	FM Volume Down �μ����ʤ��Σ���
//=============================================================================
__declspec(dllexport) int WINAPI getfmvoldown2(void)
{
	return pmdwin->getfmvoldown2();
}


//=============================================================================
//	SSG Volume Down �μ���
//=============================================================================
__declspec(dllexport) int WINAPI getssgvoldown(void)
{
	return pmdwin->getssgvoldown();
}


//=============================================================================
//	SSG Volume Down �μ����ʤ��Σ���
//=============================================================================
__declspec(dllexport) int WINAPI getssgvoldown2(void)
{
	return pmdwin->getssgvoldown2();
}


//=============================================================================
//	Rhythm Volume Down �μ���
//=============================================================================
__declspec(dllexport) int WINAPI getrhythmvoldown(void)
{
	return pmdwin->getrhythmvoldown();
}


//=============================================================================
//	Rhythm Volume Down �μ����ʤ��Σ���
//=============================================================================
__declspec(dllexport) int WINAPI getrhythmvoldown2(void)
{
	return pmdwin->getrhythmvoldown2();
}


//=============================================================================
//	ADPCM Volume Down �μ���
//=============================================================================
__declspec(dllexport) int WINAPI getadpcmvoldown(void)
{
	return pmdwin->getadpcmvoldown();
}


//=============================================================================
//	ADPCM Volume Down �μ����ʤ��Σ���
//=============================================================================
__declspec(dllexport) int WINAPI getadpcmvoldown2(void)
{
	return pmdwin->getadpcmvoldown2();
}


//=============================================================================
//	PPZ8 Volume Down �μ���
//=============================================================================
__declspec(dllexport) int WINAPI getppzvoldown(void)
{
	return pmdwin->getppzvoldown();
}


//=============================================================================
//	PPZ8 Volume Down �μ����ʤ��Σ���
//=============================================================================
__declspec(dllexport) int WINAPI getppzvoldown2(void)
{
	return pmdwin->getppzvoldown2();
}


//=============================================================================
//	�������֤ΰ�ư(pos : ms)
//=============================================================================
__declspec(dllexport) void WINAPI setpos(int pos)
{
	pmdwin->setpos(pos);
}


//=============================================================================
//	�������֤ΰ�ư(pos : count ñ��)
//=============================================================================
__declspec(dllexport) void WINAPI setpos2(int pos)
{
	pmdwin->setpos2(pos);
}


//=============================================================================
//	�������֤μ���(pos : ms)
//=============================================================================
__declspec(dllexport) int WINAPI getpos(void)
{
	return pmdwin->getpos();
}


//=============================================================================
//	�������֤μ���(pos : count ñ��)
//=============================================================================
__declspec(dllexport) int WINAPI getpos2(void)
{
	return pmdwin->getpos2();
}


//=============================================================================
//	�ʤ�Ĺ���μ���(pos : ms)
//=============================================================================
__declspec(dllexport) bool WINAPI getlength(char *filename, int *length, int *loop)
{
	return pmdwin2->getlength(filename, length, loop);
}


//=============================================================================
//	�ʤ�Ĺ���μ���(pos : count ñ��)
//=============================================================================
__declspec(dllexport) bool WINAPI getlength2(char *filename, int *length, int *loop)
{
	return pmdwin2->getlength2(filename, length, loop);
}

//=============================================================================
//	�롼�ײ���μ���
//=============================================================================
__declspec(dllexport) int WINAPI getloopcount(void)
{
	return pmdwin->getloopcount();
}


//=============================================================================
//	FM �� Register ���ϸ�� wait ����
//=============================================================================
__declspec(dllexport) void WINAPI setfmwait(int nsec)
{
	pmdwin->setfmwait(nsec);
}


//=============================================================================
//	SSG �� Register ���ϸ�� wait ����
//=============================================================================
__declspec(dllexport) void WINAPI setssgwait(int nsec)
{
	pmdwin->setssgwait(nsec);
}


//=============================================================================
//	rhythm �� Register ���ϸ�� wait ����
//=============================================================================
__declspec(dllexport) void WINAPI setrhythmwait(int nsec)
{
	pmdwin->setrhythmwait(nsec);
}


//=============================================================================
//	ADPCM �� Register ���ϸ�� wait ����
//=============================================================================
__declspec(dllexport) void WINAPI setadpcmwait(int nsec)
{
	pmdwin->setadpcmwait(nsec);
}


//=============================================================================
//	OPEN_WORK�Υݥ��󥿤μ���
//=============================================================================
__declspec(dllexport) OPEN_WORK * WINAPI getopenwork(void)
{
	return pmdwin->getopenwork();
}


//=============================================================================
//	�ѡ��ȥ���Υݥ��󥿤μ���
//=============================================================================
__declspec(dllexport) QQ * WINAPI getpartwork(int ch)
{
	return pmdwin->getpartwork(ch);
}


#ifdef __cplusplus
}
#endif
