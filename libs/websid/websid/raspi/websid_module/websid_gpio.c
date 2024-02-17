/*
 * Memory mapped access to GPIO pins (output only).
 *
 * This basically uses the "same" memory mapped approach already used in 
 * userland" (the "mmap" API is different for kernel use).
 *
 * Another change to the "userland" version is that all the APIs used during 
 * playback have been changed into macros that inline the implementation 
 * without making any subroutine calls. (This allows to use the code 
 * with no need for any additional stack allocations.)
 *
 * Some new "recommended" APIs are used for setup - but it was a mistake 
 * to even use them since they add "GPL licensing" related build problems 
 * with no tangible benefit.
 *
 * Terms of Use: This software is licensed under a CC BY-NC-SA 
 * (http://creativecommons.org/licenses/by-nc-sa/4.0/).
 */
 
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>

// amazingly that crappy (aka "new and shiny") gpio API isn't even available 
// in user space... idiots..
struct gpio_desc *_cs, *_a0, *_a1, *_a2, *_a3, *_a4, *_d0, *_d0, *_d0, *_d1, *_d2, *_d3, *_d4, *_d5, *_d6, *_d7;


 // note: use the following command to find the base address of GPIO on your device: 
 // cat /proc/iomem | grep gpio@
#define GPIO_BASE 0xFE200000 				/* CAUTION: RPi4 only!!! */


// I've kept the wiringPi based pin numbering to ease comparisons with the
// old "userland" impl
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
#define D6      10
#define D7      11


// only update masked pins in the 0..16 range.. leave the others alone
const u32 PIN_MASK = 0b11000111111111111;

// offsets to the GPSET/GPCLR registers indexed by gpio-pin
static u8 _offset_GPSET[] = {
   7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
   8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};
static u8 _offset_GPCLR[] = {
  10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
  11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11
};

// pin-mapping used in wiringPi - Raspberry Pi 4B specific (Revision)
static int _wpi_to_gpio[64] = {
  17, 18, 27, 22, 23, 24, 25, 4,
   2,  3,							// I2C  - SDA0, SCL0
   8,  7,							// SPI  - CE1, CE0
  10,  9, 11, 						// SPI  - MOSI, MISO, SCLK
  14, 15,							// UART - Tx, Rx
  28, 29, 30, 31,					// Rev 2: New GPIOs 8 though 11
   5,  6, 13, 19, 26,				// B+
  12, 16, 20, 21,					// B+
   0,  1,							// B+						wpi 30, 31

   // >32 are unused
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

#define TO_GPIO(pin) _wpi_to_gpio[pin]

volatile u32 *_gpio_map = 0;


// "local" var of "digital_write"
static u32 _dw_pin;

#define digital_write(pin, value) {\
	_dw_pin = _wpi_to_gpio[pin];\
	\
	if (value == 0) {\
		*(_gpio_map + _offset_GPCLR [_dw_pin]) = 1 << (_dw_pin & 0x1f);\
	} else {\
		*(_gpio_map + _offset_GPSET [_dw_pin]) = 1 << (_dw_pin & 0x1f);\
	}\
}

// "local" vars of "digital_bulk_write"
static u32 _b_pin_set = 0;
static u32 _b_pin_clr = 0;
static u32 _b_current_bit_mask = 1;
static u32 _b_out_mask = 0;
static u32 _b_pin;
static u32 _b_gpio_pin;
static u32 _b_toggles;

#define digital_bulk_write(toggles, mask, max_pin) {\
	 _b_pin_set = 0;\
	 _b_pin_clr = 0;\
	 _b_current_bit_mask = 1;\
	 _b_out_mask = 0;\
	_b_toggles = toggles;\
	for (_b_pin = 0; _b_pin < max_pin; ++_b_pin)\
	{\
		_b_gpio_pin = (1 << _wpi_to_gpio[_b_pin]);\
		\
		if (mask & _b_current_bit_mask) { \
			_b_out_mask |= _b_gpio_pin;\
		}\
		\
		if ((_b_toggles & _b_current_bit_mask) == 0) {\
			_b_pin_clr |= _b_gpio_pin;\
		} else {\
			_b_pin_set |= _b_gpio_pin;\
		}\
		_b_current_bit_mask <<= 1;\
	}\
	\
	*(_gpio_map + _offset_GPSET[0]) = _b_pin_set & _b_out_mask; \
	\
	/* "clear" is used to activate the SID's CS, i.e. "clearing" */\
	/* should better be done *after* the "setting"; see poke_SID */\
	*(_gpio_map + _offset_GPCLR[0]) = _b_pin_clr & _b_out_mask; \
}

static u32 _gpio_init_fail;

struct gpio_desc* gpio_output(unsigned int pin, const char *label,  int value) {
	struct gpio_desc *desc;
	
	pin = TO_GPIO(pin);
	
	// https://www.kernel.org/doc/Documentation/gpio/consumer.txt
	// "GPIO number passed to gpio_to_desc() must have been properly acquired"
	
	// "label associates a string with it that can later appear in sysfs".. 
	// *can*.. see below
	
	// the respective status can be queried by installing the "gpiod" package 
	// and then running "sudo gpioinfo" (but probably the respective info is
	// utterly useless since users of the old APIs might not show here at all..)
	
