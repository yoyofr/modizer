/*
 *	websid_module.c
 *
 *	(c) Copyright 2021 Jürgen Wothke <tinyrsid@wothke.ch>,
 *						All Rights Reserved.
 *
 *  Kernel module "websid device driver" for scripted control of a SID 
 *  chip (e.g. MOS6581). 
 *
 *
 *  API for "userland" use: 
 *  	1) to play a new song "open" the device /dev/websid (this is 
 *         not reentrant)
 *		2) use "mmap" to establish required memory mapping 
 *		3) see websid_driver_api.h: follow the fetch/feed protocol via
 *         the respective signal flags in the shared memory (see 
 *         device_driver_handler.cpp for example) 
 *		4) "close" the filehandle to the device when done with a song
 *
 *
 *  Basic "operating mode" of this driver:
 *  
 *  Simple double buffering scheme: one buffer is used to playback (here 
 *  in the driver) while the other one can be filled with new data (in 
 *  "userland") at the same time. The player drives the process by 
 *  advertising which buffer it wants to have filled next and by 
 *  signaling if it is ready to receive a new buffer. The kernel part is 
 *  very timing critical, whereas the "userland" side is not (so much). 
 *  
 *  Note: Subroutines used by playback thread are inlined to avoid 
 *  additional stack allocations during the phase there IRQs are blocked.
 *  
 *  License  
 *  
 *  Terms of Use: This software is licensed under a CC BY-NC-SA 
 *  (http://creativecommons.org/licenses/by-nc-sa/4.0/).
 *  The author does not admit liability nor provide warranty for any of 
 *  this software. This material is provided "AS-IS". Use it at your own 
 *  risk.
 */
 
// reminder: The "optimal" runtime environment is setup via kernel 
// configuration and various settings performed from the "userland" 
// side. see DeviceDriverHandler!
  
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/errno.h>

#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/preempt.h>
#include <linux/membarrier.h>

#include "websid_driver_api.h"

#define  DEVICE_NAME "websid"
#define  CLASS_NAME  "websid"


// open issue: supposedly the internal system timer counter glitches 
// when the CPU runs at 1.5GHz.. and it might glitch even more when a 
// core idles at its default 600MHz. Unfortunately it seems that 
// respective CPU clock settings can only be defined globally but not on 
// a per core level.. see /boot/config.txt (arm_freq & force_turbo).. 
// this is unfortunate since 1GHz (or whatever might be best for timer 
// counter precision) would be sufficient for core #3 driving the 
// precision playback loop.. but it may not be enough for the
// core that runs the emulator - at least not for "complex" songs.


// the below are just C-file snipets used to make this file less bulky
// (while not having to bother with kernel module makefile setup)
#include "websid_mmap.c"
#include "websid_systimer.c"
#include "websid_gpio.c"


static int _playback_core_id = 3;	// CPU to run the playback thread on
static int _dev_open_count = 0;		// allow only 1 user at a time

// -------------- util to run without any interruptions -------------

static unsigned long _irq_flags;


// Even with all the "high prio" handling of the playback thread, there 
// are still interrupts disturbing the timing. Blocking the IRQs seems 
// to be solve that problem - any glitches left should now be timing 
// flaws of the actual emulation.

// reminders: the 100Hz "arch_timer" IRQs are not disabled via "local_bh" 
// but the "local_irq*" blocking seems to cover all the relevant IRQS, 
// see tail -100 /proc/interrupts
// whereas "irqaffinity=7" in cmdline.txt does not have any beneficial 
// effect on core #3 however, using "NO_HZ_FULL" (see "Timer tick 
// handling" => "Full dynaticks system (tickless)") when compiling the 
// kernel, seems to work well to eliminate those IRQs on core #3..

