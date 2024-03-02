# wiringPi (stripped down edition)

Based on https://github.com/WiringPi/WiringPi

This version only contains the minimal feature set actually used in the RaspberryPi websid 
project. It only handles the "Raspberry Pi 4B" model and if you are trying to port the
project to a different Raspberry model then you should probably patch the Makefile to just 
use the regular "wiringPi" library instead (see comments in Makefile for how to do that).

This version here has the following "advantages" over the original:

1) It has no unnecessary dependencies - which makes it a better base for porting it
into a kernal module (which I need to do to implement a websid device driver).

2) The added digitalBulkWrite() API allows to update all used pins in one atomic write
thus reducing the risk of inconsistent "bus state" and increasing the write speed.

3) Added wpiPinToPhys() utility can be used to automatically translate the wiringPi specific
pin numbering - which may be useful if code needs to be migrated to a different library.


## License

LGPL (applies specifially to this folder but NOT to the rest of this project)
