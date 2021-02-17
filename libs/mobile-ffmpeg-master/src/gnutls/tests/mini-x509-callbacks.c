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
#include "utils.h"
#include "eagain-common.h"

/* Tests whether the verify callbacks are operational.
 * In addition gnutls_handshake_get_last_in() and gnutls_handshake_get_last_out() 
 * are tested.
 */

const char *side;
static int client_ok = 0, server_ok = 0;
static int pch_ok = 0;

static int client_callback(gnutls_session_t session)
{
	client_ok = 1;
	return 0;
}

static void verify_alpn(gnutls_session_t session)
{
	int ret;
	gnutls_datum_t selected;
	char str[64];

	snprintf(str, sizeof(str), "myproto");

	ret = gnutls_alpn_get_selected_protocol(session, &selected);
	if (ret < 0) {
		fail("gnutls_alpn_get_selected_protocol: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	if (strlen(str) != selected.size || memcmp(str, selected.data, selected.size) != 0) {
		fail("expected protocol %s, got %.*s\n", str, selected.size, selected.data);
		exit(1);
	}

	if (debug)
		success("ALPN got: %s\n", str);
}

static int post_client_hello_callback(gnutls_session_t session)
{
	/* verify that the post-client-hello callback has access to ALPN data */
	verify_alpn(session);
	pch_ok = 1;
	return 0;
}

unsigned int msg_order[] = {
	GNUTLS_HANDSHAKE_CLIENT_HELLO,
	GNUTLS_HANDSHAKE_SERVER_HELLO,
	GNUTLS_HANDSHAKE_CERTIFICATE_PKT,
	GNUTLS_HANDSHAKE_SERVER_KEY_EXCHANGE,
	GNUTLS_HANDSHAKE_CERTIFICATE_REQUEST,
	GNUTLS_HANDSHAKE_SERVER_HELLO_DONE,
	GNUTLS_HANDSHAKE_CERTIFICATE_PKT,
	GNUTLS_HANDSHAKE_CLIENT_KEY_EXCHANGE,
	GNUTLS_HANDSHAKE_FINISHED,
	GNUTLS_HANDSHAKE_FINISHED,
};

static int handshake_callback(gnutls_session_t session, unsigned int htype,
			      unsigned post, unsigned int incoming, const gnutls_datum_t *rawmsg)
{
	static unsigned idx = 0;
	unsigned int msg;

	if (msg_order[idx] != htype) {
		fail("%s: %s, expected %s\n",
		     incoming != 0 ? "Received" : "Sent",
		     gnutls_handshake_description_get_name(htype),
		     gnutls_handshake_description_get_name(msg_order
							   [idx]));
		exit(1);
	}
	idx++;

	if (incoming != 0) {
		msg = gnutls_handshake_get_last_in(session);
		if (msg != htype) {
			fail("last input message was not recorded (exp: %d, found: %d) \n", msg, htype);
			exit(1);
		}
	} else {
		msg = gnutls_handshake_get_last_out(session);
		if (msg != htype) {
			fail("last output message was not recorded (exp: %d, found: %d) \n", msg, htype);
			exit(1);
		}
	}

	return 0;
}

static int server_callback(gnutls_session_t session)
{
	server_ok = 1;

	if (gnutls_protocol_get_version(session) == GNUTLS_TLS1_2) {
		if (gnutls_handshake_get_last_in(session) !=
		    GNUTLS_HANDSHAKE_CERTIFICATE_PKT) {
			fail("client's last input message was unexpected\n");
			exit(1);
		}

		if (gnutls_handshake_get_last_out(session) !=
		    GNUTLS_HANDSHAKE_SERVER_HELLO_DONE) {
			fail("client's last output message was unexpected\n");
			exit(1);
		}
	}

	return 0;
}

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
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

static void append_alpn(gnutls_session_t session)
{
	gnutls_datum_t protocol;
	int ret;
	char str[64];

	snprintf(str, sizeof(str), "myproto");

	protocol.data = (void*)str;
	protocol.size = strlen(str);

	ret = gnutls_alpn_set_protocols(session, &protocol, 1, 0);
	if (ret < 0) {
		gnutls_perror(ret);
		exit(1);
	}
}

static
void start(const char *prio, unsigned check_order)
{
	/* Server stuff. */
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t clientx509cred;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN;

	success("trying %s\n", prio);

	client_ok = 0;
	server_ok = 0;
	pch_ok = 0;

	/* General init. */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(2);

	/* Init server */
	gnutls_certificate_allocate_credentials(&serverx509cred);
	gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM);
	gnutls_init(&server, GNUTLS_SERVER);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);
	gnutls_priority_set_direct(server, prio, NULL);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);
	gnutls_certificate_set_verify_function(serverx509cred,
						server_callback);
	gnutls_certificate_server_set_request(server, GNUTLS_CERT_REQUEST);
	gnutls_handshake_set_post_client_hello_function(server,
							post_client_hello_callback);
	if (check_order)
		gnutls_handshake_set_hook_function(server, GNUTLS_HANDSHAKE_ANY,
						   GNUTLS_HOOK_POST,
						   handshake_callback);
	append_alpn(server);

	/* Init client */
	gnutls_certificate_allocate_credentials(&clientx509cred);
	gnutls_init(&client, GNUTLS_CLIENT);
	gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);
	gnutls_priority_set_direct(client, prio, NULL);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);
	gnutls_certificate_set_verify_function(clientx509cred,
						client_callback);
	append_alpn(client);

	HANDSHAKE(client, server);

	gnutls_bye(client, GNUTLS_SHUT_RDWR);
	gnutls_bye(server, GNUTLS_SHUT_RDWR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();

	if (pch_ok == 0)
		fail("Post client hello callback wasn't called\n");

	if (server_ok == 0)
		fail("Server certificate verify callback wasn't called\n");

	if (client_ok == 0)
		fail("Client certificate verify callback wasn't called\n");

	reset_buffers();
}

void doit(void)
{
	start("NORMAL:-VERS-ALL:+VERS-TLS1.2", 1);
	start("NORMAL:-VERS-ALL:+VERS-TLS1.3", 0);
	start("NORMAL", 0);
}
