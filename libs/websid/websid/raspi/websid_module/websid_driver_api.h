#ifndef _WEBSID_DRIVER_API_H
#define _WEBSID_DRIVER_API_H

// The kernel module (aka device driver) interacts with the "userland" by 
// means of a directly shared kernel memory area. This allows to make
// the interface as direct as possible and does not introduce additional 
// dependencies (e.g. using "ioctl" would involve additional kernel
// threads that might run on other CPU cores and unnecessarily cause
// issues in parts of the system than need not even be touched..)

// The shared area starts with some flags and then contains two buffers 
// that are used to pass actual data back and forth. Any thread 
// "synchronization" is done via busy-polling of flags. The assumption is, 
// that any changes to respective u32 flags are sufficiently "atomic" and 
// the possible state-changes would even allow for certain delays without 
// breaking the logic. The buffers are only changed by the userland 
// producer and only read by the kernel thread player. The flags make sure 
// that a buffer is "exclusively owned" by either the userland or the 
// kernel space at any given moment (i.e. no concurrent updates to buffers). 

// Supposing that the two involved CPU cores might use their L2/L3 caches, 
// there might be a delay between the update of a flag on one core and the 
// visibility of that flag-change on the other core. However respective 
// flag updates are not timing critical and each side can easily 
// distinguish its own flag-update from an update made on the "other" side: 
// Each flag is designed such that it is always "set" (non-NULL) by one 
// side and "cleared" (NULL) by the other side. There is one flag for each 
// "direction": the "fetch" flag signals that the "kernel" wants data, 
// while the "feed" flag signals that the "userland" has data.

#ifdef RPI4
// for the benefit of the userland code
#define u32 uint32_t
#endif

// just some utils used for busy-waiting
#define TIMEOUT 3000000		/* 3 secs too much? */
#define TIMOUT_HANDLER(now, to) \
	u32 start, now, timeout, overflow; \
	start = now =  micros();\
	overflow = (((u32)0xffffffff) - start) <= to;	/* couter will overflow during timeout window */\
	timeout = start + to;
#define KEEP_TRYING() ((!overflow && (now<timeout)) || (overflow && ((now>start) || (now<timeout))))



#define _PAGE_SIZE	4096

// part of the shared area used for signaling (counted in u32 steps):
	// reserve a separate page for each of the 2 flags to avoid race-conditions
#define RESERVED_ENTRIES	((_PAGE_SIZE >> 2) * 2)	/* i.e. 1 page per flag */
	// offsets of the two signaling flags:
#define FETCH_FLAG_OFFSET	0					/* at start of 1st page */
#define FEED_FLAG_OFFSET	(_PAGE_SIZE >> 2) 	/* at start of 2nd page */


// part of the shared area used for buffers
#define NUM_BUFFERS 2
#define MAX_ENTRIES	1500	/* fixme: should be adapted to userland "chunk_size"; testcase Comaland_tune_3.sid */
#define INTS_PER_ENTRY 2

#define RAW_SIZE	((MAX_ENTRIES+1) * INTS_PER_ENTRY*sizeof(u32))	/* added emergency 0-marker just in case */
// make sure the buffers stay page aligned too - just in case
#define BUF_SIZE	((RAW_SIZE & 0xfffff000)+_PAGE_SIZE)	/* round up to full page */

// total size of the shared memory area in bytes
#define AREA_SIZE ((RESERVED_ENTRIES<<2)  + BUF_SIZE*NUM_BUFFERS)


/*
i.e. the used layout of the shared memory area is:

[page#0] fetch flag (contains "available fetch buffer offset")
[page#1] feed flag (contains "available feed buffer offset")
[page#3 .. ] buffer 1
[ .. ] buffer 2

the contained buffers are addresses via their offset from the 
_shared_area beginning, i.e. these offsets are never 0 for the 
two buffers and 0 can therefore be used as an "unavailable" flag
*/	

#endif