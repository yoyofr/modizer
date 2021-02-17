/*
 * Copyright (C) 2010-2014 Free Software Foundation, Inc.
 * 
 * Author: Nikos Mavrogiannopoulos
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
 */

#include "gnutls_int.h"
#include <gnutls/pkcs11.h>
#include <stdio.h>
#include <string.h>
#include "errors.h"
#include <datum.h>
#include <pkcs11_int.h>
#include <gnutls/abstract.h>
#include <pk.h>
#include <x509_int.h>
#include <tls-sig.h>
#include <algorithms.h>
#include <fips.h>
#include <abstract_int.h>

/**
 * gnutls_privkey_export_rsa_raw:
 * @key: Holds the certificate
 * @m: will hold the modulus
 * @e: will hold the public exponent
 * @d: will hold the private exponent
 * @p: will hold the first prime (p)
 * @q: will hold the second prime (q)
 * @u: will hold the coefficient
 * @e1: will hold e1 = d mod (p-1)
 * @e2: will hold e2 = d mod (q-1)
 *
 * This function will export the RSA private key's parameters found
 * in the given structure. The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum. For
 * EdDSA keys, the @y value should be %NULL.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.3.0
 **/
int
gnutls_privkey_export_rsa_raw(gnutls_privkey_t key,
				    gnutls_datum_t * m, gnutls_datum_t * e,
				    gnutls_datum_t * d, gnutls_datum_t * p,
				    gnutls_datum_t * q, gnutls_datum_t * u,
				    gnutls_datum_t * e1,
				    gnutls_datum_t * e2)
{
	return gnutls_privkey_export_rsa_raw2(key, m, e, d, p, q, u, e1, e2, 0);
}

/**
 * gnutls_privkey_export_rsa_raw2:
 * @key: Holds the certificate
 * @m: will hold the modulus
 * @e: will hold the public exponent
 * @d: will hold the private exponent
 * @p: will hold the first prime (p)
 * @q: will hold the second prime (q)
 * @u: will hold the coefficient
 * @e1: will hold e1 = d mod (p-1)
 * @e2: will hold e2 = d mod (q-1)
 * @flags: flags from %gnutls_abstract_export_flags_t
 *
 * This function will export the RSA private key's parameters found
 * in the given structure. The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.6.0
 **/
int
gnutls_privkey_export_rsa_raw2(gnutls_privkey_t key,
				    gnutls_datum_t * m, gnutls_datum_t * e,
				    gnutls_datum_t * d, gnutls_datum_t * p,
				    gnutls_datum_t * q, gnutls_datum_t * u,
				    gnutls_datum_t * e1,
				    gnutls_datum_t * e2,
				    unsigned int flags)
{
gnutls_pk_params_st params;
int ret;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}
	
	gnutls_pk_params_init(&params);

	ret = _gnutls_privkey_get_mpis(key, &params);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_params_get_rsa_raw(&params, m, e, d, p, q, u, e1, e2, flags);

	gnutls_pk_params_release(&params);

	return ret;
}

/**
 * gnutls_privkey_export_dsa_raw:
 * @key: Holds the public key
 * @p: will hold the p
 * @q: will hold the q
 * @g: will hold the g
 * @y: will hold the y
 * @x: will hold the x
 *
 * This function will export the DSA private key's parameters found
 * in the given structure. The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.3.0
 **/
int
gnutls_privkey_export_dsa_raw(gnutls_privkey_t key,
			     gnutls_datum_t * p, gnutls_datum_t * q,
			     gnutls_datum_t * g, gnutls_datum_t * y,
			     gnutls_datum_t * x)
{
	return gnutls_privkey_export_dsa_raw2(key, p, q, g, y, x, 0);
}

/**
 * gnutls_privkey_export_dsa_raw2:
 * @key: Holds the public key
 * @p: will hold the p
 * @q: will hold the q
 * @g: will hold the g
 * @y: will hold the y
 * @x: will hold the x
 * @flags: flags from %gnutls_abstract_export_flags_t
 *
 * This function will export the DSA private key's parameters found
 * in the given structure. The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.6.0
 **/
int
gnutls_privkey_export_dsa_raw2(gnutls_privkey_t key,
			     gnutls_datum_t * p, gnutls_datum_t * q,
			     gnutls_datum_t * g, gnutls_datum_t * y,
			     gnutls_datum_t * x, unsigned int flags)
{
gnutls_pk_params_st params;
int ret;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}
	
	gnutls_pk_params_init(&params);

	ret = _gnutls_privkey_get_mpis(key, &params);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_params_get_dsa_raw(&params, p, q, g, y, x, flags);

	gnutls_pk_params_release(&params);

	return ret;
}


