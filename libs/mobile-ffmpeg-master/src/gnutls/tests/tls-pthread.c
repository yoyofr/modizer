/*
 * Copyright (C) 2019 Nikos Mavrogiannopoulos
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
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <gnutls/gnutls.h>
#include <gnutls/dtls.h>
#include <signal.h>
#include <unistd.h>
#ifndef _WIN32
# include <netinet/in.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/wait.h>
# include <pthread.h>
#endif
#include <assert.h>
#include "utils.h"
#include "cert-common.h"

#define FLAG_CLI_DO_THREADS 1
#define FLAG_FALSE_START 2
#define FLAG_EARLY_START 4

#ifdef _WIN32

void doit(void)
{
	exit(77);
}

#else

/* Tests whether we can send and receive from different threads
 * using DTLS, either as server or client. DTLS is a superset of
 * TLS, so correct behavior under fork means TLS would operate too.
 */

const char *side = "";

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

#define MSG "hello1111"
#define MSG2 "xxxxxxxxxxxx"

#define NO_MSGS 128

static void *recv_thread(void *arg)
{
	gnutls_session_t session = arg;
	int ret;
	unsigned i;
	char buf[64];

	if (debug)
		success("client: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	for (i=0;i<NO_MSGS;i++) {
		/* the peer should reflect our messages */
		do {
			ret = gnutls_record_recv(session, buf, sizeof(buf));
		} while(ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);
		if (ret < 0)
			fail("client: recv failed: %s\n", gnutls_strerror(ret));
		if (ret != sizeof(MSG)-1 || memcmp(buf, MSG, sizeof(MSG)-1) != 0) {
			fail("client: recv failed; not the expected values (got: %d, exp: %d)\n", ret, (int)sizeof(MSG)-1);
		}

		if (debug)
			success("%d: client received: %.*s\n", i, ret, buf);
	}

	/* final MSG is MSG2 */
	do {
		ret = gnutls_record_recv(session, buf, sizeof(buf));
	} while(ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);
	if (ret < 0)
		fail("client: recv2 failed: %s\n", gnutls_strerror(ret));

	if (ret != sizeof(MSG2)-1 || memcmp(buf, MSG2, sizeof(MSG2)-1) != 0) {
		fail("client: recv2 failed; not the expected values\n");
	}

	do {
		ret = gnutls_record_recv(session, buf, sizeof(buf));
	} while(ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);
	if (ret != 0)
		fail("client: no EOF: %d\n", ret);

	if (debug) {
		success("client received: %.*s\n", ret, buf);
		success("closing recv thread\n");
	}

	pthread_exit(0);
}

