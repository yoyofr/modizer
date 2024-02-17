/*
	This impl controls the SID-chip directly from a "userland" thread 
	that runs on the relatively "isolated" CPU core #3. In spite of 
	giving that thread high prio (etc), the thread is not capable to 
	provide consistent 1 micro second precision:

	It seems the Linux scheduler disrupts the timing with some kind of 
	interrupt that occurs with a 100Hz frequency: introducing average 
	delays of ~7 micros (but which may occasionally peak up to 30 
	micros). Looking at "cat /proc/interrupts", the respective 
	disturbances in core #3 seem to originate from  "arch_timer" 
	(GICv2 30 Level). (Actually it depends on how you built your kernel, 
	and "dyna-tick" based systems will behave differently.)

	For regular songs that just expect some register update every 20ms 
	these glitches are irrelevant as long as the register is eventually 
	updated. 

	However there are songs that depend on very accurately timed, "fast" 
	paced SID writes that are typically performed via an NMI. They try to 
	exploit one of several SID effects that all depend on very exact 
	timing:

	The "distance" between SID writes will "normally" be at least 4 
	cycles - see 6510's "STA/STX/STY instructions" available for writing 
	to SID (under normal conditions there should be plenty of time to 
	correctly perform them in the player - but not when the logic is 
	interrupted for several micros :-( ). Delays may hurt respective 
	songs either due to some SID state not being turned on long enough 
	or to it being turned on for too long.

	(There are READ_MODIFY_WRITE operations that will trigger even 
	faster SID write sequences - but these are NOT currently triggered 
	in WebSid and they are very rarely used - Soundcheck.sid is one of 
	the exceptions that might be affected.) 

	Below an example of a typical digi-player (see LMan - Vortex.sid), 
	where a 4-writes sequence is performed within a 20 micros interval. 
	The routine plays samples at about 8kHz, i.e. it runs in a loop 
	that is repeated every ~125 clock cycles:

	;2257    A2 11       LDX #$11          TRIANGLE + GATE(i.e. start 
	                                       "attack phase") 
	;2259    8E 12 D4    STX $D412  +0
	;225C    A2 09       LDX #$09		   TEST (i.e. reset phase accu 
	                                       at 0) + GATE ; i.e. no 	
	                                       waveform means that current 
	                                       DAC level is sustained
	;225E    8E 12 D4    STX $D412	+6
	;2261    AE 08 04    LDX $0408
	;2264    8E 0F D4    STX $D40F	+14    set frequency of oscillator
	                                       for targeted sample output 
	                                       level GATE (i.e. removal of 
	                                       TEST starts the counting 
										   in phase accu)
	;2269    8E 12 D4    STX $D412	+20    "start seeking", i.e. "seek 
	                                       for the desired output level"
	                                       - this state will stay for 
	                                       ~100 clocks before the above 
	                                       code is restarted

	(see https://codebase64.org/doku.php?id=base:vicious_sid_demo_routine_explained )
										   
	The actual output level is played at line ;225E, i.e. whatever state 
	the SID has been skillfully manouvered into at this point defines 
	what is output for the next 125 clock cycles - the remaining logic 
	just makes sure to end up in the correct state: The concept is to 
	select a specific output level (see line ;2264) using a triangle 
	waveform (i.e. a waveform that linearly changes from minimum to 
	maximum output level - a saw waveform could be used just as well) by 
	letting the oscillator count for an exact(!) period of time (i.e. 
	counting starts from 0 with the write at ;2269 and stops with write 
	at ;225E). Counting for too long or not long enough always results 
	in an incorrect output level. Here it is crucial that the duration 
	that the TEST bit has been set is correct to the cycle!

	Half of this problem could be compensated for by a hack: If a delay 
	is observed during a write that clears the TEST bit, then the next 
	write that sets the TEST bit could just be delayed by the same amount. 
	However, if the delay strikes in the second write, nothing can be 
	done since the SIDs oscillator has already counted too far.. which 
	cannot be undone. Additionally, in the short period between where 
	the actual waveform is turned on (see ;2259) to where it is "frozen" 
	(;225E) a delay might still have some effect.

	This FallbackHandler impl obviously has flaws that cannot be fixed 
	in userland and it is therefore the "fallback" that is used while 
	nothing better is available.
*/

#include <iostream>             // std::cout
#include <unistd.h>             // nice, usleep
#include <thread>               // std::thread
#include <mutex>                // std::mutex, std::unique_lock
#include <condition_variable>   // std::condition_variable
#include <atomic>               // std::atomic, std::atomic_flag, ATOMIC_FLAG_INIT
#include <deque>

#include "rpi4_utils.h"
#include "gpio_sid.h"

#include "fallback_handler.h"

using namespace std;

