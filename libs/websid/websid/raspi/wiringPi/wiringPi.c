/*
 * wiringPi: stripped down edition (Raspberry Pi 4B only with reduced API)
 *	Arduino look-a-like Wiring library for the Raspberry Pi
 *	Copyright (c) 2012-2017 Gordon Henderson
 *	Additional code for pwmSetClock by Chris Hall <chris@kchall.plus.com>
 *
 *	Thanks to code samples from Gert Jan van Loo and the
 *	BCM2835 ARM Peripherals manual, however it's missing
 *	the clock section /grr/mutter/
 ***********************************************************************
 * This file is part of wiringPi:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as
 *    published by the Free Software Foundation, either version 3 of the
 *    License, or (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with wiringPi.
 *    If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

// Revisions:
//  10 Apr 2021:
//      Threw out all the stuff that I am not actually using in my RP4 specific
//      project and added API that I actually need (see "added" in header file)
//
//	19 Jul 2012:
//		Moved to the LGPL
//		Added an abstraction layer to the main routines to save a tiny
//		bit of run-time and make the clode a little cleaner (if a little
//		larger)
//		Added waitForInterrupt code
//		Added piHiPri code
//
//	 9 Jul 2012:
//		Added in support to use the /sys/class/gpio interface.
//	 2 Jul 2012:
//		Fixed a few more bugs to do with range-checking when in GPIO mode.
//	11 Jun 2012:
//		Fixed some typos.
//		Added c++ support for the .h file
//		Added a new function to allow for using my "pin" numbers, or native
//			GPIO pin numbers.
//		Removed my busy-loop delay and replaced it with a call to delayMicroseconds
//
//	02 May 2012:
//		Added in the 2 UART pins
//		Change maxPins to numPins to more accurately reflect purpose


#include <stdio.h>
#include <stdarg.h>	// va_start,va_end
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>	// write
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <math.h>

#include "wiringPi.h"

#define VERSION "2.60"
#define VERSION_MAJOR 2
#define VERSION_MINOR 60


// Environment Variables

#define	ENV_DEBUG	"WIRINGPI_DEBUG"
#define	ENV_CODES	"WIRINGPI_CODES"
#define	ENV_GPIOMEM	"WIRINGPI_GPIOMEM"


// Access from ARM Running Linux
//	Taken from Gert/Doms code. Some of this is not in the manual
//	that I can find )-:
//
// Updates in September 2015 - all now static variables (and apologies for the caps)
//	due to the Pi v2, v3, etc. and the new /dev/gpiomem interface

static volatile unsigned int GPIO_BASE ;
static volatile unsigned int GPIO_CLOCK_BASE ;

#define	PAGE_SIZE		(4*1024)
#define	BLOCK_SIZE		(4*1024)

static unsigned int usingGpioMem    = FALSE ;
static          int wiringPiSetuped = FALSE ;


// Locals to hold pointers to the hardware

static volatile unsigned int *gpio ;
static volatile unsigned int *clk ;

// piGpioBase:
//	The base address of the GPIO memory mapped hardware IO

#define	GPIO_PERI_BASE_2711 0xFE000000

static volatile unsigned int piGpioBase = 0 ;

// Misc

static int wiringPiMode = WPI_MODE_UNINITIALISED ;

// Debugging & Return codes

int wiringPiDebug       = FALSE ;
int wiringPiReturnCodes = FALSE ;


// Doing it the Arduino way with lookup tables...
//	Yes, it's probably more innefficient than all the bit-twidling, but it
//	does tend to make it all a bit clearer. At least to me!

// pinToGpio:
//	Take a Wiring pin (0 through X) and re-map it to the BCM_GPIO pin
//	Cope for 3 different board revisions here.

static int *pinToGpio ;

// Revision 2:	e.g. used in RaspberryPi4B

static int pinToGpioR2 [64] =
{
  17, 18, 27, 22, 23, 24, 25, 4,	// From the Original Wiki - GPIO 0 through 7:	wpi  0 -  7
   2,  3,				// I2C  - SDA0, SCL0				wpi  8 -  9
   8,  7,				// SPI  - CE1, CE0				wpi 10 - 11
  10,  9, 11, 				// SPI  - MOSI, MISO, SCLK			wpi 12 - 14
  14, 15,				// UART - Tx, Rx				wpi 15 - 16
  28, 29, 30, 31,			// Rev 2: New GPIOs 8 though 11			wpi 17 - 20
   5,  6, 13, 19, 26,			// B+						wpi 21, 22, 23, 24, 25
  12, 16, 20, 21,			// B+						wpi 26, 27, 28, 29
   0,  1,				// B+						wpi 30, 31

// Padding:

  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	// ... 47
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	// ... 63
} ;


// physToGpio:
//	Take a physical pin (1 through 26) and re-map it to the BCM_GPIO pin
//	Cope for 2 different board revisions here.
//	Also add in the P5 connector, so the P5 pins are 3,4,5,6, so 53,54,55,56

static int *physToGpio ;

static int physToGpioR2 [64] =
{
  -1,		// 0
  -1, -1,	// 1, 2
   2, -1,
   3, -1,
   4, 14,
  -1, 15,
  17, 18,
  27, -1,
  22, 23,
  -1, 24,
  10, -1,
   9, 25,
  11,  8,
  -1,  7,	// 25, 26

// B+

   0,  1,
   5, -1,
   6, 12,
  13, -1,
  19, 16,
  26, 20,
  -1, 21,

// the P5 connector on the Rev 2 boards:

  -1, -1,
  -1, -1,
  -1, -1,
  -1, -1,
  -1, -1,
  28, 29,
  30, 31,
  -1, -1,
  -1, -1,
  -1, -1,
  -1, -1,
} ;

// gpioToGPFSEL:
//	Map a BCM_GPIO pin to it's Function Selection
//	control port. (GPFSEL 0-5)
//	Groups of 10 - 3 bits per Function - 30 bits per port

static uint8_t gpioToGPFSEL [] =
{
  0,0,0,0,0,0,0,0,0,0,
  1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,3,3,
  4,4,4,4,4,4,4,4,4,4,
  5,5,5,5,5,5,5,5,5,5,
} ;


// gpioToShift
//	Define the shift up for the 3 bits per pin in each GPFSEL port

static uint8_t gpioToShift [] =
{
  0,3,6,9,12,15,18,21,24,27,
  0,3,6,9,12,15,18,21,24,27,
  0,3,6,9,12,15,18,21,24,27,
  0,3,6,9,12,15,18,21,24,27,
  0,3,6,9,12,15,18,21,24,27,
  0,3,6,9,12,15,18,21,24,27,
} ;


// gpioToGPSET:
//	(Word) offset to the GPIO Set registers for each GPIO pin

static uint8_t gpioToGPSET [] =
{
   7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
   8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
} ;

// gpioToGPCLR:
//	(Word) offset to the GPIO Clear registers for each GPIO pin

static uint8_t gpioToGPCLR [] =
{
  10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
  11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
} ;


// GPPUD:
//	GPIO Pin pull up/down register

//#define	GPPUD	37

/* 2711 has a different mechanism for pin pull-up/down/enable  */
//#define GPPUPPDN0                57        /* Pin pull-up/down for pins 15:0  */
//#define GPPUPPDN1                58        /* Pin pull-up/down for pins 31:16 */
//#define GPPUPPDN2                59        /* Pin pull-up/down for pins 47:32 */
//#define GPPUPPDN3                60        /* Pin pull-up/down for pins 57:48 */

