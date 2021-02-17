/*
 * Copyright (C) 2012 Free Software Foundation, Inc.
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

#if defined(_WIN32)

int main()
{
	exit(77);
}

#else

#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <gnutls/dtls.h>
#include <signal.h>

#include "utils.h"

static void terminate(void);

/* This program tests the ability to transfer various data size
 * by the record layer, under different ciphersuites.
 */

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

static unsigned char server_cert_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIICVjCCAcGgAwIBAgIERiYdMTALBgkqhkiG9w0BAQUwGTEXMBUGA1UEAxMOR251\n"
    "VExTIHRlc3QgQ0EwHhcNMDcwNDE4MTMyOTIxWhcNMDgwNDE3MTMyOTIxWjA3MRsw\n"
    "GQYDVQQKExJHbnVUTFMgdGVzdCBzZXJ2ZXIxGDAWBgNVBAMTD3Rlc3QuZ251dGxz\n"
    "Lm9yZzCBnDALBgkqhkiG9w0BAQEDgYwAMIGIAoGA17pcr6MM8C6pJ1aqU46o63+B\n"
    "dUxrmL5K6rce+EvDasTaDQC46kwTHzYWk95y78akXrJutsoKiFV1kJbtple8DDt2\n"
    "DZcevensf9Op7PuFZKBroEjOd35znDET/z3IrqVgbtm2jFqab7a+n2q9p/CgMyf1\n"
    "tx2S5Zacc1LWn9bIjrECAwEAAaOBkzCBkDAMBgNVHRMBAf8EAjAAMBoGA1UdEQQT\n"
    "MBGCD3Rlc3QuZ251dGxzLm9yZzATBgNVHSUEDDAKBggrBgEFBQcDATAPBgNVHQ8B\n"
    "Af8EBQMDB6AAMB0GA1UdDgQWBBTrx0Vu5fglyoyNgw106YbU3VW0dTAfBgNVHSME\n"
    "GDAWgBTpPBz7rZJu5gakViyi4cBTJ8jylTALBgkqhkiG9w0BAQUDgYEAaFEPTt+7\n"
    "bzvBuOf7+QmeQcn29kT6Bsyh1RHJXf8KTk5QRfwp6ogbp94JQWcNQ/S7YDFHglD1\n"
    "AwUNBRXwd3riUsMnsxgeSDxYBfJYbDLeohNBsqaPDJb7XailWbMQKfAbFQ8cnOxg\n"
    "rOKLUQRWJ0K3HyXRMhbqjdLIaQiCvQLuizo=\n" "-----END CERTIFICATE-----\n";

const gnutls_datum_t server_cert = { server_cert_pem,
	sizeof(server_cert_pem)
};

