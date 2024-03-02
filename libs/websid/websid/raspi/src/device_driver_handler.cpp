/*
 * Interacts with a websid device-driver (aka kernel module) that 
 * performs the timing critical SID chip pokes.. respective interactions 
 * are performed using a shared-memory area provided by the kernel module.
 */

#include <fcntl.h>
#include <unistd.h>		// close
#include <iostream>		// cout
#include <unistd.h>		// sleep
#include <csignal>
#include <sys/mman.h>

#include "../websid_module/websid_driver_api.h"

#include "rpi4_utils.h"
#include "gpio_sid.h"

#include "device_driver_handler.h"


using namespace std;


void DeviceDriverHandler::detectDeviceDriver(int *fd, volatile uint32_t **buffer_base) {
	// check of device driver is installed on the machine

	*fd = open("/dev/websid", O_RDWR);
		
	if((*fd) >= 0) {		
		*buffer_base = (uint32_t*) mmap(0, AREA_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, *fd, 0);
		
		if ((*buffer_base) == MAP_FAILED)	{
			cout << "error: could not map shared memory" << endl;

			close(*fd);
			*fd = -1;
			*buffer_base = 0;
		} else {
//			cout << "connected to /dev/websid " << (uint32_t)(*buffer_base) << endl;
		}
	} else {
//		cout << "error: could not connect to /dev/websid\n" << endl;
	}
}

// conceptually these are all DeviceDriverHandler instance vars, but 
// since there is never more than one DeviceDriverHandler instance .. 
// I'll rather keep the *.h file as an interface without cluttering it 
// with irrelevant implementation details..

static int _device_fd = -1;

static volatile uint8_t *_buffer_base;
static int32_t _buffer_offset;	// offset (in bytes, relative to _buffer_base)

// flags used to send signals between userland and kernel space
static volatile uint32_t *_fetch_flag_ptr = 0;	
static volatile uint32_t *_feed_flag_ptr = 0;


DeviceDriverHandler::DeviceDriverHandler(int device_fd, volatile uint32_t *buffer_base)  
		: PlaybackHandler() {
		
	_device_fd = device_fd;
	_buffer_base = (volatile uint8_t *)buffer_base;	// page aligned
	
	if (_buffer_base == 0) {
		cout << "error: no valid device buffer available" << endl;
		exit(-1);
	}
	
	_fetch_flag_ptr = (volatile uint32_t *)(_buffer_base + (FETCH_FLAG_OFFSET << 2));
	_feed_flag_ptr = (volatile uint32_t *)(_buffer_base + (FEED_FLAG_OFFSET << 2));

	gpioInitClockSID();
	
	// necessary to avoid: "sched: RT throttling activated"
	// Defines the period in μs (microseconds) to be considered as 100% of CPU bandwidth.
	system("sudo echo '600000000' > /proc/sys/kernel/sched_rt_period_us");	//  default: 1000000	
	
	// precondition!: run this thread on isolated core#2 - otherwise it will starve regular system
	scheduleRT(); 
	
	// disable to avoid having the logs filled with these:
	system("sudo echo '600' > /proc/sys/kernel/hung_task_timeout_secs");	//  default: 120
	system("sudo echo '1' >  /sys/module/rcupdate/parameters/rcu_cpu_stall_suppress");	//  default: 0	
}

DeviceDriverHandler::~DeviceDriverHandler() {

	// the device driver "close" of the current user will cause the 
	// current kthread to exit and an "open" from the next user will 
	// re-init the _fetch_flag_ptr & _feed_flag_ptr flags and therefore 
	// there is no need for respective cleanups here
	
    if( (_buffer_base == 0) || munmap((void*)_buffer_base, AREA_SIZE) ) {
		cout << "error: failed to unmap device" << endl;
	} else {
		_buffer_base = 0;
	}
	
	close(_device_fd);
	
	gpioTeardownSID();
}

// timeout during playback might use 20ms but since PSID "INIT" calls may 
// take longer, keep it simple (first SID writes may occur at the end of
// song slow/lengthy INIT, i.e. the respective 1st buffer may cover several
// seconds..)
#define DRIVER_TIMEOUT 10000000

// just in case
#define barrier() __asm__ __volatile__("": : :"memory")