/**
* MT save management of the buffers available for double-buffering.
* 
* Throttles "buffer producer" thread if necessary. 
*/
class BufferPool {
	mutex _mtx;
	condition_variable _cv;
    deque<uint32_t *> _pool;
public:
	BufferPool(uint32_t *b1, uint32_t *b2) {		
		// initially available buffers (hardcoded to 2)
		_pool.push_back(b1);
		_pool.push_back(b2);
	}
	/**
	* Hands a buffer to the "producer" thread and blocks caller until 
	* a buffer is available.
	*/
	uint32_t *fetchBuffer() {
		// block caller until there is a buffer available..  
		unique_lock<mutex> lck(_mtx);
		
		while (_pool.empty()) {
			_cv.wait(lck);
		}
		uint32_t *result= _pool.back(); // here it doesn't matter which end
		_pool.pop_back();
		
		return result;
	}
	/**
	* Lets "consumer" thread return an unused buffer back to the pool. 
	*/
	void yieldBuffer(uint32_t *buf) {
		// block caller until there is a "fillable" buffer available..  
		unique_lock<mutex> lck(_mtx);
		_pool.push_front(buf);				// doesn't matter which end
		
		_cv.notify_all();	// wake up waiting main if necessarry
	}
};

/**
* MT save "FIFO work queue" containing the buffers that can be played 
* by the PlaybackThread.
*/
class WorkQueue {
	mutex _mtx;
    deque<uint32_t *> _work;
public:
	uint32_t *popBuffer() {
		unique_lock<mutex> lck(_mtx);
		if (_work.empty()) return 0;
		
		uint32_t *result= _work.back(); // remove oldest from back
		_work.pop_back();
		
		return result;
	}
	void pushBuffer(uint32_t *buf) {
		unique_lock<mutex> lck(_mtx);
		
		_work.push_front(buf);	// add new in front
		
		// in worst case the playback thread will just be busy polling 
		// and there is no need to wake it up
	}
};


// this hack does not seem to be worth the trouble.. especially when
// kernel is compiled with NO_HZ_FULL and the respective cmdline.txt
// config - which seems to keep interrupts off cpu core #3 nicely..
//#define FIX_TIMING_HACK

#ifdef FIX_TIMING_HACK
#define TEST_BITMASK 	0x08
#endif

/**
* Re-plays the "SID poke" instructions fed by the main thread.
*
* Respective instructions are referred to as "scripts" below and they 
* come in ~20ms batches. A simple double buffering is used to let main 
* thread feed instructions while this one is playing. The separate 
* playback thread decouples the actual emulator from the timing critical 
* SID access.
*
* known limitation: the 32-bit counter will overflow every ~70 minutes 
* and the below logic will not currently handle this gracefully.  
* todo: check if use of the full 64-bit counter would cause any relevent
* slowdown
*/
class PlaybackThread {
	
	// each script buffer entry consists of 2 uint32_t values where the 
	// first value is an offset (in micros) to be matched against the 
	// system timer and the second value contains the sid-register and 
	// value to be written:
	// value 1) bits 31-0; timestamp in micros; a 0 marks the end of the 
	//          script timestamps are relative to the start of the playback 
	//          and must be adjusted for matching with the system timer
	// value 2) bits 31-16: unused; bits 15-8: SID register offset; 
	//          bits 7-0: value to write
	
	// the below two buffers are always "owned" by one specific thread at 
	// a time and there is no concurrent access to the buffer
	uint32_t _script_buf1[(MAX_ENTRIES+1) * 2]; 
	uint32_t _script_buf2[(MAX_ENTRIES+1) * 2]; 
	
	uint32_t _reset_script[49] = {
		// reset all SID registers to 0
		1, (0x0100),
		1, (0x0200),
		1, (0x0300),
		1, (0x0400),
		1, (0x0500),
		1, (0x0600),
		1, (0x0700),
		1, (0x0800),
		1, (0x0900),
		1, (0x0A00),
		1, (0x0B00),
		1, (0x0C00),
		1, (0x0D00),
		1, (0x0E00),
		1, (0x0F00),
		1, (0x1000),
		1, (0x1100),
		1, (0x1200),
		1, (0x1300),
		1, (0x1400),
		1, (0x1500),
		1, (0x1600),
		1, (0x1700),
		1, (0x1800),
		0
	};
	
	// MT safe stuff which is actually used concurrently:
	
