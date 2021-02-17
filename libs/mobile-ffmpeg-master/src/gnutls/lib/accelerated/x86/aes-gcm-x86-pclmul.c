/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
 * Copyright (C) 2018 Red Hat, Inc.
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
 * The following code is an implementation of the AES-128-GCM cipher
 * using intel's AES instruction set.
 */

#include "errors.h"
#include "gnutls_int.h"
#include <gnutls/crypto.h>
#include "errors.h"
#include <aes-x86.h>
#include <x86-common.h>
#include <nettle/memxor.h>
#include <byteswap.h>

#define GCM_BLOCK_SIZE 16

/* GCM mode */

typedef struct {
	uint64_t hi, lo;
} u128;

/* This is the gcm128 structure used in openssl. It
 * is compatible with the included assembly code.
 */
struct gcm128_context {
	union {
		uint64_t u[2];
		uint32_t d[4];
		uint8_t c[16];
	} Yi, EKi, EK0, len, Xi, H;
	u128 Htable[16];
};

struct aes_gcm_ctx {
	AES_KEY expanded_key;
	struct gcm128_context gcm;
	unsigned finished;
	unsigned auth_finished;
};

void gcm_init_clmul(u128 Htable[16], const uint64_t Xi[2]);
void gcm_ghash_clmul(uint64_t Xi[2], const u128 Htable[16],
		     const uint8_t * inp, size_t len);
void gcm_gmult_clmul(uint64_t Xi[2], const u128 Htable[16]);

static void aes_gcm_deinit(void *_ctx)
{
	struct aes_gcm_ctx *ctx = _ctx;

	zeroize_temp_key(ctx, sizeof(*ctx));
	gnutls_free(ctx);
}

