/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
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

#ifndef GNUTLS_LIB_CRYPTO_BACKEND_H
#define GNUTLS_LIB_CRYPTO_BACKEND_H

#include <gnutls/crypto.h>

#define gnutls_crypto_single_cipher_st gnutls_crypto_cipher_st
#define gnutls_crypto_single_mac_st gnutls_crypto_mac_st
#define gnutls_crypto_single_digest_st gnutls_crypto_digest_st

typedef struct {
	gnutls_cipher_init_func init;
	gnutls_cipher_setkey_func setkey;
	gnutls_cipher_setiv_func setiv;
	gnutls_cipher_getiv_func getiv;
	gnutls_cipher_encrypt_func encrypt;
	gnutls_cipher_decrypt_func decrypt;
	gnutls_cipher_aead_encrypt_func aead_encrypt;
	gnutls_cipher_aead_decrypt_func aead_decrypt;
	gnutls_cipher_deinit_func deinit;
	gnutls_cipher_auth_func auth;
	gnutls_cipher_tag_func tag;

	/* Not needed for registered on run-time. Only included
	 * should define it. */
	int (*exists) (gnutls_cipher_algorithm_t);	/* true/false */
} gnutls_crypto_cipher_st;

typedef struct {
	gnutls_mac_init_func init;
	gnutls_mac_setkey_func setkey;
	gnutls_mac_setnonce_func setnonce;
	gnutls_mac_hash_func hash;
	gnutls_mac_output_func output;
	gnutls_mac_deinit_func deinit;
	gnutls_mac_fast_func fast;
	gnutls_mac_copy_func copy;

	/* Not needed for registered on run-time. Only included
	 * should define it. */
	int (*exists) (gnutls_mac_algorithm_t);
} gnutls_crypto_mac_st;

typedef struct {
	gnutls_digest_init_func init;
	gnutls_digest_hash_func hash;
	gnutls_digest_output_func output;
	gnutls_digest_deinit_func deinit;
	gnutls_digest_fast_func fast;
	gnutls_digest_copy_func copy;

	/* Not needed for registered on run-time. Only included
	 * should define it. */
	int (*exists) (gnutls_digest_algorithm_t);
} gnutls_crypto_digest_st;

typedef struct {
	int (*hkdf_extract) (gnutls_mac_algorithm_t,
			     const void *key, size_t keysize,
			     const void *salt, size_t saltsize,
			     void *output);
	int (*hkdf_expand) (gnutls_mac_algorithm_t,
			    const void *key, size_t keysize,
			    const void *info, size_t infosize,
			    void *output, size_t length);
	int (*pbkdf2) (gnutls_mac_algorithm_t,
		       const void *key, size_t keysize,
		       const void *salt, size_t saltsize,
		       unsigned iter_count,
		       void *output, size_t length);
} gnutls_crypto_kdf_st;

typedef struct gnutls_crypto_rnd {
	int (*init) (void **ctx); /* called prior to first usage of randomness */
	int (*rnd) (void *ctx, int level, void *data, size_t datasize);
	void (*rnd_refresh) (void *ctx);
	void (*deinit) (void *ctx);
	int (*self_test) (void); /* this should not require rng initialization */
} gnutls_crypto_rnd_st;

typedef void *bigint_t;

/**
 * gnutls_bigint_format_t:
 * @GNUTLS_MPI_FORMAT_USG: Raw unsigned integer format.
 * @GNUTLS_MPI_FORMAT_STD: Raw signed integer format, always a leading
 *   zero when positive.
 *
 * Enumeration of different bignum integer encoding formats.
 */
typedef enum {
	/* raw unsigned integer format */
	GNUTLS_MPI_FORMAT_USG = 0,
	/* raw signed integer format - always a leading zero when positive */
	GNUTLS_MPI_FORMAT_STD = 1,
	/* raw unsigned integer format, little endian format */
	GNUTLS_MPI_FORMAT_ULE = 2
} gnutls_bigint_format_t;