uint32_t DeviceDriverHandler::fetchBuffer() {
	// blocks until a buffer is available in the kernal driver.. 

	// typically new buffers must be supplied every 17-20ms,
	// i.e. if it takes longer to fetch a new buffer then the
	// playback is doomed anyway
	
	uint32_t addr_offset = 0;// error that means "unavailable"

	TIMOUT_HANDLER(now, DRIVER_TIMEOUT);	
	while (KEEP_TRYING()) {	
		// availability of the buffer should be detected as quickly 
		// as possible since any time wasted here substracts from the 
		// 17-20ms available to produce the next buffer
				
		barrier(); 
		if (*_fetch_flag_ptr) {
			// pass "ownership" of the buffer to "userland"
			addr_offset = *_fetch_flag_ptr;
			(*_fetch_flag_ptr) = 0;
			barrier(); 
			break;
		}
		now = micros();
	}	
	return addr_offset;
}

void DeviceDriverHandler::flushBuffer(uint32_t addr_offset) {
	// it might be prudent to signal that the respective buffer area 
	// has been updated and respective caches (if any) need to be 
	// reloaded.. (though it seems to not make any difference... 
	// i.e. below use of msync might be useless..)

	void* start_page = (void*)(_buffer_base + addr_offset);	// is page aligned
	
	// 8 bytes per entry and a 4-byte end marker
	uint32_t len = (_script_len << 3) + sizeof(uint32_t);	

	// MS_SYNC always seems to throw an EINVAL error here...	supposedly:
	//  "EINVAL addr is not a multiple of PAGESIZE; or any bit other than
	//			MS_ASYNC | MS_INVALIDATE | MS_SYNC is set in flags; or
	//			both MS_SYNC and MS_ASYNC are set in flags." 
	// => except that this doc is once again incorrect garbage

	if(len && msync(start_page, len, MS_INVALIDATE)) {
		cout << "error msync: start: "<< (uint32_t)start_page <<
				" len: " << len << " errno: " << errno << endl;
	}
	pushFeedFlag(addr_offset) ;		
}

void DeviceDriverHandler::pushFeedFlag(uint32_t addr_offset) {
	// one would hope that this change becomes visible after the 
	// previously updated buffer.. otherwise the player might play
	// inconsistent buffer data..
	
	barrier(); 
	*_feed_flag_ptr = addr_offset;
	barrier(); 

	if(addr_offset) {
		msync((void*)(_buffer_base + (FEED_FLAG_OFFSET<<2)), 
				sizeof(uint32_t), MS_INVALIDATE);	// flush that page
	}
	barrier(); 
}

extern void int_callback(int s);

void DeviceDriverHandler::feedBuffer(uint32_t addr_offset) {	
	TIMOUT_HANDLER(now, DRIVER_TIMEOUT);
	
	barrier();
	while ((volatile uint32_t)(*_feed_flag_ptr) && KEEP_TRYING()) {
		now = micros();	/* wait until kernel retrieved the previous one */ 
		barrier();
	}

	if ((volatile uint32_t)(*_feed_flag_ptr)) {
		// this should never happen.. it would have to be the kernel-side 
		// stall or the "SID reset" script, but by the time that script 
		// gets used we should not be here!
		
		cout << "error: feedBuffer illegal state" << endl;
		int_callback(1); // call directly.. SIGINT takes too long
		_script_buf = 0;		// disable write ops until SIGINT performs its job
		
	} else {
		flushBuffer(addr_offset);				
	}
}

void DeviceDriverHandler::recordBegin() {
	_buffer_offset =  fetchBuffer();
	if (_buffer_offset == 0) {
		cout << "timeout: device driver is not responding" << endl;		
		int_callback(1); // call directly.. SIGINT takes too long
		_script_buf = 0;		// disable write ops until SIGINT performs its job
	} else {
		_script_buf = (uint32_t*)(_buffer_base + _buffer_offset);
	}
	_script_len = 0;
}

void DeviceDriverHandler::recordEnd() {
	if (!_script_buf) {
		// exit sequence has already been initiated (see recordBegin())
	} else {
		// buffer is never reset and it will still contain the
		// garbage from longer previous scripts: set an "end marker"
		_script_buf[_script_len<<1] = 0;
		
		feedBuffer(_buffer_offset);

		_script_buf = 0;	// redundant
		_script_len = 0;	// redundant	
	}
}
