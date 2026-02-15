2.16.0 2025-12-23
* Add USBSID support (#84)
* Display the actual used SID model(s) (#79)
* Fix invalid filter range error (#94)
* Add --version option (#99)
* Replace non-ASCII characters with ASCII when iconv is not enabled (#97)
* Correctly restore ANSI console (#100)
* Correctly display play errors (#101)
* Handle negative fade out values in an useful way (#93)
* Init driver output before parsing config (#89)
* Build windows binaries with dynamic SIMD dispatch



2.15.2 2025-11-02
* Look for builder headers in the correct path



2.15.1 2025-09-21
* Fixed cross-compiling replacing AC_CHECK_FILE (#76)
* Adjust song length when forcing C64 model (#83)



2.15.0 2025-06-29
* fix a few typos (#74)
* Implement fadeout (#72)



2.14.1 2025-05-17
* Fix crash when using -b (#70)



2.14.0 2025-05-11
* Ported to new play API (#67)
* Removed support for old library version (#68)
* Minor fixes and optimizations



2.13.0 2025-04-13
* Force precision to 16 in audio drivers (fixes #63)
* Avoid 100% CPU usage when paused
* Display audio driver in use in verbose mode



2.12.0 2024-12-01
* Avoid crash when not using audio (#60)
* Fixed buffer size on various backends



2.11.0 2024-11-03
* Added support for combined wave strength arguments. (#51)
* Made audio buffer size configurable (#52)
* Properly check for player errors (#39)



2.10.0 2024-10-06
* Added support for muting samples (#5)
* Allow setting filter range from command line (#47)



2.9.0 2024-08-12
* Use correct freq table when using custom speed (#37)
* Updated filter presets from libSidplayEZ
* Capitalize filterRange6581 .ini parameter (#40)
* Fixed docs for -o option
* Reworked player logic, fixes looping



2.8.0 2024-06-09
* be explicit if Songlenth DB is not loaded (#35)
* look for Songlengths file in data path (#35)
* require a c++11 compiler
* added sanity check for filter parameters (#36)



2.7.0 2024-03-29
* added ability to adjust the uCox parameter
* allow setting the combined waveforms strength
* enable libout123 by default if found
* improved the ALSA backend



2.6.2 2024-01-11
* Really fix build with autoconf-2.72



2.6.1 2024-01-07
* Update m4 macro, fixes an issue with autoconf 2.72 (#29)
* Fix an uninitialized variable



2.6.0 2024-01-01
* Added filter curve switch plus recommended 6581 filter setting



2.5.1 2023-12-23
* Get rid of PATH_MAX (#24)



2.5.0 2023-06-02
* Correctly detect notes (fixes #21)
* Fix building on Windows Unicode
* display config file location in high verbose mode
* stilview: handle character conversion
* some work toward making playback exact to millisecond (#4)
