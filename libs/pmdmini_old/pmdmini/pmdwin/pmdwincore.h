//=============================================================================
//	Professional Music Driver [P.M.D.] version 4.8
//			Programmed By M.Kajihara
//			Windows Converted by C60
//=============================================================================

#ifndef PMDWINCORE_H
#define PMDWINCORE_H

#include <stdlib.h>
#include "opnaw.h"
#include "ppz8l.h"
#include "ppsdrv.h"
#include "p86drv.h"
#include "pmdwin.h"

#if !defined _WIN32
#include	<cstdint>
typedef std::int64_t _int64;
#include "types.h"
#endif

typedef int Sample;

#if defined _WIN32

typedef struct stereosampletag
{
	Sample left;
	Sample right;
} StereoSample;


#pragma pack( push, enter_include1 )
#pragma pack(2)

typedef struct stereo16bittag
{
	short left;
	short right;
} Stereo16bit;

typedef struct pcmendstag
{
	ushort pcmends;
	ushort pcmadrs[256][2];
} PCMEnds;

#pragma pack( pop, enter_include1 )

#else

typedef struct stereosampletag
{
	Sample left __attribute__((aligned(4)));
	Sample right __attribute__((aligned(4)));
} StereoSample __attribute__((aligned(4)));


typedef struct stereo16bittag
{
	short left __attribute__((aligned(2)));
	short right __attribute__((aligned(2)));
} Stereo16bit __attribute__((aligned(2)));

