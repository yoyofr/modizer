/*
Simple PWM type "cpu fan" controller 
(see https://www.instructables.com/PWM-Regulated-Fan-Based-on-CPU-Temperature-for-Ras/).

Note: I hindsight, a cpu fan may well be overkill and it might be a better/simpler
solution to install a decent passive cooling element instead of a fan.


This impl abuses the unused "camera GPIO" to control the cpu fan: This avoids wasting 
more GPIO pins on the header and more importantly avoids update-conflicts with the
regular lower-range GPIO control regs. (The camera GPIO seems to be on some
kind of "GPIO expansion" which is probably connected via I2C.)

Obviously this impl can only be used when you don't intend to actually use
a camera. And it is probably a good idea to then diable the regular camera
driver completely:

/etc/modprobe.d/raspi-blacklist.conf
	#disable camera driver
	blacklist bcm2835-v412

The "CAM_GPIO" signal is exposed on the respective camera-connector and a 
respective control wire can be easily soldered to the base of the respective 
connector. (Other unused "system"-GPIOs are more difficult to salvage since
their respective traces on the PCB are much smaller and would require a 
microscope to solder).
	
The below logic could obviously be patched to use any other GPIO pin instead.
	
depends on:
	sudo apt-get install -y libgpiod-dev


useful: the below command can be used to show what the various GPIO 
pins are currently used for ("sudo apt-get install gpiod" if necessary):

	gpioinfo 
	
	
to install as a service the cpufan.service (edit the file so it points to 
"cpufan" executable) must be copied to:
	/etc/systemd/system/
	
and the service then is started using:

	systemctl start cpufan

and to automatically start on boot:	
	systemctl enable cpufan

	
*/

#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>

#include <signal.h>
void(*sigset(int sig, void (*disp)(int)))(int);


static struct gpiod_chip *_chip;
static struct gpiod_line *_line;


typedef void (*callback_function)(int);

int init(callback_function callback) {
	_chip = gpiod_chip_open("/dev/gpiochip1");	/* GPIO expansion */
	if (!_chip) {
		fprintf(stderr, "error: gpiod_chip_open\n");
		return -1;
	}
	_line = gpiod_chip_get_line(_chip, 5);	/* CAM_GPIO */
	if (!_line) {
		fprintf(stderr, "error: gpiod_chip_get_line\n");
		gpiod_chip_close(_chip);
		return -1;
	}

	int req = gpiod_line_request_output(_line, "cpufan", 1);
	if (req) {
		fprintf(stderr, "error: gpiod_line_request_output\n");
		gpiod_chip_close(_chip);
		return -1;
	}
	
    sigset(SIGINT, callback);	// handle ctrl-C
	
	return 0;
}

int teardown() {
	gpiod_chip_close(_chip);
	return 0;
}

static int _abort = 0;
void do_abort(int s) {
	_abort = 1;
}


double get_cpu_temp() {
	double temp;
	FILE *fp = fopen ("/sys/class/thermal/thermal_zone0/temp", "r");
	if (fp == NULL) {
		fprintf(stderr, "error: could not read cpu temp\n");
		return 1000;	// better safe than sorry
	}
	fscanf (fp, "%lf", &temp);
	temp /= 1000;
	fclose (fp);
	return temp;
}

// fan speed will change only of the difference of temperature is higher than hysteresis
int _hyst = 1;

double _cpu_temp_old = 0;
double _fan_speed_old = 0;

#define STEPS 3

// 20% seems to be the limit where my fan actually turns;
// in idle mode temp seems to stabilize at 44°C with fanspeed 26;
// while playing a SID temp goes up to 48/50°C with fanspeed 34
// very little fanspeed is actually needed.. and the stock fan spun 
// much faster than necessarry..

int _temp_steps[STEPS] = {40, 41, 70};	// °C
int _speed_steps[STEPS] = {0, 20, 100};	// %

int update_fan_speed() {
	int fan_speed;
	int cpu_temp = get_cpu_temp();
		
	// calculate desired fan speed
	if (abs(cpu_temp - _cpu_temp_old) > _hyst) {
		if (cpu_temp < _temp_steps[0]) {
			// below first value, fan will run at min speed.
			fan_speed = _speed_steps[0];
		} else if (cpu_temp >= _temp_steps[STEPS - 1]) {
			// above last value, fan will run at max speed
			fan_speed = _speed_steps[STEPS - 1];
		} else {
			// if temperature is between 2 steps, fan speed is calculated by linear interpolation
			for (int i = 0; i<STEPS-1; i++) {
				if ((cpu_temp >= _temp_steps[i]) && (cpu_temp < _temp_steps[i + 1])) {
					fan_speed = round((_speed_steps[i + 1] - _speed_steps[i])
									 / (_temp_steps[i + 1] - _temp_steps[i])
									 * (cpu_temp - _temp_steps[i])
									 + _speed_steps[i]);
				}
			}
		}					 
		_fan_speed_old = fan_speed;
		_cpu_temp_old = cpu_temp;

//		fprintf(stderr, "temp: %d\n", cpu_temp);
		
		return fan_speed;
	}
	return _fan_speed_old;
}


#define HZ 25
#define PULSE_STEPS 10

// use 10 speed levels
#define SLEEP_MICROS 1000000/HZ/PULSE_STEPS


int main() {
	if (init(do_abort)) return -1;
	
	while (!_abort) {
		int fanspeed = update_fan_speed();	// once per second
//		fprintf(stderr, "fan: %d\n", fanspeed);
		
		fanspeed /= PULSE_STEPS;
		
		for (int i= 0; i<HZ; i++) {		// per "pulse"
			for (int j= 0; j<PULSE_STEPS; j++) {	// create the pulse
				
				gpiod_line_set_value(_line, j>fanspeed ? 0 : 1);
				
				usleep(SLEEP_MICROS);
			}
		}
	}
	
//fprintf(stderr, "aborted\n");	
	return teardown();
}
