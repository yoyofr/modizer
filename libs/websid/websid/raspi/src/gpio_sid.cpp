#include <stdio.h>

#include "gpio_sid.h"

// uncomment USE_ORIG_WIRINGPI to use the original wiringPi library,
// e.g. to ease migration to a non-RPi4 device:
 
//#define USE_ORIG_WIRINGPI

#ifdef USE_ORIG_WIRINGPI
#include <wiringPi.h> 
#else
#include "../wiringPi/wiringPi.h"
#endif

#include "rpi4_utils.h"

// it seems the optimized "bulk write" works fine
#define USE_FAST_WRITE

// pin modes:
#define WPI 	1
#define GPIO	2
#define PHYS	3

#define USE_MODE	WPI

// this is the scheme originally used in SidBerry
	
#if(USE_MODE==WPI)
#define CS      3
#define A0      8
#define A1      9
#define A2      7
#define A3      0
#define A4      2
#define D0      15
#define D1      16
#define D2      1
#define D3      4
#define D4      5
#define D5      6
#define D6      10	// SPI chip select 0	might be a bad choice for pin?
#define D7      11	// SPI chip select 1

#define CLK     29
#endif

// corresponding mappings (for RPi4B) generated from the above
#if(USE_MODE==GPIO)
#define CS      22
#define A0      2
#define A1      3
#define A2      4
#define A3      17
#define A4      27
#define D0      14
#define D1      15
#define D2      18
#define D3      23
#define D4      24
#define D5      25
#define D6      8
#define D7      7

#define CLK     21
#endif

#if(USE_MODE==PHYS)
#define CS      15
#define A0      3
#define A1      5
#define A2      7
#define A3      11
#define A4      13
#define D0      8
#define D1      10
#define D2      12
#define D3      16
#define D4      18
#define D5      22
#define D6      24
#define D7      26

#define CLK     40

#endif

// mask for bulk write: only update masked pins in the 0..16 range.. 
// leave the others (and the 3 included SPI pins) alone
const uint32_t PIN_MASK = 0b11000111111111111;

// -------------------------------------------------

#ifndef USE_ORIG_WIRINGPI


void printPinMappings() {
	// util used to check what the wiringPi mappings correspond to..
	
	fprintf(stderr, "wiringPi pins\n\n");
	
	fprintf(stderr, "#define CS\t%d\n", CS);
	fprintf(stderr, "#define A0\t%d\n", A0);
	fprintf(stderr, "#define A1\t%d\n", A1);
	fprintf(stderr, "#define A2\t%d\n", A2);
	fprintf(stderr, "#define A3\t%d\n", A3);
	fprintf(stderr, "#define A4\t%d\n", A4);
	fprintf(stderr, "#define D0\t%d\n", D0);
	fprintf(stderr, "#define D1\t%d\n", D1);
	fprintf(stderr, "#define D2\t%d\n", D2);
	fprintf(stderr, "#define D3\t%d\n", D3);
	fprintf(stderr, "#define D4\t%d\n", D4);
	fprintf(stderr, "#define D5\t%d\n", D5);
	fprintf(stderr, "#define D6\t%d\n", D6);
	fprintf(stderr, "#define D7\t%d\n", D7);


	fprintf(stderr, "\nnative GPIO pins\n\n");
	
	fprintf(stderr, "#define CS\t%d\n", wpiPinToGpio(CS));
	fprintf(stderr, "#define A0\t%d\n", wpiPinToGpio(A0));
	fprintf(stderr, "#define A1\t%d\n", wpiPinToGpio(A1));
	fprintf(stderr, "#define A2\t%d\n", wpiPinToGpio(A2));
	fprintf(stderr, "#define A3\t%d\n", wpiPinToGpio(A3));
	fprintf(stderr, "#define A4\t%d\n", wpiPinToGpio(A4));
	fprintf(stderr, "#define D0\t%d\n", wpiPinToGpio(D0));
	fprintf(stderr, "#define D1\t%d\n", wpiPinToGpio(D1));
	fprintf(stderr, "#define D2\t%d\n", wpiPinToGpio(D2));
	fprintf(stderr, "#define D3\t%d\n", wpiPinToGpio(D3));
	fprintf(stderr, "#define D4\t%d\n", wpiPinToGpio(D4));
	fprintf(stderr, "#define D5\t%d\n", wpiPinToGpio(D5));
	fprintf(stderr, "#define D6\t%d\n", wpiPinToGpio(D6));
	fprintf(stderr, "#define D7\t%d\n", wpiPinToGpio(D7));

	
	fprintf(stderr, "\nphysical pins\n\n");
	
	fprintf(stderr, "#define CS\t%d\n", wpiPinToPhys(CS));
	fprintf(stderr, "#define A0\t%d\n", wpiPinToPhys(A0));
	fprintf(stderr, "#define A1\t%d\n", wpiPinToPhys(A1));
	fprintf(stderr, "#define A2\t%d\n", wpiPinToPhys(A2));
	fprintf(stderr, "#define A3\t%d\n", wpiPinToPhys(A3));
	fprintf(stderr, "#define A4\t%d\n", wpiPinToPhys(A4));
	fprintf(stderr, "#define D0\t%d\n", wpiPinToPhys(D0));
	fprintf(stderr, "#define D1\t%d\n", wpiPinToPhys(D1));
	fprintf(stderr, "#define D2\t%d\n", wpiPinToPhys(D2));
	fprintf(stderr, "#define D3\t%d\n", wpiPinToPhys(D3));
	fprintf(stderr, "#define D4\t%d\n", wpiPinToPhys(D4));
	fprintf(stderr, "#define D5\t%d\n", wpiPinToPhys(D5));
	fprintf(stderr, "#define D6\t%d\n", wpiPinToPhys(D6));
	fprintf(stderr, "#define D7\t%d\n", wpiPinToPhys(D7));
}
#endif