static int
aes_gcm_cipher_init(gnutls_cipher_algorithm_t algorithm, void **_ctx,
		    int enc)
{
	/* we use key size to distinguish */
	if (algorithm != GNUTLS_CIPHER_AES_128_GCM &&
	    algorithm != GNUTLS_CIPHER_AES_256_GCM)
		return GNUTLS_E_INVALID_REQUEST;

	*_ctx = gnutls_calloc(1, sizeof(struct aes_gcm_ctx));
	if (*_ctx == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	return 0;
}

static int
aes_gcm_cipher_setkey(void *_ctx, const void *userkey, size_t keysize)
{
	struct aes_gcm_ctx *ctx = _ctx;
	int ret;

	CHECK_AES_KEYSIZE(keysize);

	ret =
	    aesni_set_encrypt_key(userkey, keysize * 8,
				  ALIGN16(&ctx->expanded_key));
	if (ret != 0)
		return gnutls_assert_val(GNUTLS_E_ENCRYPTION_FAILED);

	aesni_ecb_encrypt(ctx->gcm.H.c, ctx->gcm.H.c,
			  GCM_BLOCK_SIZE, ALIGN16(&ctx->expanded_key), 1);

	ctx->gcm.H.u[0] = bswap_64(ctx->gcm.H.u[0]);
	ctx->gcm.H.u[1] = bswap_64(ctx->gcm.H.u[1]);

	gcm_init_clmul(ctx->gcm.Htable, ctx->gcm.H.u);

	return 0;
}

static int aes_gcm_setiv(void *_ctx, const void *iv, size_t iv_size)
{
	struct aes_gcm_ctx *ctx = _ctx;

	if (iv_size != GCM_BLOCK_SIZE - 4)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	memset(ctx->gcm.Xi.c, 0, sizeof(ctx->gcm.Xi.c));
	memset(ctx->gcm.len.c, 0, sizeof(ctx->gcm.len.c));

	memcpy(ctx->gcm.Yi.c, iv, GCM_BLOCK_SIZE - 4);
	ctx->gcm.Yi.c[GCM_BLOCK_SIZE - 4] = 0;
	ctx->gcm.Yi.c[GCM_BLOCK_SIZE - 3] = 0;
	ctx->gcm.Yi.c[GCM_BLOCK_SIZE - 2] = 0;
	ctx->gcm.Yi.c[GCM_BLOCK_SIZE - 1] = 1;

	aesni_ecb_encrypt(ctx->gcm.Yi.c, ctx->gcm.EK0.c,
			  GCM_BLOCK_SIZE, ALIGN16(&ctx->expanded_key), 1);
	ctx->gcm.Yi.c[GCM_BLOCK_SIZE - 1] = 2;
	ctx->finished = 0;
	ctx->auth_finished = 0;
	return 0;
}

static void
gcm_ghash(struct aes_gcm_ctx *ctx, const uint8_t * src, size_t src_size)
{
	size_t rest = src_size % GCM_BLOCK_SIZE;
	size_t aligned_size = src_size - rest;

	if (aligned_size > 0)
		gcm_ghash_clmul(ctx->gcm.Xi.u, ctx->gcm.Htable, src,
				aligned_size);

	if (rest > 0) {
		memxor(ctx->gcm.Xi.c, src + aligned_size, rest);
		gcm_gmult_clmul(ctx->gcm.Xi.u, ctx->gcm.Htable);
	}
}

static inline void
ctr_encrypt_last(struct aes_gcm_ctx *ctx, const uint8_t * src,
		 uint8_t * dst, size_t pos, size_t length)
{
	uint8_t tmp[GCM_BLOCK_SIZE];
	uint8_t out[GCM_BLOCK_SIZE];

	memcpy(tmp, &src[pos], length);
	aesni_ctr32_encrypt_blocks(tmp, out, 1,
				   ALIGN16(&ctx->expanded_key),
				   ctx->gcm.Yi.c);

	memcpy(&dst[pos], out, length);

}

static int
aes_gcm_encrypt(void *_ctx, const void *src, size_t src_size,
		void *dst, size_t length)
{
	struct aes_gcm_ctx *ctx = _ctx;
	int blocks = src_size / GCM_BLOCK_SIZE;
	int exp_blocks = blocks * GCM_BLOCK_SIZE;
	int rest = src_size - (exp_blocks);
	uint32_t counter;

	if (unlikely(ctx->finished))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if (blocks > 0) {
		aesni_ctr32_encrypt_blocks(src, dst,
					   blocks,
					   ALIGN16(&ctx->expanded_key),
					   ctx->gcm.Yi.c);

		counter = _gnutls_read_uint32(ctx->gcm.Yi.c + 12);
		counter += blocks;
		_gnutls_write_uint32(counter, ctx->gcm.Yi.c + 12);
	}

	if (rest > 0) {	/* last incomplete block */
		ctr_encrypt_last(ctx, src, dst, exp_blocks, rest);
		ctx->finished = 1;
	}

	gcm_ghash(ctx, dst, src_size);
	ctx->gcm.len.u[1] += src_size;

	return 0;
}

static int
aes_gcm_decrypt(void *_ctx, const void *src, size_t src_size,
		void *dst, size_t dst_size)
{
	struct aes_gcm_ctx *ctx = _ctx;
	int blocks = src_size / GCM_BLOCK_SIZE;
	int exp_blocks = blocks * GCM_BLOCK_SIZE;
	int rest = src_size - (exp_blocks);
	uint32_t counter;

	if (unlikely(ctx->finished))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	gcm_ghash(ctx, src, src_size);
	ctx->gcm.len.u[1] += src_size;

	if (blocks > 0) {
		aesni_ctr32_encrypt_blocks(src, dst,
					   blocks,
					   ALIGN16(&ctx->expanded_key),
					   ctx->gcm.Yi.c);

		counter = _gnutls_read_uint32(ctx->gcm.Yi.c + 12);
		counter += blocks;
		_gnutls_write_uint32(counter, ctx->gcm.Yi.c + 12);
	}

	if (rest > 0) {	/* last incomplete block */
		ctr_encrypt_last(ctx, src, dst, exp_blocks, rest);
		ctx->finished = 1;
	}

	return 0;
}

static int aes_gcm_auth(void *_ctx, const void *src, size_t src_size)
{
	struct aes_gcm_ctx *ctx = _ctx;

	if (unlikely(ctx->auth_finished))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	gcm_ghash(ctx, src, src_size);
	ctx->gcm.len.u[0] += src_size;

	if (src_size % GCM_BLOCK_SIZE != 0)
		ctx->auth_finished = 1;

	return 0;
}


static void aes_gcm_tag(void *_ctx, void *tag, size_t tagsize)
{
	struct aes_gcm_ctx *ctx = _ctx;
	uint8_t buffer[GCM_BLOCK_SIZE];
	uint64_t alen, clen;

	alen = ctx->gcm.len.u[0] * 8;
	clen = ctx->gcm.len.u[1] * 8;

	_gnutls_write_uint64(alen, buffer);
	_gnutls_write_uint64(clen, &buffer[8]);

	gcm_ghash_clmul(ctx->gcm.Xi.u, ctx->gcm.Htable, buffer,
			GCM_BLOCK_SIZE);

	ctx->gcm.Xi.u[0] ^= ctx->gcm.EK0.u[0];
	ctx->gcm.Xi.u[1] ^= ctx->gcm.EK0.u[1];

	memcpy(tag, ctx->gcm.Xi.c, MIN(GCM_BLOCK_SIZE, tagsize));
}

#include "aes-gcm-aead.h"

const gnutls_crypto_cipher_st _gnutls_aes_gcm_pclmul = {
	.init = aes_gcm_cipher_init,
	.setkey = aes_gcm_cipher_setkey,
	.setiv = aes_gcm_setiv,
	.aead_encrypt = aes_gcm_aead_encrypt,
	.aead_decrypt = aes_gcm_aead_decrypt,
	.encrypt = aes_gcm_encrypt,
	.decrypt = aes_gcm_decrypt,
	.deinit = aes_gcm_deinit,
	.tag = aes_gcm_tag,
	.auth = aes_gcm_auth,
};