#define RUN_UNDISTURBED(undisturbed_func) \
	/* CAUTION: the undisturbed_func MUST NOT use anything depending on */\
	/* interrupts (e.g. printk, etc) or it will freeze! */\
	/* https://kernel.readthedocs.io/en/sphinx-samples/kernel-hacking.html */\
	\
	get_cpu();						/* aka preempt_disable(); prevent thread migration */\
									/* (this might be redundant - see "isolcpus=3") */\
	local_irq_save(_irq_flags);		/* disable hard IRQs	(include/linux/irqflags.h) */\
	local_bh_disable();				/* disable local IRQs	(include/linux/interrupt.h) */\
	\
	undisturbed_func();\
	\
	local_bh_enable();				/* re-enable local IRQs */\
	local_irq_restore(_irq_flags);	/* re-enable hard IRQs */\
	put_cpu();

// temporarily reenable IRQs 
#define SCHEDULE() \
	local_bh_enable();\
	local_irq_restore(_irq_flags);\
	put_cpu();\
	schedule();\
	get_cpu();\
	local_irq_save(_irq_flags);\
	local_bh_disable();

	
// ------------ setup of the basic /dev/websid driver API -------------------------

static int	websid_open(   struct inode *inodep, struct file *fp);
static int	websid_release(struct inode *inodep, struct file *fp);
static int websid_mmap(struct file *file, struct vm_area_struct *vma);

// note: respective file ops are never scheduled on CPU core #3 but 
// always on one of the other cores - while the background playback 
// thread always runs on core #3

static struct file_operations websid_fops = {
	.owner =	THIS_MODULE,
	.mmap = 	websid_mmap,
	.open = 	websid_open,
	.release =	websid_release,
};

static struct miscdevice websid_device = {
	.minor =	MISC_DYNAMIC_MINOR,
	.name =		DEVICE_NAME,
	.fops =		&websid_fops,
	.mode  =	0x666		// let everyone use the device
};

#define OK_STATUS 0
#define ERR_PREVIOUS_FETCH_NOT_CLEARED 1
#define ERR_WRONG_FEED_BUFFER 2
#define ERR_COUNTER_OVERFLOW 3


static volatile u32 _player_status = OK_STATUS; 
static volatile u32 _run_next_script = 0;		// used to interrupt
static u32 _feed_index = 0; 					// 0 or 1

// time at which the song actually started playing: must be reset at 
// the beginning of a new song. The timestamps in the "play scripts" 
// are treated as offsets to this base. KNOWN LIMITATION: "32-bit 
// lo-timer counter" impl is prone to overflow once every ~70 minutes
static u32 _ts_offset;

// just a sanity check: next buffer to be delivered by the userland side
static u32 _expecting = 0;	

// this is called before the player plays one of the buffers..
// i.e. the userland must always be pointed to the *other* buffer, and if
// _fetch_flag_ptr was already set then this would be an error!

static u8 *_buf_t0;	// tmp var used in push_fetch_buffer macro
#define push_fetch_buffer() {\
	if (((volatile u32)*_fetch_flag_ptr)) {\
		/* this should be dead code..*/\
		_player_status = ERR_PREVIOUS_FETCH_NOT_CLEARED;\
		_run_next_script = 0;\
	} else {\
		_feed_index ^= 1; /* _feed_index ? 0 : 1 */\
		_buf_t0 = (u8*) (_feed_index ? _script_buf1 : _script_buf0);\
		/*mb();*/\
		_expecting = *_fetch_flag_ptr = ((u32)_buf_t0) - ((u32)_shared_area);\
		/*mb();*/\
		/*_buf_t0[0] = 0; *//* an end-marker might be a usefull precaution - */\
							/* but rather keep all buffer writes to the */\
							/* "userland" producer. */\
	}\
}

static struct task_struct *_player_thread;
static volatile u32 _player_done = 1;		// status of playback kthread


// note: the "real" buffer offsets start after the 2 pages with the 
// flags, i.e. anything < 2*4096 can be used as a special marker
#define RESET_SCRIPT_MARKER 1

