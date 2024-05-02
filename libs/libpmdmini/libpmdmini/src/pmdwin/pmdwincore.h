//=============================================================================
//	Professional Music Driver [P.M.D.] version 4.8
//			Programmed By M.Kajihara
//			Windows Converted by C60
//=============================================================================

#ifndef PMDWINCORE_H
#define PMDWINCORE_H

#include "portability_fmpmdcore.h"
#include "opnaw.h"
#include "ppz8l.h"
#include "ppsdrv.h"
#include "p86drv.h"
#include "ipmdwin.h"
#include "ifileio.h"



//=============================================================================
//	バージョン情報
//=============================================================================
#define	DLLVersion			 52		// 上１桁：major, 下２桁：minor version

typedef int32_t Sample;

#if defined _WIN32



#pragma pack( push, enter_include1 )
#pragma pack(2)

typedef struct pcmendstag
{
	uint16_t pcmends;
	uint16_t pcmadrs[256][2];
} PCMEnds;

#pragma pack( pop, enter_include1 )

#else

typedef struct pcmendstag
{
	uint16_t pcmends __attribute__((aligned(2)));
	uint16_t pcmadrs[256][2] __attribute__((aligned(2)));
} PCMEnds;

#endif


#define	PVIHeader	"PVI2"
#define	PPCHeader	"ADPCM DATA for  PMD ver.4.4-  "
//#define ver 		"4.8o"
//#define vers		0x48
//#define verc		"o"
//#define date		"Jun.19th 1997"

//#define max_part1	22		// ０クリアすべきパート数(for PMDPPZ)
#define max_part2	11		// 初期化すべきパート数  (for PMDPPZ)

#define mdata_def	64
#define voice_def	 8
#define effect_def	64

#define fmvd_init	0		// ９８は８８よりもＦＭ音源を小さく


//=============================================================================
//	PMDWin class
//=============================================================================

class PMDWIN : public IPMDWIN, ISETFILEIO
{
public:
	PMDWIN();
	virtual ~PMDWIN();
	
	// interpositioned IUnknown
	uint32_t WINAPI AddRef(void);
	uint32_t WINAPI Release(void);
	
	// IPCMMUSICDRIVER
	bool WINAPI init(TCHAR *path);
	int32_t WINAPI music_load(TCHAR *filename);
	int32_t WINAPI music_load2(uint8_t *musdata, int32_t size);
	TCHAR* WINAPI getmusicfilename(TCHAR *dest);
	void WINAPI music_start(void);
	void WINAPI music_stop(void);
	int32_t WINAPI getloopcount(void);
	bool WINAPI getlength(TCHAR *filename, int32_t *length, int32_t *loop);
	int32_t WINAPI getpos(void);
	void WINAPI setpos(int32_t pos);
	void WINAPI getpcmdata(int16_t *buf, int32_t nsamples);
	
	// IFMPMD
	bool WINAPI loadrhythmsample(TCHAR *path);
	bool WINAPI setpcmdir(TCHAR **path);
	void WINAPI setpcmrate(int32_t rate);
	void WINAPI setppzrate(int32_t rate);
	void WINAPI setfmcalc55k(bool flag);
	void WINAPI setppzinterpolation(bool ip);
	void WINAPI setfmwait(int32_t nsec);
	void WINAPI setssgwait(int32_t nsec);
	void WINAPI setrhythmwait(int32_t nsec);
	void WINAPI setadpcmwait(int32_t nsec);
	void WINAPI fadeout(int32_t speed);
	void WINAPI fadeout2(int32_t speed);
	void WINAPI setpos2(int32_t pos);
	int32_t WINAPI getpos2(void);
	TCHAR* WINAPI getpcmfilename(TCHAR *dest);
	TCHAR* WINAPI getppzfilename(TCHAR *dest, int32_t bufnum);
	bool WINAPI getlength2(TCHAR *filename, int32_t *length, int32_t *loop);
	