static unsigned char server_key_pem[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIICXAIBAAKBgQDXulyvowzwLqknVqpTjqjrf4F1TGuYvkrqtx74S8NqxNoNALjq\n"
    "TBMfNhaT3nLvxqResm62ygqIVXWQlu2mV7wMO3YNlx696ex/06ns+4VkoGugSM53\n"
    "fnOcMRP/PciupWBu2baMWppvtr6far2n8KAzJ/W3HZLllpxzUtaf1siOsQIDAQAB\n"
    "AoGAYAFyKkAYC/PYF8e7+X+tsVCHXppp8AoP8TEZuUqOZz/AArVlle/ROrypg5kl\n"
    "8YunrvUdzH9R/KZ7saNZlAPLjZyFG9beL/am6Ai7q7Ma5HMqjGU8kTEGwD7K+lbG\n"
    "iomokKMOl+kkbY/2sI5Czmbm+/PqLXOjtVc5RAsdbgvtmvkCQQDdV5QuU8jap8Hs\n"
    "Eodv/tLJ2z4+SKCV2k/7FXSKWe0vlrq0cl2qZfoTUYRnKRBcWxc9o92DxK44wgPi\n"
    "oMQS+O7fAkEA+YG+K9e60sj1K4NYbMPAbYILbZxORDecvP8lcphvwkOVUqbmxOGh\n"
    "XRmTZUuhBrJhJKKf6u7gf3KWlPl6ShKEbwJASC118cF6nurTjuLf7YKARDjNTEws\n"
    "qZEeQbdWYINAmCMj0RH2P0mvybrsXSOD5UoDAyO7aWuqkHGcCLv6FGG+qwJAOVqq\n"
    "tXdUucl6GjOKKw5geIvRRrQMhb/m5scb+5iw8A4LEEHPgGiBaF5NtJZLALgWfo5n\n"
    "hmC8+G8F0F78znQtPwJBANexu+Tg5KfOnzSILJMo3oXiXhf5PqXIDmbN0BKyCKAQ\n"
    "LfkcEcUbVfmDaHpvzwY9VEaoMOKVLitETXdNSxVpvWM=\n"
    "-----END RSA PRIVATE KEY-----\n";

const gnutls_datum_t server_key = { server_key_pem,
	sizeof(server_key_pem)
};


/* A very basic TLS client, with anonymous authentication.
 */

#define MAX_BUF 24*1024

static void client(int fd, const char *prio, int ign)
{
	int ret;
	unsigned i;
	char buffer[MAX_BUF + 1];
	const char* err;
	gnutls_anon_client_credentials_t anoncred;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;
	/* Need to enable anonymous KX specifically. */

	global_init();
	memset(buffer, 2, sizeof(buffer));

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(7);
	}

	gnutls_anon_allocate_client_credentials(&anoncred);
	gnutls_certificate_allocate_credentials(&x509_cred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	/* Use default priorities */
	ret = gnutls_priority_set_direct(session, prio, &err);
	if (ret < 0) {
		fail("error setting priority: %s\n", err);
		exit(1);
	}

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret < 0) {
		fail("client (%s): Handshake has failed (%s)\n\n", prio,
		     gnutls_strerror(ret));
		exit(1);
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	if (debug)
		success("client: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	/* Test sending */
	for (i = 1; i < 16384; i++) {
		do {
			ret = gnutls_record_send(session, buffer, i);
		} while (ret == GNUTLS_E_AGAIN
			 || ret == GNUTLS_E_INTERRUPTED);

		if (ret < 0) {
			fail("server (%s): Error sending %d byte packet: %s\n", prio, i, gnutls_strerror(ret));
			terminate();
		}
	}

	/* Try sending a bit more */
	i = 21056;
	do {
		ret = gnutls_record_send(session, buffer, i);
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	if (ret < 0) {
		fail("server (%s): Error sending %d byte packet: %s\n",
		     prio, i, gnutls_strerror(ret));
		exit(1);
	} else if (ign == 0 && ret != 16384) {
		fail("server (%s): Error sending %d byte packet; sent %d bytes instead of 16384\n", prio, i, ret);
		exit(1);
	}
	
	ret = gnutls_bye(session, GNUTLS_SHUT_WR);
	if (ret < 0) {
		fail("server (%s): Error sending alert\n", prio);
		exit(1);
	}

	/* make sure we are not blocked forever */
	gnutls_record_set_timeout(session, 10000);

	/* Test receiving */
	do {
		do {
			ret = gnutls_record_recv(session, buffer, MAX_BUF);
		} while (ret == GNUTLS_E_AGAIN
			 || ret == GNUTLS_E_INTERRUPTED);
	} while (ret > 0);

	if (ret == 0) {
		if (debug)
			success
			    ("client: Peer has closed the TLS connection\n");
		goto end;
	} else if (ret < 0) {
		if (ret != 0) {
			fail("client: Error: %s\n", gnutls_strerror(ret));
			exit(1);
		}
	}

      end:

	close(fd);

	gnutls_deinit(session);

	gnutls_anon_free_client_credentials(anoncred);
	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();
}


/* These are global */
pid_t child;

static void terminate(void)
{
	kill(child, SIGTERM);
	exit(1);
}

static void server(int fd, const char *prio, int ign)
{
	int ret;
	unsigned i;
	const char* err;
	char buffer[MAX_BUF + 1];
	gnutls_session_t session;
	gnutls_anon_server_credentials_t anoncred;
	gnutls_certificate_credentials_t x509_cred;

	/* this must be called once in the program
	 */
	global_init();
	memset(buffer, 0, sizeof(buffer));

	if (debug) {
		gnutls_global_set_log_function(server_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_certificate_allocate_credentials(&x509_cred);
	gnutls_certificate_set_x509_key_mem(x509_cred, &server_cert,
					    &server_key,
					    GNUTLS_X509_FMT_PEM);

	gnutls_anon_allocate_server_credentials(&anoncred);

	gnutls_init(&session, GNUTLS_SERVER);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	ret = gnutls_priority_set_direct(session, prio, &err);
	if (ret < 0) {
		fail("error setting priority: %s\n", err);
		exit(1);
	}

	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);
	if (ret < 0) {
		close(fd);
		gnutls_deinit(session);
		fail("server (%s): Handshake has failed (%s)\n\n", prio,
		     gnutls_strerror(ret));
		terminate();
	}
	if (debug)
		success("server: Handshake was completed\n");

	if (debug)
		success("server: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	/* Here we do both a receive and a send test because if valgrind
	 * detects an error on the peer, the main process will never know.
	 */

	/* Test receiving */
	do {
		do {
			ret = gnutls_record_recv(session, buffer, MAX_BUF);
		} while (ret == GNUTLS_E_AGAIN
			 || ret == GNUTLS_E_INTERRUPTED);
	} while (ret > 0);

	if (ret < 0) {
		fail("client: Error: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	if (ret != 0) {
		fail("expected closure alert! Got: %d\n", ret);
	}

	/* Test sending */
	for (i = 1; i < 16384; i++) {
		do {
			ret = gnutls_record_send(session, buffer, i);
		} while (ret == GNUTLS_E_AGAIN
			 || ret == GNUTLS_E_INTERRUPTED);

		if (ret < 0) {
			fail("server (%s): Error sending %d byte packet: %s\n", prio, i, gnutls_strerror(ret));
			terminate();
		}
	}

	/* Try sending a bit more */
	i = 21056;
	do {
		ret = gnutls_record_send(session, buffer, i);
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	if (ret < 0) {
		fail("server (%s): Error sending %d byte packet: %s\n",
		     prio, i, gnutls_strerror(ret));
		terminate();
	} else if (ign == 0 && ret != 16384) {
		fail("server (%s): Error sending %d byte packet; sent %d bytes instead of 16384\n", prio, i, ret);
		terminate();
	}

	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(fd);
	gnutls_deinit(session);

	gnutls_anon_free_server_credentials(anoncred);
	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static void start(const char *name, const char *prio, int ign)
{
	int fd[2];
	int ret;
	int status;

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
	if (ret < 0) {
		perror("socketpair");
		exit(1);
	}

	child = fork();
	if (child < 0) {
		perror("fork");
		fail("fork");
		exit(1);
	}

	if (child) {
		/* parent */
		success("testing %s\n", name);
		close(fd[1]);
		server(fd[0], prio, ign);
		wait(&status);
		check_wait_status(status);
	} else {
		close(fd[0]);
		client(fd[1], prio, ign);
		exit(0);
	}
}

#define AES_CBC "NONE:+VERS-TLS1.0:-CIPHER-ALL:+AES-128-CBC:+SHA1:+SIGN-ALL:+COMP-NULL:+ANON-ECDH:+CURVE-ALL"
#define AES_CBC_SHA256 "NONE:+VERS-TLS1.2:-CIPHER-ALL:+RSA:+AES-128-CBC:+AES-256-CBC:+SHA256:+SIGN-ALL:+COMP-NULL:+ANON-ECDH:+CURVE-ALL"
#define AES_GCM "NONE:+VERS-TLS1.2:-CIPHER-ALL:+RSA:+AES-128-GCM:+MAC-ALL:+SIGN-ALL:+COMP-NULL:+ANON-ECDH:+CURVE-ALL"
#define AES_CCM "NONE:+VERS-TLS1.2:-CIPHER-ALL:+RSA:+AES-128-CCM:+MAC-ALL:+SIGN-ALL:+COMP-NULL:+ANON-ECDH:+CURVE-ALL"
#define AES_CCM_8 "NONE:+VERS-TLS1.2:-CIPHER-ALL:+RSA:+AES-128-CCM-8:+MAC-ALL:+SIGN-ALL:+COMP-NULL:+ANON-ECDH:+CURVE-ALL"

#define ARCFOUR_SHA1 "NONE:+VERS-TLS1.0:-CIPHER-ALL:+ARCFOUR-128:+SHA1:+SIGN-ALL:+COMP-NULL:+ANON-ECDH:+CURVE-ALL"
#define ARCFOUR_MD5 "NONE:+VERS-TLS1.0:-CIPHER-ALL:+ARCFOUR-128:+MD5:+SIGN-ALL:+COMP-NULL:+ANON-ECDH:+CURVE-ALL:+RSA"

#define NULL_SHA1 "NONE:+VERS-TLS1.0:-CIPHER-ALL:+NULL:+SHA1:+SIGN-ALL:+COMP-NULL:+ANON-ECDH:+RSA:+CURVE-ALL"

#define CHACHA_POLY1305 "NONE:+VERS-TLS1.2:-CIPHER-ALL:+RSA:+CHACHA20-POLY1305:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ECDHE-RSA:+CURVE-ALL"

#define TLS13_AES_GCM "NONE:+VERS-TLS1.3:-CIPHER-ALL:+RSA:+AES-128-GCM:+MAC-ALL:+SIGN-ALL:+COMP-NULL:+GROUP-ALL"
#define TLS13_AES_CCM "NONE:+VERS-TLS1.3:-CIPHER-ALL:+RSA:+AES-128-CCM:+MAC-ALL:+SIGN-ALL:+COMP-NULL:+GROUP-ALL"
#define TLS13_CHACHA_POLY1305 "NONE:+VERS-TLS1.3:-CIPHER-ALL:+RSA:+CHACHA20-POLY1305:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+GROUP-ALL"

static void ch_handler(int sig)
{
	return;
}

void doit(void)
{
	signal(SIGCHLD, ch_handler);

	start("aes-cbc", AES_CBC, 1);
	start("aes-cbc-sha256", AES_CBC_SHA256, 1);
	start("aes-gcm", AES_GCM, 0);
	start("aes-ccm", AES_CCM, 0);
	start("aes-ccm-8", AES_CCM_8, 0);

	if (!gnutls_fips140_mode_enabled()) {
		start("null-sha1", NULL_SHA1, 0);

		start("arcfour-sha1", ARCFOUR_SHA1, 0);
		start("arcfour-md5", ARCFOUR_MD5, 0);
		start("chacha20-poly1305", CHACHA_POLY1305, 0);
		start("tls13-chacha20-poly1305", TLS13_CHACHA_POLY1305, 0);
	}

	start("tls13-aes-gcm", TLS13_AES_GCM, 0);
	start("tls13-aes-ccm", TLS13_AES_CCM, 0);

}

#endif				/* _WIN32 */
