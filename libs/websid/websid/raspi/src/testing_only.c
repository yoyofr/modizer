/*
 * Just a stripped down driver that plays ping/pong with the 
 * websid "device driver" which is used to investigate the 
 * random system crashes..
 */

volatile uint32_t *_fetch_flag;
volatile uint32_t *_feed_flag;

uint32_t fetchBuffer() {
	uint32_t addr_offset = 0;	// means "unavailable" (must be handled as an error in "userland")

	TIMOUT_HANDLER(now, TIMEOUT);
	
	while (KEEP_TRYING()) {	
		// availability of the buffer should be detected as quickly as possible since any
		// time wasted here substracts from the 17-20ms available to produce the next buffer
		
		if (*_fetch_flag) {
			// pass "ownership" of the buffer to "userland"
			addr_offset = *_fetch_flag;
			(*_fetch_flag) = 0;		// kernel side will not update this flag before it has been reset to 0 here
			break;
		}
		now = micros();
	}	
	return addr_offset;
}

void feedBuffer(uint32_t addr_offset) {
	TIMOUT_HANDLER(now, TIMEOUT);
	
	while ((volatile uint32_t)(*_feed_flag) && KEEP_TRYING()) { 		
		now = micros();	/* wait until kernel retrieved the previous one */ 
	}

	if ((volatile uint32_t)(*_feed_flag)) {
		// this should never happen.. it would have to be the kernel-side stall or the "SID reset" script, 
		// but by the time that script gets used we should not be here!
		
		cout << "error: feedBuffer illegal state" << endl;
		_max_frames = 0;			// this should trigger a regular teardown..
		
	} else {
		*_feed_flag = addr_offset;
	}
}

void runTest() {
		// disable stall/hung etc
	system("sudo echo '600000000' > /proc/sys/kernel/sched_rt_period_us");		//  default: 1000000
	system("sudo echo '-1' > /proc/sys/kernel/sched_rt_runtime_us");			//  default: 950000
	system("sudo echo '600' > /proc/sys/kernel/hung_task_timeout_secs");		//  default: 120
	system("sudo echo '1' >  /sys/module/rcupdate/parameters/rcu_cpu_stall_suppress");	//  default: 0

	
	int fd;
	volatile uint32_t *buffer_base;
	DeviceDriverHandler::detectDeviceDriver(&fd, &buffer_base);
	
	if(buffer_base == 0) {
		fprintf(stderr, "ERROR: cannot connect to device driver.\n");
		exit(-1);
	}
	
	_fetch_flag = (volatile uint32_t *)((uint8_t*)buffer_base + (FETCH_FLAG_OFFSET << 2));
	_feed_flag = (volatile uint32_t *)((uint8_t*)buffer_base + (FEED_FLAG_OFFSET << 2));

	int32_t buffer_offset= 0;
	volatile uint32_t *out;	
	
	if (0xffffffff - micros() < 170000000*1.5) {
		fprintf(stderr, "WARNING: overflow might interfer with test\n");
	}
	
	
	_max_frames= 100000000;	// ca 17 secs without usleep
	for (uint32_t i = 0; i < _max_frames; i++) {
		// just ping/pong back and forth..

		buffer_offset = fetchBuffer();		//	corrsponds to:	_playbackHandler->recordBegin();
		if (!buffer_offset)	{
			fprintf(stderr, "error no buffer in %d\n", i);
			break;
		}
		
		// make some minimal update to the target buffer
		out = (volatile uint32_t*)(((char*)buffer_base) + buffer_offset);
		if (i & 0x1) {
			out[0]=1; 				// a garbage timestamp - player can deal with this..
			out[1]=0x18<<8; 		// volume 0		
			out[2]=0; 	// end marker
		} else {
			out[0]=1; 
			out[1]=(0x18<<8) | 0xf; // volume f
			out[2]=0; 	// end marker
		}
		
		feedBuffer(buffer_offset);		//	corresponds to:	_playbackHandler->recordEnd();

//		usleep(1);		// this crap is much slower than 1 micro!
		if (!_max_frames) break;
	}	
	fprintf(stderr, "done %d\n", _max_frames);
}