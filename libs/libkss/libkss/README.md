# LIBKSS

LIBKSS is a music player library for MSX music formats, forked from MSXplug.
Supported formats are .kss, .mgs, .bgm, .opx, .mpk, .mbm.

# How to build

The following step builds `libkss.a` library.

```
$ git clone --recursive https://github.com/digital-sound-antiques/libkss.git
$ cd libkss
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build .
```

# KSS2WAV Example
You can also build the KSS to WAV converter example as follows.

```
$ cmake --build . --target kss2wav
```

# Note
The [kss-drivers] submodule on which libkss depends, does NOT follow the libkss's license.
See README of the submodule.

[kss-drivers]: https://github.com/digital-sound-antiques/kss-drivers/

## Purge drivers
If you would like to build libkss without kss-drivers, define `EXCLUDE_DRIVER_ALL` macro in CMakeLists.txt.

```
# CMakeLists.txt
cmake_minimum_required(VERSION 2.8)
project(libkss)

add_compile_definitions(EXCLUDE_DRIVER_ALL)
# add_compile_definitions(EXCLUDE_DRIVER_MGSDRV)
# add_compile_definitions(EXCLUDE_DRIVER_KINROU)
# add_compile_definitions(EXCLUDE_DRIVER_MBMDRV)
# add_compile_definitions(EXCLUDE_DRIVER_MPK106)
# add_compile_definitions(EXCLUDE_DRIVER_MPK103)
# add_compile_definitions(EXCLUDE_DRIVER_OPX4KSS)
...
```

## Load driver binary at runtime
The driver binary can be loaded from a file or memory at runtime using the following functions.

```
int KSS_set_<driver>(const uint8_t *data, uint32_t size);
int KSS_load_<driver>(const char *filename);
```

where `<dirver>` is mgsdrv, mbmdrv, kinrou, mpk106, mpk103, opx2kss or fmbios.

# Acknowledgement
- NEZplug and MBM2KSS by Mamiya - http://nezplug.sourceforge.net/
- MGSDRV by GIGAMIX/Ain - http://www.gigamixonline.com/mgsdrv/
- KINROU5 by Keiichi Kuroda
- MPK by K-KAZ
- OPX4KSS by Mikasen
- MPK2KSS by Naruto
