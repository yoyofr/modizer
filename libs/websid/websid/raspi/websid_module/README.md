# WebSid (playback device driver)

################################################################################

CAUTION: This implementation has been tuned to reliably work with a specific 
set of kernel features and a specifc runtime configuration. There is a high 
probability that it will lead to system crashes when used on a differently 
configured system!

Specifically:

1) CPU cores #2 and #3 must be completely isolated from regular use by the
   kernel (see instructions in parent folder README.md). If other system critical
   tasks are still scheduled on the respective cores, then these will likely
   be starved - leading to a system crash and potential FS corruption.
   
2) When compiling the kernel the "Timer tick handling" must be set to 
   "Full dynaticks system" - aka NO_HZ_FULL (I also disabled "Old Idle dynaticks 
   config" but that probably isn't necessary.). This mode should also benefit 
   the regular local thread playback (see parent folder) since it will remove 
   the 100Hz "arch_timer" IRQs that that seem to be the biggest source of 
   timing disruptions. (I am using the regular "Desktop" model from the 
   "Preemption Model" selection - but I guess the other modes might also work."

Make sure to not have any unsaved work on the device when using this driver!
Also it is a good idea to make a system backup before first using it!   

################################################################################


This driver replays micro-second exact writes to a connected MOS8580 or
MOS6581 SID chip. Respective "playlists" are fed to the driver using a memory 
area that is shared with the device driver via "mmap". This driver is an 
alternative implementation to the respective built-in "userland" 
implementation already provided in the parent "raspi" folder. The respective
userland player will automatically use this driver whenever it detects it.

The cycle exact timing will not make any difference to most "normal" SID songs, 
but it is crucial for timing critical songs (e.g. songs that use "advanced" 
digi-sample techniques). You can check /var/log/kern.log - where the driver will
log the timing performance whenever a song is done playing. (Whan all the 
numbers are 0 it means that every GPIO write was performed in exactly the
right micro second.)
	

## Howto build/install

	sudo make
	
	This kernel module can then be installed/uninstalled via:

		sudo insmod websid_module.ko
	and 
		sudo rmmod websid_module.ko
	
	(check for errors: tail -f  /var/log/kern.log )

	

## Dependencies

You'll need a Raspberry Pi 4B and the Linux kernel source must have been installed and compiled
(see: https://www.stephenwagner.com/2020/03/17/how-to-compile-linux-kernel-raspberry-pi-4-raspbian/ ).

This module uses a "CC BY-NC-SA" license and it will not work on a regular stock kernel due to the
fact that a stock kernel refuses access to APIs like "sched_set_fifo" and "gpio_free" to non-GPL 
modules. So in order to use this module on your machine you'll have to disable the silly GPL 
checks before building your kernel. see: 
kernel/linux/include/linux/license.h  (just "return 1;" in "license_is_gpl_compatible")
PS: don't worry, rebuilding a previously built kernel with only this change takes less than 5 minutes..


## Background information

A note on timing constraints: a simple "+1 micro" polling loop manages a maximum 
of 19 loops (on RP4/1.5GHz)! probably less in most cases.. i.e. there may not be
much processing time available when the SID write happens immediately after the
script switch. Granted, the shortest SID update sequence will take 4 micros (which
is the fastest write operation of a 6502 CPU) but some time is always lost during the 
GPIO writes.. so there might be 2-3 micros avaiable in the worst case (i.e. when 
a script ends in the middle of a fast sequence..)


SID instructions in playback scripts are typically sparse, i.e. some writes are made,
and then nothing happends for a while, until the next writes are made. Which means that
a script here may end before the "time window" that it covers has completely elapsed (since
there were no additional writes scheduled in that window). The play loop will then already
start to wait for the next script eventhough that script is not yet needed to be played
for a while.

When the play loop waits for the next script, there are different possible senarions:
1) "userland" was faster and the "feed" is already available: The longer the previous
   script took to handle, the more likely this scenario, e.g. "digi-player songs" should fall
   into this category since they keep poking SID registers continually. It is a
   minority of songs.
2) "userland" was slower and the "feed" is not yet available.
   2a) previous script only covered a short portion of the playback interval: this is the
       norm for most songs which just poke some SID registers initially and then wait
       for the next frame
   2b) "userland" is too slow to keep up with the playback speed. Based on the experience
       with the "userland"-only impl this should NOT happen or the "userland" version should
       have experienced the same issue (which it did not). One difference to the "userland"
       impl is that the "buffer toggle" only allows to feed 1 buffer in advance, whereas
       the deque based "userland" impl may have allowed 2 buffers in those special cases
       where the playback was done handling the script but the current playback interval
       was still ongoing (this might have added some "load averaging" in those special cases).
	   This scenario might occur when using a Raspberry device that is too slow or a device 
	   that has automatically clocked down due to cpu overheating..


	   
It seems there are some core specific system threads, e.g.: 
 (cpuhp=cpu hotplug, migration=distributes workload across CPU cores,
 ksoftirq=thread raised to handle unserved software interrupts)   

pi@raspberrypi:~ $ ps -eo pid,ppid,ni,comm | grep /3
   25     2   0 cpuhp/3
   26     2   - migration/3
   27     2   0 ksoftirqd/3
   28     2   0 kworker/3:0
   29     2 -20 kworker/3:0H

some of which (e.g. "rcuog/3") are already handled on a different core due to the 
used cmdline.txt settings. 

The above still are on core #3, e.g. see:

pi@raspberrypi:~ $ ps -o psr 25
  PSR
  3
(or use "ps -eF" to check respective PSR for all running processes)

.. and it seems that they cannot be migrated elsewhere (e.g. "taskset -pc 2 25" fails..)

The device driver tries to no completely starve these other threads by 
proactively calling schedule() whenever there is time for it available.

   
Note: The kernel will notice the hijacking of core #3 and would log 
lengthy "stall" and "hung_task" warnings to  /var/log/kern.log 
The websid player automatically disables those warnings (only when using 
the "device driver playback method" by changing the respective configuration 
files, e.g.

   /sys/module/rcupdate/parameters/rcu_cpu_stall_suppress
   /proc/sys/kernel/hung_task_timeout_secs

 The original settings are restored when rebooting.
	

## License
Terms of Use: This software is licensed under a CC BY-NC-SA 
(http://creativecommons.org/licenses/by-nc-sa/4.0/).