	BufferPool* _buffer_pool;		// available buffers that can be filled
	WorkQueue _work_queue;	
	atomic<bool> _run_next_script;	// used to end the thread's loop
		
#ifdef FIX_TIMING_HACK
	// partially compensate timing issues
	uint8_t _test_bit[3];	// current state of the SID voice's test bit
	uint32_t _delay[3];		// delay observed setting a SID voice's the test-bit
#endif
	void playScript(uint32_t *buf, uint32_t *ts_offset) {
		uint32_t i = 0;		
		uint32_t ts = buf[i];

		if (!(*ts_offset)) {
			(*ts_offset) = micros(); 	// sync playback with current counter
		}
				
		while(ts != 0) {
			uint32_t &data = buf[i+1];
			uint8_t reg = (data>>8) & 0x1f;
			uint8_t value = data & 0xff;
			
#ifdef FIX_TIMING_HACK
			uint8_t voice_idx = 0;
			uint8_t voice_reg = reg;	
			if (reg < 7) {}
			else if (reg <= 13) { voice_idx = 1; voice_reg = reg - 7; }
			else if (reg <= 20) { voice_idx = 2; voice_reg = reg - 14; }
#endif						
			ts+= (*ts_offset);
			i+= 2;			// 2x uint32_t per entry
			
			uint32_t now = micros();
			if (now > ts) {
				// problem: the current write has been delayed
				
#ifdef FIX_TIMING_HACK
				if (voice_reg == 0x4) {	// voice's waveform control
					if (value & TEST_BITMASK) {
						// if clearing was delayed then setting should be equally delayed
						ts += _delay[voice_idx];
						
						// osc already counted too long.. nothing to be done about that
						_delay[voice_idx] = 0;
					} else {
						// if clearing was delayed then setting should be equally delayed
						_delay[voice_idx] = _test_bit[voice_idx] ? now-ts : 0;
					}
					_test_bit[voice_idx] = value & TEST_BITMASK;
				}
#endif				
			} else {
				
#ifdef FIX_TIMING_HACK
				if (voice_reg == 0x4) {	// voice's waveform control
					if (value & TEST_BITMASK) {
						// if clearing was delayed then setting should be equally delayed
						ts += _delay[voice_idx];
					}
					_test_bit[voice_idx] = value & TEST_BITMASK;
					_delay[voice_idx] = 0;
				}
#endif				
				// as long as the player has not been delayed by some interrupt, 
				// then ts should always be in the future when we get here..
				while (micros()< ts) {/* just wait and poll  */}
			}

			gpioPokeSID(reg, value);

			ts = buf[i];			
		}
	}
	
	void playLoop() {
		migrateThreadToCore(3);
		
		uint32_t ts_offset= 0;
		
		while (_run_next_script) {
			uint32_t *buf = 0;
			while (buf == 0) { 
				buf = _work_queue.popBuffer();
				// just wait for main to feed some data
				if (!_run_next_script) return;
			}			
			if (buf == (uint32_t*)_reset_script) {
				_run_next_script = false;	// this is the last run
			}
			
			playScript(buf, &ts_offset);
			
			// done with this buffer
			_buffer_pool->yieldBuffer(buf);			
		}
	}
	
public:
	PlaybackThread() {
		// if core #3 has been properly configured in /boot/cmdline.txt then 
		// this thread should be pretty "alone" on that core.. but just in case:		
		scheduleRT();	// XXXX error: this changes the main thread... not the player thread!!!
		
		_run_next_script = true;
		_buffer_pool = new BufferPool(_script_buf1, _script_buf2);
	}
	virtual ~PlaybackThread() {		
		delete _buffer_pool;
	}
	void resetSID()  {
		// feed a script that resets SID to mute 
		// annoying beeps when interrupting a song..
		feedBuffer(_reset_script);
	}
	uint32_t *fetchBuffer() {
		return _buffer_pool->fetchBuffer();
	}
	// mandatory: buf must have been obtained via fetchBuffer()
	void feedBuffer(uint32_t *buf) {
		_work_queue.pushBuffer(buf);
	}
	
	thread start() {
		return thread([=] { playLoop(); });
	}
};

// conceptually these are all FallbackHandler instance vars, but since 
// there is never more than FallbackHandler one instance .. I'll rather 
// keep the *.h file as an interface without cluttering it with irrelevant 
// implementation details..

static PlaybackThread *_p;
static thread _player_thread;

FallbackHandler::FallbackHandler() : PlaybackHandler() {
	gpioInitSID();
	
	_p = new PlaybackThread();
	_player_thread = _p->start();	
}

FallbackHandler::~FallbackHandler() {
	_p->resetSID();	// play "SID reset" script before exit	
	_player_thread.join();
	
	delete _p;
	gpioTeardownSID();
}

void FallbackHandler::recordBegin() {
	_script_buf = _p->fetchBuffer(); // playback thread throttles this
	_script_len = 0;	
}

void FallbackHandler::recordEnd() {
	_script_buf[_script_len<<1] = 0;	// end marker
	
	_p->feedBuffer((uint32_t*)_script_buf);
	_script_buf = 0;	// redundant
	_script_len = 0;	// redundant
}