typedef struct pcmendstag
{
	ushort pcmends __attribute__((aligned(2)));
	ushort pcmadrs[256][2] __attribute__((aligned(2)));
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

class PMDWIN : public IPMDWIN
{
public:
	PMDWIN();
	~PMDWIN();
	
	// IUnknown
#if defined _WIN32
	HRESULT WINAPI QueryInterface(
           /* [in] */ REFIID riid,
           /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
	ULONG WINAPI AddRef(void);
	ULONG WINAPI Release(void);
#endif

	// IPCMMUSICDRIVER
	bool WINAPI init(TCHAR *path);
	int WINAPI music_load(TCHAR *filename);
	int WINAPI music_load2(uchar *musdata, int size);
	TCHAR* WINAPI getmusicfilename(TCHAR *dest);
	void WINAPI music_start(void);
	void WINAPI music_stop(void);
	int WINAPI getloopcount(void);
	bool WINAPI getlength(TCHAR *filename, int *length, int *loop);
	int WINAPI getpos(void);
	void WINAPI setpos(int pos);
	void WINAPI getpcmdata(short *buf, int nsamples);
	
	// IFMPMD
	bool WINAPI loadrhythmsample(TCHAR *path);
	bool WINAPI setpcmdir(TCHAR **path);
	void WINAPI setpcmrate(int rate);
	void WINAPI setppzrate(int rate);
	void WINAPI setfmcalc55k(bool flag);
	void WINAPI setppzinterpolation(bool ip);
	void WINAPI setfmwait(int nsec);
	void WINAPI setssgwait(int nsec);
	void WINAPI setrhythmwait(int nsec);
	void WINAPI setadpcmwait(int nsec);
	void WINAPI fadeout(int speed);
	void WINAPI fadeout2(int speed);
	void WINAPI setpos2(int pos);
	int WINAPI getpos2(void);
	TCHAR* WINAPI getpcmfilename(TCHAR *dest);
	TCHAR* WINAPI getppzfilename(TCHAR *dest, int bufnum);
	bool WINAPI getlength2(TCHAR *filename, int *length, int *loop);
	
	// IPMDWIN
	void WINAPI setppsuse(bool value);
	void WINAPI setrhythmwithssgeffect(bool value);
	void WINAPI setpmd86pcmmode(bool value);
	bool WINAPI getpmd86pcmmode(void);
	void WINAPI setppsinterpolation(bool ip);
	void WINAPI setp86interpolation(bool ip);
	int WINAPI maskon(int ch);
	int WINAPI maskoff(int ch);
	void WINAPI setfmvoldown(int voldown);
	void WINAPI setssgvoldown(int voldown);
	void WINAPI setrhythmvoldown(int voldown);
	void WINAPI setadpcmvoldown(int voldown);
	void WINAPI setppzvoldown(int voldown);
	int WINAPI getfmvoldown(void);
	int WINAPI getfmvoldown2(void);
	int WINAPI getssgvoldown(void);
	int WINAPI getssgvoldown2(void);
	int WINAPI getrhythmvoldown(void);
	int WINAPI getrhythmvoldown2(void);
	int WINAPI getadpcmvoldown(void);
	int WINAPI getadpcmvoldown2(void);
	int WINAPI getppzvoldown(void);
	int WINAPI getppzvoldown2(void);
	char* WINAPI getmemo(char *dest, uchar *musdata, int size, int al);
	char* WINAPI getmemo2(char *dest, uchar *musdata, int size, int al);
	char* WINAPI getmemo3(char *dest, uchar *musdata, int size, int al);
	int	WINAPI fgetmemo(char *dest, TCHAR *filename, int al);
	int	WINAPI fgetmemo2(char *dest, TCHAR *filename, int al);
	int	WINAPI fgetmemo3(char *dest, TCHAR *filename, int al);
	TCHAR* WINAPI getppcfilename(TCHAR *dest);
	TCHAR* WINAPI getppsfilename(TCHAR *dest);
	TCHAR* WINAPI getp86filename(TCHAR *dest);
	int WINAPI ppc_load(TCHAR *filename);
	int WINAPI pps_load(TCHAR *filename);
	int WINAPI p86_load(TCHAR *filename);
	int WINAPI ppz_load(TCHAR *filename, int bufnum);
	OPEN_WORK* WINAPI getopenwork(void);
	QQ* WINAPI getpartwork(int ch);
	
private:
	FM::OPNAW	opna;
	PPZ8		ppz8;
	PPSDRV		ppsdrv;
	P86DRV		p86drv;
	FilePath			filepath;
	
	OPEN_WORK open_work;
	QQ FMPart[NumOfFMPart], SSGPart[NumOfSSGPart], ADPCMPart, RhythmPart;
	QQ ExtPart[NumOfExtPart], DummyPart, EffPart, PPZ8Part[NumOfPPZ8Part];
	
	PMDWORK pmdwork;
	EFFWORK effwork;
	
	Stereo16bit	wavbuf2[nbufsample];
	StereoSample	wavbuf[nbufsample];
	StereoSample	wavbuf_conv[nbufsample];
	
	char	*pos2;						// buf に余っているサンプルの先頭位置
	int		us2;						// buf に余っているサンプル数
	_int64	upos;						// 演奏開始からの時間(μs)
	_int64	fpos;						// fadeout2 開始時間
	int		seed;						// 乱数の種
	
	uchar mdataarea[mdata_def*1024];
	uchar vdataarea[voice_def*1024];		//@不要？
	uchar edataarea[effect_def*1024];		//@不要？
	PCMEnds pcmends;
	
protected:
	int uRefCount;		// 参照カウンタ
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
	uchar *rhythmon(QQ *qq, uchar *bx, int al, int *result);
	void effgo(QQ *qq, int al);
	void eff_on2(QQ *qq, int al);
	void eff_main(QQ *qq, int al);
	void effplay(void);
	void efffor(const int *si);
	void effend(void);
	void effsweep(void);
	uchar *pdrswitch(QQ *qq, uchar *si);
	char* WINAPI _getmemo(char *dest, uchar *musdata, int size, int al);
	char* WINAPI _getmemo2(char *dest, uchar *musdata, int size, int al);
	char* WINAPI _getmemo3(char *dest, uchar *musdata, int size, int al);
	int	WINAPI _fgetmemo(char *dest, TCHAR *filename, int al);
	int	WINAPI _fgetmemo2(char *dest, TCHAR *filename, int al);
	int	WINAPI _fgetmemo3(char *dest, TCHAR *filename, int al);
	
	int silence_fmpart(QQ *qq);
	void keyoff(QQ *qq);
	void kof1(QQ *qq);
	void keyoffp(QQ *qq);
	void keyoffm(QQ *qq);
	void keyoff8(QQ *qq);
	void keyoffz(QQ *qq);
	int ssgdrum_check(QQ *qq, int al);
	uchar *commands(QQ *qq, uchar *si);
	uchar *commandsp(QQ *qq, uchar *si);
	uchar *commandsr(QQ *qq, uchar *si);
	uchar *commandsm(QQ *qq, uchar *si);
	uchar *commands8(QQ *qq, uchar *si);
	uchar *commandsz(QQ *qq, uchar *si);
	uchar *special_0c0h(QQ *qq, uchar *si, uchar al);
	uchar *_vd_fm(QQ *qq, uchar *si);
	uchar *_vd_ssg(QQ *qq, uchar *si);
	uchar *_vd_pcm(QQ *qq, uchar *si);
	uchar *_vd_rhythm(QQ *qq, uchar *si);
	uchar *_vd_ppz(QQ *qq, uchar *si);
	uchar *comt(uchar *si);
	uchar *comat(QQ *qq, uchar *si);
	uchar *comatm(QQ *qq, uchar *si);
	uchar *comat8(QQ *qq, uchar *si);
	uchar *comatz(QQ *qq, uchar *si);
	uchar *comstloop(QQ *qq, uchar *si);
	uchar *comedloop(QQ *qq, uchar *si);
	uchar *comexloop(QQ *qq, uchar *si);
	uchar *extend_psgenvset(QQ *qq, uchar *si);
	int lfoinit(QQ *qq, int al);
	int lfoinitp(QQ *qq, int al);
	uchar *lfoset(QQ *qq, uchar *si);
	uchar *psgenvset(QQ *qq, uchar *si);
	uchar *rhykey(uchar *si);
	uchar *rhyvs(uchar *si);
	uchar *rpnset(uchar *si);
	uchar *rmsvs(uchar *si);
	uchar *rmsvs_sft(uchar *si);
	uchar *rhyvs_sft(uchar *si);
	
	uchar *vol_one_up_psg(QQ *qq, uchar *si);
	uchar *vol_one_up_pcm(QQ *qq, uchar *si);
	uchar *vol_one_down(QQ *qq, uchar *si);
	uchar *portap(QQ *qq, uchar *si);
	uchar *portam(QQ *qq, uchar *si);
	uchar *portaz(QQ *qq, uchar *si);
	uchar *psgnoise_move(uchar *si);
	uchar *mdepth_count(QQ *qq, uchar *si);
	uchar *toneadr_calc(QQ *qq, int dl);
	void neiroset(QQ *qq, int dl);
	
	int oshift(QQ *qq, int al);
	int oshiftp(QQ *qq, int al);
	void fnumset(QQ *qq, int al);
	void fnumsetp(QQ *qq, int al);
	void fnumsetm(QQ *qq, int al);
	void fnumset8(QQ *qq, int al);
	void fnumsetz(QQ *qq, int al);
	uchar *panset(QQ *qq, uchar *si);
	uchar *panset_ex(QQ *qq, uchar *si);
	uchar *pansetm(QQ *qq, uchar *si);
	uchar *panset8(QQ *qq, uchar *si);
	uchar *pansetz(QQ *qq, uchar *si);
	uchar *pansetz_ex(QQ *qq, uchar *si);
	void panset_main(QQ *qq, int al);
	uchar calc_panout(QQ *qq);
	uchar *calc_q(QQ *qq, uchar *si);
	void fm_block_calc(int *cx, int *ax);
	int ch3_setting(QQ *qq);
	void cm_clear(int *ah, int *al);
	void ch3mode_set(QQ *qq);
	void ch3_special(QQ *qq, int ax, int cx);
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
	int lfo(QQ *qq);
	int lfop(QQ *qq);
	uchar *lfoswitch(QQ *qq, uchar *si);
	void lfoinit_main(QQ *qq);
	void lfo_change(QQ *qq);
	void lfo_exit(QQ *qq);
	void lfin1(QQ *qq);
	void lfo_main(QQ *qq);
	int rnd(int ax);
	void fmlfo_sub(QQ *qq, int al, int bl, uchar *vol_tbl);
	void volset_slot(int dh, int dl, int al);
	void porta_calc(QQ *qq);
	int soft_env(QQ *qq);
	int soft_env_main(QQ *qq);
	int soft_env_sub(QQ *qq);
	int ext_ssgenv_main(QQ *qq);
	void esm_sub(QQ *qq, int ah);
	void psgmsk(void);
	int get07(void);
	void md_inc(QQ *qq);
	uchar *pcmrepeat_set(QQ *qq, uchar * si);
	uchar *pcmrepeat_set8(QQ *qq, uchar * si);
	uchar *ppzrepeat_set(QQ *qq, uchar * si);
	uchar *pansetm_ex(QQ *qq, uchar * si);
	uchar *panset8_ex(QQ *qq, uchar * si);
	uchar *pcm_mml_part_mask(QQ *qq, uchar * si);
	uchar *pcm_mml_part_mask8(QQ *qq, uchar * si);
	uchar *ppz_mml_part_mask(QQ *qq, uchar * si);
	void pcmstore(ushort pcmstart, ushort pcmstop, uchar *buf);
	void pcmread(ushort pcmstart, ushort pcmstop, uchar *buf);
	
	uchar *hlfo_set(QQ *qq, uchar *si);
	uchar *vol_one_up_fm(QQ *qq, uchar *si);
	uchar *porta(QQ *qq, uchar *si);
	uchar *slotmask_set(QQ *qq, uchar *si);
	uchar *slotdetune_set(QQ *qq, uchar *si);
	uchar *slotdetune_set2(QQ *qq, uchar *si);
	void fm3_partinit(QQ *qq, uchar *ax);
	uchar *fm3_extpartset(QQ *qq, uchar *si);
	uchar *ppz_extpartset(QQ *qq, uchar *si);
	uchar *volmask_set(QQ *qq, uchar *si);
	uchar *fm_mml_part_mask(QQ *qq, uchar *si);
	uchar *ssg_mml_part_mask(QQ *qq, uchar *si);
	uchar *rhythm_mml_part_mask(QQ *qq, uchar *si);
	uchar *_lfoswitch(QQ *qq, uchar *si);
	uchar *_volmask_set(QQ *qq, uchar *si);
	uchar *tl_set(QQ *qq, uchar *si);
	uchar *fb_set(QQ *qq, uchar *si);
	uchar *fm_efct_set(QQ *qq, uchar *si);
	uchar *ssg_efct_set(QQ *qq, uchar *si);
	void fout(void);
	void neiro_reset(QQ *qq);
	int music_load3(uchar *musdata, int size, TCHAR *current_dir);
	int ppc_load2(TCHAR *filename);
	int ppc_load3(uchar *pcmdata, int size);
	TCHAR *search_pcm(TCHAR *dest, const TCHAR *filename, const TCHAR *current_dir);
	void swap(int *a, int *b);
};


#endif // PMDWINCORE_H