/**
 * gnutls_privkey_export_ecc_raw:
 * @key: Holds the public key
 * @curve: will hold the curve
 * @x: will hold the x-coordinate
 * @y: will hold the y-coordinate
 * @k: will hold the private key
 *
 * This function will export the ECC private key's parameters found
 * in the given structure. The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * In EdDSA curves the @y parameter will be %NULL and the other parameters
 * will be in the native format for the curve.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.3.0
 **/
int
gnutls_privkey_export_ecc_raw(gnutls_privkey_t key,
				       gnutls_ecc_curve_t * curve,
				       gnutls_datum_t * x,
				       gnutls_datum_t * y,
				       gnutls_datum_t * k)
{
	return gnutls_privkey_export_ecc_raw2(key, curve, x, y, k, 0);
}

/**
 * gnutls_privkey_export_ecc_raw2:
 * @key: Holds the public key
 * @curve: will hold the curve
 * @x: will hold the x-coordinate
 * @y: will hold the y-coordinate
 * @k: will hold the private key
 * @flags: flags from %gnutls_abstract_export_flags_t
 *
 * This function will export the ECC private key's parameters found
 * in the given structure. The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * In EdDSA curves the @y parameter will be %NULL and the other parameters
 * will be in the native format for the curve.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.6.0
 **/
int
gnutls_privkey_export_ecc_raw2(gnutls_privkey_t key,
				       gnutls_ecc_curve_t * curve,
				       gnutls_datum_t * x,
				       gnutls_datum_t * y,
				       gnutls_datum_t * k,
				       unsigned int flags)
{
gnutls_pk_params_st params;
int ret;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}
	
	gnutls_pk_params_init(&params);

	ret = _gnutls_privkey_get_mpis(key, &params);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_params_get_ecc_raw(&params, curve, x, y, k, flags);

	gnutls_pk_params_release(&params);

	return ret;
}

/**
 * gnutls_privkey_export_gost_raw2:
 * @key: Holds the public key
 * @curve: will hold the curve
 * @digest: will hold the digest
 * @paramset: will hold the GOST parameter set ID
 * @x: will hold the x-coordinate
 * @y: will hold the y-coordinate
 * @k: will hold the private key
 * @flags: flags from %gnutls_abstract_export_flags_t
 *
 * This function will export the GOST private key's parameters found
 * in the given structure. The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * Note: parameters will be stored with least significant byte first. On
 * version 3.6.3 this was incorrectly returned in big-endian format.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.6.3
 **/
int
gnutls_privkey_export_gost_raw2(gnutls_privkey_t key,
				       gnutls_ecc_curve_t * curve,
				       gnutls_digest_algorithm_t * digest,
				       gnutls_gost_paramset_t * paramset,
				       gnutls_datum_t * x,
				       gnutls_datum_t * y,
				       gnutls_datum_t * k,
				       unsigned int flags)
{
	gnutls_pk_params_st params;
	int ret;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	gnutls_pk_params_init(&params);

	ret = _gnutls_privkey_get_mpis(key, &params);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_params_get_gost_raw(&params, curve, digest, paramset,
					  x, y, k, flags);

	gnutls_pk_params_release(&params);

	return ret;
}

