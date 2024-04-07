# webDS

Copyright (C) 2019-2023 Juergen Wothke

This is a JavaScript/WebAudio plugin of kode54's "2SF decoder" (based on DeSmuME). This 
plugin is designed to work with my generic WebAudio ScriptProcessor music player (see separate project). 

It allows to play "Nintendo DS" music (.2SF/.MINI2SF files).

A live demo of this program can be found here: http://www.wothke.ch/webDS/


## Credits
The real work is in the DeSmuME emulator http://desmume.org/
and in whatever changes kode54 had to apply to use it as a music player 
https://www.foobar2000.org/components/view/foo_input_vio2sf. 


## Project
It includes large parts of the original "2SF Decoder" (see twosfplug.cpp) and "vio2sf" (see vio2sf folder) 
code.

All the "Web" specific additions (i.e. the whole point of this project) are contained in the 
"emscripten" subfolder. ) The main interface between the JavaScript/WebAudio world and the 
original C code is the adapter.c file. 

Some patches were necessary within the original codebase (these can be located by looking for EMSCRIPTEN if-defs):
vio2sf had some non-portable unaligned data access burried within its code - which I fixed as much as needed to 
make it run with EMSCRIPTEN. (vio2sf also seems to have a messed up idea regarding the meaning of "big endianess".)


## Howto build

You'll need Emscripten (http://kripken.github.io/emscripten-site/docs/getting_started/downloads.html). The make script 
is designed for use of emscripten version 1.37.29 (unless you want to create WebAssembly output, older versions might 
also still work).

The below instructions assume that the webDS project folder has been moved into the main emscripten 
installation folder (maybe not necessary) and that a command prompt has been opened within the 
project's "emscripten" sub-folder, and that the Emscripten environment vars have been previously 
set (run emsdk_env.bat).

The Web version is then built using the makeEmscripten.bat that can be found in this folder. The 
script will compile directly into the "emscripten/htdocs" example web folder, were it will create 
the backend_ds.js library. (To create a clean-build you have to delete any previously built libs in the 
'built' sub-folder!) The content of the "htdocs" can be tested by first copying it into some 
document folder of a web server. 


## Depencencies

The current version requires version >=1.2.1 of my https://bitbucket.org/wothke/webaudio-player/

This project comes without any music files, so you'll also have to get your own and place them
in the htdocs/music folder (you can configure them in the 'songs' list in index.html).


## License

Except for the usage example in the "htdocs" sub-folder this project is distributed under the GPL.