	// IPMDWIN
	void WINAPI setppsuse(bool value);
	void WINAPI setrhythmwithssgeffect(bool value);
	void WINAPI setpmd86pcmmode(bool value);
	bool WINAPI getpmd86pcmmode(void);
	void WINAPI setppsinterpolation(bool ip);
	void WINAPI setp86interpolation(bool ip);
	int32_t WINAPI maskon(int32_t ch);
	int32_t WINAPI maskoff(int32_t ch);
	void WINAPI setfmvoldown(int32_t voldown);
	void WINAPI setssgvoldown(int32_t voldown);
	void WINAPI setrhythmvoldown(int32_t voldown);
	void WINAPI setadpcmvoldown(int32_t voldown);
	void WINAPI setppzvoldown(int32_t voldown);
	int32_t WINAPI getfmvoldown(void);
	int32_t WINAPI getfmvoldown2(void);
	int32_t WINAPI getssgvoldown(void);
	int32_t WINAPI getssgvoldown2(void);
	int32_t WINAPI getrhythmvoldown(void);
	int32_t WINAPI getrhythmvoldown2(void);
	int32_t WINAPI getadpcmvoldown(void);
	int32_t WINAPI getadpcmvoldown2(void);
	int32_t WINAPI getppzvoldown(void);
	int32_t WINAPI getppzvoldown2(void);
	char* WINAPI getmemo(char *dest, uint8_t *musdata, int32_t size, int32_t al);
	char* WINAPI getmemo2(char *dest, uint8_t *musdata, int32_t size, int32_t al);
	char* WINAPI getmemo3(char *dest, uint8_t *musdata, int32_t size, int32_t al);
	int32_t	WINAPI fgetmemo(char *dest, TCHAR *filename, int32_t al);
	int32_t	WINAPI fgetmemo2(char *dest, TCHAR *filename, int32_t al);
	int32_t	WINAPI fgetmemo3(char *dest, TCHAR *filename, int32_t al);
	TCHAR* WINAPI getppcfilename(TCHAR *dest);
	TCHAR* WINAPI getppsfilename(TCHAR *dest);
	TCHAR* WINAPI getp86filename(TCHAR *dest);
	int32_t WINAPI ppc_load(TCHAR *filename);
	int32_t WINAPI pps_load(TCHAR *filename);
	int32_t WINAPI p86_load(TCHAR *filename);
	int32_t WINAPI ppz_load(TCHAR *filename, int32_t bufnum);
	OPEN_WORK* WINAPI getopenwork(void);
	QQ* WINAPI getpartwork(int32_t ch);
	
	// IFILESTREAM
	void WINAPI setfileio(IFILEIO* pfileio);
	
private:
	FM::OPNAW*	opna;
	PPZ8*		ppz8;
	PPSDRV*		ppsdrv;
	P86DRV*		p86drv;
	FilePath	filepath;
	FileIO*		fileio;
	IFILEIO*	pfileio;
	
	OPEN_WORK open_work;
	QQ FMPart[NumOfFMPart], SSGPart[NumOfSSGPart], ADPCMPart, RhythmPart;
	QQ ExtPart[NumOfExtPart], DummyPart, EffPart, PPZ8Part[NumOfPPZ8Part];
	
	PMDWORK pmdwork;
	EFFWORK effwork;
	
	Stereo16bit	wavbuf2[nbufsample];
	StereoSample	wavbuf[nbufsample];
	StereoSample	wavbuf_conv[nbufsample];
	
	char	*pos2;						// buf に余っているサンプルの先頭位置
	int32_t		us2;						// buf に余っているサンプル数
	int64_t	upos;						// 演奏開始からの時間(μs)
	int64_t	fpos;						// fadeout2 開始時間
	int32_t		seed;						// 乱数の種
	
	uint8_t mdataarea[mdata_def*1024];
	uint8_t vdataarea[voice_def*1024];		//@不要？
	uint8_t edataarea[effect_def*1024];		//@不要？
	PCMEnds pcmends;
	
protected:
	int32_t uRefCount;		// 参照カウンタ
	void _init(void);
	void opnint_start(void);
	void data_init(void);
	void opn_init(void);
	void mstop(void);
	void mstop_f(void);
	void silence(void);
	void mstart(void);
	void mstart_f(void);
	void play_init(void);
	void setint(void);
	void calc_tb_tempo(void);
	void calc_tempo_tb(void);
	void settempo_b(void);
	void FM_Timer_main(void);
	void TimerA_main(void);
	void TimerB_main(void);
	void mmain(void);
	void syousetu_count(void);
	void fmmain(QQ *qq);
	void psgmain(QQ *qq);
	void rhythmmain(QQ *qq);
	void adpcmmain(QQ *qq);
	void pcm86main(QQ *qq);
	void ppz8main(QQ *qq);
	uint8_t *rhythmon(QQ *qq, uint8_t *bx, int32_t al, int32_t *result);
	void effgo(QQ *qq, int32_t al);
	void eff_on2(QQ *qq, int32_t al);
	void eff_main(QQ *qq, int32_t al);
	void effplay(void);
	void efffor(const int32_t *si);
	void effend(void);
	void effsweep(void);
	uint8_t *pdrswitch(QQ *qq, uint8_t *si);
	char* WINAPI _getmemo(char *dest, uint8_t *musdata, int32_t size, int32_t al);
	char* WINAPI _getmemo2(char *dest, uint8_t *musdata, int32_t size, int32_t al);
	char* WINAPI _getmemo3(char *dest, uint8_t *musdata, int32_t size, int32_t al);
	int32_t	WINAPI _fgetmemo(char *dest, TCHAR *filename, int32_t al);
	int32_t	WINAPI _fgetmemo2(char *dest, TCHAR *filename, int32_t al);
	int32_t	WINAPI _fgetmemo3(char *dest, TCHAR *filename, int32_t al);
	
