#ifndef st_HardwareClock_h_
#define st_HardwareClock_h_

#include <stdint.h>

namespace st
{

class HardwareClock
{
public:
	HardwareClock();

	void Reset();
	void Update();

	float GetDeltaTime() const;
	double GetTime() const;		
	
private:
	double m_clockToSeconds;

	uint64_t m_startAbsTime;
	uint64_t m_lastAbsTime;

	double m_time;
	float m_deltaTime;
};

}

#endif
