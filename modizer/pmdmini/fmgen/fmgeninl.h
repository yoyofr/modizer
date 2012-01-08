// ---------------------------------------------------------------------------
//	FM Sound Generator
//	Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//	$Id: fmgeninl.h,v 1.1 2001/04/23 22:25:34 kaoru-k Exp $

#ifndef FM_GEN_INL_H
#define FM_GEN_INL_H

// ---------------------------------------------------------------------------
//	������Σ�
//	
#define FM_PI			3.14159265358979323846

#define FM_SINEPRESIS	2			// EG�ȥ������Ȥ����٤κ�  0(��)-2(��)

#define FM_CLENTS		(0xc00 << FM_SINEPRESIS)	// 0x400(C++) 0x800(asm) �ʾ� 0xc00 ̤��

#define FM_OPSINBITS	10
#define FM_OPSINENTS	(1 << FM_OPSINBITS)

#define FM_EGCBITS		18			// eg �� count �Υ��ե���
#define FM_LFOCBITS		14
#define FM_PGBITS		9

#define FM_ISHIFT		3

#define FM_RATIOBITS	12			// 8-12 ���餤�ޤǡ�

namespace FM
{

// ---------------------------------------------------------------------------
//	Operator
//

//	AM �Υ�٥������
inline void Operator::SetAML(uint l)
{
	aml = l & (FM_LFOENTS - 1);
}

//	PM �Υ�٥������
inline void Operator::SetPML(uint l)
{
	pml = l & (FM_LFOENTS - 1);
}

//	�ե����ɥХå��Хåե��򥯥ꥢ
inline void Operator::ResetFB()
{
	out = out2 = 0;
}

//	��������
inline void Operator::KeyOn()
{
	if (!keyon)
	{
		keyon = true;
		if (phase == off || phase == release)
		{
			ShiftPhase(attack);
			EGUpdate();
			out = out2 = 0;
			pgcount = 0;
		}
	}
}

//	��������
inline void	Operator::KeyOff()
{
	if (keyon)
	{
		keyon = false;
		ShiftPhase(release);
	}
}

//	���ڥ졼���ϲ�Ư�椫��
inline int Operator::IsOn()
{
	return phase - off;
}

//	Detune (0-7)
inline void Operator::SetDT(uint dt)
{
	detune = dt * 0x20, paramchanged = true;
}

//	DT2 (0-3)
inline void Operator::SetDT2(uint dt2)
{
	detune2 = dt2 & 3, paramchanged = true;
}

//	Multiple (0-15)
inline void Operator::SetMULTI(uint mul)	
{ 
	multiple = mul, paramchanged = true;
}

//	Total Level (0-127) (0.75dB step)
inline void Operator::SetTL(uint _tl, bool csm)
{
	if (!csm)
		tl = _tl, paramchanged = true;
	tll = _tl;
}

//	Attack Rate (0-63)
inline void Operator::SetAR(uint _ar)
{
	ar = _ar; paramchanged = true;
}

//	Decay Rate (0-63)
inline void Operator::SetDR(uint _dr)
{ 
	dr = _dr; paramchanged = true;
}

//	Sustain Rate (0-63)
inline void Operator::SetSR(uint _sr)		
{ 
	sr = _sr; paramchanged = true;
}

//	Sustain Level (0-127)
inline void Operator::SetSL(uint _sl)		
{ 
	sl = _sl; paramchanged = true;
}

//	Release Rate (0-63)
inline void Operator::SetRR(uint _rr)		
{ 
	rr = _rr; paramchanged = true;
}

//	Keyscale (0-3)
inline void Operator::SetKS(uint _ks)		
{ 
	ks = _ks; paramchanged = true; 
}

//	SSG-type Envelop (0-15)
inline void Operator::SetSSGEC(uint ssgec)	
{ 
	ssgtype = ssgec; 
}

inline void Operator::SetAMON(bool on)		
{ 
	amon = on;  
	paramchanged = true;
}

inline void Operator::Mute(bool m)
{
	mute = m;
	paramchanged = true;
}

inline void Operator::SetMS(uint _ms)
{
	ms = _ms;
	paramchanged = true;
}

// ---------------------------------------------------------------------------
//	4-op Channel

//	���ڥ졼���μ��� (LFO) ������
inline void Channel4::SetType(OpType type)
{
	for (int i=0; i<4; i++)
		op[i].type = type;
}

//	����ա��ե����ɥХå��졼�Ȥ����� (0-7)
inline void Channel4::SetFB(uint feedback)
{
	fb = fbtable[feedback];
}

//	OPNA �� LFO ������
inline void Channel4::SetMS(uint ms)
{
	op[0].SetMS(ms);
	op[1].SetMS(ms);
	op[2].SetMS(ms);
	op[3].SetMS(ms);
}

//	�����ͥ롦�ޥ���
inline void Channel4::Mute(bool m)
{
	for (int i=0; i<4; i++)
		op[i].Mute(m);
}

//	�����ѥ�᡼����Ʒ׻�
inline void Channel4::Refresh()
{
	for (int i=0; i<4; i++)
		op[i].paramchanged = true;
}

// ---------------------------------------------------------------------------
//
//
inline void StoreSample(Sample& dest, ISample data)
{
	if (sizeof(Sample) == 2)
		dest = (Sample) Limit(dest + data, 0x7fff, -0x8000);
	else
		dest += data;
}

}

#endif // FM_GEN_INL_H
