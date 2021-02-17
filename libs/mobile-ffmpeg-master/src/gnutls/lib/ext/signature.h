/*
 * Copyright (C) 2002-2012 Free Software Foundation, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#ifndef GNUTLS_LIB_EXT_SIGNATURE_H
#define GNUTLS_LIB_EXT_SIGNATURE_H

/* signature algorithms extension
 */

#include <hello_ext.h>

extern const hello_ext_entry_st ext_mod_sig;

gnutls_sign_algorithm_t
_gnutls_session_get_sign_algo(gnutls_session_t session,
			      gnutls_pcert_st * cert,
			      gnutls_privkey_t privkey,
			      unsigned client_cert,
			      gnutls_kx_algorithm_t kx_algorithm);
int _gnutls_sign_algorithm_parse_data(gnutls_session_t session,
				      const uint8_t * data,
				      size_t data_size);
int _gnutls_sign_algorithm_write_params(gnutls_session_t session,
					gnutls_buffer_st * extdata);
int _gnutls_session_sign_algo_enabled(gnutls_session_t session,
				      gnutls_sign_algorithm_t sig);

static inline void
gnutls_sign_algorithm_set_server(gnutls_session_t session,
				 gnutls_sign_algorithm_t sign)
{
	session->security_parameters.server_sign_algo = sign;
}

static inline void
gnutls_sign_algorithm_set_client(gnutls_session_t session,
				 gnutls_sign_algorithm_t sign)
{
	session->security_parameters.client_sign_algo = sign;
}

#endif /* GNUTLS_LIB_EXT_SIGNATURE_H */
