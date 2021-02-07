//=============================================================================
//		8 Channel PCM Driver ��PPZ8�� Unit(Light Version)
//			Programmed by UKKY
//			Windows Converted by C60
//=============================================================================

#ifndef PPZ8L_H
#define PPZ8L_H

#ifdef HAVE_WINDOWS_H
# include	<windows.h>
#endif
//#include	"types.h"
#include "compat.h"


#define	SOUND_44K		 44100

#define		_PPZ8_VER		"1.07"
#define		RATE_DEF	 SOUND_44K
#define		VNUM_DEF			12
#define		PCM_CNL_MAX 		 8
#define		X_N0			  0x80
#define		DELTA_N0		   127

//	DLL �� �����
#define	_PPZ8_OK				 	  0		// ���ｪλ
#define	_ERR_OPEN_PPZ_FILE			  1		// PVI/PZI �򳫤��ʤ��ä�
#define	_ERR_WRONG_PPZ_FILE		 	  2		// PVI/PZI �η������ۤʤäƤ���
#define	_WARNING_PPZ_ALREADY_LOAD	  3		// PVI/PZI �Ϥ��Ǥ��ɤ߹��ޤ�Ƥ���

#define	_ERR_OUT_OF_MEMORY			 99		// �������ݤǤ��ʤ��ä�

typedef int				Sample;
typedef	unsigned char	uchar;
typedef	unsigned int	uint;

typedef struct channelworktag
{
	int		PCM_ADD_L;				// ���ɥ쥹������ LOW
	int		PCM_ADD_H;				// ���ɥ쥹������ HIGH
	int		PCM_ADDS_L;				// ���ɥ쥹������ LOW�ʸ����͡�
	int		PCM_ADDS_H;				// ���ɥ쥹������ HIGH�ʸ����͡�
	int		PCM_SORC_F;				// ���ǡ����κ����졼��
	int		PCM_FLG;				// �����ե饰
	int		PCM_VOL;				// �ܥ�塼��
	int		PCM_PAN;				// PAN
	int		PCM_NUM;				// PCM�ֹ�
	int		PCM_LOOP_FLG;			// �롼�׻��ѥե饰
	uchar	*PCM_NOW;				// ���ߤ���
	int		PCM_NOW_XOR;			// ���ߤ��͡ʾ�������
	uchar	*PCM_END;				// ���ߤν�λ���ɥ쥹
	uchar	*PCM_END_S;				// �����ν�λ���ɥ쥹
	uchar	*PCM_LOOP;				// �롼�׳��ϥ��ɥ쥹
	uint	PCM_LOOP_START;			// ��˥��ʥ롼�׳��ϥ��ɥ쥹
	uint	PCM_LOOP_END;			// ��˥��ʥ롼�׽�λ���ɥ쥹
	bool	pviflag;				// PVI �ʤ� true
} CHANNELWORK;

#ifdef WINDOWS
#pragma pack( push, enter_include1 )
#pragma pack(1)
#endif

typedef struct pziheadertag
{
	char	header[4];				// 'PZI1'
	char	dummy1[0x0b-4];			// ͽ����
	uchar	pzinum;					// PZI�ǡ����������
	char	dummy2[0x20-0x0b-1];	// ͽ����
	struct	{
		DWORD	startaddress;		// ��Ƭ���ɥ쥹
		DWORD	size;				// �ǡ�����
		DWORD	loop_start;			// �롼�׳��ϥݥ���
		DWORD	loop_end;			// �롼�׽�λ�ݥ���
		WORD	rate;				// �������ȿ�
	} __PACKED__ pcmnum[128];
} __PACKED__ PZIHEADER;

typedef struct pviheadertag
{
	char	header[4];				// 'PVI2'
	char	dummy1[0x0b-4];			// ͽ����
	uchar	pvinum;					// PVI�ǡ����������
	char	dummy2[0x10-0x0b-1];	// ͽ����
	struct	{
		WORD	startaddress;		// ��Ƭ���ɥ쥹
		WORD	endaddress;			// �ǡ�����
	} __PACKED__ pcmnum[128];
} __PACKED__ PVIHEADER;

#ifdef WINDOWS
#pragma pack( pop, enter_include1 )
#endif


class PPZ8
{
public:
	PPZ8();
	~PPZ8();
	
	bool Init(uint rate, bool ip);				// 00H �����
	bool Play(int ch, int bufnum, int num);		// 01H PCM ȯ��
	bool Stop(int ch);							// 02H PCM ���
	int Load(char *filename, int bufnum);		// 03H PVI/PZI�ե�������ɤ߹���
	bool SetVol(int ch, int vol);				// 07H �ܥ�塼������
	bool SetOntei(int ch, uint ontei);			// 0BH �������ȿ�������
	bool SetLoop(int ch, uint loop_start, uint loop_end);
												// 0EH �롼�ץݥ��󥿤�����
	void AllStop(void);							// 12H (PPZ8)�����
	bool SetPan(int ch, int pan);				// 13H (PPZ8)PAN����
	bool SetRate(uint rate, bool ip);			// 14H (PPZ8)�졼������
	bool SetSourceRate(int ch, int rate);		// 15H (PPZ8)���ǡ������ȿ�����
	void SetAllVolume(int vol);					// 16H (PPZ8)���Υܥ�桼��������86B Mixer)
	void SetVolume(int vol);
	//PCMTMP_SET		;17H PCM�ƥ�ݥ������
	//ADPCM_EM_SET		;18H (PPZ8)ADPCM���ߥ�졼��
	//REMOVE_FSET		;19H (PPZ8)�������ե饰����
	//FIFOBUFF_SET		;1AH (PPZ8)FIFO�Хåե����ѹ�
	//RATE_SET		;1BH (PPZ8)WSS�ܺ٥졼������

	void Mix(Sample* dest, int nsamples);

	PZIHEADER PCME_WORK[2];						// PCM�β����إå���
	bool	pviflag[2];							// PVI �ʤ� true
	char	PVI_FILE[2][_MAX_PATH];				// �ե�����̾

private:
	void	WORK_INIT(void);					// ��������
	bool	interpolation;						// �䴰���뤫��
	int		AVolume;
	CHANNELWORK	channelwork[PCM_CNL_MAX];		// �ƥ����ͥ�Υ��
	uchar	*XMS_FRAME_ADR[2];					// XMS�ǳ��ݤ������ꥢ�ɥ쥹�ʥ�˥���
	int		PCM_VOLUME;							// 86B Mixer���Υܥ�塼��
												// (DEF=12)
	int		volume;								// ���Υܥ�塼��
												// (opna unit �ߴ�)
	int		DIST_F;								// �������ȿ�
	
//	static Sample VolumeTable[16][256];			// ���̥ơ��֥�
	Sample VolumeTable[16][256];				// ���̥ơ��֥�
	void	MakeVolumeTable(int vol);			// ���̥ơ��֥�κ���
};

#endif	// PPZ8L_H