/* Multi precision integer arithmetic */
typedef struct gnutls_crypto_bigint {
	int (*bigint_init) (bigint_t*);
	int (*bigint_init_multi) (bigint_t*, ...);
	void (*bigint_release) (bigint_t n);
	void (*bigint_clear) (bigint_t n);	/* zeros the int */
	/* 0 for equality, > 0 for m1>m2, < 0 for m1<m2 */
	int (*bigint_cmp) (const bigint_t m1, const bigint_t m2);
	/* as bigint_cmp */
	int (*bigint_cmp_ui) (const bigint_t m1, unsigned long m2);
	/* r = a % b */
	int (*bigint_modm) (bigint_t r, const bigint_t a, const bigint_t b);
	/* a = b -> ret == a */
	int (*bigint_set) (bigint_t a, const bigint_t b);
	bigint_t (*bigint_copy) (const bigint_t a);
	/* a = b -> ret == a */
	int (*bigint_set_ui) (bigint_t a, unsigned long b);
	unsigned int (*bigint_get_nbits) (const bigint_t a);
	/* w = b ^ e mod m */
	int (*bigint_powm) (bigint_t w, const bigint_t b,
				 const bigint_t e, const bigint_t m);
	/* w = a + b mod m */
	int (*bigint_addm) (bigint_t w, const bigint_t a,
				 const bigint_t b, const bigint_t m);
	/* w = a - b mod m */
	int (*bigint_subm) (bigint_t w, const bigint_t a,
				 const bigint_t b, const bigint_t m);
	/* w = a * b mod m */
	int (*bigint_mulm) (bigint_t w, const bigint_t a,
				 const bigint_t b, const bigint_t m);
	/* w = a + b */ int (*bigint_add) (bigint_t w,
					       const bigint_t a,
					       const bigint_t b);
	/* w = a - b */ int (*bigint_sub) (bigint_t w,
					       const bigint_t a,
					       const bigint_t b);
	/* w = a * b */
	int (*bigint_mul) (bigint_t w, const bigint_t a,
				const bigint_t b);
	/* w = a + b */
	int (*bigint_add_ui) (bigint_t w, const bigint_t a,
				   unsigned long b);
	/* w = a - b */
	int (*bigint_sub_ui) (bigint_t w, const bigint_t a,
				   unsigned long b);
	/* w = a * b */
	int (*bigint_mul_ui) (bigint_t w, const bigint_t a,
				   unsigned long b);
	/* q = a / b */
	int (*bigint_div) (bigint_t q, const bigint_t a,
				const bigint_t b);
	/* 0 if prime */
	int (*bigint_prime_check) (const bigint_t pp);

	/* reads a bigint from a buffer */
	/* stores a bigint into the buffer.  returns
	 * GNUTLS_E_SHORT_MEMORY_BUFFER if buf_size is not sufficient to
	 * store this integer, and updates the buf_size;
	 */
	int (*bigint_scan) (bigint_t m, const void *buf, size_t buf_size,
				 gnutls_bigint_format_t format);
	int (*bigint_print) (const bigint_t a, void *buf,
			     size_t * buf_size,
			     gnutls_bigint_format_t format);
} gnutls_crypto_bigint_st;

/* Additional information about the public key, filled from
 * SubjectPublicKeyInfo parameters. When there are no parameters,
 * the pk field will be set to GNUTLS_PK_UNKNOWN.
 */
typedef struct gnutls_x509_spki_st {
	/* We can have a key which is of type RSA, but a certificate
	 * of type RSA-PSS; the value here will be the expected value
	 * for signatures (i.e., RSA-PSS) */
	gnutls_pk_algorithm_t pk;

	/* the digest used by RSA-PSS */
	gnutls_digest_algorithm_t rsa_pss_dig;

	/* the size of salt used by RSA-PSS */
	unsigned int salt_size;

	/* if non-zero, the legacy value for PKCS#7 signatures will be
	 * written for RSA signatures. */
	unsigned int legacy;

	/* the digest used by ECDSA/DSA */
	gnutls_digest_algorithm_t dsa_dig;

	/* flags may include GNUTLS_PK_FLAG_REPRODUCIBLE for
	 * deterministic ECDSA/DSA */
	unsigned int flags;
} gnutls_x509_spki_st;

#define GNUTLS_MAX_PK_PARAMS 16

typedef struct {
	bigint_t params[GNUTLS_MAX_PK_PARAMS];
	unsigned int params_nr;	/* the number of parameters */
	unsigned int pkflags; /* gnutls_pk_flag_t */
	unsigned int qbits; /* GNUTLS_PK_DH */
	gnutls_ecc_curve_t curve; /* GNUTLS_PK_EC, GNUTLS_PK_ED25519, GNUTLS_PK_GOST* */
	gnutls_group_t dh_group; /* GNUTLS_PK_DH - used by ext/key_share */
	gnutls_gost_paramset_t gost_params; /* GNUTLS_PK_GOST_* */
	gnutls_datum_t raw_pub; /* used by x25519 */
	gnutls_datum_t raw_priv;

	unsigned int seed_size;
	uint8_t seed[MAX_PVP_SEED_SIZE];
	gnutls_digest_algorithm_t palgo;
	/* public key information */
	gnutls_x509_spki_st spki;

	gnutls_pk_algorithm_t algo;
} gnutls_pk_params_st;

