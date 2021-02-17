/*
 * Copyright (C) 2013 Nikos Mavrogiannopoulos
 * Copyright (C) 2017 Red Hat, Inc.
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

#include "gnutls_int.h"
#include "auth.h"
#include "errors.h"
#include "num.h"
#include <ext/alpn.h>

static int _gnutls_alpn_recv_params(gnutls_session_t session,
				    const uint8_t * data,
				    size_t data_size);
static int _gnutls_alpn_send_params(gnutls_session_t session,
				    gnutls_buffer_st * extdata);

static void _gnutls_alpn_deinit_data(gnutls_ext_priv_data_t priv);


const hello_ext_entry_st ext_mod_alpn = {
	.name = "ALPN",
	.tls_id = 16,
	.gid = GNUTLS_EXTENSION_ALPN,
	/* this extension must be parsed even on resumption */
	.client_parse_point = GNUTLS_EXT_MANDATORY,
	.server_parse_point = GNUTLS_EXT_MANDATORY,
	.validity = GNUTLS_EXT_FLAG_TLS | GNUTLS_EXT_FLAG_DTLS |
		    GNUTLS_EXT_FLAG_CLIENT_HELLO | GNUTLS_EXT_FLAG_EE |
		    GNUTLS_EXT_FLAG_TLS12_SERVER_HELLO,
	.recv_func = _gnutls_alpn_recv_params,
	.send_func = _gnutls_alpn_send_params,
	.deinit_func = _gnutls_alpn_deinit_data,
	.cannot_be_overriden = 1
};

