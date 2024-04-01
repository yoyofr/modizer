/*
 * gbsplay is a Gameboy sound player
 *
 * NAS sound output plugin
 *
 * Based on the libaudiooss nas backend, which was largely rewritten
 * by me (Tobias Diedrich), I'd dare to say the only function left from
 * the xmms code is nas_find_device, but I did not check that. :-)
 *
 * 2004-2020 (C) by Christian Garbs <mitch@cgarbs.de>
 *                  Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <audio/audiolib.h>

#include "common.h"
#include "plugout.h"

#define NAS_BUFFER_SAMPLES 8192

#if GBS_BYTE_ORDER == GBS_ORDER_LITTLE_ENDIAN
#define AuFormatLinearSigned16_NATIVE AuFormatLinearSigned16LSB
#else
#define AuFormatLinearSigned16_NATIVE AuFormatLinearSigned16MSB
#endif

static const struct timespec ONE_MILLISECOND = {
	.tv_sec = 0,
	.tv_nsec = 1000000
};

/* global variables */
static AuServer *nas_server;
static AuFlowID             nas_flow;

/* forward function declarations */
static long    nas_open(enum plugout_endian *endian, long rate, long *buffer_bytes, const struct plugout_metadata metadata);
static ssize_t nas_write(const void *buf, size_t count);
static void    nas_close();

/* descripton of this plugin */
const struct output_plugin plugout_nas = {
	.name = "nas",
	.description = "NAS sound driver",
	.open = nas_open,
	.write = nas_write,
	.close = nas_close,
};

/**
 * Use AuGetErrorText to convert NAS status code into
 * readable error message.
 * NOTE: This function is not thread-safe.
 *
 * @param status  NAS status code to convert
 * @return  Pointer to static buffer with the resulting error message
 */
static char *nas_error(AuStatus status)
{
	static char s[100];

	AuGetErrorText(nas_server, status, s, sizeof(s));

	return s;
}

/**
 * Get ID of first NAS device supporting 2 channels (we need stereo sound)
 *
 * @param aud  NAS server connection handle
 * @return  Device ID or AuNone if 2 channels are not supported on this server
 */
static AuDeviceID nas_find_device(AuServer *aud)
{
	long i;
	for (i=0; i < AuServerNumDevices(aud); i++) {
		AuDeviceAttributes *dev = AuServerDevice(aud, i);
		if ((AuDeviceKind(dev) == AuComponentKindPhysicalOutput) &&
		    AuDeviceNumTracks(dev) == 2) {
			return AuDeviceIdentifier(dev);
		}
	}
	return AuNone;
}


/**
 * Setup connection to NAS server.
 * This includes finding a suitable output device and setting
 * up the flow chain.
 *
 * @param endian  requested endianness
 * @param rate  requested samplerate
 * @param buffer_bytes  pointer to requested buffer_size,
 *                      value update to actual buffer_size.
 * @param metadata  player name and currently playing filename (basename only)
 * @return  0 on success
 */
static long nas_open(enum plugout_endian *endian, long rate, long *buffer_bytes, const struct plugout_metadata metadata)
{
	char *text = "";
	unsigned char nas_format;
	AuElement nas_elements[3];
	AuStatus  status = AuBadValue;
	AuDeviceID nas_device;

	UNUSED(buffer_bytes);
	UNUSED(metadata);

	switch (*endian) {
	case PLUGOUT_ENDIAN_BIG:	nas_format = AuFormatLinearSigned16MSB;     break;
	case PLUGOUT_ENDIAN_LITTLE:	nas_format = AuFormatLinearSigned16LSB;     break;
	default:			nas_format = AuFormatLinearSigned16_NATIVE; break;
	}

	/* open server connection */
	nas_server = AuOpenServer(NULL, 0, NULL, 0, NULL, &text);
	if (!nas_server) {
		fprintf(stderr, _("%s: Can't open server: %s\n"), plugout_nas.name, text);
		goto err;
	}

	/* search device with 2 channels */
	nas_device = nas_find_device(nas_server);
	if (nas_device == AuNone) {
		fprintf(stderr, _("%s: Can't find device with 2 channels\n"), plugout_nas.name);
		goto err_close;
	}

	/* create audio flow */
	nas_flow = AuCreateFlow(nas_server, NULL);
	if (!nas_flow) {
		fprintf(stderr, _("%s: Can't create audio flow\n"), plugout_nas.name);
		goto err_close;
	}

	/*
	 * fill audio flow chain
	 *
	 * We only use an import client and export device, for volume
	 * control a MultipyConstant could be added.
	 */
	AuMakeElementImportClient(nas_elements, rate, nas_format, 2, AuTrue,
	                          NAS_BUFFER_SAMPLES, NAS_BUFFER_SAMPLES / 2,
	                          0, NULL);
	AuMakeElementExportDevice(nas_elements+1, 0, nas_device, rate,
	                          AuUnlimitedSamples, 0, NULL);
	AuSetElements(nas_server, nas_flow, AuTrue, 2, nas_elements, &status);
	if (status != AuSuccess) {
		fprintf(stderr, _("%s: Can't set audio elements: %s\n"), plugout_nas.name, nas_error(status));
		goto err_close;
	}

	/* start sound flow */
	AuStartFlow(nas_server, nas_flow, &status);
	if (status != AuSuccess) {
		fprintf(stderr, _("%s: Can't start audio flow: %s\n"), plugout_nas.name, nas_error(status));
		goto err_close;
	}

	return 0;
err_close:
	AuCloseServer(nas_server);
	nas_server = NULL;
err:
	return -1;
}

/**
 * send count bytes of audio data to NAS server
 *
 * We unconditionally use AuWriteElement and look
 * at the error code to determine wether or not
 * the servers internal buffers are filled.
 * If they are we use sleep to wait and retry
 * until the server accepts data again.
 *
 * @param buf  pointer to 16bit stereo audio data
 * @param count  length of audio data in bytes
 * @return  the number of bytes actually written
 */
static ssize_t nas_write(const void *buf, size_t count)
{
	long maxlen = NAS_BUFFER_SAMPLES * 2;
	size_t numwritten = 0;

	while (numwritten < count) {
		AuStatus as = AuBadValue;
		long writelen = count - numwritten;

		if (writelen > maxlen) writelen = maxlen;

		AuWriteElement(nas_server, nas_flow, 0, writelen, (void*)buf, AuFalse, &as);
		/*
		 * Dispose of waiting event messages as we don't implement
		 * event handling for simplicity reasons.
		 */
		AuSync(nas_server, AuTrue);
		if (as == AuBadValue) {
			/*
			 * Does not fit in remaining buffer space,
			 * wait a bit and try again
			 */
			nanosleep(&ONE_MILLISECOND, NULL);
			continue;
		}
		if (as != AuSuccess) {
			/*
			 * Unexpected error happened
			 */
			fprintf(stderr, "%s\n", nas_error(as));
			break;
		}
		numwritten += writelen;
		buf += writelen;
	}

	return numwritten;
}

/**
 * stop flow and close NAS server connection.
 */
static void nas_close()
{
	if (nas_server) {
		AuStopFlow(nas_server, nas_flow, NULL);
		AuCloseServer(nas_server);
	}
}