//static volatile unsigned int piGpioPupOffset = 0 ;

/*
 * Functions
 *********************************************************************************
 */


/*
 * wiringPiFailure:
 *	Fail. Or not.
 *********************************************************************************
 */

int wiringPiFailure (int fatal, const char *message, ...)
{
  va_list argp ;
  char buffer [1024] ;

  if (!fatal && wiringPiReturnCodes)
    return -1 ;

  va_start (argp, message) ;
    vsnprintf (buffer, 1023, message, argp) ;
  va_end (argp) ;

  fprintf (stderr, "%s", buffer) ;
  exit (EXIT_FAILURE) ;

  return 0 ;
}


/*
 * wpiPinToGpio:
 *	Translate a wiringPi Pin number to native GPIO pin number.
 *	Provided for external support.
 *********************************************************************************
 */

int wpiPinToGpio (int wpiPin)
{
  return pinToGpio [wpiPin & 63] ;
}


/*
 * physPinToGpio:
 *	Translate a physical Pin number to native GPIO pin number.
 *	Provided for external support.
 *********************************************************************************
 */

int physPinToGpio (int physPin)
{
  return physToGpio [physPin & 63] ;
}

/*
 * wpiPinToPhys:
 *	Translate a wiringPi Pin number to physical Pin number.
 *	Provided for external support.
 *********************************************************************************
 */
