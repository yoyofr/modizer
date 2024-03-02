/**
* API for controlling the SID chip via GPIO.
*
* Except for the added GPIO clock, the pin configuration used here 
* is identical to the one used in SidBerry, i.e. it should be possible to use all 
* respective devices with this configuration (you may need to force the
* player into CLOCK_1MHZ mode if the device does not actually support the
* GPIO clock). 
*/
#ifndef RPI4_GPIO_SID_H
#define RPI4_GPIO_SID_H

#include  <stdint.h>

#define CLOCK_PAL 985248
#define CLOCK_NTSC 1022727
#define CLOCK_1MHZ 1000000


void gpioInitSID();
void gpioInitClockSID();

void gpioTeardownSID();

void gpioPokeSID(uint8_t addr, uint8_t value);
void gpioSetClockSID(uint32_t freq);

#endif