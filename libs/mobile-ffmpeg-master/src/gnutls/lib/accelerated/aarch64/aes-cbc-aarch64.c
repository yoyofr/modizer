/*
 * Copyright (C) 2011-2016 Free Software Foundation, Inc.
 * Copyright (C) 2016-2018 Red Hat, Inc.
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
 * using aarch64 instruction set. 
 */

#include "errors.h"
#include "gnutls_int.h"
#include <gnutls/crypto.h>
#include "errors.h"
#include <aes-aarch64.h>
#include <aarch64-common.h>

struct aes_ctx {
	AES_KEY expanded_key;
	uint8_t iv[16];
	int enc;
};

static int
aes_cipher_init(gnutls_cipher_algorithm_t algorithm, void **_ctx, int enc)
{
	/* we use key size to distinguish */
	if (algorithm != GNUTLS_CIPHER_AES_128_CBC
	    && algorithm != GNUTLS_CIPHER_AES_192_CBC
	    && algorithm != GNUTLS_CIPHER_AES_256_CBC)
		return GNUTLS_E_INVALID_REQUEST;

	*_ctx = gnutls_calloc(1, sizeof(struct aes_ctx));
	if (*_ctx == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	((struct aes_ctx *) (*_ctx))->enc = enc;

	return 0;
}

static int
aes_aarch64_cipher_setkey(void *_ctx, const void *userkey, size_t keysize)
{
	struct aes_ctx *ctx = _ctx;
	int ret;

	CHECK_AES_KEYSIZE(keysize);

	if (ctx->enc)
		ret =
		    aes_v8_set_encrypt_key(userkey, keysize * 8,
					  ALIGN16(&ctx->expanded_key));
	else
		ret =
		    aes_v8_set_decrypt_key(userkey, keysize * 8,
					  ALIGN16(&ctx->expanded_key));

	if (ret != 0)
		return gnutls_assert_val(GNUTLS_E_ENCRYPTION_FAILED);

	return 0;
}

static int
aes_aarch64_encrypt(void *_ctx, const void *src, size_t src_size,
	    void *dst, size_t dst_size)
{
	struct aes_ctx *ctx = _ctx;

	if (unlikely(src_size % 16 != 0))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	aes_v8_cbc_encrypt(src, dst, src_size, ALIGN16(&ctx->expanded_key),
			  ctx->iv, 1);
	return 0;
}

static int
aes_aarch64_decrypt(void *_ctx, const void *src, size_t src_size,
	    void *dst, size_t dst_size)
{
	struct aes_ctx *ctx = _ctx;

	if (unlikely(src_size % 16 != 0))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	aes_v8_cbc_encrypt(src, dst, src_size, ALIGN16(&ctx->expanded_key),
			  ctx->iv, 0);

	return 0;
}

static int aes_setiv(void *_ctx, const void *iv, size_t iv_size)
{
	struct aes_ctx *ctx = _ctx;

	if (iv_size != 16)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	memcpy(ctx->iv, iv, 16);
	return 0;
}

static void aes_deinit(void *_ctx)
{
	struct aes_ctx *ctx = _ctx;

	zeroize_temp_key(ctx, sizeof(*ctx));
	gnutls_free(ctx);
}

const gnutls_crypto_cipher_st _gnutls_aes_cbc_aarch64 = {
	.init = aes_cipher_init,
	.setkey = aes_aarch64_cipher_setkey,
	.setiv = aes_setiv,
	.encrypt = aes_aarch64_encrypt,
	.decrypt = aes_aarch64_decrypt,
	.deinit = aes_deinit,
};

