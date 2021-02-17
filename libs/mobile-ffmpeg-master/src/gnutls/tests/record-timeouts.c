/*
 * Copyright (C) 2015 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
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
#include <assert.h>
#include <gnutls/gnutls.h>
#include "eagain-common.h"
#include "cert-common.h"

#include "utils.h"

const char *side = "";

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

/* This tests gnutls_record_set_timeout() operation
 */

#define MAX_BUF 16384
static char b1[MAX_BUF + 1];
static char buffer[MAX_BUF + 1];
static unsigned int expected_val = -1;
static unsigned int called = 1;

static int pull_timeout_func(gnutls_transport_ptr_t ptr, unsigned int ms)
{
	called = 1;
	if (ms == 0 && expected_val != 0) {
		fail("Expected timeout value: %u, got %u\n", expected_val, ms);
		exit(1);
	} else if (ms != expected_val && ms == GNUTLS_INDEFINITE_TIMEOUT) {
		fail("Expected timeout value: %u, got %u\n", expected_val, ms);
		exit(1);
	}
	return 1;
}

#define MAX_VALS 4
static const int vals[MAX_VALS] = {0, 1000, 5000, GNUTLS_INDEFINITE_TIMEOUT};

static void start(const char *prio)
{
	/* Server stuff. */
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t clientx509cred;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN, i;
	/* Need to enable anonymous KX specifically. */
	int transferred = 0;

	success("trying %s\n", prio);

	/* General init. */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	/* Init server */
	gnutls_certificate_allocate_credentials(&serverx509cred);
	gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM);

	gnutls_init(&server, GNUTLS_SERVER);

	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);
	assert(gnutls_priority_set_direct(server, prio, NULL) >= 0);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	/* Init client */
	assert(gnutls_certificate_allocate_credentials(&clientx509cred)>=0);
	gnutls_init(&client, GNUTLS_CLIENT);
	assert(gnutls_priority_set_direct(client, prio, NULL) >= 0);
	assert(gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				      clientx509cred)>=0);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_pull_timeout_function(client, pull_timeout_func);
	gnutls_transport_set_ptr(client, client);

	memset(b1, 0, sizeof(b1));
	HANDSHAKE(client, server);

	if (debug)
		success("Handshake established\n");

	memset(b1, 1, MAX_BUF);

	/* Try sending various other sizes */
	for (i = 1; i < 128; i++) {
		called = 0;
		gnutls_record_set_timeout(client, vals[i%MAX_VALS]);
		expected_val = vals[i%MAX_VALS];

		TRANSFER(client, server, b1, i, buffer, MAX_BUF);

		if (called == 0 && expected_val != 0) {
			fail("pull timeout callback was not called for %d!\n", vals[i%MAX_VALS]);
			exit(1);
		} else if (called != 0 && expected_val == 0) {
			fail("pull timeout callback was called for %d!\n", vals[i%MAX_VALS]);
			exit(1);
		}
	}
	if (debug)
		fputs("\n", stdout);

	gnutls_bye(client, GNUTLS_SHUT_RDWR);
	gnutls_bye(server, GNUTLS_SHUT_RDWR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();
	reset_buffers();
}

void doit(void)
{
	start("NORMAL:-VERS-ALL:+VERS-TLS1.2");
	start("NORMAL:-VERS-ALL:+VERS-TLS1.3");
	start("NORMAL");
}
