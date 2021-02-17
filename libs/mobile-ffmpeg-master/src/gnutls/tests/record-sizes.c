/*
 * Copyright (C) 2008-2012 Free Software Foundation, Inc.
 *
 * Author: Simon Josefsson
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gnutls/gnutls.h>
#include "eagain-common.h"

#include "utils.h"

const char *side = "";

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

/* This test attempts to transfer various sizes using ARCFOUR-128.
 */

#define MAX_BUF 16384
static char b1[MAX_BUF + 1];
static char buffer[MAX_BUF + 1];

void doit(void)
{
	/* Server stuff. */
	gnutls_anon_server_credentials_t s_anoncred;
	const gnutls_datum_t p3 =
	    { (unsigned char *) pkcs3, strlen(pkcs3) };
	static gnutls_dh_params_t dh_params;
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_anon_client_credentials_t c_anoncred;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN, i;
	/* Need to enable anonymous KX specifically. */
	int ret, transferred = 0;

	if (gnutls_fips140_mode_enabled()) {
		exit(77);
	}

	/* General init. */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	/* Init server */
	gnutls_anon_allocate_server_credentials(&s_anoncred);
	gnutls_dh_params_init(&dh_params);
	gnutls_dh_params_import_pkcs3(dh_params, &p3, GNUTLS_X509_FMT_PEM);
	gnutls_anon_set_server_dh_params(s_anoncred, dh_params);
	gnutls_init(&server, GNUTLS_SERVER);
	gnutls_priority_set_direct(server,
				   "NONE:+VERS-TLS1.2:+ARCFOUR-128:+MD5:+MAC-ALL:+SIGN-ALL:+COMP-NULL:+ANON-DH",
				   NULL);
	gnutls_credentials_set(server, GNUTLS_CRD_ANON, s_anoncred);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	/* Init client */
	gnutls_anon_allocate_client_credentials(&c_anoncred);
	gnutls_init(&client, GNUTLS_CLIENT);
	gnutls_priority_set_direct(client,
				   "NONE:+VERS-TLS1.2:+CIPHER-ALL:+ARCFOUR-128:+MD5:+MAC-ALL:+SIGN-ALL:+COMP-NULL:+ANON-DH",
				   NULL);
	gnutls_credentials_set(client, GNUTLS_CRD_ANON, c_anoncred);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	memset(b1, 0, sizeof(b1));
	HANDSHAKE(client, server);

	if (debug)
		success("Handshake established\n");

	memset(b1, 1, MAX_BUF);

	/* try the maximum allowed */
	ret = gnutls_record_send(client, b1, MAX_BUF);
	if (ret < 0) {
		fprintf(stderr, "Error sending %d bytes: %s\n",
			(int) MAX_BUF, gnutls_strerror(ret));
		exit(1);
	}

	if (ret != MAX_BUF) {
		fprintf(stderr, "Couldn't send %d bytes\n", (int) MAX_BUF);
		exit(1);
	}

	ret = gnutls_record_recv(server, buffer, MAX_BUF);
	if (ret < 0) {
		fprintf(stderr, "Error receiving %d bytes: %s\n",
			(int) MAX_BUF, gnutls_strerror(ret));
		exit(1);
	}

	if (ret != MAX_BUF) {
		fprintf(stderr, "Couldn't receive %d bytes, received %d\n",
			(int) MAX_BUF, ret);
		exit(1);
	}

	if (memcmp(b1, buffer, MAX_BUF) != 0) {
		fprintf(stderr, "Buffers do not match!\n");
		exit(1);
	}

	/* Try sending various other sizes */
	for (i = 1; i < 128; i++) {
		TRANSFER(client, server, b1, i, buffer, MAX_BUF);
	}
	if (debug)
		fputs("\n", stdout);



	gnutls_bye(client, GNUTLS_SHUT_RDWR);
	gnutls_bye(server, GNUTLS_SHUT_RDWR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_anon_free_client_credentials(c_anoncred);
	gnutls_anon_free_server_credentials(s_anoncred);

	gnutls_dh_params_deinit(dh_params);

	gnutls_global_deinit();
}

