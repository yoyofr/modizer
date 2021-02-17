/*
 * Copyright (C) 2015-2016 Red Hat, Inc.
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

/* This program tests the various certificate key exchange methods supported
 * in gnutls */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gnutls/gnutls.h>
#include "utils.h"
#include "common-cert-key-exchange.h"
#include "cert-common.h"

void doit(void)
{
#ifdef ENABLE_SSL3
	global_init();
	try_x509("SSL 3.0 with anon-dh", "NORMAL:-VERS-ALL:+VERS-SSL3.0:-KX-ALL:+ANON-DH", GNUTLS_KX_ANON_DH, GNUTLS_SIGN_UNKNOWN, GNUTLS_SIGN_UNKNOWN);
	try_x509("SSL 3.0 with dhe-rsa no cert", "NORMAL:-VERS-ALL:+VERS-SSL3.0:-KX-ALL:+DHE-RSA", GNUTLS_KX_DHE_RSA, GNUTLS_SIGN_UNKNOWN, GNUTLS_SIGN_UNKNOWN);

	try_x509("SSL 3.0 with rsa no cert", "NORMAL:-VERS-ALL:+VERS-SSL3.0:-KX-ALL:+RSA", GNUTLS_KX_RSA, GNUTLS_SIGN_UNKNOWN, GNUTLS_SIGN_UNKNOWN);
	try_x509_cli("SSL 3.0 with dhe-rsa cert", "NORMAL:-VERS-ALL:+VERS-SSL3.0:-KX-ALL:+DHE-RSA", GNUTLS_KX_DHE_RSA, GNUTLS_SIGN_UNKNOWN, GNUTLS_SIGN_UNKNOWN, USE_CERT);
	try_x509_cli("SSL 3.0 with rsa cert", "NORMAL:-VERS-ALL:+VERS-SSL3.0:-KX-ALL:+RSA", GNUTLS_KX_RSA, GNUTLS_SIGN_UNKNOWN, GNUTLS_SIGN_UNKNOWN, USE_CERT);

	try_x509_cli("SSL 3.0 with dhe-rsa ask cert", "NORMAL:-VERS-ALL:+VERS-SSL3.0:-KX-ALL:+DHE-RSA", GNUTLS_KX_DHE_RSA, GNUTLS_SIGN_UNKNOWN, GNUTLS_SIGN_UNKNOWN, ASK_CERT);
	try_x509_cli("SSL 3.0 with rsa ask cert", "NORMAL:-VERS-ALL:+VERS-SSL3.0:-KX-ALL:+RSA", GNUTLS_KX_RSA, GNUTLS_SIGN_UNKNOWN, GNUTLS_SIGN_UNKNOWN, ASK_CERT);
	gnutls_global_deinit();
#else
	exit(77);
#endif
}
