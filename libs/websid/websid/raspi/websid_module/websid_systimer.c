/*
 * Direct access to the system timer's built-in counter via its memory mapped registers.
 */
 
#include <linux/io.h>

// (bcm2711: The physical (hardware) base address for the system timers is 0x7E00 3000)
#define ST_BASE 0xFE003000 	/* system timer base address; same used in "userland" memmap - CAUTION: RPi4 only!!! */

#define ST_CLO_OFFSET 0x4	/* offset: System Timer Counter Lower 32 bits */
#define ST_CHI_OFFSET 0x8	/* offset: System Timer Counter Higher 32 bits */

static volatile u32 *_timer_regs = 0;


// todo: check what the actual limitations of ktime_get_ns() are... would it actually 
// allow for more precise timing or is it one of those joke APIs that round to the closest 
// milli anyway?

 
int system_timer_mmap(void) {
	// see https://mindplusplus.wordpress.com/2013/08/09/accessing-the-raspberry-pis-1mhz-timer-via-kernel-driver/

	_timer_regs = (volatile u32*)ioremap(ST_BASE, SZ_4K);
	if (!_timer_regs) {
		printk("websid: fatal error - system_timer_mmap\n");
		return 0;
	}
	
	printk(KERN_INFO "websid: system_timer_mmap completed\n");
	return 1;
}

void system_timer_unmmap(void) {
	if (_timer_regs) iounmap(_timer_regs); // release mapping

	printk(KERN_INFO "websid: system_timer_unmmap completed\n");
}

#define micros() _timer_regs[1]

