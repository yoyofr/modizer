# reminder: kernel module must be built using "sudo make". Also it depends on the kernel 
# soures to have been installed on the device! 
# see https://www.raspberrypi.org/documentation/linux/kernel/building.md
# see https://www.stephenwagner.com/2020/03/17/how-to-compile-linux-kernel-raspberry-pi-4-raspbian/

# the built module can then be loaded/unloaded using:
#sudo insmod websid_module.ko
#sudo rmmod websid_module.ko


CXX = g++ -fnon-call-exceptions -std=c++11
CXXFLAGS = -DRPI4 -fno-rtti -Wall -D__STDC_LIMIT_MACROS
LDFLAGS = -funroll-loops -Ofast


obj-m+=websid_module.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(shell pwd) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(shell pwd) clean
	
	
# XXX temp remove
test.o: test.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

test: test.o
	 g++ $(LDFLAGS)  test.o  -lm -lpthread -lwiringPi -o test
