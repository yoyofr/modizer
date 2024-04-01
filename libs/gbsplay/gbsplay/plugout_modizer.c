/*
 * gbsplay is a Gameboy sound player
 *
 * MODIZER file writer output plugin
 *
 * 2004-2022 (C) by Christian Garbs <mitch@cgarbs.de> for gbsplay
 * 2024 (C) by Yohann Magnien for this specfic plugout
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#include <unistd.h>
#include <fcntl.h>

#include "common.h"
#include "plugout.h"


static long modizer_gbs_open(enum plugout_endian *endian,
                        long rate, long *buffer_bytes,
                        const struct plugout_metadata metadata)
{
    *buffer_bytes=512*4;
    printf("rate: %d size: %d endian: %d\n",rate,*buffer_bytes,*endian);
	return 0;
}

extern void gbs_update(unsigned char* pSound,int lBytes);

static ssize_t modizer_gbs_write(const void *buf, size_t count)
{
	//return write(fd, buf, count);
    
    gbs_update(buf,count);
    return count;
}

static void modizer_gbs_close()
{
	
}

const struct output_plugin plugout_modizer = {
	.name = "modizer",
	.description = "Modizer interface",
	.open = modizer_gbs_open,
	.write = modizer_gbs_write,
	.close = modizer_gbs_close
};