int wpiPinToPhys(int wpiPin) {
	int i;
	
	int gpioPinOut = wpiPinToGpio(wpiPin);	
	int physPinOut = -1;
	
	// search for physical pin mapping
	for(i = 0; i<64; i++) {
		if (physPinToGpio(i) == gpioPinOut) {
			physPinOut = i;
			break;
		}
	}
	return physPinOut;
}


/*
 *********************************************************************************
 * Core Functions
 *********************************************************************************
 */

// Port function select bits (see "5.2 Register View")

#define	FSEL_INPT		0b000
#define	FSEL_OUTP		0b001
#define	FSEL_ALT0		0b100
#define	FSEL_ALT1		0b101
#define	FSEL_ALT2		0b110
#define	FSEL_ALT3		0b111
#define	FSEL_ALT4		0b011
#define	FSEL_ALT5		0b010

static uint8_t gpioToGpClkALT0 [] =
{
          0,         0,         0,         0, FSEL_ALT0, FSEL_ALT0, FSEL_ALT0,         0,	//  0 ->  7
          0,         0,         0,         0,         0,         0,         0,         0, 	//  8 -> 15
          0,         0,         0,         0, FSEL_ALT5, FSEL_ALT5,         0,         0, 	// 16 -> 23
          0,         0,         0,         0,         0,         0,         0,         0,	// 24 -> 31
  FSEL_ALT0,         0, FSEL_ALT0,         0,         0,         0,         0,         0,	// 32 -> 39
          0,         0, FSEL_ALT0, FSEL_ALT0, FSEL_ALT0,         0,         0,         0,	// 40 -> 47
          0,         0,         0,         0,         0,         0,         0,         0,	// 48 -> 55
          0,         0,         0,         0,         0,         0,         0,         0,	// 56 -> 63
} ;

/*
 * pinMode:
 *	Sets the mode of a pin to be input, output or PWM output
 *********************************************************************************
 */

void pinMode (int pin, int mode)
{	
	int    fSel, shift, alt;

	if (wiringPiMode == WPI_MODE_PINS)
		pin = pinToGpio [pin] ;
	else if (wiringPiMode == WPI_MODE_PHYS)
		pin = physToGpio [pin] ;
	else if (wiringPiMode != WPI_MODE_GPIO)
		return ;


	fSel    = gpioToGPFSEL [pin] ;
	shift   = gpioToShift  [pin] ;

	if (mode == GPIO_CLOCK) {
		if ((alt = gpioToGpClkALT0 [pin]) == 0)	// Not a GPIO_CLOCK pin
			return ;

		*(gpio + fSel) = (*(gpio + fSel) & ~(7 << shift)) | (alt << shift) ;
		usleep (110) ;
		gpioClockSet      (pin, 1000000) ;	//set the clock frequency to 1MHz
		
	} else if (mode == OUTPUT) {
		*(gpio + fSel) = (*(gpio + fSel) & ~(7 << shift)) | (1 << shift) ;
		
	} else {
		fprintf(stderr, "error: pinMode (int pin, int mode) only supports OUTPUT mode\n");
	}
}


/*
 * digitalWrite:
 *	Set an output bit
 *********************************************************************************
 */

void digitalWrite (int pin, int value)
{
	if (wiringPiMode == WPI_MODE_PINS)
		pin = pinToGpio [pin] ;
	else if (wiringPiMode == WPI_MODE_PHYS)
		pin = physToGpio [pin] ;
	else if (wiringPiMode != WPI_MODE_GPIO)
		return ;

	if (value == LOW)
		*(gpio + gpioToGPCLR [pin]) = 1 << (pin & 31) ;
	else
		*(gpio + gpioToGPSET [pin]) = 1 << (pin & 31) ;
}

/*
 * digitalWriteAll:
 *	Pi Specific
 *	Write a maxPin-bit word to the first maxPin GPIO pins - try to do it as
 *	fast as possible. The "mask" allows to specifically exclude pins in that 
 *  range (e.g. so as not to touch standard SPI pins, etc)
 * note: the above two functions cannot be combined since they overwrite the
 * same registers
 *********************************************************************************
 */
