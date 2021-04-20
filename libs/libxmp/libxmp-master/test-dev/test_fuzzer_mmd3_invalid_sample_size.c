#include "test.h"

/* This input caused hangs in the MMD2/MMD3 loader due to attempting to
 * initialize several very large samples.
 */

TEST(test_fuzzer_mmd3_invalid_sample_size)
{
	xmp_context opaque;
	struct xmp_module_info info;
	int ret;

	opaque = xmp_create_context();
	ret = xmp_load_module(opaque, "data/f/load_mmd3_invalid_sample_size.med");
	fail_unless(ret == -XMP_ERROR_LOAD, "module load");

	xmp_free_context(opaque);
}
END_TEST
