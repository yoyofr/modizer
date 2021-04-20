#include "test.h"

/* This input caused uninitialized reads in the LZW depacker
 * when called from the Digital Symphony loader due to using
 * the uncompressed size as the compressed size bound. This
 * issue presumably could also have occured with valid modules.
 */

TEST(test_fuzzer_sym_truncated_lzw)
{
	xmp_context opaque;
	struct xmp_module_info info;
	int ret;

	opaque = xmp_create_context();
	ret = xmp_load_module(opaque, "data/f/load_sym_truncated_lzw.sym");
	fail_unless(ret == 0, "module load");

	xmp_free_context(opaque);
}
END_TEST
