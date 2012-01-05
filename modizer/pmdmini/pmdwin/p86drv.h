//=============================================================================
//		86B PCM Driver ��P86DRV�� Unit
//			programmed by M.Kajihara 96/01/16
//			Windows Converted by C60
//=============================================================================

#ifndef P86DRV_H
#define P86DRV_H

#ifdef HAVE_WINDOWS_H
# include <windows.h>
#endif
#include "types.h"
#include "compat.h"

//	DLL �� �����
#define	_P86DRV_OK					  0		// ���ｪλ
#define	_ERR_OPEN_P86_FILE			  1		// P86 �򳫤��ʤ��ä�
#define	_ERR_WRONG_P86_FILE		 	  2		// P86 �η������ۤʤäƤ���
#define	_WARNING_P86_ALREADY_LOAD	  3		// P86 �Ϥ��Ǥ��ɤ߹��ޤ�Ƥ���
#define	_ERR_OUT_OF_MEMORY			 99		// �������ݤǤ��ʤ��ä�

#define	SOUND_44K				  44100
#define	SOUND_22K				  22050
#define	SOUND_11K				  11025

#define	P86DRV_VER				 "1.1c"
#define	MAX_P86						256
#define vers					   0x11
#define date			"Sep.11th 1996"


typedef int				Sample;
typedef	unsigned char	uchar;
typedef	unsigned int	uint;


#ifdef WINDOWS
#pragma pack( push, enter_include1 )
#pragma pack(1)
#endif

typedef struct p86headertag		// header(original)
{
	char	header[12];			// "PCM86 DATA",0,0
	uchar	Version;
	char	All_Size[3];
	struct {
		uchar	start[3];
		uchar	size[3];
	} __PACKED__ pcmnum[MAX_P86];
} __PACKED__ P86HEADER;

#ifdef WINDOWS
#pragma pack( pop, enter_include1 )
#endif

typedef struct p86headertag2	// header(for PMDWin, int alignment)
{
	char	header[12];			// "PCM86 DATA",0,0
	int		Version;
	int		All_Size;
	struct {
		int		start;
		int		size;
	} pcmnum[MAX_P86];
} P86HEADER2;


const int ratetable[] = {
	4135, 5513, 8270, 11025, 16540, 22050, 33080, 44100
};


class P86DRV
{
public:
	P86DRV();
	~P86DRV();
	
	bool Init(uint r, bool ip);						// �����
	bool Stop(void);								// P86 ���
	bool Play(void);								// P86 ����
	bool Keyoff(void);								// P86 keyoff
	int Load(char *filename);						// P86 �ɤ߹���
	bool SetRate(uint r, bool ip);					// �졼������
	void SetVolume(int volume);						// ���β���Ĵ����
	bool SetVol(int _vol);							// ��������
	bool SetOntei(int rate, uint ontei);			// �������ȿ�������
	bool SetPan(int flag, int data);				// PAN ����
	bool SetNeiro(int num);							// PCM �ֹ�����
	bool SetLoop(int loop_start, int loop_end, int release_start, bool adpcm);
													// �롼������
	void Mix(Sample* dest, int nsamples);			// ����
	char	p86_file[_MAX_PATH];					// �ե�����̾
	P86HEADER2 p86header;							// P86 �β����إå���

private:
	bool	interpolation;							// �䴰���뤫��
	int		rate;									// �������ȿ�
	int		srcrate;								// ���ǡ����μ��ȿ�
	uint	ontei;									// ����(fnum)
	int		vol;									// ����
	uchar	*p86_addr;								// P86 ��¸�ѥ���ݥ���
	uchar	*start_ofs;								// ȯ����PCM�ǡ�������
	int		start_ofs_x;							// ȯ����PCM�ǡ������ϡʾ�������
	int		size;									// �Ĥꥵ����
	uchar	*_start_ofs;							// ȯ������PCM�ǡ�������
	int		_size;									// PCM�ǡ���������
	int		addsize1;								// PCM���ɥ쥹�û��� (������)
	int		addsize2;								// PCM���ɥ쥹�û��� (������)
	uchar	*repeat_ofs;							// ��ԡ��ȳ��ϰ���
	int		repeat_size;							// ��ԡ��ȸ�Υ�����
	uchar	*release_ofs;							// ��꡼�����ϰ���
	int		release_size;							// ��꡼����Υ�����
	bool	repeat_flag;							// ��ԡ��Ȥ��뤫�ɤ�����flag
	bool	release_flag1;							// ��꡼�����뤫�ɤ�����flag
	bool	release_flag2;							// ��꡼���������ɤ�����flag

	int		pcm86_pan_flag;		// �ѥ�ǡ�����(bit0=��/bit1=��/bit2=��)
	int		pcm86_pan_dat;		// �ѥ�ǡ�����(���̤򲼤��륵���ɤβ�����)
	bool	play86_flag;							// ȯ����?flag

	int		AVolume;
//	static	Sample VolumeTable[16][256];			// ���̥ơ��֥�
	Sample VolumeTable[16][256];					// ���̥ơ��֥�

	int     read_char(void *value);
	void	MakeVolumeTable(int volume);
	void	double_trans(Sample* dest, int nsamples);
	void	double_trans_g(Sample* dest, int nsamples);
	void	left_trans(Sample* dest, int nsamples);
	void	left_trans_g(Sample* dest, int nsamples);
	void	right_trans(Sample* dest, int nsamples);
	void	right_trans_g(Sample* dest, int nsamples);
	void	double_trans_i(Sample* dest, int nsamples);
	void	double_trans_g_i(Sample* dest, int nsamples);
	void	left_trans_i(Sample* dest, int nsamples);
	void	left_trans_g_i(Sample* dest, int nsamples);
	void	right_trans_i(Sample* dest, int nsamples);
	void	right_trans_g_i(Sample* dest, int nsamples);
	bool	add_address(void);
};

#endif	// P86DRV_H
