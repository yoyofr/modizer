/*
 * wiringPi.h: stripped down edition ("Raspberry Pi 4B" only, with reduced API)
 *	Arduino like Wiring library for the Raspberry Pi.
 *	Copyright (c) 2012-2017 Gordon Henderson
 ***********************************************************************
 * This file is part of wiringPi:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with wiringPi.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

#ifndef	__WIRING_PI_H__
#define	__WIRING_PI_H__

// C doesn't have true/false by default and I can never remember which
//	way round they are, so ...
//	(and yes, I know about stdbool.h but I like capitals for these and I'm old)

#ifndef	TRUE
#  define	TRUE	(1==1)
#  define	FALSE	(!TRUE)
#endif

// GCC warning suppressor

#define	UNU	__attribute__((unused))


// Handy defines

// wiringPi modes

#define	WPI_MODE_PINS		 0
#define	WPI_MODE_GPIO		 1
#define	WPI_MODE_PHYS		 3
#define	WPI_MODE_UNINITIALISED	-1

// Pin modes

#define	OUTPUT			 1			/* THE ONLY MODES LEFT IN THIS STRIPPED DOWN VERSION */
#define	GPIO_CLOCK		 3


#define	LOW				 0
#define	HIGH			 1

// Failure modes

#define	WPI_ALMOST	(1==2)


// Function prototypes
//	c++ wrappers thanks to a comment by Nick Lott
//	(and others on the Raspberry Pi forums)

#ifdef __cplusplus
extern "C" {
#endif

// Core wiringPi functions

extern int  wiringPiSetup       (void) ;
extern int  wiringPiSetupGpio   (void) ;
extern int  wiringPiSetupPhys   (void) ;

extern int wiringPiTeardown (void);

extern          void pinMode             (int pin, int mode) ;

void gpioClockSet (int pin, int freq); // added minimal improvements

// @param value: LOW or HIGH
extern          void digitalWrite        (int pin, int value) ;

// On-Board Raspberry Pi hardware specific stuff

// @param toggles  each bit acts as a LOW/HIGH switch for the corresponding pin (WPI_MODE_PINS)
// @param mask    each bit controls if the corresponding pin is updated
// @param maxPin    maximum pin # to be considered
extern          void digitalBulkWrite   (uint32_t toggles, uint32_t mask, uint8_t maxPin) ;	// added


// for testing purposes
int wpiPinToGpio (int wpiPin);
int physPinToGpio (int physPin);
int wpiPinToPhys(int wpiPin);		// added

#ifdef __cplusplus
}
#endif

#endif
