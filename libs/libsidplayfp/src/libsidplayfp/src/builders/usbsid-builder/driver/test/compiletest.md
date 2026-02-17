# Compilation test
```shell
pushd src ; rm *.o ; g++ --verbose $(pkg-config --cflags --libs libusb-1.0) -c *.cpp $(pkg-config --cflags --libs libusb-1.0) -Wall -Werror ; rm *.o ; popd
```
