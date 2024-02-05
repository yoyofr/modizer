#include <stdio.h>
#include <math.h>

#ifndef M_PI
#define M_PI		(3.14159265358979323846)
#endif /* M_PI */

int main(int ac, char **av)
{
  printf("#include \"sintbl.hpp\"\n");

  {
    int i;
    printf("\nint const sintbl[] = {\n");
    for (i = 0; i < 2048; i++) {
      printf("%10d,\n", (int)(0xffff * sin(i/2048.0*2.0*M_PI)));
    }
    printf("};\n");
  }

  {
    int i;
    printf("\nint const powtbl[] = {\n");
    for (i = 0; i <= 1024; i++) {
      printf("%10d,\n", (int)(0x10000 * pow(2.0, (i-512)/512.0)));
    }
    printf("};\n");
  }

  {
    int i;
    int block;

    int const fnum[] = {
      0x026a, 0x028f, 0x02b6, 0x02df,
      0x030b, 0x0339, 0x036a, 0x039e,
      0x03d5, 0x0410, 0x044e, 0x048f,
    };

    /* (int)(880.0 * 256.0 * pow(2.0, (note-0x51)/12.0)); // bias 256 times */
    /* 0x45 seems to be 440Hz (a4) and 0x51 is 880Hz (a5) */
    printf("\nint const frequencyTable[] = {\n");
    for (block = -1; block < 9; block++) {
      for (i = 0; i < 12; i++) {
	double freq = fnum[i] * (166400.0 / 3) * pow(2.0, block-21);
	printf("%10d,\n", (int)(256.0 * freq));
      }
    }
    printf("};\n");

    printf("\nint const keycodeTable[] = {\n");
    /* It may be used for calculating detune amount and rate conversion by KS */
    for (block = -1; block < 9; block++) {
      for (i = 0; i < 12; i++) {
	/* see p.204 */
	int  f8 = (fnum[i] >>  7) & 1;
	int  f9 = (fnum[i] >>  8) & 1;
	int f10 = (fnum[i] >>  9) & 1;
	int f11 = (fnum[i] >> 10) & 1;
	int  n4 = f11;
	int  n3 = f11&(f10|f9|f8) | (~f11&f10&f9&f8);
	int note = n4*2 + n3;
	/* see p.207 */
	printf("%10d,\n", block*4 + note);
      }
    }
    printf("};\n");
  }

  {
    int freq;
    printf("\nint const keyscaleTable[] = {\n");
    printf("%d,\n", 0);
    for (freq = 1; freq < 8192; freq++) {
      printf("%d,\n", (int)(log((double)freq) / 9.03 * 32.0) - 1);
      // About 32 at 8368[Hz] (o9c). 9.03 =:= ln 8368
    }
    printf("};\n");
  }

  {
    int i
    printf("\nint const attackOut[] = {\n");
    for (i = 0; i < 1024; i++) {
      printf("  0x%04x,\n", (int)(((0x7fff+0x03a5)*30.0) / (30.0+i)) - 0x03a5);
    }
    printf("};\n");
  }

  return 0;
}
