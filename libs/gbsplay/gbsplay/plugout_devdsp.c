/*
 * gbsplay is a Gameboy sound player
 *
 * header file for /dev/dsp sound output plugin
 *
 * 2003-2020 (C) by Christian Garbs <mitch@cgarbs.de>
 *                  Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include "common.h"
#include "plugout.h"

static int fd;

static long devdsp_open(enum plugout_endian *endian, long rate, long *buffer_bytes, const struct plugout_metadata metadata)
{
	int c;
	int flags;

	UNUSED(metadata);
	
	if ((fd = open("/dev/dsp", O_WRONLY|O_NONBLOCK)) == -1) {
		fprintf(stderr, _("Could not open /dev/dsp: %s\n"), strerror(errno));
		return -1;
	}
	if ((flags = fcntl(fd, F_GETFL)) == -1) {
		fprintf(stderr, _("fcntl(F_GETFL) failed: %s\n"), strerror(errno));
	} else if (fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) == -1) {
		fprintf(stderr, _("fcntl(F_SETFL, flags&~O_NONBLOCK) failed: %s\n"), strerror(errno));
	}

	switch (*endian) {
	case PLUGOUT_ENDIAN_BIG:        c = AFMT_S16_BE; break;
	case PLUGOUT_ENDIAN_LITTLE:     c = AFMT_S16_LE; break;
	case PLUGOUT_ENDIAN_AUTOSELECT: c = AFMT_S16_NE; break;
	}
	if ((ioctl(fd, SNDCTL_DSP_SETFMT, &c)) == -1) {
		fprintf(stderr, _("ioctl(fd, SNDCTL_DSP_SETFMT, %d) failed: %s\n"), c, strerror(errno));
		return -1;
	}
	c = rate;
	if ((ioctl(fd, SNDCTL_DSP_SPEED, &c)) == -1) {
		fprintf(stderr, _("ioctl(fd, SNDCTL_DSP_SPEED, %ld) failed: %s\n"), rate, strerror(errno));
		return -1;
	}
	if (c != rate) {
		fprintf(stderr, _("Requested rate %ldHz, got %dHz.\n"), rate, c);
		rate = c;
	}
	c=1;
	if ((ioctl(fd, SNDCTL_DSP_STEREO, &c)) == -1) {
		fprintf(stderr, _("ioctl(fd, SNDCTL_DSP_STEREO, %d) failed: %s\n"), c, strerror(errno));
		return -1;
	}
	c=(4 /* max fragments */ << 16) + 11 /* selector for 2048 bytes fragment size */;
	if ((ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &c)) == -1)
		fprintf(stderr, _("ioctl(fd, SNDCTL_DSP_SETFRAGMENT, %08x) failed: %s\n"), c, strerror(errno));

	/* get the actual fragment size we got */
	if ((ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &c)) == -1) {
		fprintf(stderr, _("ioctl(fd, SNDCTL_DSP_GETBLKSIZE) failed: %s\n"), strerror(errno));
		return -1;
	}
	*buffer_bytes = c;

	return 0;
}

static ssize_t devdsp_write(const void *buf, size_t count)
{
	return write(fd, buf, count);
}

static void devdsp_close()
{
	(void)close(fd);
}

const struct output_plugin plugout_devdsp = {
	.name = "oss",
	.description = "OSS sound driver",
	.open = devdsp_open,
	.write = devdsp_write,
	.close = devdsp_close,
};
