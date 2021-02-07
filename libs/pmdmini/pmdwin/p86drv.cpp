//=============================================================================
//		86B PCM Driver ��P86DRV�� Unit
//			programmed by M.Kajihara 96/01/16
//			Windows Converted by C60
//=============================================================================

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<math.h>
#include	"p86drv.h"
#include	"util.h"

//-----------------------------------------------------------------------------
//	���󥹥ȥ饯�����ǥ��ȥ饯��
//-----------------------------------------------------------------------------
P86DRV::P86DRV()
{
	rate = SOUND_44K;			// �������ȿ�
	p86_addr = NULL;			// P86 buffer
	interpolation = false;
	
	memset(&p86header, 0, sizeof(p86header));
	memset(p86_file, 0, sizeof(p86_file));

	srcrate = ratetable[3];		// 16.54kHz
	rate = ratetable[7];		// 44.1kHz
	pcm86_pan_flag = false;		// �ѥ�ǡ�����(bit0=��/bit1=��/bit2=��)
	pcm86_pan_dat = 0;			// �ѥ�ǡ�����(���̤򲼤��륵���ɤβ�����)
	
	start_ofs = NULL;
	start_ofs_x = 0;
	size = 0;
	_start_ofs = NULL;
	_size = 0;
	addsize1 = 0;
	addsize2 = 0;
	ontei = 0;
	repeat_ofs = NULL;
	repeat_size = 0;
	release_ofs = NULL;
	release_size = 0;
	repeat_flag = false;
	release_flag1 = false;
	release_flag2 = false;
	play86_flag = false;

	SetVolume(0);
}


P86DRV::~P86DRV()
{
	if(p86_addr != NULL) {
		free(p86_addr);			// ���곫��
	}
}


//-----------------------------------------------------------------------------
//	�����
//-----------------------------------------------------------------------------
bool P86DRV::Init(uint r, bool ip)
{
	SetVolume(0);
	SetRate(r, ip);
	return true;
}


//-----------------------------------------------------------------------------
//	�������ȿ����켡�䴰��������
//-----------------------------------------------------------------------------
bool P86DRV::SetRate(uint r, bool ip)
{
	uint	_ontei;
	
	rate = r;
	interpolation = ip;

	_ontei = (uint)((unsigned long long)ontei * srcrate / rate);
	addsize2 = (_ontei & 0xffff) >> 4;
	addsize1 = _ontei >> 16;

	return true;
}


//-----------------------------------------------------------------------------
//	����Ĵ����
//-----------------------------------------------------------------------------
void P86DRV::SetVolume(int volume)
{
	MakeVolumeTable(volume);
}

int P86DRV::read_char(void *value)
{
	int temp;
	
	if ((*(uchar *)value) & 0x80)
		temp = -1;
	else
		temp = 0;

	memcpy(&temp,value,sizeof(char));
		
	return temp;
}


//-----------------------------------------------------------------------------
//	���̥ơ��֥����
//-----------------------------------------------------------------------------
void P86DRV::MakeVolumeTable(int volume)
{
	int		i, j;
	int		AVolume_temp;
	double	temp;
	
	AVolume_temp = (int)(0x1000 * pow(10.0, volume / 40.0));
	if(AVolume != AVolume_temp) {
		AVolume = AVolume_temp;
		for(i = 0; i < 16; i++) {
			temp = pow(2.0, (i + 15) / 2.0) * AVolume / 0x18000;
			for(j = 0; j < 256; j++) {
				VolumeTable[i][j] = (Sample)(read_char(&j) * temp);
			}
		}
	}
}


