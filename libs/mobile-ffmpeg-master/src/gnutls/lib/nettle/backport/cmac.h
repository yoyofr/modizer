/* backport of cmac.h

   CMAC mode, as specified in RFC4493

   Copyright (C) 2017 Red Hat, Inc.

   Contributed by Nikos Mavrogiannopoulos

   This file is part of GNU Nettle.

   GNU Nettle is free software: you can redistribute it and/or
   modify it under the terms of either:

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at your
       option) any later version.

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at your
       option) any later version.

   or both in parallel, as here.

   GNU Nettle is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see https://www.gnu.org/licenses/.
*/

#ifndef GNUTLS_LIB_NETTLE_BACKPORT_CMAC_H
#define GNUTLS_LIB_NETTLE_BACKPORT_CMAC_H

#include <nettle/nettle-types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CMAC128_DIGEST_SIZE 16

#define cmac128_set_key _gnutls_backport_nettle_cmac128_set_key
#define cmac128_update _gnutls_backport_nettle_cmac128_update
#define cmac128_digest _gnutls_backport_nettle_cmac128_digest
#define cmac_aes128_set_key _gnutls_backport_nettle_cmac_aes128_set_key
#define cmac_aes128_update _gnutls_backport_nettle_cmac_aes128_update
#define cmac_aes128_digest _gnutls_backport_nettle_cmac_aes128_digest
#define cmac_aes256_set_key _gnutls_backport_nettle_cmac_aes256_set_key
#define cmac_aes256_update _gnutls_backport_nettle_cmac_aes256_update
#define cmac_aes256_digest _gnutls_backport_nettle_cmac_aes256_digest

struct cmac128_ctx
{
  /* Key */
  union nettle_block16 K1;
  union nettle_block16 K2;

  /* MAC state */
  union nettle_block16 X;

  /* Block buffer */
  union nettle_block16 block;
  size_t index;
};

void
cmac128_set_key(struct cmac128_ctx *ctx, const void *cipher,
		nettle_cipher_func *encrypt);
void
cmac128_update(struct cmac128_ctx *ctx, const void *cipher,
	       nettle_cipher_func *encrypt,
	       size_t msg_len, const uint8_t *msg);
void
cmac128_digest(struct cmac128_ctx *ctx, const void *cipher,
	       nettle_cipher_func *encrypt,
	       unsigned length,
	       uint8_t *digest);


#define CMAC128_CTX(type) \
  { struct cmac128_ctx ctx; type cipher; }

/* NOTE: Avoid using NULL, as we don't include anything defining it. */
#define CMAC128_SET_KEY(self, set_key, encrypt, cmac_key)	\
  do {								\
    (set_key)(&(self)->cipher, (cmac_key));			\
    if (0) (encrypt)(&(self)->cipher, ~(size_t) 0,		\
		     (uint8_t *) 0, (const uint8_t *) 0);	\
    cmac128_set_key(&(self)->ctx, &(self)->cipher,		\
		(nettle_cipher_func *) (encrypt));		\
  } while (0)

#define CMAC128_UPDATE(self, encrypt, length, src)		\
  cmac128_update(&(self)->ctx, &(self)->cipher,			\
	      (nettle_cipher_func *)encrypt, (length), (src))

#define CMAC128_DIGEST(self, encrypt, length, digest)		\
  (0 ? (encrypt)(&(self)->cipher, ~(size_t) 0,			\
		 (uint8_t *) 0, (const uint8_t *) 0)		\
     : cmac128_digest(&(self)->ctx, &(self)->cipher,		\
		  (nettle_cipher_func *) (encrypt),		\
		  (length), (digest)))

struct cmac_aes128_ctx CMAC128_CTX(struct aes128_ctx);

void
cmac_aes128_set_key(struct cmac_aes128_ctx *ctx, const uint8_t *key);

void
cmac_aes128_update(struct cmac_aes128_ctx *ctx,
		   size_t length, const uint8_t *data);

void
cmac_aes128_digest(struct cmac_aes128_ctx *ctx,
		   size_t length, uint8_t *digest);

struct cmac_aes256_ctx CMAC128_CTX(struct aes256_ctx);

void
cmac_aes256_set_key(struct cmac_aes256_ctx *ctx, const uint8_t *key);

void
cmac_aes256_update(struct cmac_aes256_ctx *ctx,
		   size_t length, const uint8_t *data);

void
cmac_aes256_digest(struct cmac_aes256_ctx *ctx,
		   size_t length, uint8_t *digest);

#ifdef __cplusplus
}
#endif

#endif /* GNUTLS_LIB_NETTLE_BACKPORT_CMAC_H */
