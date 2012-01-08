// ---------------------------------------------------------------------------
//	FM sound generator common timer module
//	Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//	$Id: fmtimer.h,v 1.1 2001/04/23 22:25:34 kaoru-k Exp $

#ifndef FM_TIMER_H
#define FM_TIMER_H

#include "types.h"

// ---------------------------------------------------------------------------

namespace FM
{
	class Timer
	{
	public:
		void	Reset();
		bool	Count(int32 us);
		int32	GetNextEvent();
	
	protected:
		virtual void SetStatus(uint bit) = 0;
		virtual void ResetStatus(uint bit) = 0;

		void	SetTimerBase(uint clock);
		void	SetTimerA(uint addr, uint data);
		void	SetTimerB(uint data);
		void	SetTimerControl(uint data);
		
		uint8	status;
		uint8	regtc;
	
	private:
		virtual void TimerA() {}
		uint8	regta[2];
		
		int32	timera, timera_count;
		int32	timerb, timerb_count;
		int32	timer_step;
	};

// ---------------------------------------------------------------------------
//	�����
//
inline void Timer::Reset()
{
	timera_count = 0;
	timerb_count = 0;
}

} // namespace FM

#endif // FM_TIMER_H