//-----------------------------------------------------------------------------
//	P86 �ɤ߹���
//-----------------------------------------------------------------------------
int P86DRV::Load(char *filename)
{
	FILE		*fp;
	int			i, size;
	P86HEADER	_p86header;
	P86HEADER2	p86header2;
	
	Stop();

	p86_file[0] = '\0';	
	if((fp = fopen(filename, "rb")) == NULL) {
		if(p86_addr != NULL) {
			free(p86_addr);		// ����
			p86_addr = NULL;
			memset(&p86header, 0, sizeof(p86header));
		}
		return _ERR_OPEN_P86_FILE;						//	�ե����뤬�����ʤ�
	}
	
		size = (int)GetFileSize_s(filename);		// �ե����륵����
		fread(&_p86header, 1, sizeof(_p86header), fp);
		
		// P86HEADER �� P86HEADER2 ���Ѵ�
		memset(&p86header2, 0, sizeof(p86header2));
		
		for(i = 0; i < MAX_P86; i++) {
			p86header2.pcmnum[i].start = 
				_p86header.pcmnum[i].start[0] +
				_p86header.pcmnum[i].start[1] * 0x100 + 
				_p86header.pcmnum[i].start[2] * 0x10000 - 0x610;
			p86header2.pcmnum[i].size = 
				_p86header.pcmnum[i].size[0] +
				_p86header.pcmnum[i].size[1] * 0x100 + 
				_p86header.pcmnum[i].size[2] * 0x10000;
		}
		
		if(memcmp(&p86header, &p86header2, sizeof(p86header)) == 0) {
			strcpy(p86_file, filename);
			return _WARNING_P86_ALREADY_LOAD;		// Ʊ���ե�����
		}
		
		if(p86_addr != NULL) {
			free(p86_addr);		// ���ä�����
			p86_addr = NULL;
		}
		
		memcpy(&p86header, &p86header2, sizeof(p86header));
		
		size -= sizeof(_p86header);

		if((p86_addr = (uchar *)malloc(size)) == NULL) {
			return _ERR_OUT_OF_MEMORY;			// ���꤬���ݤǤ��ʤ�
		}

		fread(p86_addr, size, 1, fp);
	
		//	�ե�����̾��Ͽ
		strcpy(p86_file, filename);
	
		fclose(fp);

	return _P86DRV_OK;
}


//-----------------------------------------------------------------------------
//	PCM �ֹ�����
//-----------------------------------------------------------------------------
bool P86DRV::SetNeiro(int num)
{
	_start_ofs = p86_addr + p86header.pcmnum[num].start;
	_size = p86header.pcmnum[num].size;
	repeat_flag = false;
	release_flag1 = false;
	return true;
}


//-----------------------------------------------------------------------------
//	PAN ����
//-----------------------------------------------------------------------------
bool P86DRV::SetPan(int flag, int data)
{
	pcm86_pan_flag = flag;
	pcm86_pan_dat = data;
	return true;	
}


//-----------------------------------------------------------------------------
//	��������
//-----------------------------------------------------------------------------
bool P86DRV::SetVol(int _vol)
{
	vol = _vol;
	return true;
}


//-----------------------------------------------------------------------------
//	�������ȿ�������
//		_srcrate : ���ϥǡ����μ��ȿ�
//			0 : 4.13kHz
//			1 : 5.52kHz
//			2 : 8.27kHz
//			3 : 11.03kHz
//			4 : 16.54kHz
//			5 : 22.05kHz
//			6 : 33.08kHz
//			7 : 44.1kHz
//		_ontei : ���겻��
//-----------------------------------------------------------------------------
bool P86DRV::SetOntei(int _srcrate, uint _ontei)
{
	if(_srcrate < 0 || _srcrate > 7) return false;
	if(_ontei > 0x1fffff) return false;

	ontei = _ontei;
	srcrate = ratetable[_srcrate];
	
	_ontei = (uint)((unsigned long long)_ontei * srcrate / rate);

	addsize2 = (_ontei & 0xffff) >> 4;
	addsize1 = _ontei >> 16;
	return true;
}


