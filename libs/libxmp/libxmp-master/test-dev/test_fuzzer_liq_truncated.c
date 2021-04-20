#include "test.h"

/* These inputs caused uninitialized reads in the Liquid Tracker
 * loader due to not checking for EOF when loading instruments.
 */

TEST(test_fuzzer_liq_truncated)
{
	xmp_context opaque;
	struct xmp_module_info info;
	int ret;

	opaque = xmp_create_context();
	ret = xmp_load_module(opaque, "data/f/load_liq_truncated.liq");
	fail_unless(ret == -XMP_ERROR_LOAD, "module load");

	ret = xmp_load_module(opaque, "data/f/load_liq_truncated2.liq");
	fail_unless(ret == -XMP_ERROR_LOAD, "module load");

	xmp_free_context(opaque);
}
END_TEST
