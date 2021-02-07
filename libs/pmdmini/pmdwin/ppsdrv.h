//=============================================================================
//		SSG PCM Driver ��PPSDRV�� Unit
//			Original Programmed	by NaoNeko.
//			Modified		by Kaja.
//			Windows Converted by C60
//=============================================================================

#ifndef PPSDRV_H
#define PPSDRV_H

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif
#include "types.h"
//#include "psg.h"
#include "compat.h"

//	DLL �� �����
#define	_PPSDRV_OK					  0		// ���ｪλ
#define	_ERR_OPEN_PPS_FILE			  1		// PPS �򳫤��ʤ��ä�
#define	_ERR_WRONG_PPS_FILE		 	  2		// PPS �η������ۤʤäƤ���
#define	_WARNING_PPS_ALREADY_LOAD	  3		// PPS �Ϥ��Ǥ��ɤ߹��ޤ�Ƥ���
#define	_ERR_OUT_OF_MEMORY			 99		// �������ݤǤ��ʤ��ä�

#define	SOUND_44K		 44100
#define	SOUND_22K		 22050
#define	SOUND_11K		 11025

#define	PPSDRV_VER		"0.37"
#define	MAX_PPS				14
//#define vers		0x03
//#define date		"1994/06/08"


typedef int				Sample;
typedef	unsigned char	uchar;
typedef	unsigned int	uint;


#ifdef WINDOWS
#pragma pack( push, enter_include1 )
#pragma pack(1)
#endif

typedef struct ppsheadertag
{
	struct {
		WORD	address;				// ��Ƭ���ɥ쥹
		WORD	leng;					// �ǡ�����
		BYTE	toneofs;				// ����
		BYTE	volumeofs;				// ����
	} __PACKED__ pcmnum[MAX_PPS];
} __PACKED__ PPSHEADER;


#ifdef WINDOWS
#pragma pack( pop, enter_include1 )
#endif


class PPSDRV
{
public:
	PPSDRV();
	~PPSDRV();
	
	bool Init(uint r, bool ip);						//     �����
	bool Stop(void);								// 00H PDR ���
	bool Play(int num, int shift, int volshift);	// 01H PDR ����
	bool Check(void);								// 02H �����椫�ɤ���check
	bool SetParam(int paramno, bool data);			// 05H PDR�ѥ�᡼��������
	bool GetParam(int paramno);						// 06H PDR�ѥ�᡼���μ���
	
	int Load(char *filename);						// PPS �ɤ߹���
	bool SetRate(uint r, bool ip);					// �졼������
	void SetVolume(int vol);						// ��������
	void Mix(Sample* dest, int nsamples);			// ����

	PPSHEADER ppsheader;							// PCM�β����إå���
	char	pps_file[_MAX_PATH];					// �ե�����̾

private:
	bool	interpolation;							// �䴰���뤫��
	int		rate;
	Sample	*dataarea1;								// PPS ��¸�ѥ���ݥ���
	bool	single_flag;							// ñ���⡼�ɤ���
	bool	low_cpu_check_flag;						// ���ȿ�Ⱦʬ�Ǻ�������
	bool	keyon_flag;								// Keyon �椫��
	Sample	EmitTable[16];
	Sample	*data_offset1;
	Sample	*data_offset2;
	int		data_xor1;								// ���ߤΰ���(������)
	int		data_xor2;								// ���ߤΰ���(������)
	int		tick1;
	int		tick2;
	int		tick_xor1;
	int		tick_xor2;
	int		data_size1;
	int		data_size2;
	int		volume1;
	int		volume2;
	Sample	keyoff_vol;
//	PSG		psg;									// @����

};

#endif	// PPSDRV_H