// built-in script to reset all SID registers to 0 (to mute any beeps
// when quitting the player )
static u32 _reset_script[49] = {
	1, (0x0100),	// ts = 1, reg<<8 | value = 0x0100
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

// perf logging
static u32 _log_enabled = 1;		// enable/disable logging
static u32 _log_script_count;		// nummber of processed scripts
static u32 _log_max_delay;			// maximum timing error that was observed
static u32 _log_start_delay_count;	// issues that occured at the first entry 
static u32 _log_other_delay_count;	// issues that occured later in a script


// "local" vars of "play_script"
static u32 _p_now;
static u32 _p_i;
static volatile u32 _p_ts;
static volatile u32 *_p_data;
static volatile u32 *_p_buf= 0;

// inlined to avoid additional stack allocations
#define play_script() {\
	_p_i = 0;\
	_p_ts = _p_buf[_p_i];\
	\
	if (_p_ts && !_ts_offset) {\
	 	/* sync playback with current counter pos */\
		_ts_offset = micros();\
	}\
	\
	while (_p_ts != 0) {\
		\
		_p_data = &(_p_buf[_p_i+1]);\
		\
		_p_ts += _ts_offset;\
		if ((_p_ts < _ts_offset) && (_p_buf != _reset_script)) { /* counter overflow */\
			_player_status = ERR_COUNTER_OVERFLOW;\
			/*mb();*/\
			*_feed_flag_ptr = RESET_SCRIPT_MARKER; /* trigger exit sequence */\
			/*mb();*/\
			return;\
		}\
		_p_i += 2;			/* 2x u32 per entry.. */\
		\
		_p_now = micros();\
		if (_p_buf == _reset_script) {\
			/* ts in the "reset script" are garbage */\
			TIMOUT_HANDLER(now, 3); /* 3 micro delay to give SID time between updates*/\
			while (KEEP_TRYING()) { now = micros(); /*mb();*/ }\
		} else if (_p_now > _p_ts) {\
			/* problem: we are already to late (maybe the "userland" feed was too slow?) */\
			/* the problem would probably affect all the following timestamps as well, */\
			/* so it is better to just write off the lost time and take it from here */\
			\
			if (_log_enabled) {\
				u32 diff= _p_now - _p_ts;\
				if (diff > _log_max_delay) _log_max_delay = diff;\
				\
				if (_ts_offset <= _p_now+1) {\
					_log_start_delay_count++;\
				} else {\
					_log_other_delay_count++;\
				}\
			}\
			_ts_offset += (_p_now-_p_ts) + 1; /* write off the delay */\
			\
		} else {\
			/* detected "gaps" might be used to "do other stuff" as an */\
			/* optimization.. but hopefully that won't be necessary*/\
			if ((_p_ts-micros()) > 100) {\
				SCHEDULE();	/* it seems: cond_resched or rcu_all_qs doesn't work here.. */\
			}\
			while (micros() < _p_ts) { /* just wait and poll  */ /*mb();*/ }\
		}\
		\
		poke_SID(((*_p_data) >> 8) & 0x1f, (*_p_data) & 0xff);\
		\
		_p_ts = _p_buf[_p_i];\
	}\
	_log_script_count++;\
}

void play_loop(void) {
	_log_script_count = 0;
	_log_max_delay = 0;	
	_log_start_delay_count = _log_other_delay_count = 0;

	end_play_loop:		/* silly construct used to ease 1:1 conversion into a macro */
	
	while (_run_next_script) {
		_p_buf = 0;
	
		/* 1st "fetch" has been triggered during init */
		while (_p_buf == 0) {
			/*mb()*/;
			_p_buf = (volatile u32*) (*_feed_flag_ptr);	/* wait for "userland" feed */
			/* at this stage "_p_buf" is still a relative offset (not an */
			/* absolute address) just wait for main to feed some data */
			if (!_run_next_script) goto end_play_loop;
		}
		
		if (((u32)_p_buf) == RESET_SCRIPT_MARKER) {
			/* special marker: "userland" closed the connection */
			_p_buf = _reset_script;
			/*mb();*/
			(*_fetch_flag_ptr) = 0;	/* stop fetching new buffers */
//			(*_feed_flag_ptr) = 0;	/* don't ackn to block more feeding */			
			/*mb();*/
			
			_run_next_script = 0; /* exit the playback thread after "reset-script" */
		} else {			
			if (((u32)_p_buf) != _expecting) {	/* sanity check - todo: remove later */
				_player_status = ERR_WRONG_FEED_BUFFER;
				_run_next_script= 0;
				return;
			}
			
			/* convert "_p_buf" offset to absolute address */
			_p_buf = (volatile u32*)(((u32)_shared_area) + ((u32)_p_buf));
			
			/*mb();*/
			(*_feed_flag_ptr) = 0;	/* ackn reception of the buffer */
			/*mb();*/
								
			push_fetch_buffer();	/* immediately request the other buffer to */
									/* be filled so it should be ready by the */
									/* time it is needed */
		}
		/*mb();*/
		play_script();		
	}
}