//-----------------------------------------------------------------------------
//	��ԡ�������
//-----------------------------------------------------------------------------
bool P86DRV::SetLoop(int loop_start, int loop_end, int release_start, bool adpcm)
{
	int		ax, dx, _dx;

	repeat_flag = true;
	release_flag1 = false;
	dx = _dx = _size;
	
	
	// ����� = ��ԡ��ȳ��ϰ���
	ax = loop_start;
	if(ax >= 0) {
		// ���ξ��
		if(adpcm) ax *= 32;
		if(ax >= _size - 1) ax = _size - 2;		// ����������ȿ�к�
		if(ax < 0) ax = 0;

		repeat_size = _size - ax;		// ��ԡ��ȥ����������ΤΥ�����-������
		repeat_ofs = _start_ofs + ax;	// ��ԡ��ȳ��ϰ��֤�������ͤ�û�
	} else {
		// ��ξ��
		ax = -ax;
		if(adpcm) ax *= 32;
		dx -= ax;
		if(dx < 0) {							// ����������ȿ�к�
			ax = _dx;
			dx = 0;
		}
		
		repeat_size = ax;	// ��ԡ��ȥ�������neg(������)
		repeat_ofs = _start_ofs + dx;	//��ԡ��ȳ��ϰ��֤�(���Υ�����-������)��û�
	
	}
	
	// ������ = ��ԡ��Ƚ�λ����
	ax = loop_end;
	if(ax > 0) {
		// ���ξ��
		if(adpcm) ax *= 32;
		if(ax >= _size - 1) ax = _size - 2;		// ����������ȿ�к�
		if(ax < 0) ax = 0;

		_size = ax;
		dx -= ax;
		// ��ԡ��ȥ���������(�쥵����-��������)�����
		repeat_size -= dx;
	} else if(ax < 0) {
		// ��ξ��
		ax = -ax;
		if(adpcm) ax *= 32;
		if(ax > repeat_size) ax = repeat_size;
		repeat_size -= ax;	// ��ԡ��ȥ���������neg(������)�����
		_size -= ax;			// ����Υ�������������ͤ����
	}
	
	// ������ = ��꡼�����ϰ���
	ax = release_start;
	if((ushort)ax != 0x8000) {				// 8000H�ʤ����ꤷ�ʤ�
		// release���ϰ��� = start���֤�����
		release_ofs = _start_ofs;
		
		// release_size = ����size������
		release_size = _dx;
		
		// ��꡼�����������
		release_flag1 = true;
		if(ax > 0) {
			// ���ξ��
			if(adpcm) ax *= 32;
			if(ax >= _size - 1) ax = _size - 2;		// ����������ȿ�к�
			if(ax < 0) ax = 0;
			
			// �꡼�������������ΤΥ�����-������
			release_size -= ax;
			
			// ��꡼�����ϰ��֤�������ͤ�û�
			release_ofs += ax;
		} else {
			// ��ξ��
			ax = -ax;
			if(adpcm) ax *= 32;
			if(ax > _size) ax = _size;
			
			// ��꡼����������neg(������)
			release_size = ax;
			
			_dx -= ax;
			
			// ��꡼�����ϰ��֤�(���Υ�����-������)��û�
			release_ofs += _dx;
		}
	}
	return true;
}
	
	
//-----------------------------------------------------------------------------
//	P86 ����
//-----------------------------------------------------------------------------
bool P86DRV::Play(void)
{
	start_ofs = _start_ofs;
	start_ofs_x = 0;
	size = _size;

	play86_flag = true;
	release_flag2 = false;
	return true;
}


//-----------------------------------------------------------------------------
//	P86 ���
//-----------------------------------------------------------------------------
bool P86DRV::Stop(void)
{
	play86_flag = false;
	return true;
}


//-----------------------------------------------------------------------------
//	P86 keyoff
//-----------------------------------------------------------------------------
bool P86DRV::Keyoff(void)
{
	if(release_flag1 == true) {		// ��꡼�������ꤵ��Ƥ��뤫?
		start_ofs = release_ofs;
		size = release_size;
		release_flag2 = true;		// ��꡼������
	} else {
		play86_flag = false;
	}
	return true;
}


//-----------------------------------------------------------------------------
//	����
//-----------------------------------------------------------------------------
void P86DRV::Mix(Sample* dest, int nsamples)
{
	if(play86_flag == false) return;
	if(size <= 1) {		// �켡����к�
		play86_flag = false;
		return;	
	}

//	double_trans(dest, nsamples); return;		// @test

	if(interpolation) {
		switch(pcm86_pan_flag) {
			case 0 : double_trans_i(dest, nsamples); break;
			case 1 : left_trans_i(dest, nsamples); break;
			case 2 : right_trans_i(dest, nsamples); break;
			case 3 : double_trans_i(dest, nsamples); break;
			case 4 : double_trans_g_i(dest, nsamples); break;
			case 5 : left_trans_g_i(dest, nsamples); break;
			case 6 : right_trans_g_i(dest, nsamples); break;
			case 7 : double_trans_g_i(dest, nsamples); break;
		}
	} else {
		switch(pcm86_pan_flag) {
			case 0 : double_trans(dest, nsamples); break;
			case 1 : left_trans(dest, nsamples); break;
			case 2 : right_trans(dest, nsamples); break;
			case 3 : double_trans(dest, nsamples); break;
			case 4 : double_trans_g(dest, nsamples); break;
			case 5 : left_trans_g(dest, nsamples); break;
			case 6 : right_trans_g(dest, nsamples); break;
			case 7 : double_trans_g(dest, nsamples); break;
		}
	}
}


