/*
 * Handling of the memory area shared between kernel and "userland".
 * 
 * There is exactly one contiguous, page aligned area that is shared 
 * and "userland" will get that address via the "mmap" call.
 * 
 * When logical "buffer pointers" are later exchanged between kernel and
 * "userland" these are always relative to the start of the shared memory area.
 */

#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/mman.h>

#include "websid_driver_api.h"

u32 *_raw_buffer; 
static u8 *_shared_area = 0;	// pointer to page aligned area

	// push an "empty" buffer to the "userland" "producer" for filling: only one buffer 
	// can be pushed here at any time and it must have been "consumed" by "userland"
	// before a new one can be pushed (set in kernel .. cleared in "userland")

volatile u32 *_fetch_flag_ptr = 0;	// relative pos into _shared_area

	// push a filled buffer to the playback thread: only one buffer can be pushed here
	// at any time and it must have been "consumed" by the playback thread before the
	// next one can be pushed (set in "userland" .. cleared in kernel)

volatile u32 *_feed_flag_ptr = 0;	// relative pos into _shared_area

// the two data buffers used for double-buffering
u32 *_script_buf0;
u32 *_script_buf1; 

static int setup_shared_mem(void) {
	u32 i;
	
	// setup buffers that can be mapped directly into "userland";
	// using kernel mem should ensure that there are no sudden 
	// page faults that would require interrupts..

	// alloc/reserve a big enough, page aligned buffer (so that used relative 
	// address offsets will start at 0)
	
	// optimistic allocation without _any_ attempt to free memory at all
	_raw_buffer = kmalloc(AREA_SIZE + 2*PAGE_SIZE, 
					GFP_KERNEL & ~__GFP_RECLAIM & ~__GFP_MOVABLE);
	
	if (_raw_buffer) {
		_shared_area = (u8 *)(((u32)_raw_buffer + PAGE_SIZE -1) & PAGE_MASK);	// page aligned
			
		// make both buffers page aligned just in case
		_script_buf0 = (u32*)(_shared_area + RESERVED_ENTRIES*sizeof(u32));
		_script_buf1 = (u32*)(((void*)(_script_buf0))+BUF_SIZE);
		
		// init with "end markers" just in case
		for (i = 0; i < (BUF_SIZE >> 2); i++) {
			_script_buf0[i] = _script_buf1[i] = 0;
		}
		
		_fetch_flag_ptr = (volatile u32*)(_shared_area + (FETCH_FLAG_OFFSET << 2));
		_feed_flag_ptr = (volatile u32*)(_shared_area + (FEED_FLAG_OFFSET << 2));
		
		(*_fetch_flag_ptr) = (*_feed_flag_ptr) = 0; // init flags
		
		printk("websid: setup_shared_mem success\n");
		return 1;
	} else {
		_shared_area = 0;
		_script_buf0 = _script_buf1 = 0;
		_fetch_flag_ptr = _feed_flag_ptr = 0;
		
		printk("websid: setup_shared_mem failed\n");
		return 0;
	}
}

static void teardown_shared_mem(void) {
	_script_buf0 = _script_buf1 = 0;
	
	if (_raw_buffer) kfree(_raw_buffer);
	_raw_buffer = 0;
	_shared_area = 0;
}

// device's memory map method
static int websid_mmap(struct file *file, struct vm_area_struct *vma)
{			
	// see http://www.cs.binghamton.edu/~ghose/CS529/sharedmemmap.htm
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long size = vma->vm_end - vma->vm_start;	// size of the memory area

//	printk("websid: starting websid_mmap\n");

	if (offset & ~PAGE_MASK) {
//		printk("websid: offset not aligned: %ld\n", offset);
		return -ENXIO;
	}   
/*
	if (size < AREA_SIZE) { 
		printk("websid: size too big %ld %d\n", size, AREA_SIZE);
		return(-ENXIO);
	}
	*/	
	if ((vma->vm_flags & VM_WRITE) && !(vma->vm_flags & VM_SHARED)) {
//	     printk("websid: writeable mappings must be shared, rejecting\n");
	     return(-EINVAL);
	}

	// don't not want to have this area swapped out, lock it
	vma->vm_flags |= ( VM_LOCKED | VM_DONTEXPAND | VM_DONTDUMP );

	// enter pages into mapping of application:
	if (remap_pfn_range(vma, vma->vm_start,
						virt_to_phys((void*)_shared_area) >> PAGE_SHIFT, 
						size, PAGE_SHARED)) {
//		printk("websid: remap page range failed\n");
		return -ENXIO;
	}

//	printk("websid: success websid_mmap\n");	
	return(0);
}

static void websid_unmmap(struct file *file) {
	// it would seem logical to have some counterpart to remap_pfn_range - which would 
	// allow the kernel to remove a kernel mapping when the "userland" closes the file 
	// descriptor to the device.. it just makes no sense that the mappings are still 
	// active after the device is already "disconnected".. if such an API exists - I 
	// have not found it (so for now this is a useless no-op and old processes can 
	// still mess with the driver's memory even after they are no longer supposed to 
	// be able to do so):-(
	
	// reminder: file->private_data might be used to pass around data if necessary
}