static int
_gnutls_alpn_recv_params(gnutls_session_t session,
			 const uint8_t * data, size_t data_size)
{
	unsigned int i;
	int ret;
	const uint8_t *p = data;
	unsigned len1, len;
	alpn_ext_st *priv;
	gnutls_ext_priv_data_t epriv;
	int selected_protocol_index;

	ret =
	    _gnutls_hello_ext_get_priv(session, GNUTLS_EXTENSION_ALPN,
					 &epriv);
	if (ret < 0)
		return 0;

	priv = epriv;

	DECR_LENGTH_RET(data_size, 2, GNUTLS_E_UNEXPECTED_PACKET_LENGTH);
	len = _gnutls_read_uint16(p);
	p += 2;

	if (len == 0 || len > (size_t)data_size)
		return
		    gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	if (session->security_parameters.entity == GNUTLS_SERVER) {
		selected_protocol_index = MAX_ALPN_PROTOCOLS+1;

		while (data_size > 0) {
			DECR_LENGTH_RET(data_size, 1, GNUTLS_E_UNEXPECTED_PACKET_LENGTH);
			len1 = *p;
			p += 1;
			DECR_LENGTH_RET(data_size, len1, GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

			if (len1 == 0)
				return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

			for (i = 0; i < priv->size; i++) {
				if (priv->protocol_size[i] == len1
				    && memcmp(p, priv->protocols[i],
					      len1) == 0) {

					if (priv->flags & GNUTLS_ALPN_SERVER_PRECEDENCE) {
						if (selected_protocol_index > (int)i) {
							selected_protocol_index = i;
							priv->selected_protocol =
							    priv->protocols[i];
							priv->selected_protocol_size =
							    priv->protocol_size[i];
							break;
						}
					} else {
						priv->selected_protocol =
						    priv->protocols[i];
						priv->selected_protocol_size =
						    priv->protocol_size[i];
						return 0;
					}
				}
			}
			p += len1;
		}
	} else {
		DECR_LENGTH_RET(data_size, 1, GNUTLS_E_UNEXPECTED_PACKET_LENGTH);
		len1 = *p;
		p += 1;
		DECR_LENGTH_RET(data_size, len1, GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

		for (i = 0; i < priv->size; i++) {
			if (priv->protocol_size[i] == len1
			    && memcmp(p, priv->protocols[i], len1) == 0) {
				priv->selected_protocol =
				    priv->protocols[i];
				priv->selected_protocol_size =
				    priv->protocol_size[i];
				break;
			}
		}
		/*p += len1;*/
	}

	if (priv->selected_protocol == NULL
	    && (priv->flags & GNUTLS_ALPN_MAND))
		return gnutls_assert_val(GNUTLS_E_NO_APPLICATION_PROTOCOL);

	return 0;
}

static int
_gnutls_alpn_send_params(gnutls_session_t session,
			 gnutls_buffer_st * extdata)
{
	unsigned i;
	int total_size = 0, ret;
	alpn_ext_st *priv;
	gnutls_ext_priv_data_t epriv;

	ret =
	    _gnutls_hello_ext_get_priv(session, GNUTLS_EXTENSION_ALPN,
					 &epriv);
	if (ret < 0)
		return 0;

	priv = epriv;

	if (priv->size == 0)
		return 0;

	if (session->security_parameters.entity == GNUTLS_SERVER) {
		if (priv->selected_protocol_size == 0)
			return 0;

		ret =
		    _gnutls_buffer_append_prefix(extdata, 16,
						 priv->
						 selected_protocol_size +
						 1);
		if (ret < 0)
			return gnutls_assert_val(ret);

		total_size += 2;

		ret =
		    _gnutls_buffer_append_data_prefix(extdata, 8,
						      priv->
						      selected_protocol,
						      priv->
						      selected_protocol_size);
		if (ret < 0)
			return gnutls_assert_val(ret);

		total_size += 1 + priv->selected_protocol_size;
	} else {
		int t = 0;
		for (i = 0; i < priv->size; i++)
			t += priv->protocol_size[i] + 1;

		ret = _gnutls_buffer_append_prefix(extdata, 16, t);
		if (ret < 0)
			return gnutls_assert_val(ret);

		total_size += 2;

		for (i = 0; i < priv->size; i++) {
			ret =
			    _gnutls_buffer_append_data_prefix(extdata, 8,
							      priv->
							      protocols[i],
							      priv->
							      protocol_size
							      [i]);
			if (ret < 0)
				return gnutls_assert_val(ret);

			total_size += 1 + priv->protocol_size[i];
		}
	}

	return total_size;
}

/**
 * gnutls_alpn_get_selected_protocol:
 * @session: is a #gnutls_session_t type.
 * @protocol: will hold the protocol name
 *
 * This function allows you to get the negotiated protocol name. The
 * returned protocol should be treated as opaque, constant value and
 * only valid during the session life.
 *
 * The selected protocol is the first supported by the list sent
 * by the client.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.
 *
 * Since 3.2.0
 **/
int
gnutls_alpn_get_selected_protocol(gnutls_session_t session,
				  gnutls_datum_t * protocol)
{
	alpn_ext_st *priv;
	int ret;
	gnutls_ext_priv_data_t epriv;

	ret =
	    _gnutls_hello_ext_get_priv(session, GNUTLS_EXTENSION_ALPN,
					 &epriv);
	if (ret < 0) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	priv = epriv;

	if (priv->selected_protocol_size == 0)
		return
		    gnutls_assert_val
		    (GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	protocol->data = priv->selected_protocol;
	protocol->size = priv->selected_protocol_size;

	return 0;
}

/**
 * gnutls_alpn_set_protocols:
 * @session: is a #gnutls_session_t type.
 * @protocols: is the protocol names to add.
 * @protocols_size: the number of protocols to add.
 * @flags: zero or a sequence of %gnutls_alpn_flags_t
 *
 * This function is to be used by both clients and servers, to declare
 * the supported ALPN protocols, which are used during negotiation with peer.
 *
 * See %gnutls_alpn_flags_t description for the documentation of available
 * flags.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.
 *
 * Since 3.2.0
 **/
int
gnutls_alpn_set_protocols(gnutls_session_t session,
			  const gnutls_datum_t * protocols,
			  unsigned protocols_size, unsigned int flags)
{
	int ret;
	alpn_ext_st *priv;
	gnutls_ext_priv_data_t epriv;
	unsigned i;

	ret =
	    _gnutls_hello_ext_get_priv(session, GNUTLS_EXTENSION_ALPN,
					 &epriv);
	if (ret < 0) {
		priv = gnutls_calloc(1, sizeof(*priv));
		if (priv == NULL) {
			gnutls_assert();
			return GNUTLS_E_MEMORY_ERROR;
		}
		epriv = priv;
		_gnutls_hello_ext_set_priv(session,
					     GNUTLS_EXTENSION_ALPN, epriv);
	} else
		priv = epriv;

	if (protocols_size > MAX_ALPN_PROTOCOLS)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	for (i = 0; i < protocols_size; i++) {
		if (protocols[i].size >= MAX_ALPN_PROTOCOL_NAME)
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

		memcpy(priv->protocols[i], protocols[i].data,
		       protocols[i].size);
		priv->protocol_size[i] = protocols[i].size;
		priv->size++;
	}
	priv->flags = flags;

	return 0;
}


static void _gnutls_alpn_deinit_data(gnutls_ext_priv_data_t priv)
{
	gnutls_free(priv);
}