static
void do_thread_stuff(gnutls_session_t session)
{
	int ret;
	unsigned i;
	pthread_t id;
	void *rval;

	sec_sleep(1);
	/* separate sending from receiving */
	ret = pthread_create(&id, NULL, recv_thread, session);
	if (ret != 0) {
		fail("error in pthread_create\n");
	}

	for (i=0;i<NO_MSGS;i++) {
		do {
			ret = gnutls_record_send(session, MSG, sizeof(MSG)-1);
		} while(ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);
		if (ret != sizeof(MSG)-1) {
			fail("client: send failed: %s\n", gnutls_strerror(ret));
		}
	}

	do {
		ret = gnutls_record_send(session, MSG2, sizeof(MSG2)-1);
	} while(ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);
	if (ret != sizeof(MSG2)-1) {
		fail("client: send2 failed: %s\n", gnutls_strerror(ret));
	}

	if (debug)
		success("closing sending thread\n");

	do {
		ret = gnutls_bye(session, GNUTLS_SHUT_WR);
	} while(ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	/* the receiving thread will receive the EOF and close */

	assert(pthread_join(id, &rval)==0);
	assert(rval == 0);

}

static void do_reflect_stuff(gnutls_session_t session)
{
	char buf[64];
	unsigned buf_size;
	int ret;

	do {
		do {
			ret = gnutls_record_recv(session, buf, sizeof(buf));
		} while(ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);
		if (ret < 0) {
			fail("server: recv failed: %s\n", gnutls_strerror(ret));
		}

		if (ret == 0) {
			break;
		}

		buf_size = ret;
		if (debug) {
			success("server received: %.*s\n", buf_size, buf);
		}

		do {
			ret = gnutls_record_send(session, buf, buf_size);
		} while(ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);
		if (ret < 0) {
			fail("server: send failed: %s\n", gnutls_strerror(ret));
		}
		if (debug)
			success("reflected %d\n", ret);
	} while(1);

	do {
		gnutls_bye(session, GNUTLS_SHUT_WR);
	} while(ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);
}

static void client(int fd, const char *prio, unsigned flags)
{
	int ret;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;
	unsigned init_flags = GNUTLS_CLIENT;

	global_init();

	if (debug) {
		side = "client";
		gnutls_global_set_log_function(tls_log_func);
		gnutls_global_set_log_level(4711);
	}

	assert(gnutls_certificate_allocate_credentials(&x509_cred)>=0);

	if (flags & FLAG_FALSE_START)
		init_flags |= GNUTLS_ENABLE_FALSE_START;

	assert(gnutls_init(&session, init_flags) >= 0);
	gnutls_handshake_set_timeout(session, 60 * 1000);

	assert(gnutls_priority_set_direct(session, prio, NULL)>=0);

	assert(gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred)>=0);

	gnutls_transport_set_int(session, fd);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret < 0) {
		fail("client: Handshake failed: %s\n", gnutls_strerror(ret));
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	if (flags & FLAG_CLI_DO_THREADS)
		do_thread_stuff(session);
	else
		do_reflect_stuff(session);

	close(fd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();
}

static void server(int fd, const char *prio, unsigned flags)
{
	int ret;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;
	unsigned init_flags = GNUTLS_SERVER;

	/* this must be called once in the program
	 */
	global_init();

#if 0
	if (debug) {
		side = "server";
		gnutls_global_set_log_function(tls_log_func);
		gnutls_global_set_log_level(4711);
	}
#endif

	assert(gnutls_certificate_allocate_credentials(&x509_cred)>=0);
	assert(gnutls_certificate_set_x509_key_mem(x509_cred, &server_cert,
					    &server_key,
					    GNUTLS_X509_FMT_PEM)>=0);

	if (flags & FLAG_EARLY_START)
		init_flags |= GNUTLS_ENABLE_EARLY_START;

	assert(gnutls_init(&session, init_flags)>=0);
	gnutls_handshake_set_timeout(session, 60 * 1000);

	assert(gnutls_priority_set_direct(session, prio, NULL)>=0);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
	} while (ret < 0 && gnutls_error_is_fatal(ret) == 0);
	if (ret < 0) {
		close(fd);
		gnutls_deinit(session);
		fail("server: Handshake has failed (%s)\n\n",
		     gnutls_strerror(ret));
	}
	if (debug)
		success("server: Handshake was completed\n");

	if (debug)
		success("server: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	if (flags & FLAG_CLI_DO_THREADS)
		do_reflect_stuff(session);
	else
		do_thread_stuff(session);


	close(fd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static
void run(const char *str, const char *prio, unsigned flags)
{
	int fd[2];
	int ret;
	pid_t child;

	if (str)
		success("running %s\n", str);

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
	if (ret < 0) {
		perror("socketpair");
		exit(1);
	}

	child = fork();
	if (child < 0) {
		perror("fork");
		fail("fork");
	}

	if (child) {
		int status;
		/* parent */

		close(fd[1]);
		client(fd[0], prio, flags);
		wait(&status);
		check_wait_status(status);
	} else {
		close(fd[0]);
		server(fd[1], prio, flags);
		exit(0);
	}
}

void doit(void)
{
	signal(SIGPIPE, SIG_IGN);
	run("default, threaded client", "NORMAL", FLAG_CLI_DO_THREADS);
	run("default, threaded server", "NORMAL", 0);
	run("tls1.2, threaded client", "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.2", FLAG_CLI_DO_THREADS);
	run("tls1.2, threaded server", "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.2", 0);
	run("tls1.2 false start, threaded client", "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.2", FLAG_CLI_DO_THREADS|FLAG_FALSE_START);
	run("tls1.2 false start, threaded server", "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.2", FLAG_FALSE_START);
	run("tls1.3 early start, threaded client", "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.3", FLAG_CLI_DO_THREADS|FLAG_EARLY_START);
	run("tls1.3 early start, threaded server", "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.3", FLAG_EARLY_START);
}
#endif				/* _WIN32 */
