// ---------------------------------------------------------------------------
//	FM Sound Generator
//	Copyright (C) cisc 1998, 2001.
// ---------------------------------------------------------------------------
//	$Id: fmgen.h,v 1.1 2001/04/23 22:25:34 kaoru-k Exp $

#ifndef FM_GEN_H
#define FM_GEN_H

#include "types.h"
#ifndef WINDOWS
# define __stdcall
#endif

// ---------------------------------------------------------------------------
//	出力サンプルの型
//
#define FM_SAMPLETYPE	int32				// int16 or int32

// ---------------------------------------------------------------------------
//	定数その１
//	静的テーブルのサイズ

#define FM_LFOBITS		8					// 変更不可
#define FM_TLBITS		7

// ---------------------------------------------------------------------------

#define FM_TLENTS		(1 << FM_TLBITS)
#define FM_LFOENTS		(1 << FM_LFOBITS)
#define FM_TLPOS		(FM_TLENTS/4)

// ---------------------------------------------------------------------------

namespace FM
{	
	//	Types ----------------------------------------------------------------
	typedef FM_SAMPLETYPE	Sample;
	typedef int32 			ISample;

	enum OpType { typeN=0, typeM=1 };
	
	//	Tables (グローバルなものや asm から参照されるもの等) -----------------
	void MakeTable();
	void MakeTimeTable(uint ratio);
	extern uint32 tltable[];
	extern int32  cltable[];
	extern uint32 dltable[];
	extern int    pmtable[2][8][FM_LFOENTS];
	extern uint   amtable[2][4][FM_LFOENTS];
	extern uint   aml, pml;
	extern int    pmv;		// LFO 変化レベル

	void StoreSample(ISample& dest, int data);

	//	Operator -------------------------------------------------------------
	class Operator
	{
	public:
		Operator();
		static void MakeTable();
		static void	MakeTimeTable(uint ratio);
		
		ISample	Calc(ISample in);
		ISample	CalcL(ISample in);
		ISample CalcFB(uint fb);
		ISample CalcFBL(uint fb);
		ISample CalcN(uint noise);
		void	Prepare();
		void	KeyOn();
		void	KeyOff();
		void	Reset();
		void	ResetFB();
		int		IsOn();

		void	SetDT(uint dt);
		void	SetDT2(uint dt2);
		void	SetMULTI(uint multi);
		void	SetTL(uint tl, bool csm);
		void	SetKS(uint ks);
		void	SetAR(uint ar);
		void	SetDR(uint dr);
		void	SetSR(uint sr);
		void	SetRR(uint rr);
		void	SetSL(uint sl);
		void	SetSSGEC(uint ssgec);
		void	SetFNum(uint fnum);
		void	SetDPBN(uint dp, uint bn);
		void	SetMode(bool modulator);
		void	SetAMON(bool on);
		void	SetMS(uint ms);
		void	Mute(bool);
		
		static void SetAML(uint l);
		static void SetPML(uint l);
		
	private:
		typedef uint32 Counter;
		
		ISample	out, out2;

	//	Phase Generator ------------------------------------------------------
		uint32	PGCalc();
		uint32	PGCalcL();

		uint	dp;			// ΔP
		uint	detune;		// Detune
		uint	detune2;	// DT2
		uint	multiple;	// Multiple
		uint32	pgcount;	// Phase 現在値
		uint32	pgdcount;	// Phase 差分値
		int32	pgdcountl;	// Phase 差分値 >> x

	//	Envelope Generator ---------------------------------------------------
		enum	EGPhase { next, attack, decay, sustain, release, off };
		
		void	EGCalc();
		void	ShiftPhase(EGPhase nextphase);
		void	ShiftPhase2();
		void	SetEGRate(uint);
		void	EGUpdate();
		
		OpType	type;		// OP の種類 (M, N...)
		uint	bn;			// Block/Note
		int		eglevel;	// EG の出力値
		int		eglvnext;	// 次の phase に移る値
		int32	egstep;		// EG の次の変移までの時間
		int32	egstepd;	// egstep の時間差分
		int		egtransa;	// EG 変化の割合 (for attack)
		int		egtransd;	// EG 変化の割合 (for decay)
		int		egout;		// EG+TL を合わせた出力値
		int		tlout;		// TL 分の出力値
		int		pmd;		// PM depth
		int		amd;		// AM depth

		uint	ksr;		// key scale rate
		EGPhase	phase;
		uint*	ams;
		uint8	ms;

		bool	keyon;		// current key state
		
		uint8	tl;			// Total Level	 (0-127)
		uint8	tll;		// Total Level Latch (for CSM mode)
		uint8	ar;			// Attack Rate   (0-63)
		uint8	dr;			// Decay Rate    (0-63)
		uint8	sr;			// Sustain Rate  (0-63)
		uint8	sl;			// Sustain Level (0-127)
		uint8	rr;			// Release Rate  (0-63)
		uint8	ks;			// Keyscale      (0-3)
		uint8	ssgtype;	// SSG-Type Envelop Control

		bool	amon;		// enable Amplitude Modulation
		bool	paramchanged;	// パラメータが更新された
		bool	mute;
		
	//	Tables ---------------------------------------------------------------
		enum TableIndex { dldecay = 0, dlattack = 0x400, };
		
		static Counter ratetable[64];
		static uint32 multable[4][16];

	//	friends --------------------------------------------------------------
		friend class Channel4;
		friend void __stdcall FM_NextPhase(Operator* op);
	};
	
	//	4-op Channel ---------------------------------------------------------
	class Channel4
	{
	public:
		Channel4();
		void SetType(OpType type);
		
		ISample Calc();
		ISample CalcL();
		ISample CalcN(uint noise);
		ISample CalcLN(uint noise);
		void SetFNum(uint fnum);
		void SetFB(uint fb);
		void SetKCKF(uint kc, uint kf);
		void SetAlgorithm(uint algo);
		int Prepare();
		void KeyControl(uint key);
		void Reset();
		void SetMS(uint ms);
		void Mute(bool);
		void Refresh();
		
	private:
		static const uint8 fbtable[8];
		uint	fb;
		int		buf[4];
		int*	in[3];			// 各 OP の入力ポインタ
		int*	out[3];			// 各 OP の出力ポインタ
		int*	pms;

	public:
		Operator op[4];
	};
}

#endif // FM_GEN_H