/**
 * gnutls_pk_flag_t:
 * @GNUTLS_PK_FLAG_NONE: No flag.
 *
 * Enumeration of public-key flag.
 */
typedef enum {
	GNUTLS_PK_FLAG_NONE = 0,
	GNUTLS_PK_FLAG_PROVABLE = 1,
	GNUTLS_PK_FLAG_REPRODUCIBLE = 2
} gnutls_pk_flag_t;

#define FIX_SIGN_PARAMS(params, flags, dig) do {		\
	if ((flags) & GNUTLS_PRIVKEY_FLAG_REPRODUCIBLE) {	\
		(params).flags |= GNUTLS_PK_FLAG_REPRODUCIBLE;	\
		(params).dsa_dig = (dig);			\
	}							\
} while (0)

void gnutls_pk_params_release(gnutls_pk_params_st * p);
void gnutls_pk_params_clear(gnutls_pk_params_st * p);
void gnutls_pk_params_init(gnutls_pk_params_st * p);


#define MAX_PUBLIC_PARAMS_SIZE 4	/* ok for RSA and DSA */

/* parameters should not be larger than this limit */
#define DSA_PUBLIC_PARAMS 4
#define DH_PUBLIC_PARAMS 4
#define RSA_PUBLIC_PARAMS 2
#define ECC_PUBLIC_PARAMS 2
#define GOST_PUBLIC_PARAMS 2


#define MAX_PRIV_PARAMS_SIZE GNUTLS_MAX_PK_PARAMS	/* ok for RSA and DSA */

/* parameters should not be larger than this limit */
#define DSA_PRIVATE_PARAMS 5
#define DH_PRIVATE_PARAMS 5
#define RSA_PRIVATE_PARAMS 8
#define ECC_PRIVATE_PARAMS 3
#define GOST_PRIVATE_PARAMS 3

#if MAX_PRIV_PARAMS_SIZE - RSA_PRIVATE_PARAMS < 0
#error INCREASE MAX_PRIV_PARAMS
#endif

#if MAX_PRIV_PARAMS_SIZE - ECC_PRIVATE_PARAMS < 0
#error INCREASE MAX_PRIV_PARAMS
#endif

#if MAX_PRIV_PARAMS_SIZE - GOST_PRIVATE_PARAMS < 0
#error INCREASE MAX_PRIV_PARAMS
#endif

#if MAX_PRIV_PARAMS_SIZE - DSA_PRIVATE_PARAMS < 0
#error INCREASE MAX_PRIV_PARAMS
#endif


/* params are:
 * RSA:
 *  [0] is modulus
 *  [1] is public exponent
 *  [2] is private exponent (private key only)
 *  [3] is prime1 (p) (private key only)
 *  [4] is prime2 (q) (private key only)
 *  [5] is coefficient (u == inverse of p mod q) (private key only)
 *  [6] e1 == d mod (p-1)
 *  [7] e2 == d mod (q-1)
 *
 *  note that for libgcrypt that does not use the inverse of q mod p,
 *  we need to perform conversions using fixup_params().
 *
 * DSA:
 *  [0] is p
 *  [1] is q
 *  [2] is g
 *  [3] is y (public key)
 *  [4] is x (private key only)
 *
 * DH: as DSA
 *
 * ECC:
 *  [0] is prime
 *  [1] is order
 *  [2] is A
 *  [3] is B
 *  [4] is Gx
 *  [5] is Gy
 *  [6] is x
 *  [7] is y
 *  [8] is k (private key)
 */

#define ECC_X 0
#define ECC_Y 1
#define ECC_K 2

#define GOST_X 0
#define GOST_Y 1
#define GOST_K 2

#define DSA_P 0
#define DSA_Q 1
#define DSA_G 2
#define DSA_Y 3
#define DSA_X 4

#define DH_P 0
#define DH_Q 1
#define DH_G 2
#define DH_Y 3
#define DH_X 4

#define RSA_MODULUS 0
#define RSA_PUB 1
#define RSA_PRIV 2
#define RSA_PRIME1 3
#define RSA_PRIME2 4
#define RSA_COEF 5
#define RSA_E1 6
#define RSA_E2 7

/**
 * gnutls_direction_t:
 * @GNUTLS_IMPORT: Import direction.
 * @GNUTLS_EXPORT: Export direction.
 *
 * Enumeration of different directions.
 */
