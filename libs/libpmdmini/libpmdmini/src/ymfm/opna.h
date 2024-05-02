// BSD 3-Clause License
//
// Copyright (c) 2021, Aaron Giles
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.



// ---------------------------------------------------------------------------
//	class OPN/OPNA
//	OPN/OPNA に良く似た音を生成する音源ユニット
//	
//	interface:
//	bool Init(uint32_t clock, uint32_t rate, bool, const TCHAR* path);
//		初期化．このクラスを使用する前にかならず呼んでおくこと．
//		OPNA の場合はこの関数でリズムサンプルを読み込む
//
//		clock:	OPNA のクロック周波数(Hz)
//
//		rate:	生成する PCM の標本周波数(Hz)
//
//		path:	リズムサンプルのパス(OPNA のみ有効)
//				省略時はカレントディレクトリから読み込む
//				文字列の末尾には '\' や '/' などをつけること
//
//		返り値	初期化に成功すれば true
//
//	bool LoadRhythmSample(const TCHAR* path)
//		Rhythm サンプルを読み直す．
//		path は Init の path と同じ．
//	
//	bool SetRate(uint32_t clock, uint32_t rate, bool)
//		クロックや PCM レートを変更する
//		引数等は Init を参照のこと．
//	
//	void Mix(FM_SAMPLETYPE* dest, int nsamples)
//		Stereo PCM データを nsamples 分合成し， dest で始まる配列に
//		加える(加算する)
//		・dest には sample*2 個分の領域が必要
//		・格納形式は L, R, L, R... となる．
//		・あくまで加算なので，あらかじめ配列をゼロクリアする必要がある
//		・FM_SAMPLETYPE が short 型の場合クリッピングが行われる.
//		・この関数は音源内部のタイマーとは独立している．
//		  Timer は Count と GetNextEvent で操作する必要がある．
//	
//	void Reset()
//		音源をリセット(初期化)する
//
//	void SetReg(uint32_t reg, uint32_t data)
//		音源のレジスタ reg に data を書き込む
//	
//	uint32_t GetReg(uint32_t reg)
//		音源のレジスタ reg の内容を読み出す
//		読み込むことが出来るレジスタは PSG, ADPCM の一部，ID(0xff) とか
//	
//	uint32_t ReadStatus()/ReadStatusEx()
//		音源のステータスレジスタを読み出す
//		ReadStatusEx は拡張ステータスレジスタの読み出し(OPNA)
//		busy フラグは常に 0
//	
//	bool Count(uint32 t)
//		音源のタイマーを t [μ秒] 進める．
//		音源の内部状態に変化があった時(timer オーバーフロー)
//		true を返す
//
//	uint32 GetNextEvent()
//		音源のタイマーのどちらかがオーバーフローするまでに必要な
//		時間[μ秒]を返す
//		タイマーが停止している場合は ULONG_MAX を返す… と思う
//	
//	void SetVolumeFM(int db)/SetVolumePSG(int db) ...
//		各音源の音量を＋－方向に調節する．標準値は 0.
//		単位は約 1/2 dB，有効範囲の上限は 20 (10dB)
//


#ifndef FM_OPNA_H
#define FM_OPNA_H

#include "portability_fmgen.h"
#include "headers_fmgen.h"
#include "file_fmgen.h"
#include "ymfm_opn.h"


// ---------------------------------------------------------------------------
//	出力サンプルの型
//
#define FM_SAMPLETYPE	int32_t				// int16 or int32


// we use an int64_t as emulated time, as a 16.48 fixed point value
using emulated_time = int64_t;



namespace FM
{
	typedef FM_SAMPLETYPE		Sample;
	typedef int32_t				ISample;
	
	class OPNA : public ymfm::ymfm_interface
	{
	public:
		static constexpr uint32_t DEFAULT_CLOCK = 3993600*2;
		
		// construction
		OPNA(IFILEIO * pfileio);
		~OPNA();
		
		void setfileio(IFILEIO* pfileio);
		
		bool Init(uint32_t c, uint32_t r, bool ip = false, const TCHAR* path = NULL);
		bool SetRate(uint32_t r);
		bool SetRate(uint32_t c, uint32_t r, bool = false);
		bool LoadRhythmSample(const TCHAR*);
		void Reset();
		
		void SetVolumeFM(int db);
		void SetVolumePSG(int db);
		void SetVolumeADPCM(int db);
		void SetVolumeRhythmTotal(int db);
		void SetVolumeRhythm(int index, int db);
		
		void SetReg(uint32_t addr, uint32_t data);
		uint32_t GetReg(uint32_t addr);
		
		uint32_t ReadStatus();
		uint32_t ReadStatusEx();
		
		bool Count(uint32_t us);
		uint32_t GetNextEvent();
		
		void Mix(Sample* buffer, int nsamples);
		
	protected:
		IFILEIO* pfileio;							// ファイルアクセス関連のクラスライブラリ
		
		// internal state
		ymfm::ym2608 m_chip;
		uint32_t m_clock;
		uint64_t m_clocks;
		typename ymfm::ym2608::output_data m_output;
		emulated_time m_step;
		emulated_time m_pos;
		
		uint32_t rate;								// FM 音源合成レート
		
		struct Rhythm
		{
			uint8_t		pan;		// ぱん
			int8_t		level;		// おんりょう
			int			volume;		// おんりょうせってい
			int16_t*	sample;		// さんぷる
			uint32_t	size;		// さいず
			uint32_t	pos;		// いち
			uint32_t	step;		// すてっぷち
			uint32_t	rate;		// さんぷるのれーと
		};
		
		bool frhythm_adpcm_rom;
		
		static constexpr int32_t FM_TLBITS = 7;
		static constexpr int32_t FM_TLENTS = (1 << FM_TLBITS);
		static constexpr int32_t FM_TLPOS = (FM_TLENTS / 4);
		
		int32_t tltable[FM_TLENTS + FM_TLPOS];
		
		Rhythm	rhythm[6];
		int32_t	rhythmtvol;
		int8_t	rhythmtl;		// リズム全体の音量
		uint8_t	rhythmkey;		// リズムのキー
		
		// internal state
		std::vector<uint8_t> m_data[ymfm::ACCESS_CLASSES];
		
		emulated_time output_step;
		emulated_time output_pos;
		
		emulated_time timer_period[2];
		emulated_time timer_count[2];
		uint8_t reg27;
		
		void RhythmMix(Sample* buffer, uint32_t count);
		
		void StoreSample(Sample& dest, ISample data);
		
		// generate one output sample of output
		virtual void generate(emulated_time output_start, emulated_time output_step, int32_t* buffer);
		
		// write data to the ADPCM-A buffer
		void write_data(ymfm::access_class type, uint32_t base, uint32_t length, uint8_t const* src);
		
		// simple getters
		uint32_t sample_rate() const;
		
		// handle a read from the buffer
		virtual uint8_t ymfm_external_read(ymfm::access_class type, uint32_t offset) override;
		
		virtual void ymfm_external_write(ymfm::access_class type, uint32_t address, uint8_t data) override;
		
		virtual void ymfm_sync_mode_write(uint8_t data) override;
		
		virtual void ymfm_set_timer(uint32_t tnum, int32_t duration_in_clocks) override;
		
	};
}


inline int Limit(int v, int max, int min) {
	return ymfm::clamp(v, min, max);
}


#endif // FM_OPNA_H