void gpioInitClockSID(){
	// lets verify that the different mappings all work the same..
#if(USE_MODE==WPI)
	wiringPiSetup();
#endif
#if(USE_MODE==GPIO)
	wiringPiSetupGpio();
#endif
#if(USE_MODE==PHYS)
	wiringPiSetupPhys();
#endif
	
#ifndef USE_ORIG_WIRINGPI
//	printPinMappings();
#endif	

#ifndef USE_ORIG_WIRINGPI
	// to verify that the different mappings all work the same..
//	printPinMappings();
#endif	

	pinMode(CLK, GPIO_CLOCK);
	gpioSetClockSID(CLOCK_PAL);
//	gpioSetClockSID(CLOCK_1MHZ);
}

void gpioTeardownSID(){
	wiringPiTeardown();
}

void gpioInitSID(){
	gpioInitClockSID();

	pinMode(D0, OUTPUT);
	pinMode(D1, OUTPUT);
	pinMode(D2, OUTPUT);
	pinMode(D3, OUTPUT);
	pinMode(D4, OUTPUT);
	pinMode(D5, OUTPUT);
	pinMode(D6, OUTPUT);
	pinMode(D7, OUTPUT);
	
	pinMode(A0, OUTPUT);
	pinMode(A1, OUTPUT);
	pinMode(A2, OUTPUT);
	pinMode(A3, OUTPUT);
	pinMode(A4, OUTPUT);
	
	pinMode(CS, OUTPUT);
		
#ifdef USE_ORIG_WIRINGPI
	digitalWrite(D0, LOW);
	digitalWrite(D1, LOW);
	digitalWrite(D2, LOW);
	digitalWrite(D3, LOW);
	digitalWrite(D4, LOW);
	digitalWrite(D5, LOW);
	digitalWrite(D6, LOW);
	digitalWrite(D7, LOW);
	
	digitalWrite(A0, LOW);
	digitalWrite(A1, LOW);	
	digitalWrite(A2, LOW);
	digitalWrite(A3, LOW);
	digitalWrite(A4, LOW);

	digitalWrite(CS, HIGH);
#else
	digitalBulkWrite(0b1000, PIN_MASK, 17); // set CS HIGH(1) / all the rest LOW(0)
#endif
}

void gpioSetClockSID(uint32_t freq) {
	gpioClockSet (CLK, freq);
}

void gpioPokeSID(uint8_t addr, uint8_t value) {
	static uint32_t ts;

#if !defined(USE_FAST_WRITE) || defined(USE_ORIG_WIRINGPI)
	// old inefficient way that can be used with original wiringPi

	// set address
	digitalWrite(A0, (addr & 0x01));
	digitalWrite(A1, (addr & 0x02) >> 1);
	digitalWrite(A2, (addr & 0x04) >> 2);
	digitalWrite(A3, (addr & 0x08) >> 3);
	digitalWrite(A4, (addr & 0x10) >> 4);
	
	// set data
	digitalWrite(D0, (value & 0x01));
	digitalWrite(D1, (value & 0x02) >> 1);
	digitalWrite(D2, (value & 0x04) >> 2);
	digitalWrite(D3, (value & 0x08) >> 3);
	digitalWrite(D4, (value & 0x10) >> 4);
	digitalWrite(D5, (value & 0x20) >> 5);
	digitalWrite(D6, (value & 0x40) >> 6);
	digitalWrite(D7, (value & 0x80) >> 7);

	// strobe the CS pin
	digitalWrite(CS, LOW);
#else 	
	// optimized atomic GPIO update using:

	digitalBulkWrite(
		// below bit operations could be reduced by rearranging the 
		// actual board wiring.. but lets presume than it doesn't matter..
		
		((addr & 0x08) >> 3) | 	// A3	(pin#0, i.e. bit #0)
		((value & 0x04) >> 1) | // D2
		((addr & 0x10) >> 2) | 	// A4
								// CS   set LOW here.. to strobe below
		((value & 0x08) << 1) | // D3
		((value & 0x10) << 1) | // D4
		((value & 0x20) << 1) | // D5
		((addr & 0x04) << 5) |	// A2	
		((addr & 0x01) << 8) |	// A0
		((addr & 0x02) << 8) |	// A1
		((value & 0x40) << 4) |	// D6
		((value & 0x80) << 4) |	// D7
		// unused (SPI pin)
		// unused (SPI pin)
		// unused (SPI pin)
		((value & 0x01) << 15) |// D0
		((value & 0x02) << 15), // D1 (pin #16, i.e. bit #16)
		PIN_MASK, 17
	);	
		
#endif
	
	ts= micros(); ts+= 2;	// 2 micro wait seems to work better (than just 1 micro)
	while (micros() <=  ts) {}

	digitalWrite(CS, HIGH);
}