void test(void) {
	// it seems that blocking the IRQs for 5 minutes with this simple
	// polling loop does not crash the system, i.e. the crashing must come 
	// from something else.. (is it the kernel side or maybe the userland
	// that causes the crash?)

	// the access for the memory mapped system counter does not seem to 
	// be the problem either..
	
	u32 s= micros();
	
	u32 e= s+ 1000000*300; // just wait for 3 minutes then exit
	if (e<s) return;
	
	while (micros() < e) {/*busy wait*/}
	return;
}

static void init_player(void) {
	_run_next_script = 1;	
	_ts_offset = 0;			// reset base time for next played scripts
	
	_feed_index = 0; 
	*_feed_flag_ptr = *_fetch_flag_ptr = 0;
	/*mb();*/
	
	push_fetch_buffer();	// signal "open for business" to "userland"
}

int sid_player(void *data) {	
	_player_done= 0;
	
	init_player();
	
//	printk(KERN_INFO "websid: player thread start\n");		
	
	RUN_UNDISTURBED(play_loop);
	
	
	// issue: without blocking IRQs there are actual timing glitches (up to 9 micros) - but 
	// full-dyna-ticks running on both #2&#3 crashes quickly with this, even without any disabled IRQ
//	play_loop();

//	RUN_UNDISTURBED(test); // check if this might already crash the system

	_player_done= 1;

	// logging can only be done once IRQs have been re-enabled
	switch (_player_status) {
		case OK_STATUS:
			printk(KERN_INFO "websid: player thread ended\n");
			break;
		case ERR_PREVIOUS_FETCH_NOT_CLEARED:
			printk(KERN_INFO "websid: player thread aborted: ERR_PREVIOUS_FETCH_NOT_CLEARED\n");
			break;
		case ERR_WRONG_FEED_BUFFER:
			printk(KERN_INFO "websid: player thread aborted: ERR_WRONG_FEED_BUFFER\n");
			break;
		case ERR_COUNTER_OVERFLOW:
			printk(KERN_INFO "websid: player thread aborted: ERR_COUNTER_OVERFLOW\n");
			break;
		default:
			printk(KERN_INFO "websid: player thread aborted: %d\n", _player_status);
	}

	return 0;
}

static void start_player_core3(void) {
	// this code here may be running on any core (except core #3)
		
	_player_thread = kthread_create(sid_player, NULL, "sid_player");	
	kthread_bind(_player_thread, _playback_core_id);
	
	// unfortunately this does not pervent all interrupts:
    // give prio to task (API only available for GPL.. FUCK YOU MORONS!)
	sched_set_fifo(_player_thread);	
	
	wake_up_process(_player_thread);
}


static int websid_open(struct inode *inodep, struct file *filep) {
	// this code here may be running on any core (except core #3)

	printk(KERN_INFO "websid: open called on /dev/websid\n");

	if (_dev_open_count) return -EBUSY;		// only 1 user allowed
	_dev_open_count++;

	start_player_core3();

	printk(KERN_INFO "websid: open started playback thread\n");
	
	/* dev/websid is a virtual (and thus non-seekable) filesystem */
	return stream_open(inodep, filep);
}


