//=============================================================================
//		opnaw : OPNA unit with wait
//			ver 0.01
//=============================================================================

#ifndef OPNAW_H
#define OPNAW_H

#include	"opna.h"

// wait �Ƿ׻�����ʬ���������� buffer size(samples)
#define		WAIT_PCM_BUFFER_SIZE  65536


namespace FM {

	class OPNAW : public OPNA
	{
	public:
		OPNAW();
		~OPNAW();
	
		bool	SetRate(uint c, uint r, bool ipflag = false);

		void	SetFMWait(int nsec);				// FM wait(nsec)
		void	SetSSGWait(int nsec);				// SSG wait(nsec)
		void	SetRhythmWait(int nsec);			// Rhythm wait(nsec)
		void	SetADPCMWait(int nsec);				// ADPCM wait(nsec)

		int		GetFMWait(void);					// FM wait(nsec)
		int		GetSSGWait(void);					// SSG wait(nsec)
		int		GetRhythmWait(void);				// Rhythm wait(nsec)
		int		GetADPCMWait(void);					// ADPCM wait(nsec)

		void	SetReg(uint addr, uint data);		// �쥸��������
		void	Mix(Sample* buffer, int nsamples);	// ����
		void	ClearBuffer(void);					// �����Хåե����ꥢ
	
	private:
		Sample	pre_buffer[WAIT_PCM_BUFFER_SIZE * 2];
												// wait ���ι�����������
		int		fmwait;								// FM Wait(nsec)
		int		ssgwait;							// SSG Wait(nsec)
		int		rhythmwait;							// Rhythm Wait(nsec)
		int		adpcmwait;							// ADPCM Wait(nsec)
	
		int		fmwaitcount;						// FM Wait(count*1000)
		int		ssgwaitcount;						// SSG Wait(count*1000)
		int		rhythmwaitcount;					// Rhythm Wait(count*1000)
		int		adpcmwaitcount;						// ADPCM Wait(count*1000)
	
		int		read_pos;							// �񤭹��߰���
		int		write_pos;							// �ɤ߽Ф�����
		int		count2;								// count ������ʬ(*1000)	
	
		void	CalcWaitPCM(int waitcount);			// SetReg() wait ���� PCM ��׻�
	};
}

#endif	// OPNAW_H
