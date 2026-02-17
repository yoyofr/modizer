#!/usr/bin/env bash

# pkg-config amounts to => -I/usr/local/include/libusb-1.0 -L/usr/local/lib -lusb-1.0

echo "**** Creating build directories ****"
mkdir -p build/regular/cpp
mkdir -p build/regular/c

echo "**** Creating Vice lib build directories ****"
mkdir -p build/vice/cpp
mkdir -p build/vice//c

echo "**** Cleaning previous builds ****"
rm -f build/*.so
rm -f build/regular/*.o
rm -f build/regular/*.a
rm -f build/regular/cpp/*.o
rm -f build/regular/c/*.o
rm -f build/regular/cpp/*.a
rm -f build/regular/c/*.a
rm -f build/regular/*.so
rm -f build/vice/*.o
rm -f build/vice/*.a
rm -f build/vice/cpp/*.o
rm -f build/vice/c/*.o
rm -f build/vice/cpp/*.a
rm -f build/vice/c/*.a
rm -f build/vice/*.so

echo "**** Building with G++ ****"
g++ --verbose ../src/USBSID.cpp -c -fPIC -Wall -Werror $(pkg-config --cflags --libs libusb-1.0) -o build/regular/cpp/USBSID.o
g++ --verbose ../src/USBSIDInterface.cpp -c -fPIC -Wall -Werror  $(pkg-config --cflags --libs libusb-1.0) -o build/regular/cpp/USBSIDInterface.o
ar rcs build/regular/cpp/USBSIDPico.a build/regular/cpp/*.o
g++ --verbose -shared -o build/regular/USBSIDPico-cpp.so build/regular/cpp/*.o $(pkg-config --libs libusb-1.0)

echo "**** Building with GCC ****"
gcc --verbose ../src/USBSID.cpp -c -fPIC -Wall -Werror $(pkg-config --cflags --libs libusb-1.0) -o build/regular/c/USBSID.o
gcc --verbose ../src/USBSIDInterface.cpp -c -fPIC -Wall -Werror $(pkg-config --cflags --libs libusb-1.0) -o build/regular/c/USBSIDInterface.o
ar rcs build/regular/c/USBSIDPico.a build/regular/c/*.o
gcc --verbose -shared -o build/regular/USBSIDPico-c.so build/regular/c/*.o $(pkg-config --libs libusb-1.0)

mv build/regular/*.so build/

echo "**** Vice lib test build ****"

echo "**** Building with G++ ****"
g++ --verbose ../libusbsiddrv-vice/USBSID.cpp -c -fPIC -Wall -Werror $(pkg-config --cflags --libs libusb-1.0) -o build/vice/cpp/USBSID.o
g++ --verbose ../libusbsiddrv-vice/USBSIDInterface.cpp -c -fPIC -Wall -Werror  $(pkg-config --cflags --libs libusb-1.0) -o build/vice/cpp/USBSIDInterface.o
ar rcs build/vice/cpp/USBSIDPico.a build/vice/cpp/*.o
g++ --verbose -shared -o build/vice/USBSIDPico-vicelib-cpp.so build/vice/cpp/*.o $(pkg-config --libs libusb-1.0)

echo "**** Building with GCC ****"
gcc --verbose ../libusbsiddrv-vice/USBSID.cpp -c -fPIC -Wall -Werror $(pkg-config --cflags --libs libusb-1.0) -o build/vice/c/USBSID.o
gcc --verbose ../libusbsiddrv-vice/USBSIDInterface.cpp -c -fPIC -Wall -Werror $(pkg-config --cflags --libs libusb-1.0) -o build/vice/c/USBSIDInterface.o
ar rcs build/vice/c/USBSIDPico.a build/vice/c/*.o
gcc --verbose -shared -o build/vice/USBSIDPico-vicelib-c.so build/vice/c/*.o $(pkg-config --libs libusb-1.0)

mv build/vice/*.so build/