	int32_t silence_fmpart(QQ *qq);
	void keyoff(QQ *qq);
	void kof1(QQ *qq);
	void keyoffp(QQ *qq);
	void keyoffm(QQ *qq);
	void keyoff8(QQ *qq);
	void keyoffz(QQ *qq);
	int32_t ssgdrum_check(QQ *qq, int32_t al);
	uint8_t *commands(QQ *qq, uint8_t *si);
	uint8_t *commandsp(QQ *qq, uint8_t *si);
	uint8_t *commandsr(QQ *qq, uint8_t *si);
	uint8_t *commandsm(QQ *qq, uint8_t *si);
	uint8_t *commands8(QQ *qq, uint8_t *si);
	uint8_t *commandsz(QQ *qq, uint8_t *si);
	uint8_t *special_0c0h(QQ *qq, uint8_t *si, uint8_t al);
	uint8_t *_vd_fm(QQ *qq, uint8_t *si);
	uint8_t *_vd_ssg(QQ *qq, uint8_t *si);
	uint8_t *_vd_pcm(QQ *qq, uint8_t *si);
	uint8_t *_vd_rhythm(QQ *qq, uint8_t *si);
	uint8_t *_vd_ppz(QQ *qq, uint8_t *si);
	uint8_t *comt(uint8_t *si);
	uint8_t *comat(QQ *qq, uint8_t *si);
	uint8_t *comatm(QQ *qq, uint8_t *si);
	uint8_t *comat8(QQ *qq, uint8_t *si);
	uint8_t *comatz(QQ *qq, uint8_t *si);
	uint8_t *comstloop(QQ *qq, uint8_t *si);
	uint8_t *comedloop(QQ *qq, uint8_t *si);
	uint8_t *comexloop(QQ *qq, uint8_t *si);
	uint8_t *extend_psgenvset(QQ *qq, uint8_t *si);
	int32_t lfoinit(QQ *qq, int32_t al);
	int32_t lfoinitp(QQ *qq, int32_t al);
	uint8_t *lfoset(QQ *qq, uint8_t *si);
	uint8_t *psgenvset(QQ *qq, uint8_t *si);
	uint8_t *rhykey(uint8_t *si);
	uint8_t *rhyvs(uint8_t *si);
	uint8_t *rpnset(uint8_t *si);
	uint8_t *rmsvs(uint8_t *si);
	uint8_t *rmsvs_sft(uint8_t *si);
	uint8_t *rhyvs_sft(uint8_t *si);
	
	uint8_t *vol_one_up_psg(QQ *qq, uint8_t *si);
	uint8_t *vol_one_up_pcm(QQ *qq, uint8_t *si);
	uint8_t *vol_one_down(QQ *qq, uint8_t *si);
	uint8_t *portap(QQ *qq, uint8_t *si);
	uint8_t *portam(QQ *qq, uint8_t *si);
	uint8_t *portaz(QQ *qq, uint8_t *si);
	uint8_t *psgnoise_move(uint8_t *si);
	uint8_t *mdepth_count(QQ *qq, uint8_t *si);
	uint8_t *toneadr_calc(QQ *qq, int32_t dl);
	void neiroset(QQ *qq, int32_t dl);
	
