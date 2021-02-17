/*
 * Copyright (C) 2016 Red Hat, Inc.
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

/*
 * The following code is an implementation of the AES-128-CBC cipher
 * using intel's AES instruction set. 
 */

#include "errors.h"
#include "gnutls_int.h"
#include <gnutls/crypto.h>
#include "errors.h"
#ifdef HAVE_LIBNETTLE
# include <nettle/aes.h>		/* for key generation in 192 and 256 bits */
# include "sha-aarch64.h"
# include "aes-aarch64.h"
#endif
#include "aarch64-common.h"

#ifdef HAVE_GETAUXVAL
# include <sys/auxv.h>
# ifdef AT_HWCAP
#  define USE_AUXVAL
# endif
#endif

#if defined(__GNUC__)
__attribute__((visibility("hidden")))
#elif defined(__SUNPRO_C)
__hidden
#endif
unsigned int _gnutls_arm_cpuid_s = 0;


/* Our internal bit-string for cpu capabilities. Should be set
 * in GNUTLS_CPUID_OVERRIDE */
#define EMPTY_SET 1

static void capabilities_to_cpuid(unsigned capabilities)
{
	_gnutls_arm_cpuid_s = 0;

	if (capabilities & EMPTY_SET)
		return;

	_gnutls_arm_cpuid_s |= capabilities;
}

/* Correspond to asm/hwcap.h for aarch64 */
#ifdef USE_AUXVAL
#define HWCAP_ASIMD  (1 << 1)
#define HWCAP_AES    (1 << 3)
#define HWCAP_PMULL  (1 << 4)
#define HWCAP_SHA1   (1 << 5)
#define HWCAP_SHA2   (1 << 6)
#define HWCAP_SHA3   (1 << 17)
#define HWCAP_SHA512 (1 << 21)
#endif

static void discover_caps(unsigned int *caps)
{
#ifdef USE_AUXVAL
	unsigned long c;

	c = getauxval(AT_HWCAP);
	if (c & HWCAP_ASIMD)
		*caps |= ARMV7_NEON;
	if (c & HWCAP_AES)
		*caps |= ARMV8_AES;
	if (c & HWCAP_PMULL)
		*caps |= ARMV8_PMULL;
	if (c & HWCAP_SHA1)
		*caps |= ARMV8_SHA1;
	if (c & HWCAP_SHA2)
		*caps |= ARMV8_SHA256;
	if (c & HWCAP_SHA512)
		*caps |= ARMV8_SHA512;
#endif
}

static
void _register_aarch64_crypto(unsigned capabilities)
{
	int ret;

	if (capabilities == 0) {
		discover_caps(&_gnutls_arm_cpuid_s);
	} else {
		capabilities_to_cpuid(capabilities);
	}

	if (_gnutls_arm_cpuid_s & ARMV8_SHA1) {
		_gnutls_debug_log("Aarch64 SHA1 was detected\n");

		ret =
		    gnutls_crypto_single_digest_register(GNUTLS_DIG_SHA1,
							 80,
							 &_gnutls_sha_aarch64, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_mac_register
		    (GNUTLS_MAC_SHA1, 80, &_gnutls_hmac_sha_aarch64, 0);
		if (ret < 0) {
			gnutls_assert();
		}
	}

	if (_gnutls_arm_cpuid_s & ARMV8_SHA256) {
		_gnutls_debug_log("Aarch64 SHA2 was detected\n");

		ret =
		    gnutls_crypto_single_digest_register(GNUTLS_DIG_SHA224,
							 80,
							 &_gnutls_sha_aarch64, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_mac_register
		    (GNUTLS_MAC_SHA224, 80, &_gnutls_hmac_sha_aarch64, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_digest_register(GNUTLS_DIG_SHA256,
							 80,
							 &_gnutls_sha_aarch64, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_mac_register
		    (GNUTLS_MAC_SHA256, 80, &_gnutls_hmac_sha_aarch64, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_digest_register(GNUTLS_DIG_SHA384,
							 80,
							 &_gnutls_sha_aarch64, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_mac_register
		    (GNUTLS_MAC_SHA384, 80, &_gnutls_hmac_sha_aarch64, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_digest_register(GNUTLS_DIG_SHA512,
							 80,
							 &_gnutls_sha_aarch64, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_mac_register
		    (GNUTLS_MAC_SHA512, 80, &_gnutls_hmac_sha_aarch64, 0);
		if (ret < 0) {
			gnutls_assert();
		}
	}

	if (_gnutls_arm_cpuid_s & ARMV8_AES) {
		_gnutls_debug_log("Aarch64 AES was detected\n");

		if (_gnutls_arm_cpuid_s & ARMV8_PMULL) {
			_gnutls_debug_log("Aarch64 PMULL was detected\n");

			ret =
			    gnutls_crypto_single_cipher_register
			    (GNUTLS_CIPHER_AES_128_GCM, 90,
			     &_gnutls_aes_gcm_aarch64, 0);
			if (ret < 0) {
					gnutls_assert();
				}

			ret =
			    gnutls_crypto_single_cipher_register
			    (GNUTLS_CIPHER_AES_256_GCM, 90,
			     &_gnutls_aes_gcm_aarch64, 0);
			if (ret < 0) {
				gnutls_assert();
			}
		}

		ret =
		    gnutls_crypto_single_cipher_register
		    (GNUTLS_CIPHER_AES_128_CBC, 90, &_gnutls_aes_cbc_aarch64, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_cipher_register
		    (GNUTLS_CIPHER_AES_256_CBC, 90, &_gnutls_aes_cbc_aarch64, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_cipher_register
		    (GNUTLS_CIPHER_AES_128_CCM, 90, &_gnutls_aes_ccm_aarch64, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_cipher_register
		    (GNUTLS_CIPHER_AES_256_CCM, 90, &_gnutls_aes_ccm_aarch64, 0);
		if (ret < 0) {
			gnutls_assert();
		}
	}

	return;
}


void register_aarch64_crypto(void)
{
	unsigned capabilities = 0;
	char *p;
	p = secure_getenv("GNUTLS_CPUID_OVERRIDE");
	if (p) {
		capabilities = strtol(p, NULL, 0);
	}

	_register_aarch64_crypto(capabilities);
}