	if(gpio_request(pin, label)) {
		printk("websid: gpio_request failed on pin: %d\n", pin);
		_gpio_init_fail = 1;
		return 0;
	}
	
	// this shows the respective pin as files in /sys/class/gpio/gpio..
	// which seems to be pretty pointless: neither the chosen "label" is
	// used nor does it show who is using the pins.. bloody waste of time
//    gpio_export(pin, 0);
	
	desc = gpio_to_desc(pin);
	if (!desc || gpiod_direction_output(desc, value)) {	// equivalent to the old "pinOutputMode" impl
		printk("websid: gpiod_direction_output failed on pin: %d\n", pin);
		_gpio_init_fail = 1;
		return 0;
	}
		
	return desc;
}

int gpio_init(struct device *dev) {	
	_gpio_init_fail= 0;
	
	// the "new" API is not used to actually access the GPIO later and
	// its only benefit is that it might provide nicer info on the OS
	// level to show what some GPIO pin is used for:	
	_cs = gpio_output(CS, "CS", 1);
	
	_a0 = gpio_output(A0, "A0", 0);
	_a1 = gpio_output(A1, "A1", 0);
	_a2 = gpio_output(A2, "A2", 0);
	_a3 = gpio_output(A3, "A3", 0);
	_a4 = gpio_output(A4, "A4", 0);

	_d0 = gpio_output(D0, "D0", 0);
	_d1 = gpio_output(D1, "D1", 0);
	_d2 = gpio_output(D2, "D2", 0);
	_d3 = gpio_output(D3, "D3", 0);
	_d4 = gpio_output(D4, "D4", 0);
	_d5 = gpio_output(D5, "D5", 0);
	_d6 = gpio_output(D6, "D6", 0);
	_d7 = gpio_output(D7, "D7", 0);

	if (_gpio_init_fail) {
		printk(KERN_INFO "websid: gpio_init failed\n");
		return 0;
	}
	_gpio_map = (u32*)ioremap(GPIO_BASE, SZ_4K);	// this should already be uncached
	
	if (!_gpio_map) {
		printk("websid: gpio_init ioremap failed\n");
		return 0;
	}
	
// the same default pin init could have been performed using:
//	digital_bulk_write(0b1000, PIN_MASK, 17); // set "CS" HIGH(1) / all the rest LOW(0)

	printk(KERN_INFO "websid: gpio_init completed\n");
	return 1;
}

void gpio_release(void) {	
	if (_gpio_map) {
		gpio_free(TO_GPIO(D0));
		gpio_free(TO_GPIO(D1));
		gpio_free(TO_GPIO(D2));
		gpio_free(TO_GPIO(D3));
		gpio_free(TO_GPIO(D4));
		gpio_free(TO_GPIO(D5));
		gpio_free(TO_GPIO(D6));
		gpio_free(TO_GPIO(D7));

		gpio_free(TO_GPIO(A0));
		gpio_free(TO_GPIO(A1));
		gpio_free(TO_GPIO(A2));
		gpio_free(TO_GPIO(A3));
		gpio_free(TO_GPIO(A4));

		gpio_free(TO_GPIO(CS));

		iounmap(_gpio_map); // release mapping
	} else {
		printk(KERN_INFO "websid: gpio release... nothing was started\n");
	}

	printk(KERN_INFO "websid: gpio released\n");
}


static u32 _poke_ts;
static u32 _poke_addr;	// use more bits than input to avoid issues when shifting
static u32 _poke_value;

#define poke_SID(addr, value) {\
	_poke_addr = addr;\
	_poke_value = value;\
	\
	digital_bulk_write(\
		/* below bit operations could be reduced by rearranging the */\
		/* actual board wiring.. but lets presume than it doesn't matter..*/\
		\
		((_poke_addr & 0x08) >> 3) | 	/* A3	(pin#0, i.e. bit #0)*/\
		((_poke_value & 0x04) >> 1) |	/* D2*/\
		((_poke_addr & 0x10) >> 2) | 	/* A4*/\
										/* CS   set LOW here.. to strobe below*/\
		((_poke_value & 0x08) << 1) |	/* D3*/\
		((_poke_value & 0x10) << 1) |	/* D4*/\
		((_poke_value & 0x20) << 1) |	/* D5*/\
		((_poke_addr & 0x04) << 5) |	/* A2*/\
		((_poke_addr & 0x01) << 8) |	/* A0*/\
		((_poke_addr & 0x02) << 8) |	/* A1*/\
		((_poke_value & 0x40) << 4) |	/* D6*/\
		((_poke_value & 0x80) << 4) |	/* D7*/\
		/* unused (SPI pin)*/\
		/* unused (SPI pin)*/\
		/* unused (SPI pin)*/\
		((_poke_value & 0x01) << 15) |	/* D0*/\
		((_poke_value & 0x02) << 15),	/* D1 (pin #16, i.e. bit #16) */\
		PIN_MASK, 17\
	);\
	\
	_poke_ts = micros(); _poke_ts+= 2;	/* 2 seems to work better than 1 (maybe due to the clocks not being in sync)*/\
	while (micros() <=  _poke_ts) {}\
	\
	digital_write(CS, 1);\
}
