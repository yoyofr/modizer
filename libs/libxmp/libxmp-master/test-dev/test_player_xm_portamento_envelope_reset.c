#include "test.h"

/* When a tone portamento effect is performed, we expect it to always
 * reset the envelope if the previous envelope already ended. If it
 * still didn't end, reset envelope in XM (and IT compatible GXX mode)
 * but not in standard IT mode
 */
TEST(test_player_xm_portamento_envelope_reset)
{
	compare_mixer_data(
		"data/xm_portamento_envelope_reset.xm",
		"data/xm_portamento_envelope_reset.data");
}
END_TEST
