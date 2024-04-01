/*
 * gbsplay is a Gameboy sound player
 *
 * 2006-2020 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <alloca.h>

#include <alsa/asoundlib.h>

#include "common.h"
#include "plugout.h"

/* Handle for the PCM device */
snd_pcm_t *pcm_handle;

#if GBS_BYTE_ORDER == GBS_ORDER_LITTLE_ENDIAN
#define SND_PCM_FORMAT_S16_NE SND_PCM_FORMAT_S16_LE
#else
#define SND_PCM_FORMAT_S16_NE SND_PCM_FORMAT_S16_BE
#endif

static long alsa_open(enum plugout_endian *endian, long rate, long *buffer_bytes, const struct plugout_metadata metadata)
{
	const char *pcm_name = "default";
	int fmt, err;
	unsigned exact_rate;
	snd_pcm_hw_params_t *hwparams;
	snd_pcm_uframes_t buffer_frames = *buffer_bytes / 4;
	snd_pcm_uframes_t period_frames;

	UNUSED(metadata);

	switch (*endian) {
	case PLUGOUT_ENDIAN_BIG:    fmt = SND_PCM_FORMAT_S16_BE; break;
	case PLUGOUT_ENDIAN_LITTLE: fmt = SND_PCM_FORMAT_S16_LE; break;
	default:                    fmt = SND_PCM_FORMAT_S16_NE; break;
	}

	snd_pcm_hw_params_alloca(&hwparams);

	if ((err = snd_pcm_open(&pcm_handle, pcm_name, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		fprintf(stderr, _("Could not open ALSA PCM device '%s': %s\n"), pcm_name, snd_strerror(err));
		return -1;
	}

	if ((err = snd_pcm_hw_params_any(pcm_handle, hwparams)) < 0) {
		fprintf(stderr, _("snd_pcm_hw_params_any failed: %s\n"), snd_strerror(err));
		return -1;
	}

	if ((err = snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		fprintf(stderr, _("snd_pcm_hw_params_set_access failed: %s\n"), snd_strerror(err));
		return -1;
	}

	if ((err = snd_pcm_hw_params_set_format(pcm_handle, hwparams, fmt)) < 0) {
		fprintf(stderr, _("snd_pcm_hw_params_set_format failed: %s\n"), snd_strerror(err));
		return -1;
	}

	exact_rate = rate;
	if ((err = snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &exact_rate, 0)) < 0) {
		fprintf(stderr, _("snd_pcm_hw_params_set_rate_near failed: %s\n"), snd_strerror(err));
		return -1;
	}
	if (rate != exact_rate) {
		fprintf(stderr, _("Requested rate %ldHz, got %dHz.\n"),
		        rate, exact_rate);
	}

	if ((err = snd_pcm_hw_params_set_channels(pcm_handle, hwparams, 2)) < 0) {
		fprintf(stderr, _("snd_pcm_hw_params_set_channels failed: %s\n"), snd_strerror(err));
		return -1;
	}

	if ((err = snd_pcm_hw_params_set_buffer_size_near(pcm_handle, hwparams, &buffer_frames)) < 0) {
		fprintf(stderr, _("snd_pcm_hw_params_set_buffer_size_near failed: %s\n"), snd_strerror(err));
	}

	period_frames = buffer_frames / 2;
	if ((err = snd_pcm_hw_params_set_period_size_near(pcm_handle, hwparams, &period_frames, 0)) < 0) {
		fprintf(stderr, _("snd_pcm_hw_params_set_period_size_near failed: %s\n"), snd_strerror(err));
	}

	if ((err = snd_pcm_hw_params(pcm_handle, hwparams)) < 0) {
		fprintf(stderr, _("snd_pcm_hw_params failed: %s\n"), snd_strerror(err));
		return -1;
	}

	*buffer_bytes = buffer_frames * 4;

	return 0;
}

static long is_suspended(snd_pcm_sframes_t retval)
{
#ifdef HAVE_ESTRPIPE
	return retval == -ESTRPIPE;
#else
	return snd_pcm_state(pcm_handle) == SND_PCM_STATE_SUSPENDED;
#endif
}

static ssize_t alsa_write(const void *buf, size_t count)
{
	snd_pcm_sframes_t retval;

	do {
		retval = snd_pcm_writei(pcm_handle, buf, count / 4);
		if (!is_suspended(retval))
			break;

		/* resume from suspend */
		while (snd_pcm_resume(pcm_handle) == -EAGAIN)
			sleep(1);
	} while (1);
	if (retval < 0) {
		fprintf(stderr, _("snd_pcm_writei failed: %s\n"), snd_strerror(retval));
		snd_pcm_prepare(pcm_handle);
	}
	return retval;
}

static void alsa_close()
{
	snd_pcm_drop(pcm_handle);
	snd_pcm_close(pcm_handle);
}

const struct output_plugin plugout_alsa = {
	.name = "alsa",
	.description = "ALSA sound driver",
	.open = alsa_open,
	.write = alsa_write,
	.close = alsa_close,
};
