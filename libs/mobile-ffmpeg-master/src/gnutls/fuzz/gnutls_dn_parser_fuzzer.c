/*
# Copyright 2016 Nikos Mavrogiannopoulos
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
################################################################################
*/

#include <assert.h>
#include <stdint.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "fuzzer.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	gnutls_datum_t out, raw;
	gnutls_x509_dn_t dn;
	int ret;

	raw.data = (unsigned char *)data;
	raw.size = size;

	ret = gnutls_x509_dn_init(&dn);
	assert(ret >= 0);

	ret = gnutls_x509_dn_import(dn, &raw);
	if (ret < 0)
		goto cleanup;

	/* If properly loaded, try to re-export in string */
	ret = gnutls_x509_dn_get_str(dn, &out);
	if (ret < 0) {
		goto cleanup;
	}

	gnutls_free(out.data);

cleanup:
	gnutls_x509_dn_deinit(dn);
	return 0;
}