void digitalBulkWrite   (uint32_t toggles, uint32_t mask, uint8_t maxPin)
{
	uint32_t pinSet = 0 ;
	uint32_t pinClr = 0 ;
	uint32_t currentBitMask = 1 ;
	uint32_t outMask = 0 ;
	int pin ;

	for (pin = 0 ; pin < maxPin ; ++pin)
	{
		uint32_t gpioPin = (1 << pinToGpio [pin]);

		if (mask & currentBitMask) {
			outMask |= gpioPin;
		}

		if ((toggles & currentBitMask) == 0) {
			pinClr |= gpioPin ;
		} else {
			pinSet |= gpioPin ;
		}
		currentBitMask <<= 1 ;
	}	
	*(gpio + gpioToGPSET [0]) = pinSet & outMask;

	// "clear" is used to activate the SID's CS, i.e. "clearing" 
	// should better be done *after* the "setting" (for a more general
	// purpose solution the user should be able to control what is
	// done first..)
	*(gpio + gpioToGPCLR [0]) = pinClr & outMask ;
}


/*
 * wiringPiSetup:
 *	Must be called once at the start of your program execution.
 *
 * Default setup: Initialises the system into wiringPi Pin mode and uses the
 *	memory mapped hardware directly.
 *
 * Changed now to revert to "gpio" mode if we're running on a Compute Module.
 *********************************************************************************
 */