//-----------------------------------------------------------------------------
//	������ʰ켡��֤����
//-----------------------------------------------------------------------------
void P86DRV::double_trans_i(Sample* dest, int nsamples)
{
	int		i;
	Sample	data;

	for(i = 0; i < nsamples; i++) {
		data = (VolumeTable[vol][*start_ofs] * (0x1000-start_ofs_x)
				+ VolumeTable[vol][*(start_ofs+1)] * start_ofs_x) >> 12;
		*dest++ += data;
		*dest++ += data;

		if(add_address()) {
			play86_flag = false;
			return;
		}
	}
}


//-----------------------------------------------------------------------------
//	������ʵ��ꡢ�켡��֤����
//-----------------------------------------------------------------------------
void P86DRV::double_trans_g_i(Sample* dest, int nsamples)
{
	int		i;
	Sample	data;


	for(i = 0; i < nsamples; i++) {
		data = (VolumeTable[vol][*start_ofs] * (0x1000-start_ofs_x)
				+ VolumeTable[vol][*(start_ofs+1)] * start_ofs_x) >> 12;
		*dest++ += data;
		*dest++ -= data;

		if(add_address()) {
			play86_flag = false;
			return;
		}
	}
}


//-----------------------------------------------------------------------------
//	�����ʰ켡��֤����
//-----------------------------------------------------------------------------
void P86DRV::left_trans_i(Sample* dest, int nsamples)
{
	int		i;
	Sample	data;

	for(i = 0; i < nsamples; i++) {
		data = (VolumeTable[vol][*start_ofs] * (0x1000-start_ofs_x)
				+ VolumeTable[vol][*(start_ofs+1)] * start_ofs_x) >> 12;
		*dest++ += data;
		data = data * pcm86_pan_dat / (256 / 2);
		*dest++ += data;

		if(add_address()) {
			play86_flag = false;
			return;
		}
	}
}


//-----------------------------------------------------------------------------
//	�����ʵ��ꡢ�켡��֤����
//-----------------------------------------------------------------------------
void P86DRV::left_trans_g_i(Sample* dest, int nsamples)
{
	int		i;
	Sample	data;

	for(i = 0; i < nsamples; i++) {
		data = (VolumeTable[vol][*start_ofs] * (0x1000-start_ofs_x)
				+ VolumeTable[vol][*(start_ofs+1)] * start_ofs_x) >> 12;
		*dest++ += data;
		data = data * pcm86_pan_dat / (256 / 2);
		*dest++ -= data;

		if(add_address()) {
			play86_flag = false;
			return;
		}
	}
}


//-----------------------------------------------------------------------------
//	�����ʰ켡��֤����
//-----------------------------------------------------------------------------
void P86DRV::right_trans_i(Sample* dest, int nsamples)
{
	int		i;
	Sample	data, data2;

	for(i = 0; i < nsamples; i++) {
		data = (VolumeTable[vol][*start_ofs] * (0x1000-start_ofs_x)
				+ VolumeTable[vol][*(start_ofs+1)] * start_ofs_x) >> 12;
		data2 = data * pcm86_pan_dat / (256 / 2);
		*dest++ += data2;
		*dest++ += data;

		if(add_address()) {
			play86_flag = false;
			return;
		}
	}
}


