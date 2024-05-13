# SpectreZX

Copyright (C) 2015-2023 Juergen Wothke

This is a JavaScript/WebAudio plugin of ZXTune. This plugin is designed to work with my 
generic WebAudio ScriptProcessor music player (see separate project). 

It plays mostly ZX Spectrum chiptune music using tracker formats like: Chip Tracker v1.xx, 
Digital Music Maker, Digital Studio AY/Covox, Extreme Tracker v1.xx, ProDigiTracker v0.xx, 
SQ Digital Tracker, Sample Tracker. (It also supports various packed and archived file formats.)

This "project" is based on ZXTune version "zxtune-r3150": As far as Z80 emulation concerned
the project includes most of the  original code including the 3rd party dependencies. 
Other 3rd party emulators(etc) have been removed (since I already have those
as separate standalone players or I don't need them) and I don't want to make this player 
any bigger than it already is (e.g. FLAC, mp3, ogg, sidplayfp, vorbis, xmp). 
(Some of the unused 'boost' stuff was stripped off and RAR support was also removed.)

All the "Web" specific additions (i.e. the whole point of this project) are contained in the 
"emscripten" subfolder (more info there). The main interface between the JavaScript/WebAudio 
world and the original C++ code is the adapter.cpp file.

An online demo can be found here: https://www.wothke.ch/spectreZX/


## Credits
This "project" is based on ZXTune (see http://zxtune.bitbucket.org/)


## Howto build
You'll need Emscripten (http://kripken.github.io/emscripten-site/docs/getting_started/downloads.html). The make script 
is designed for use of emscripten version 1.37.29 (unless you want to create WebAssembly output, older versions might 
also still work).

The below instructions assume that the 'spectrezx' project folder has been moved into the main emscripten 
installation folder (maybe not necessary) and that a command prompt has been opened within the 
project's "emscripten" sub-folder, and that the Emscripten environment vars have been previously 
set (run emsdk_env.bat).

The Web version is then built using the makeEmscripten.bat that can be found in this folder. The 
script will compile directly into the "emscripten/htdocs" example web folder, were it will create 
the backend_zxtune.js library. The content of the "htdocs" can be tested by first copying it into some 
document folder of a web server. 

## Dependencies
The current version requires version >=1.2.1 of my https://bitbucket.org/wothke/webaudio-player/


## License
This library is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or (at
your option) any later version. This library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

