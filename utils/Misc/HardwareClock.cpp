#include "HardwareClock.h"
#include <mach/mach_time.h>

namespace st
{


HardwareClock::HardwareClock()
{
	mach_timebase_info_data_t info;
	mach_timebase_info(&info);
	m_clockToSeconds = (double)info.numer/info.denom/1000000000.0;

	Reset();
}


void HardwareClock::Reset()
{
	m_startAbsTime = mach_absolute_time();
	m_lastAbsTime = m_startAbsTime;
	
	m_time = m_startAbsTime*m_clockToSeconds;
	m_deltaTime = 1.0f/60.0f;
}


void HardwareClock::Update()
{
	const uint64_t currentTime = mach_absolute_time();
	const uint64_t dt = currentTime - m_lastAbsTime;

	m_time = currentTime*m_clockToSeconds;
	m_deltaTime = (double)dt*m_clockToSeconds;

	m_lastAbsTime = currentTime;
}


float HardwareClock::GetDeltaTime() const
{
	return m_deltaTime;
}


double HardwareClock::GetTime() const
{
	return m_time;
}

}
