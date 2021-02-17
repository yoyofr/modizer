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

#ifndef GNUTLS_LIB_DATUM_H
#define GNUTLS_LIB_DATUM_H

# include "gnutls_int.h"

/* This will copy the provided data in @dat. If the provided data are
 * NULL or zero-size @dat will be NULL as well.
 */
attr_warn_unused_result attr_nonnull((1))
int _gnutls_set_datum(gnutls_datum_t * dat, const void *data,
		      size_t data_size);

/* This will always return a non-NULL, and zero-terminated string in @dat.
 */
attr_warn_unused_result attr_nonnull((1))
int _gnutls_set_strdatum(gnutls_datum_t * dat, const void *data,
			 size_t data_size);


inline static
void _gnutls_free_datum(gnutls_datum_t * dat)
{
	if (dat != NULL) {
		gnutls_free(dat->data);
		dat->size = 0;
	}
}

inline static attr_nonnull_all
void _gnutls_free_temp_key_datum(gnutls_datum_t * dat)
{
	if (dat->data != NULL) {
		zeroize_temp_key(dat->data, dat->size);
		gnutls_free(dat->data);
	}

	dat->size = 0;
}

inline static attr_nonnull_all
void _gnutls_free_key_datum(gnutls_datum_t * dat)
{
	if (dat->data != NULL) {
		zeroize_key(dat->data, dat->size);
		gnutls_free(dat->data);
	}

	dat->size = 0;
}

#endif /* GNUTLS_LIB_DATUM_H */