static int websid_release(struct inode *inodep, struct file *filep) {
	// this code here may be running on any core (except core #3)
	
	TIMOUT_HANDLER(now, TIMEOUT);
		
	if (_dev_open_count > 0) {
		printk(KERN_INFO "websid: release called on /dev/websid\n");
		
		
		if (!_player_done) {
			// trigger playback thread's exit sequence..
			// these might create race-conditions..
			*_fetch_flag_ptr = 0;
			*_feed_flag_ptr = RESET_SCRIPT_MARKER;
			/*mb();*/
			
			while (KEEP_TRYING()) {	// wait for player to exit
				if (_player_done) break;
				now= micros();				
				/*mb();*/
			}
			if (!_player_done) {
				// try to interrupt whatever "while" the player might be stuck in
				_run_next_script = 0;
				_p_ts = 0;
				
				printk(KERN_INFO "websid: release warning - player did not quit normally\n");
			}
//			printk(KERN_INFO "websid: release triggered SID reset\n");
		} else {
//			printk(KERN_INFO "websid: OOPS player already quit..\n");
		}

		if (_log_enabled) {
			printk(KERN_INFO "websid stats - frames: %u, max delay: %u c1#: %u c2#: %u\n", 
				_log_script_count, _log_max_delay,
				_log_start_delay_count, _log_other_delay_count);
		}
		
		websid_unmmap(filep);

		_dev_open_count--;   // re-enable next "open"

		printk(KERN_INFO "websid: release completed\n");
		
		return 0;
	} else {
		printk(KERN_INFO "websid error: release before open\n");
		return -ENODEV;	// error
	}
}


static u32 _module_registered = 0;

static int __init websid_init(void) {
	// should never be called on core #3 (add test just in case)
	if (smp_processor_id() == 3) printk(KERN_INFO "websid: ERROR - init is running on core #3!\n");
	
	printk(KERN_INFO "websid: loading device driver\n");

	if (!setup_shared_mem()) {
		return -ENODEV;	// error
	}

	if (!system_timer_mmap()) {
		teardown_shared_mem();
		return -ENODEV;	// error
	}
			
	if (misc_register(&websid_device)) {
		system_timer_unmmap();
		teardown_shared_mem();
		return -ENODEV;	// error
	}
			
	if (!gpio_init(websid_device.this_device)) {
		misc_deregister(&websid_device);
		system_timer_unmmap();
		teardown_shared_mem();		
		return -ENODEV;	// error
	};
			
	_module_registered = 1;
	printk(KERN_INFO "websid: registered /dev/websid\n");		
	return 0;
}

static void stop_plackback_loop(void) {
	*_feed_flag_ptr = RESET_SCRIPT_MARKER;
}


static void __exit websid_exit(void) {
	TIMOUT_HANDLER(now, TIMEOUT);
	
	// should never be called on core #3 (add test just in case)
	if (smp_processor_id() == 3) printk(KERN_INFO "websid: ERROR - exit is running on core #3!\n");
	
	if (_module_registered) {		
		printk(KERN_INFO "websid: unloading device driver\n");
		
		if (_dev_open_count) {
			printk(KERN_INFO "websid: waiting for open connection to close\n");
			while (_dev_open_count && KEEP_TRYING()) { /*mb();*/ }	// 3s grace period
			
			if (_dev_open_count) {
				printk(KERN_INFO "websid: connection is still open - this might crash\n");

				// normally the loop should already have been 
				// quit via "release" when the file was closed..
				stop_plackback_loop();
			}
		}	

		gpio_release();
		system_timer_unmmap();	
		teardown_shared_mem();	
		
		misc_deregister(&websid_device);
		_module_registered = 0;
		printk(KERN_INFO "websid: unloading completed\n");
	} else {
		printk(KERN_INFO "websid: unloading fail - wasn't registered\n");
	}
}

module_init(websid_init);
module_exit(websid_exit); 

/*
The used "sched_set_fifo" and "gpio_free" kernel functions by default 
are only exported to "GPL" licensed modules. Before compiling your kernel 
from source (which you'll have to do anyway as a prerequisite to build this 
kernel module). Make sure to edit kernel/linux/scripts/mod/modpost.c
and/or kernel/linux/include/linux/license.h to disable the silly GPL checks.

Or you have to comment out use of the respective APIs - which might cause 
somewhat worse timing precision.
*/
MODULE_LICENSE("CC BY-NC-SA");
MODULE_AUTHOR("Jürgen Wothke");
MODULE_DESCRIPTION("websid device driver");
MODULE_VERSION("0.5"); 

