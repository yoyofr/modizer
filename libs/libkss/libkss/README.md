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

You can also build the KSS to WAV converter binary as follows.

```
$ cmake --build . --target kss2wav
```

# NOTE
The `kss-drivers` submodule on which libkss depends, does NOT follow the libkss's license.
See README of the submodule.
