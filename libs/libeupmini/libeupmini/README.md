# webEuphony

Copyright (C) 2023 Juergen Wothke

This is a JavaScript/WebAudio plugin of "eupplayer" used to emulate music from .eup files.
.
 
Here some background info taken from that project:

"EUPHONY format music data was broadly used with past years favourite machine Fujitsu FM TOWNS.


HEat-Oh! is the first EUP creation software and text editor that was published in FMTOWNS magazine. 
HEat-Oh! is free software (= freeware) developed by TaroPYON (now taro), the name stands for 
"High EUP active tool". It is a tool for MML compiling and creating of .EUP format file, but, 
because of its characteristics, it acts also as a powerful text editor."


In addition to porting the thing to the Web, I pimped up the original project by adding a 
"state-of-the-art" OPN2 emulation, adding stereo-support, fixing the broken bitch-bending logic and
adding various minor improvements.


This plugin is designed to work with my generic WebAudio ScriptProcessor music player (see separate project)
but the API exposed by the lib can be used in any JavaScript program (it should look familiar to anyone 
that has ever done some sort of music player plugin). 


A live demo of this program can be found here: http://www.wothke.ch/webEuphony/



## CAUTION

The "eupplayer" related source code uses a japanese character encoding (see remaining comments in japanese; 
though I translated most of these already) and your text editor will irreversibly corrupt it faster than you 
can scream "fuck that piece of shit!". (And you'll usually not notice until after you already checked-in the 
corrupted garbage.). You should best use an editor like Sakura that actually knows what it is doing.


## Credits

The original eupplayer implementation is the work of Tomoaki Hayasaka. (the respective original source code
can meanwhile also be found here: https://github.com/tjhayasaka/eupplayer )
Copyright 1995-1997, 2000 Tomoaki Hayasaka. 

This project is based on Giangiacomo Zaffini's respective project; https://github.com/gzaffin/eupmini
which is a marginally cleaned up version of the original source code (and contains Windows specific
add-on patches - most of which I propably threw out while making my changes).

The proper OPN2 emulations that I added originate from the vgmPlay project and are copyrighted by the 
respective vgmPlay, MAME and Nuked-OPN2 authors.


## Project

All the "Web" specific additions are contained in the "emscripten" subfolder. The "original" eupplayer/eupmini 
code is in the "eupmini" folder and the additional OPN2 emulators are in "mame" and "Nuked-OPN2" folders. Be 
default the project uses the MAME emulation (my PC just isn't fast enough for Nuked-OPN2). The defines 
USE_ORIG_OPL2_IMPL and USE_NUKED_OPL2 control which emulation is used.



## Howto build

You'll need Emscripten (http://kripken.github.io/emscripten-site/docs/getting_started/downloads.html). The make script 
is designed for use of emscripten version 1.37.29 (unless you want to create WebAssembly output, older versions might 
also still work).

The below instructions assume that the webEuphony project folder has been moved into the main emscripten 
installation folder (maybe not necessary) and that a command prompt has been opened within the 
project's "emscripten" sub-folder, and that the Emscripten environment vars have been previously 
set (run emsdk_env.bat).

The Web version is then built using the makeEmscripten.bat that can be found in this folder. The 
script will compile directly into the "emscripten/htdocs" example web folder, were it will create 
the backend_eup.js library. (To create a clean-build you have to delete any previously built libs in the 
'built' sub-folder!) The content of the "htdocs" can be tested by first copying it into some 
document folder of a web server. 


## Depencencies

The current version requires version >=1.2.1 of my https://bitbucket.org/wothke/webaudio-player/

This project comes without any music files, so you'll also have to get your own and place them
in the htdocs/music folder (you can configure them in the 'songs' list in index.html).


## License

The original project uses GPL version 2 - so the same license is used here.
