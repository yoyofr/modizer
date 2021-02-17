/*
 * Copyright (C) 2000-2012 Free Software Foundation, Inc.
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

#ifndef GNUTLS_LIB_DH_H
#define GNUTLS_LIB_DH_H

const bigint_t *_gnutls_dh_params_to_mpi(gnutls_dh_params_t);

int
_gnutls_figure_dh_params(gnutls_session_t session, gnutls_dh_params_t dh_params,
		      gnutls_params_function * func, gnutls_sec_param_t sec_param);

int _gnutls_set_cred_dh_params(gnutls_dh_params_t *cparams, gnutls_sec_param_t sec_param);

#endif /* GNUTLS_LIB_DH_H */
