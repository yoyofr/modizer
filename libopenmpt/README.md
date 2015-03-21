
README
======

OpenMPT and libopenmpt
======================


How to compile
--------------


### OpenMPT

 -  Visual Studio 2008/2010 is required. Express versions won't work as they
    don't include MFC.

 -  The VST 2.4 and ASIO SDKs are needed for compiling with VST and ASIO
    support.

    If you don't want this, uncomment `#define NO_VST` and `#define NO_ASIO` in
    the file `common/BuildSettings.h`.

     -  ASIO:

        If you don't use `#define NO_ASIO`, you will need to put the ASIO SDK
        in the `include/ASIOSDK2` folder. The top level directory of the SDK is
        already named `ASIOSDK2`, so simply move that directory in the include
        folder.

        Please visit
        [steinberg.net](http://www.steinberg.net/en/company/developer.html) to
        download the SDK.

     -  VST:

        If you don't use `#define NO_VST`, you will need to put the VST 2.4 SDK
        in the `include/vstsdk2.4` folder.
        
        Note: The VST 2.4 SDK is no longer available on the Steinberg website.
        However, the VST 3.x SDKs still contains all the relevant files.
        Simply copy all files from `VST3 SDK/pluginterfaces/vst2.x` to
        `include/vstsdk2.4/pluginterfaces/vst2.x` and the content of
        `VST3 SDK/public.sdk/source/vst2.x` to
        `include/vstsdk2.4/public.sdk/source/vst2.x`. 

        Please visit
        [steinberg.net](http://www.steinberg.net/en/company/developer.html) to
        download the SDK.

    If you need further help with the VST and ASIO SDKs, get in touch with the
    main developers. 

 -  You need the DirectX SDK to enable DirectSound output. If you don't want
    this, uncomment `#define NO_DSOUND` in the file `common/BuildSettings.h`.

 -  To compile the project, open `mptrack/MPTRACK_08.SLN` (if you're using
    VS2008) or `mptrack/MPTRACK_10.SLN` (VS2010 or later) and hit the compile
    button! :)


### libopenmpt and openmpt123

 -  Autotools

    Grab a `libopenmpt-VERSION-autotools.tar.gz` tarball.

        ./configure
        make
        make check
        sudo make install

 -  Visual Studio 2010 (express version should work, but this is not tested):

     -  The libopenmpt solution is in `libopenmpt/libopenmpt.sln`.
        You will need the Winamp 5 SDK and the xmplay SDK if you want to
        compile the plugins for these 2 players:

         -  Winamp 5 SDK:

            To build libopenmpt as a winamp input plugin, copy the contents of
            `WA5.55_SDK.exe` to include/winamp/.

            Please visit
            [winamp.com](http://wiki.winamp.com/wiki/Plug-in_Developer) to
            download the SDK.
            You can disable in_openmpt in the solution configuration.

         -  xmplay SDK:

            To build libopenmpt with xmplay input plugin support, copy the
            contents of xmp-sdk.zip into include/xmplay/.

            Please visit [un4seen.com](http://www.un4seen.com/xmplay.html) to
            download to SDK.
            You can disable xmp-openmpt in the solution configuration.

     -  The openmpt123 solution is in `openmpt123/openmpt123.sln`.

 -  Makefile

    The makefile supports different build environments and targets via the
    `CONFIG=` parameter directly to the make invocation.
    Use 'make CONFIG=$newconfig clean' when switching between different configs
    because the makefile cleans only intermediates and target that are active
    for the current config and no configuration state is kept around across
    invocations.

     -  mingw-w64:

        The required version should be at least 4.4. Only 4.6 and up are
        tested.

            make CONFIG=mingw64-win32    # for win32

            make CONFIG=mingw64-win64    # for win64

     -  gcc or clang (on Unix-like systems, including Mac OS X with MacPorts):

        The minimum required compiler versions are:

         -  gcc 4.4

         -  clang 3.0

        The Makefile requires pkg-config for native builds.
        For sound output in openmpt123, PortAudio or SDL is required.
        openmpt123 can optionally use libflac and libsndfile to render PCM
        files to disk.

        When using gcc, run:

            make CONFIG=gcc

        When using clang, it is recommended to do:

            make CONFIG=clang

        Otherwise, simply run

            make

        which will try to guess the compiler based on your operating system.

     -  emscripten (on Unix-like systems):

        libopenmpt has been tested and verified to work with emscripten 1.21 or
        later (earlier versions might or might not work).

        Run:

            make CONFIG=emscripten

        Running the test suite on the command line is also supported by using
        node.js. Version 0.10.25 or greater has been tested. Earlier versions
        might or might not work. Depending on how your distribution calls the
        `node.js` binary, you might have to edit
        `build/make/config-emscripten.mk`.

    The `Makefile` supports some customizations. You might want to read the top
    which should get you some possible make settings, like e.g.
    `make DYNLINK=0` or similar. Cross compiling or different compiler would
    best be implemented via new `config-*.mk` files.

    The `Makefile` also supports building doxygen documentation by using

        make doc

    Binaries and documentation can be installed systen-wide with

        make PREFIX=/yourprefix install
        make PREFIX=/yourprefix install-doc

    `PREFIX` defaults to `/usr/local`. A `DESTDIR=` parameter is also
    supported.

 -  Android NDK

    See `build/android_ndk/README.AndroidNDK.txt`.



Coding conventions
------------------


### OpenMPT

(see below for an example)

- Functions / methods are "underlined" (The `//------` comment, see below for
  an example what it should look like).
- Place curly braces at the beginning of the line, not at the end
- Generally make use of the custom index types like `SAMPLEINDEX` or
  `ORDERINDEX` when referring to samples, orders, etc.
- When changing playback behaviour, make sure that you use the function
  `CSoundFile::IsCompatibleMode()` so that modules made with previous versions
  of MPT still sound correct (if the change is extremely small, this might be
  unnecessary)
- `CamelCase` function and variable names are preferred.

#### OpenMPT code example

~~~~{.cpp}
void Foo::Bar(int foobar)
//-----------------------
{
    while(true)
    {
        // some code
    }
}
~~~~


### libopenmpt

**Note:**
**This applies to `libopenmpt/` and `openmpt123/` directories only.**
**Use OpenMPT style (see above) otherwise.**

The code generally tries to follow these conventions, but they are not
strictly enforced and there are valid reasons to diverge from these
conventions. Using common sense is recommended.

 -  In general, the most important thing is to keep style consistent with
    directly surrounding code.
 -  Use C++ std types when possible, prefer `std::size_t` and `std::int32_t`
    over `long` or `int`. Do not use C99 std types (e.g. no pure `int32_t`)
 -  Qualify namespaces explicitly, do not use `using`.
    Members of `namespace openmpt` can be named without full namespace
    qualification.
 -  Prefer the C++ version in `namespace std` if the same functionality is
    provided by the C standard library as well. Also, include the C++
    version of C standard library headers (e.g. use `<cstdio>` instead of
    `<stdio.h>`.
 -  Do not use ANY locale-dependant C functions. For locale-dependant C++
    functionaly (especially iostream), always imbue the
    `std::locale::classic()` locale.
 -  Prefer kernel_style_names over CamelCaseNames.
 -  If a folder (or one of its parent folders) contains .clang-format,
    use clang-format v3.5 for indenting C++ and C files, otherwise:
     -  `{` are placed at the end of the opening line.
     -  Enclose even single statements in curly braces.
     -  Avoid placing single statements on the same line as the `if`.
     -  Opening parentheses are separated from keywords with a space.
     -  Opening parentheses are not separated from function names.
     -  Place spaces around operators and inside parentheses.
     -  Align `:` and `,` when inheriting or initialiasing members in a
        constructor.
     -  The pointer `*` is separated from both the type and the variable name.
     -  Use tabs for identation, spaces for formatting.
        Tabs should only appear at the very beginning of a line.
        Do not assume any particular width of the TAB character. If width is
        important for formatting reasons, use spaces.
     -  Use empty lines at will.
 -  API documentation is done with doxygen.
    Use general C doxygen for the C API.
    Use QT-style doxygen for the C++ API.

#### libopenmpt indentation example

~~~~{.cpp}
namespace openmpt {

// This is totally meaningless code and just illustrates identation.

class foo
	: public base
	, public otherbase
{

private:

	std::int32_t x;
	std::int16_t y;

public:

	foo()
		: x(0)
		, y(-1)
	{
		return;
	}

	int bar() const;

}; // class foo

int foo::bar() const {

	for ( int i = 0; i < 23; ++i ) {
		swtich ( x ) {
			case 2:
				something( y );
				break;
			default:
				something( ( y - 1 ) * 2 );
				break;
		}
	}
	if ( x == 12 ) {
		return -1;
	} else if ( x == 42 ) {
		return 1;
	}
	return 42;

}

} // namespace openmpt
~~~~


A few words from the readme of the original MPT 1.16 source drop by Olivier
---------------------------------------------------------------------------

> The sound library was originally written to support VOC/WAV and MOD files under
> DOS, and supported such things as PC-Speaker, SoundBlaster 1/2/Pro, and the
> famous Gravis UltraSound.
> 
> It was then ported to Win32 in 1995 (through the Mod95 project, mostly for use
> within Render32).
> 
> What does this mean?
> It means the code base is quite old and is showing its age (over 10 years now)
> It means that many things are poorly named (CSoundFile), and not very clean, and
> if I was to rewrite the engine today, it would look much different.
> 
> Some tips for future development and cleanup:
> - Probably the main improvement would be to separate the Song, Channel, Mixer
> and Low-level mixing routines in separate interface-based classes.
> - Get rid of globals (many globals creeped up over time, mostly because of the
> hack to allow simultaneous playback of 2 songs in Modplug Player -> ReadMix()).
> This is a major problem for writing a DShow source filter, or any other COM
> object (A DShow source would allow playback of MOD files in WMP, which would be
> much easier than rewriting a different player).
> - The MPT UI code is MFC-based, and I would say is fairly clean (as a rough
> rule, the more recent the code is, the cleaner it is), though the UI code is
> tightly integrated with the implementation (this could make it somewhat more
> difficult to implement such things as a skin-based UI - but hey, if it was easy,
> I probably would have done it already :).
