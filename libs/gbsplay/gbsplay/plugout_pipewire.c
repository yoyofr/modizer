/*
 * gbsplay is a Gameboy sound player
 *
 * 2024-2025 (C) by Christian Garbs <mitch@cgarbs.de>
 *                  Lisa Riedler <dev@riedler.wien>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#include <stdbool.h>
#include <time.h>

#include <spa/param/audio/format-utils.h>
#include <spa/utils/result.h>
#include <pipewire/pipewire.h>

#include "common.h"
#include "plugout.h"

static const int BYTES_PER_SAMPLE = 2;
static const int CHANNELS = 2;
static const int STRIDE = BYTES_PER_SAMPLE * CHANNELS;

static struct pipewire_data {
	struct pw_thread_loop *loop;
	struct pw_stream *stream;
	struct timespec buffer_fill_wait_time;
} pipewire_data;

static const struct pw_stream_events pipewire_stream_events = {
        PW_VERSION_STREAM_EVENTS,
};

static long pipewire_open(enum plugout_endian *endian, long rate, long *buffer_bytes, const struct plugout_metadata metadata)
{
	const struct spa_pod *params[2];
	uint8_t buffer[1024];
	struct pw_properties *props;
	struct spa_pod_builder pod_builder = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
	enum spa_audio_format fmt;
	int err;

	// determine endianess
	switch (*endian) {
	case PLUGOUT_ENDIAN_BIG:    fmt = SPA_AUDIO_FORMAT_S16_BE; break;
	case PLUGOUT_ENDIAN_LITTLE: fmt = SPA_AUDIO_FORMAT_S16_LE; break;
	default:                    fmt = SPA_AUDIO_FORMAT_S16;    break;
	}

	// determine buffer wait time - use 25% gbsplay buffer length (~2 wait cycles on my machine)
	pipewire_data.buffer_fill_wait_time.tv_sec = 0;
	pipewire_data.buffer_fill_wait_time.tv_nsec =
		1000000000                 // nanoseconds per second
		/ rate                     // sample rate
		* (*buffer_bytes / STRIDE) // samples in buffer
		/ 4;                       // 25% of that

	// init pipewire
	pw_init(0, NULL);

	// set main loop
	pipewire_data.loop = pw_thread_loop_new(metadata.player_name, NULL);

	// set stream metadata
	props = pw_properties_new(PW_KEY_MEDIA_TYPE,     "Audio",
				  PW_KEY_MEDIA_CATEGORY, "Playback",
				  PW_KEY_MEDIA_ROLE,     "Music",
				  NULL);

	// TODO: we could add a PW_KEY_TARGET_OBJECT to select an audio target,
	// but we can't yet pass parameters to plugouts.

	// set audio format
	params[0] = spa_format_audio_raw_build(&pod_builder, SPA_PARAM_EnumFormat,
					       &SPA_AUDIO_INFO_RAW_INIT(
						       .format = fmt,
						       .channels = CHANNELS,
						       .rate = rate));
	// triple-buffer audio
	params[1] = spa_pod_builder_add_object(&pod_builder, SPA_TYPE_OBJECT_ParamBuffers,
					       SPA_PARAM_Buffers,
					       SPA_PARAM_BUFFERS_buffers, SPA_POD_Int(3));

	// create stream
	pipewire_data.stream = pw_stream_new_simple(pw_thread_loop_get_loop(pipewire_data.loop),
						    metadata.filename,
						    props,
						    &pipewire_stream_events,
						    &pipewire_data);

	// connect the stream
        if ((err = pw_stream_connect(pipewire_data.stream,
				     PW_DIRECTION_OUTPUT,
				     PW_ID_ANY,
				     PW_STREAM_FLAG_AUTOCONNECT |
				     PW_STREAM_FLAG_MAP_BUFFERS,
				     params, 2))) {
		fprintf(stderr, _("pw_stream_connect failed: %s\n"), spa_strerror(err));
		return -1;
	}

	// run the loop
	if ((err = pw_thread_loop_start(pipewire_data.loop) != 0)) {
		fprintf(stderr, _("pw_thread_loop_start failed: %s\n"), spa_strerror(err));
		return -1;
	}

	return 0;
}

static void pipewire_pause(int pause)
{
	int err;
	pw_thread_loop_lock(pipewire_data.loop);
	if ((err = pw_stream_set_active(pipewire_data.stream, !pause)))
		fprintf(stderr, _("pw_stream_set_active failed: %s\n"), spa_strerror(err));
	pw_thread_loop_unlock(pipewire_data.loop);
}

static ssize_t pipewire_write(const void *buf, size_t count)
{
        struct pw_buffer *b;
        struct spa_buffer *spa_buf;
        int n_frames;
        uint8_t *p;
	size_t buf_sent = 0;
	size_t bytes_to_send;

	// repeat until the whole buffer is sent
	while (buf_sent < count) {
		
		const int frames_to_send = (count - buf_sent) / STRIDE;

		// wait until data can be sent by us
		while ((b = pw_stream_dequeue_buffer(pipewire_data.stream)) == NULL) {
			nanosleep(&pipewire_data.buffer_fill_wait_time, NULL);
		}

		// get send buffer
		spa_buf = b->buffer;
		if ((p = spa_buf->datas[0].data) == NULL) {
			nanosleep(&pipewire_data.buffer_fill_wait_time, NULL);
			break;
		}

		// check how much we can send
		n_frames = spa_buf->datas[0].maxsize / STRIDE;
#if PW_CHECK_VERSION(0,3,49)
		// unfortunately our CI pipeline runs Ubuntu 22.04LTS
	        // which is on 0.3.48, so we have to do this version check
		if (b->requested)
			n_frames = SPA_MIN((int)b->requested, n_frames);
#endif
		n_frames = SPA_MIN(n_frames, frames_to_send);
		bytes_to_send = n_frames * STRIDE;

		// send audio data
		memcpy(p, ((uint8_t *) buf) + buf_sent, bytes_to_send);

		spa_buf->datas[0].chunk->offset = 0;
		spa_buf->datas[0].chunk->stride = STRIDE;
		spa_buf->datas[0].chunk->size = bytes_to_send;
 
		pw_stream_queue_buffer(pipewire_data.stream, b);

		// remember what was sent
		buf_sent += bytes_to_send;
	}
	
	return 0;
}

static void pipewire_close()
{
        pw_thread_loop_stop(pipewire_data.loop);
	pw_stream_destroy(pipewire_data.stream);
        pw_thread_loop_destroy(pipewire_data.loop);
	pw_deinit();
}

const struct output_plugin plugout_pipewire = {
	.name = "pipewire",
	.description = "PipeWire sound driver",
	.open = pipewire_open,
	.pause = pipewire_pause,
	.write = pipewire_write,
	.close = pipewire_close,
};
