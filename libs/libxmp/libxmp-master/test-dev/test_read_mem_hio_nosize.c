#include <sys/types.h>
#include <sys/stat.h>
#include "test.h"
#include "../src/hio.h"

TEST(test_read_mem_hio_nosize)
{
	HIO_HANDLE *h;
	char mem;

	h = hio_open_mem(&mem, -1);
	fail_unless(h == NULL, "hio_open");

	h = hio_open_mem(&mem, 0);
	fail_unless(h == NULL, "hio_open");
}
END_TEST
