This is an enhanced version of the below. Original homegrown OPN2 implementation is replaced using "state-of-the-art" OPN2 emulation (MAME or Nuked-OPN2). 
Also fixed garbage pitch-bend implementation and added stereo-support.

Copyright
2023 Juergen Wothke


--------------------------------------------------------------------------------------------------------------------
# eupmini
Performance music driver EUPHONY (Extension ".Eup") format player using Simple DirectMedia Layer (SDL) version 2.0.x .

This project is not started from scratch, in the begining it was coded for GNU/Linux, then Windows o.s. binary version came along made by Mr.anonymous K., Mr.Sen and others (link ref. Lost and Found project).

EUPHONY format music data was broadly used with past years favourite machine Fujitsu FM TOWNS.

HEat-Oh! is the first EUP creation software and text editor that was published in FMTOWNS magazine. HEat-Oh! is free software (= freeware) developed by TaroPYON (now taro), the name stands for "High EUP active tool".
It is a tool for MML compiling and creating of .EUP format file, but, because of its characteristics, it acts also as a powerful text editor.

Copyright
1995-1997, 2000 Tomoaki Hayasaka.
Win32 porting 2002, 2003 IIJIMA Hiromitsu aka Delmonta, and anonymous K.
2018 Giangiacomo Zaffini

# License

This code is available open source under the terms of the [GNU General Public License version 2](https://opensource.org/licenses/GPL-2.0).

# How to build eupplay player

The following steps build `eupplay` on Ubuntu/Debian/GNU/LINUX o.s. box, or `eupplay.exe` on a MSYS2/MinGW-w64 Windows o.s. box, with provided Makefile, SDL2 and make.

```shell/bash shell
$ git clone https://github.com/gzaffin/eupmini.git
$ cd eupmini
$ make
```

The following steps build `eupplay` on Ubuntu/Debian/GNU/LINUX o.s. box with SDL2 and cmake.

```GNU/linux bash
$ git clone https://github.com/gzaffin/eupmini.git
$ cd eupmini
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build . --config Release --target eupplay
```

The following steps build `eupplay.exe` on a MSYS2/MinGW-w64 Windows o.s. box with SDL2 and cmake.

```msys2/mingw bash
$ git clone https://github.com/gzaffin/eupmini.git
$ cd eupmini
$ mkdir build
$ cd build
$ cmake -G "MSYS Makefiles" ..
$ cmake --build . --config Release --target eupplay
```

If MSYS Makefiles generator set with `-G "MSYS Makefiles"` cannot properly set make-utility,
then add `-DCMAKE_MAKE_PROGRAM=<[PATH]/make-utility>` PATH of make-utility (see [1])

```windows command-line interface
$ git clone https://github.com/gzaffin/eupmini.git
$ cd eupmini
$ mkdir build
$ cd build
$ cmake -G "MSYS Makefiles" -DCMAKE_MAKE_PROGRAM=mingw32-make ..
$ cmake --build . --config Release --target eupplay
```

The following steps build `eupplay.exe` on a Windows o.s. box with MSVC, vcpkg, SDL2 installed with vcpkg. Both MSVC and vcpkg have git, let's suppose git is in path environment for Windows command prompt console or Windows PowerShell console.
Choosing Windows command prompt console.

```windows command-line interface
C:\>git clone https://github.com/gzaffin/eupmini.git
C:\>cd eupmini
C:\pmdmini>mkdir build
C:\pmdmini>cd build
C:\pmdmini\build>cmake -G "Visual Studio 17 2022" -A x64 -T host=x64 -D CMAKE_TOOLCHAIN_FILE=C:/Users/gzaff/Devs/vcpkg/scripts/buildsystems/vcpkg.cmake ..
C:\pmdmini\build>cmake --build . --config Release --target eupplay
```

For the case that Visual Studio can be used

```windows command-line interface
C:\>git clone https://github.com/gzaffin/eupmini.git
C:\>cd eupmini
C:\pmdmini>mkdir build
C:\pmdmini>cd build
C:\pmdmini\build>cmake -G "Visual Studio 17 2022" -A x64 -T host=x64 -D CMAKE_TOOLCHAIN_FILE=C:/Users/gzaff/Devs/vcpkg/scripts/buildsystems/vcpkg.cmake ..
```
Now Microsoft Visual Studio can be started and eupmini solution can be built and debugged.

[1]
it is make-utility name e.g. `mingw32-make` with specified PATH if make is not within search PATH as it should be

# Links:

1. Lost and Found projects
[EUPPlayer for Windows O.S.](http://heisei.dennougedougakkai-ndd.org/pub/werkzeug/EUPPlayer/)
[BEFiS Webpage Download page](http://www.purose.net/befis/download/)
[BEFiS Webpage Download direct link to eupplay](http://www.purose.net/befis/download/lib/eupplay.lzh)
[EUP player for MSW  ver.0.08f Programmed by 仙](http://www.ym2149.com/fmtowns_eup.zip)

2. [DRMSoundwork](http://www.boreas.dti.ne.jp/~nudi-drm/)
[All available EUP and Basic file archive](http://www.boreas.dti.ne.jp/~nudi-drm/sound/townsmml/arcfiles.lzh)

3. ~~Freescale 6 to 11 directory of FM-TOWNS MUSIC Performance files~~ once it was http://mdxoarchive.webcrow.jp/  
[FCEUP.part01.rar](https://1drv.ms/u/s!Ajr-D58Azu8jiCTHyA1vUPxhrBQB?e=45oXmT)  
[FCEUP.part02.rar](https://1drv.ms/u/s!Ajr-D58Azu8jiCJQp0cOXtFV4A6K?e=3dSphi)  
[FCEUP.part03.rar](https://1drv.ms/u/s!Ajr-D58Azu8jiCODxoiyh-K0Qri2?e=y5xiRF)  
[FCEUP.part04.rar](https://1drv.ms/u/s!Ajr-D58Azu8jiCY7yolQ9048VFEj?e=R2NfL6)  
[FCEUP.part05.rar](https://1drv.ms/u/s!Ajr-D58Azu8jiCUmFNyllNmNOHr4?e=pIG7Km)  

5. [chiptune create ROPCHIPTUNE LABORATORY 3.00](http://rophon.music.coocan.jp/chiptune.htm)
[TOWNS EUP 2015](http://rophon.music.coocan.jp/chiptune.htm)

6. [MML Compiler HE386 for TownsOS, Windows NT/95/98, Linux](http://www.runser.jp/softlib.html)

7. Vcpkg official GitHub repository
[GitHub Microsoft vcpkg](https://github.com/Microsoft/vcpkg)

8. Vcpkg documentation
[vcpkg: A C++ package manager for Windows, Linux and MacOS](https://docs.microsoft.com/en-us/cpp/build/vcpkg?view=vs-2019)

9. Microsoft developer blog
[Eric Mittelette's blog](https://devblogs.microsoft.com/cppblog/vcpkg-a-tool-to-acquire-and-build-c-open-source-libraries-on-windows/)