/**
 * gnutls_privkey_import_rsa_raw:
 * @key: The structure to store the parsed key
 * @m: holds the modulus
 * @e: holds the public exponent
 * @d: holds the private exponent
 * @p: holds the first prime (p)
 * @q: holds the second prime (q)
 * @u: holds the coefficient (optional)
 * @e1: holds e1 = d mod (p-1) (optional)
 * @e2: holds e2 = d mod (q-1) (optional)
 *
 * This function will convert the given RSA raw parameters to the
 * native #gnutls_privkey_t format.  The output will be stored in
 * @key.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_privkey_import_rsa_raw(gnutls_privkey_t key,
				    const gnutls_datum_t * m,
				    const gnutls_datum_t * e,
				    const gnutls_datum_t * d,
				    const gnutls_datum_t * p,
				    const gnutls_datum_t * q,
				    const gnutls_datum_t * u,
				    const gnutls_datum_t * e1,
				    const gnutls_datum_t * e2)
{
int ret;
gnutls_x509_privkey_t xkey;

	ret = gnutls_x509_privkey_init(&xkey);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_x509_privkey_import_rsa_raw2(xkey, m, e, d, p, q, u, e1, e1);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}
	
	ret = gnutls_privkey_import_x509(key, xkey, GNUTLS_PRIVKEY_IMPORT_AUTO_RELEASE);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}
	
	return 0;

error:
	gnutls_x509_privkey_deinit(xkey);
	return ret;
}

/**
 * gnutls_privkey_import_dsa_raw:
 * @key: The structure to store the parsed key
 * @p: holds the p
 * @q: holds the q
 * @g: holds the g
 * @y: holds the y
 * @x: holds the x
 *
 * This function will convert the given DSA raw parameters to the
 * native #gnutls_privkey_t format.  The output will be stored
 * in @key.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_privkey_import_dsa_raw(gnutls_privkey_t key,
				   const gnutls_datum_t * p,
				   const gnutls_datum_t * q,
				   const gnutls_datum_t * g,
				   const gnutls_datum_t * y,
				   const gnutls_datum_t * x)
{
int ret;
gnutls_x509_privkey_t xkey;

	ret = gnutls_x509_privkey_init(&xkey);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_x509_privkey_import_dsa_raw(xkey, p, q, g, y, x);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}
	
	ret = gnutls_privkey_import_x509(key, xkey, GNUTLS_PRIVKEY_IMPORT_AUTO_RELEASE);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}
	
	return 0;

error:
	gnutls_x509_privkey_deinit(xkey);
	return ret;
}

/**
 * gnutls_privkey_import_ecc_raw:
 * @key: The key
 * @curve: holds the curve
 * @x: holds the x-coordinate
 * @y: holds the y-coordinate
 * @k: holds the k (private key)
 *
 * This function will convert the given elliptic curve parameters to the
 * native #gnutls_privkey_t format.  The output will be stored
 * in @key.
 *
 * In EdDSA curves the @y parameter should be %NULL and the @x and @k parameters
 * must be in the native format for the curve.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.0
 **/
int
gnutls_privkey_import_ecc_raw(gnutls_privkey_t key,
			      gnutls_ecc_curve_t curve,
			      const gnutls_datum_t * x,
			      const gnutls_datum_t * y,
			      const gnutls_datum_t * k)
{
int ret;
gnutls_x509_privkey_t xkey;

	ret = gnutls_x509_privkey_init(&xkey);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_x509_privkey_import_ecc_raw(xkey, curve, x, y, k);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}
	
	ret = gnutls_privkey_import_x509(key, xkey, GNUTLS_PRIVKEY_IMPORT_AUTO_RELEASE);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}
	
	return 0;

error:
	gnutls_x509_privkey_deinit(xkey);
	return ret;
}

/**
 * gnutls_privkey_import_gost_raw:
 * @key: The key
 * @curve: holds the curve
 * @digest: holds the digest
 * @paramset: holds the GOST parameter set ID
 * @x: holds the x-coordinate
 * @y: holds the y-coordinate
 * @k: holds the k (private key)
 *
 * This function will convert the given GOST private key's parameters to the
 * native #gnutls_privkey_t format.  The output will be stored
 * in @key.  @digest should be one of GNUTLS_DIG_GOSR_94,
 * GNUTLS_DIG_STREEBOG_256 or GNUTLS_DIG_STREEBOG_512.  If @paramset is set to
 * GNUTLS_GOST_PARAMSET_UNKNOWN default one will be selected depending on
 * @digest.
 *
 * Note: parameters should be stored with least significant byte first. On
 * version 3.6.3 big-endian format was used incorrectly.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.6.3
 **/
int
gnutls_privkey_import_gost_raw(gnutls_privkey_t key,
				   gnutls_ecc_curve_t curve,
				   gnutls_digest_algorithm_t digest,
				   gnutls_gost_paramset_t paramset,
				   const gnutls_datum_t * x,
				   const gnutls_datum_t * y,
				   const gnutls_datum_t * k)
{
	int ret;
	gnutls_x509_privkey_t xkey;

	ret = gnutls_x509_privkey_init(&xkey);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_x509_privkey_import_gost_raw(xkey, curve, digest, paramset, x, y, k);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}

	ret = gnutls_privkey_import_x509(key, xkey, GNUTLS_PRIVKEY_IMPORT_AUTO_RELEASE);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}

	return 0;

error:
	gnutls_x509_privkey_deinit(xkey);
	return ret;
}