//-----------------------------------------------------------------------------
//	�����ʵ��ꡢ�켡��֤����
//-----------------------------------------------------------------------------
void P86DRV::right_trans_g_i(Sample* dest, int nsamples)
{
	int		i;
	Sample	data, data2;

	for(i = 0; i < nsamples; i++) {
		data = (VolumeTable[vol][*start_ofs] * (0x1000-start_ofs_x)
				+ VolumeTable[vol][*(start_ofs+1)] * start_ofs_x) >> 12;
		data2 = data * pcm86_pan_dat / (256 / 2);
		*dest++ += data2;
		*dest++ -= data;

		if(add_address()) {
			play86_flag = false;
			return;
		}
	}
}


//-----------------------------------------------------------------------------
//	������ʰ켡��֤ʤ���
//-----------------------------------------------------------------------------
void P86DRV::double_trans(Sample* dest, int nsamples)
{
	int		i;
	Sample	data;

	for(i = 0; i < nsamples; i++) {
		data = VolumeTable[vol][*start_ofs];
		*dest++ += data;
		*dest++ += data;

		if(add_address()) {
			play86_flag = false;
			return;
		}
	}
}


//-----------------------------------------------------------------------------
//	������ʵ��ꡢ�켡��֤ʤ���
//-----------------------------------------------------------------------------
void P86DRV::double_trans_g(Sample* dest, int nsamples)
{
	int		i;
	Sample	data;


	for(i = 0; i < nsamples; i++) {
		data = VolumeTable[vol][*start_ofs];
		*dest++ += data;
		*dest++ -= data;

		if(add_address()) {
			play86_flag = false;
			return;
		}
	}
}


//-----------------------------------------------------------------------------
//	�����ʰ켡��֤ʤ���
//-----------------------------------------------------------------------------
void P86DRV::left_trans(Sample* dest, int nsamples)
{
	int		i;
	Sample	data;

	for(i = 0; i < nsamples; i++) {
		data = VolumeTable[vol][*start_ofs];
		*dest++ += data;
		data = data * pcm86_pan_dat / (256 / 2);
		*dest++ += data;

		if(add_address()) {
			play86_flag = false;
			return;
		}
	}
}


//-----------------------------------------------------------------------------
//	�����ʵ��ꡢ�켡��֤ʤ���
//-----------------------------------------------------------------------------
void P86DRV::left_trans_g(Sample* dest, int nsamples)
{
	int		i;
	Sample	data;

	for(i = 0; i < nsamples; i++) {
		data = VolumeTable[vol][*start_ofs];
		*dest++ += data;
		data = data * pcm86_pan_dat / (256 / 2);
		*dest++ -= data;

		if(add_address()) {
			play86_flag = false;
			return;
		}
	}
}


//-----------------------------------------------------------------------------
//	�����ʰ켡��֤ʤ���
//-----------------------------------------------------------------------------
void P86DRV::right_trans(Sample* dest, int nsamples)
{
	int		i;
	Sample	data, data2;

	for(i = 0; i < nsamples; i++) {
		data = VolumeTable[vol][*start_ofs];
		data2 = data * pcm86_pan_dat / (256 / 2);
		*dest++ += data2;
		*dest++ += data;

		if(add_address()) {
			play86_flag = false;
			return;
		}
	}
}


//-----------------------------------------------------------------------------
//	�����ʵ��ꡢ�켡��֤ʤ���
//-----------------------------------------------------------------------------
void P86DRV::right_trans_g(Sample* dest, int nsamples)
{
	int		i;
	Sample	data, data2;

	for(i = 0; i < nsamples; i++) {
		data = VolumeTable[vol][*start_ofs];
		data2 = data * pcm86_pan_dat / (256 / 2);
		*dest++ += data2;
		*dest++ -= data;

		if(add_address()) {
			play86_flag = false;
			return;
		}
	}
}


//-----------------------------------------------------------------------------
//	���ɥ쥹�û�
//-----------------------------------------------------------------------------
bool P86DRV::add_address(void)
{
	start_ofs_x += addsize2;
	if(start_ofs_x >= 0x1000) {
		start_ofs_x -= 0x1000;
		start_ofs++;
		size--;
	}
	start_ofs += addsize1;
	size -= addsize1;
	
	if(size > 1) {		// �켡����к�
		return false;
	} else if(repeat_flag == false || release_flag2) {
		return true;
	}
	
	size = repeat_size;
	start_ofs = repeat_ofs;	
	return false;
}