typedef enum {
	GNUTLS_IMPORT = 0,
	GNUTLS_EXPORT = 1
} gnutls_direction_t;

/* Public key algorithms */
typedef struct gnutls_crypto_pk {
	/* The params structure should contain the private or public key
	 * parameters, depending on the operation */
	int (*encrypt) (gnutls_pk_algorithm_t, gnutls_datum_t * ciphertext,
			const gnutls_datum_t * plaintext,
			const gnutls_pk_params_st * pub);
	int (*decrypt) (gnutls_pk_algorithm_t,
                        gnutls_datum_t * plaintext,
			const gnutls_datum_t * ciphertext,
			const gnutls_pk_params_st * priv);
	int (*decrypt2) (gnutls_pk_algorithm_t,
			 const gnutls_datum_t * ciphertext,
                         unsigned char * plaintext,
                         size_t paintext_size,
			 const gnutls_pk_params_st * priv);
	int (*sign) (gnutls_pk_algorithm_t, gnutls_datum_t * signature,
		     const gnutls_datum_t * data,
		     const gnutls_pk_params_st *priv,
		     const gnutls_x509_spki_st *sign);
	int (*verify) (gnutls_pk_algorithm_t, const gnutls_datum_t * data,
		       const gnutls_datum_t * sig,
		       const gnutls_pk_params_st *pub,
		       const gnutls_x509_spki_st *sign);
	/* sanity checks the public key parameters */
	int (*verify_priv_params) (gnutls_pk_algorithm_t,
			      const gnutls_pk_params_st * priv);
	int (*verify_pub_params) (gnutls_pk_algorithm_t,
			      const gnutls_pk_params_st * pub);
	int (*generate_keys) (gnutls_pk_algorithm_t, unsigned int nbits,
			 gnutls_pk_params_st *, unsigned ephemeral);
	int (*generate_params) (gnutls_pk_algorithm_t, unsigned int nbits,
			 gnutls_pk_params_st *);
	/* this function should convert params to ones suitable
	 * for the above functions
	 */
	int (*pk_fixup_private_params) (gnutls_pk_algorithm_t,
					gnutls_direction_t,
					gnutls_pk_params_st *);
#define PK_DERIVE_TLS13 1
	int (*derive) (gnutls_pk_algorithm_t, gnutls_datum_t * out,
		       const gnutls_pk_params_st * priv,
		       const gnutls_pk_params_st * pub,
		       const gnutls_datum_t *nonce,
		       unsigned int flags);

	int (*curve_exists) (gnutls_ecc_curve_t);	/* true/false */
} gnutls_crypto_pk_st;

/* priority: infinity for backend algorithms, 90 for kernel
   algorithms, lowest wins
 */
int gnutls_crypto_single_cipher_register(gnutls_cipher_algorithm_t
					 algorithm, int priority,
					 const gnutls_crypto_single_cipher_st *s,
					 int free_s);
int gnutls_crypto_single_mac_register(gnutls_mac_algorithm_t algorithm,
				      int priority,
				      const gnutls_crypto_single_mac_st *
				      s, int free_s);
int gnutls_crypto_single_digest_register(gnutls_digest_algorithm_t
					 algorithm, int priority,
					 const
					 gnutls_crypto_single_digest_st *
					 s, int free_s);

int gnutls_crypto_rnd_register(int priority,
			       const gnutls_crypto_rnd_st * s);
int gnutls_crypto_pk_register(int priority, const gnutls_crypto_pk_st * s);
int gnutls_crypto_bigint_register(int priority,
				  const gnutls_crypto_bigint_st * s);

/* Provided by crypto-backend */
int
_gnutls_prf_raw(gnutls_mac_algorithm_t mac,
		size_t master_size, const void *master,
		size_t label_size, const char *label,
		size_t seed_size, const uint8_t *seed, size_t outsize,
		char *out);

int _gnutls_gost_key_wrap(gnutls_gost_paramset_t gost_params,
			  const gnutls_datum_t *kek,
			  const gnutls_datum_t *ukm,
			  const gnutls_datum_t *cek,
			  gnutls_datum_t *enc,
			  gnutls_datum_t *imit);

int _gnutls_gost_key_unwrap(gnutls_gost_paramset_t gost_params,
			    const gnutls_datum_t *kek,
			    const gnutls_datum_t *ukm,
			    const gnutls_datum_t *enc,
			    const gnutls_datum_t *imit,
			    gnutls_datum_t *cek);

#endif /* GNUTLS_LIB_CRYPTO_BACKEND_H */
