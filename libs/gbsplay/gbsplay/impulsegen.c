/*
 * gbsplay is a Gameboy sound player
 *
 * 2003-2020 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "impulsegen.h"
#include "test.h"

static double sinc(double x)
{
	double a = M_PI*x;

	if (a == 0.0) {
		/* Never reached. */
		fprintf(stderr, "sinc(0), should not happen\n");
		return 1.0;
	}

	return sin(a)/a;
}

static double blackman(double n, double m)
{
	return 0.42 - 0.5 * cos(2*n*M_PI/m) + 0.08 * cos(4*n*M_PI/m);
}

int32_t *gen_impulsetab(long w_shift, long n_shift, double cutoff)
{
	long i, j_l;
	long width = 1 << w_shift;
	long n = 1 << n_shift;
	long m = width/2;
	long size = width*n*sizeof(int32_t);
	int32_t *pulsetab = malloc(size);
	int32_t *ptr;

	if (!pulsetab)
		return NULL;

	memset(pulsetab, 0, size);

	/* special-case for j_l == 0 to avoid sinc(0). */
	pulsetab[m-1] = IMPULSE_HEIGHT;
	ptr = &pulsetab[width];

	for (j_l=1; j_l < n; j_l ++) {
		double j = (double)j_l / n;
		int64_t sum=0.0;
		long corr;
		long n = 0;
		double div = IMPULSE_HEIGHT;
		double dcorr = cutoff;

		do {
			sum = 0;
			for (i=-m+1; i<=m; i++) {
				double xd = dcorr*IMPULSE_HEIGHT*sinc((i-j)*cutoff)*blackman(i-j+width/2,width);
				int32_t x = rint(xd);
				sum += x;
			}
			corr = IMPULSE_HEIGHT - sum;
			dcorr *= 1.0 + corr / div;
			div *= 1.3;
		} while ((corr != 0) && (n++ < 20));

		sum = 0;
		for (i=-m+1; i<=m; i++) {
			double xd = dcorr*IMPULSE_HEIGHT*sinc((i-j)*cutoff)*blackman(i-j+width/2,width);
			int32_t x = rint(xd);
			*(ptr++) = x;
			sum += x;
		}
		*(ptr - m) += IMPULSE_HEIGHT - sum;
	}

	return pulsetab;
}

test void test_gen_impulsetab(void)
{
	const long n_shift = 3;
	const long w_shift = 5;
	const double cutoff = 1.0;
	const int32_t reference[(1 << 3) * (1 << 5)] = {
		   0,    0,     0,     0,      0,     0,       0,      0,       0,      0,       0,      0,        0,       0,        0, 16777216,        0,        0,       0,        0,      0,       0,      0,       0,      0,       0,     0,      0,     0,     0,    0,    0,
		-363, 1849, -4948, 10417, -19315, 33033,  -53332,  82457, -123382, 180400, -260509, 376947,  -559631,  895199, -1780563, 16345233,  2307495, -1030846,  623143,  -414559, 285464, -197894, 135916,  -91444,  59685,  -37412, 22231, -12271,  6057, -2448,  614,   -7,
		-487, 2933, -8221, 17683, -33206, 57277,  -93060, 144571, -217096, 318167, -459858, 664550,  -981602, 1548641, -2947130, 15089751,  4990298, -2055557, 1217403,  -803887, 552216, -382888, 263457, -177806, 116556,  -73476, 43992, -24546, 12326, -5151, 1418,  -52,
		-437, 3258, -9622, 21184, -40314, 70161, -114740, 179138, -270005, 396702, -573989, 828702, -1218622, 1899280, -3482368, 13127507,  7845425, -2911747, 1684383, -1102909, 755456, -523769, 360994, -244358, 160851, -101957, 61491, -34664, 17689, -7620, 2270, -154,
		-300, 2964, -9290, 20977, -40491, 71130, -117115, 183786, -278082, 409660, -593500, 856328, -1254371, 1934043, -3435526, 10638395, 10638395, -3435526, 1934043, -1254371, 856328, -593500, 409660, -278082, 183786, -117115, 71130, -40491, 20977, -9290, 2964, -300,
		-154, 2270, -7620, 17689, -34664, 61491, -101957, 160851, -244358, 360994, -523769, 755456, -1102909, 1684383, -2911747,  7845425, 13127507, -3482368, 1899280, -1218622, 828702, -573989, 396702, -270005, 179138, -114740, 70161, -40314, 21184, -9622, 3258, -437,
		 -52, 1418, -5151, 12326, -24546, 43992,  -73476, 116556, -177806, 263457, -382888, 552216,  -803887, 1217403, -2055557,  4990298, 15089751, -2947130, 1548641,  -981602, 664550, -459858, 318167, -217096, 144571,  -93060, 57277, -33206, 17683, -8221, 2933, -487,
		  -7,  614, -2448,  6057, -12271, 22231,  -37412,  59685, -91444,  135916, -197894, 285464,  -414559,  623143, -1030846,  2307495, 16345233, -1780563,  895199,  -559631, 376947, -260509, 180400, -123382,  82457,  -53332, 33033, -19315, 10417, -4948, 1849, -363,
	};
	int32_t *pulsetab = gen_impulsetab(w_shift, n_shift, cutoff);

	ASSERT_ARRAY_EQUAL("%8d", reference, pulsetab);
}
TEST(test_gen_impulsetab);
TEST_EOF;