	int32_t oshift(QQ *qq, int32_t al);
	int32_t oshiftp(QQ *qq, int32_t al);
	void fnumset(QQ *qq, int32_t al);
	void fnumsetp(QQ *qq, int32_t al);
	void fnumsetm(QQ *qq, int32_t al);
	void fnumset8(QQ *qq, int32_t al);
	void fnumsetz(QQ *qq, int32_t al);
	uint8_t *panset(QQ *qq, uint8_t *si);
	uint8_t *panset_ex(QQ *qq, uint8_t *si);
	uint8_t *pansetm(QQ *qq, uint8_t *si);
	uint8_t *panset8(QQ *qq, uint8_t *si);
	uint8_t *pansetz(QQ *qq, uint8_t *si);
	uint8_t *pansetz_ex(QQ *qq, uint8_t *si);
	void panset_main(QQ *qq, int32_t al);
	uint8_t calc_panout(QQ *qq);
	uint8_t *calc_q(QQ *qq, uint8_t *si);
	void fm_block_calc(int32_t *cx, int32_t *ax);
	int32_t ch3_setting(QQ *qq);
	void cm_clear(int32_t *ah, int32_t *al);
	void ch3mode_set(QQ *qq);
	void ch3_special(QQ *qq, int32_t ax, int32_t cx);
	void volset(QQ *qq);
	void volsetp(QQ *qq);
	void volsetm(QQ *qq);
	void volset8(QQ *qq);
	void volsetz(QQ *qq);
	void otodasi(QQ *qq);
	void otodasip(QQ *qq);
	void otodasim(QQ *qq);
	void otodasi8(QQ *qq);
	void otodasiz(QQ *qq);
	void keyon(QQ *qq);
	void keyonp(QQ *qq);
	void keyonm(QQ *qq);
	void keyon8(QQ *qq);
	void keyonz(QQ *qq);
	int32_t lfo(QQ *qq);
	int32_t lfop(QQ *qq);
	uint8_t *lfoswitch(QQ *qq, uint8_t *si);
	void lfoinit_main(QQ *qq);
	void lfo_change(QQ *qq);
	void lfo_exit(QQ *qq);
	void lfin1(QQ *qq);
	void lfo_main(QQ *qq);
	int32_t rnd(int32_t ax);
	void fmlfo_sub(QQ *qq, int32_t al, int32_t bl, uint8_t *vol_tbl);
	void volset_slot(int32_t dh, int32_t dl, int32_t al);
	void porta_calc(QQ *qq);
	int32_t soft_env(QQ *qq);
	int32_t soft_env_main(QQ *qq);
	int32_t soft_env_sub(QQ *qq);
	int32_t ext_ssgenv_main(QQ *qq);
	void esm_sub(QQ *qq, int32_t ah);
	void psgmsk(void);
	int32_t get07(void);
	void md_inc(QQ *qq);
	uint8_t *pcmrepeat_set(QQ *qq, uint8_t * si);
	uint8_t *pcmrepeat_set8(QQ *qq, uint8_t * si);
	uint8_t *ppzrepeat_set(QQ *qq, uint8_t * si);
	uint8_t *pansetm_ex(QQ *qq, uint8_t * si);
	uint8_t *panset8_ex(QQ *qq, uint8_t * si);
	uint8_t *pcm_mml_part_mask(QQ *qq, uint8_t * si);
	uint8_t *pcm_mml_part_mask8(QQ *qq, uint8_t * si);
	uint8_t *ppz_mml_part_mask(QQ *qq, uint8_t * si);
	void pcmstore(uint16_t pcmstart, uint16_t pcmstop, uint8_t *buf);
	void pcmread(uint16_t pcmstart, uint16_t pcmstop, uint8_t *buf);
	
	uint8_t *hlfo_set(QQ *qq, uint8_t *si);
	uint8_t *vol_one_up_fm(QQ *qq, uint8_t *si);
	uint8_t *porta(QQ *qq, uint8_t *si);
	uint8_t *slotmask_set(QQ *qq, uint8_t *si);
	uint8_t *slotdetune_set(QQ *qq, uint8_t *si);
	uint8_t *slotdetune_set2(QQ *qq, uint8_t *si);
	void fm3_partinit(QQ *qq, uint8_t *ax);
	uint8_t *fm3_extpartset(QQ *qq, uint8_t *si);
	uint8_t *ppz_extpartset(QQ *qq, uint8_t *si);
	uint8_t *volmask_set(QQ *qq, uint8_t *si);
	uint8_t *fm_mml_part_mask(QQ *qq, uint8_t *si);
	uint8_t *ssg_mml_part_mask(QQ *qq, uint8_t *si);
	uint8_t *rhythm_mml_part_mask(QQ *qq, uint8_t *si);
	uint8_t *_lfoswitch(QQ *qq, uint8_t *si);
	uint8_t *_volmask_set(QQ *qq, uint8_t *si);
	uint8_t *tl_set(QQ *qq, uint8_t *si);
	uint8_t *fb_set(QQ *qq, uint8_t *si);
	uint8_t *fm_efct_set(QQ *qq, uint8_t *si);
	uint8_t *ssg_efct_set(QQ *qq, uint8_t *si);
	void fout(void);
	void neiro_reset(QQ *qq);
	int32_t music_load3(uint8_t *musdata, int32_t size, TCHAR *current_dir);
	int32_t ppc_load2(TCHAR *filename);
	int32_t ppc_load3(uint8_t *pcmdata, int32_t size);
	TCHAR *search_pcm(TCHAR *dest, const TCHAR *filename, const TCHAR *current_dir);
	void swap(int32_t *a, int32_t *b);
	
	inline int32_t Limit(int32_t v, int32_t max, int32_t min)
	{
		return v > max ? max : (v < min ? min : v);
	}
};


#endif // PMDWINCORE_H