int wiringPiSetup (void)
{
  int   fd ;

// It's actually a fatal error to call any of the wiringPiSetup routines more than once,
//	(you run out of file handles!) but I'm fed-up with the useless twats who email
//	me bleating that there is a bug in my code, so screw-em.

  if (wiringPiSetuped)
    return 0 ;

  wiringPiSetuped = TRUE ;

  if (getenv (ENV_DEBUG) != NULL)
    wiringPiDebug = TRUE ;

  if (getenv (ENV_CODES) != NULL)
    wiringPiReturnCodes = TRUE ;

  if (wiringPiDebug)
    fprintf (stderr, "wiringPi: wiringPiSetup called\n") ;

// Get the board ID information. We're not really using the information here,
//	but it will give us information like the GPIO layout scheme (2 variants
//	on the older 26-pin Pi's) and the GPIO peripheral base address.
//	and if we're running on a compute module, then wiringPi pin numbers
//	don't really many anything, so force native BCM mode anyway.


	/* 
	hardcoded for RaspberryPi4B (see piBoardId in wiringPi for other models);
	these would have been the values determined in original wiringPi impl
	  model    = 17 ;
	  rev      = 1 ;
	  mem      = 2 ;
	  maker    = 0  ;
	  overVolted = 0 ;
	  gpioLayout = 2 
	*/
    wiringPiMode = WPI_MODE_PINS ;
	
	// for "gpioLayout=2":
	pinToGpio =  pinToGpioR2 ;
    physToGpio = physToGpioR2 ;

	// raspberry 4b specific:
	piGpioBase = GPIO_PERI_BASE_2711 ;
//	piGpioPupOffset = GPPUPPDN0 ;

// Open the master /dev/ memory control device
// Device strategy: December 2016:
//	Try /dev/mem. If that fails, then
//	try /dev/gpiomem. If that fails then game over.

  if ((fd = open ("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC)) < 0)
  {
    if ((fd = open ("/dev/gpiomem", O_RDWR | O_SYNC | O_CLOEXEC) ) >= 0)	// We're using gpiomem
    {
      piGpioBase   = 0 ;
      usingGpioMem = TRUE ;
    }
    else
      return wiringPiFailure (WPI_ALMOST, "wiringPiSetup: Unable to open /dev/mem or /dev/gpiomem: %s.\n"
	"  Aborting your program because if it can not access the GPIO\n"
	"  hardware then it most certainly won't work\n"
	"  Try running with sudo?\n", strerror (errno)) ;
  }

// Set the offsets into the memory interface.
  GPIO_CLOCK_BASE = piGpioBase + 0x00101000 ;
  GPIO_BASE	  = piGpioBase + 0x00200000 ;

// Map the individual hardware components

//	GPIO:
  gpio = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, GPIO_BASE) ;
  if (gpio == MAP_FAILED)
    return wiringPiFailure (WPI_ALMOST, "wiringPiSetup: mmap (GPIO) failed: %s\n", strerror (errno)) ;

//	Clock control
  clk = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, GPIO_CLOCK_BASE) ;
  if (clk == MAP_FAILED)
    return wiringPiFailure (WPI_ALMOST, "wiringPiSetup: mmap (CLOCK) failed: %s\n", strerror (errno)) ;


  return 0;
}

int wiringPiTeardown (void)
{
	munmap((void*)gpio, BLOCK_SIZE); 
	munmap((void*)clk, BLOCK_SIZE); 
	return 0;
}


/*
 * wiringPiSetupGpio:
 *	Must be called once at the start of your program execution.
 *
 * GPIO setup: Initialises the system into GPIO Pin mode and uses the
 *	memory mapped hardware directly.
 *********************************************************************************
 */

int wiringPiSetupGpio (void)
{
  (void)wiringPiSetup () ;

  if (wiringPiDebug)
    fprintf (stderr, "wiringPi: wiringPiSetupGpio called\n") ;

  wiringPiMode = WPI_MODE_GPIO ;

  return 0 ;
}


/*
 * wiringPiSetupPhys:
 *	Must be called once at the start of your program execution.
 *
 * Phys setup: Initialises the system into Physical Pin mode and uses the
 *	memory mapped hardware directly.
 *********************************************************************************
 */

int wiringPiSetupPhys (void)
{
  (void)wiringPiSetup () ;

  if (wiringPiDebug)
    fprintf (stderr, "wiringPi: wiringPiSetupPhys called\n") ;

  wiringPiMode = WPI_MODE_PHYS ;

  return 0 ;
}

// XXX might be Raspberry Pi4 specific: lets hope the info in "BCM2711 ARM Peripherals"
// regarding the "Alternative Function Assignments" is not as flawed as some of the other 
// garbage in that doc.. (these are the same offsets used in original wiringPi so they
// might not have changed between models..)

// gpioToClk:
//	(word) Offsets to the clock Control and Divisor register
//  i.e. 0x70=CM_GP0CTL, 0x78=CM_GP1CTL, 0x80=CM_GP2CTL (below offsets count 4-byte steps)
static uint8_t gpioToClkCon [] =
{
         -1,        -1,        -1,        -1,        28,        30,        32,        -1,	//  0 ->  7
         -1,        -1,        -1,        -1,        -1,        -1,        -1,        -1, 	//  8 -> 15
         -1,        -1,        -1,        -1,        28,        30,        -1,        -1, 	// 16 -> 23
         -1,        -1,        -1,        -1,        -1,        -1,        -1,        -1,	// 24 -> 31
         28,        -1,        28,        -1,        -1,        -1,        -1,        -1,	// 32 -> 39
         -1,        -1,        28,        30,        28,        -1,        -1,        -1,	// 40 -> 47
         -1,        -1,        -1,        -1,        -1,        -1,        -1,        -1,	// 48 -> 55
         -1,        -1,        -1,        -1,        -1,        -1,        -1,        -1,	// 56 -> 63
} ;



/*
 * gpioClockSet:
 *	Set the frequency on a GPIO clock pin
 *********************************************************************************
 */
//#define OSC_CLOCK 19200000	/* older Raspberry devices*/
#define OSC_CLOCK 54000000	/* Raspberry Pi 4B*/

#define	CM_GPCTR_PASSWD		0x5A000000
#define	CM_GPCTR_SRC		1			/* always use oscillator as clock source */
#define	CM_GPCTR_BUSY		0x80
#define	CM_GPCTR_ENAB		0x10		/* enable the clock generator */

void gpioClockSet (int pin, int freq)
{
  int divi, divf ;

  pin &= 63 ;

  if (wiringPiMode == WPI_MODE_PINS)
    pin = pinToGpio [pin] ;
  else if (wiringPiMode == WPI_MODE_PHYS)
    pin = physToGpio [pin] ;
  else if (wiringPiMode != WPI_MODE_GPIO)
    return ;

  divi = OSC_CLOCK / freq ;
  // note: garbage specs incorrectly claim 1024 instead of 4096..
  divf = round(((double)OSC_CLOCK / freq-divi) * 4096);	// orig wiringPi impl always rounds off - which makes errors bigger than necessary

  if (divi > 4095) {
	fprintf(stderr, "error: invalid divf\n");
	return;
  }

  int offset = gpioToClkCon [pin];
  if (offset == -1) {
	fprintf(stderr, "error: specified pin cannot be used for clock signal");
	return;
  }
  
  *(clk + offset) = CM_GPCTR_PASSWD | CM_GPCTR_SRC;					// stop GPIO clock
  while ((*(clk + offset) & CM_GPCTR_BUSY) != 0) {}					// ... and wait

  *(clk + offset+1) = CM_GPCTR_PASSWD | (divi << 12) | divf;		// set dividers
  *(clk + offset) = CM_GPCTR_PASSWD | CM_GPCTR_ENAB | CM_GPCTR_SRC;	// Start Clock
}
