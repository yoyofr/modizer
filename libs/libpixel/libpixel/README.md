# webPixel

Copyright (C) 2019-2023 Juergen Wothke

This is a JavaScript/WebAudio plugin of "PxTone Collage" & "Organya". This plugin is designed to work with my generic WebAudio 
ScriptProcessor music player (see separate project) but the API exposed by the lib can be used in any 
JavaScript program (it should look familiar to anyone that has ever done some sort of music player plugin). 

Files that can be played are: .pttune, ptcop, .org


A live demo of this program can be found here: http://www.wothke.ch/webPixel/


## Credits

The project is based on: Daisuke "Pixel" Amaya's "PxTone Collage" player and JTE's Organya player http://www.cavestory.org/downloads/in_org108config.zip


## Project

The original PxTone Collage & Organya sources can be found in the 'src' folder. All the Web specific add-on stuff is in the emscripten folder.


## Howto build

You'll need Emscripten (http://kripken.github.io/emscripten-site/docs/getting_started/downloads.html). The make script 
is designed for use of emscripten version 1.37.29 (unless you want to create WebAssembly output, older versions might 
also still work).

The below instructions assume that the webPixel project folder has been moved into the main emscripten 
installation folder (maybe not necessary) and that a command prompt has been opened within the 
project's "emscripten" sub-folder, and that the Emscripten environment vars have been previously 
set (run emsdk_env.bat).

The Web version is then built using the makeEmscripten.bat that can be found in this folder. The 
script will compile directly into the "emscripten/htdocs" example web folder, were it will create 
the backend_pixel.js library - and an additional pixel.wasm if you are compiling WASM output (This can be enabled in the 
makeEmscripten.bat to generate WASM instead of asm.js.). 
The content of the "htdocs" can be tested by first copying it into some 
document folder of a web server. 


## Depencencies

The current version requires version >=1.2.1 of my https://bitbucket.org/wothke/webaudio-player/

This project comes without any music files, so you'll also have to get your own and place them
in the htdocs/music folder (you can configure them in the 'songs' list in index.html).


## License for PxTone player (extends to my respective JavaScript "backend_pixel" code here)

Copyright (c) 2016 STUDIO PIXEL

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files
(the "Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

## License for Organya player

            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
                    Version 2, December 2004

 Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>

 Everyone is permitted to copy and distribute verbatim or modified
 copies of this license document, and changing it is allowed as long
 as the name is changed.

            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

  0. You just DO WHAT THE FUCK YOU WANT TO.

## License for OggVorbis (used in PxTone)

USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS GOVERNED BY A 
BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE IN 'COPYING'. 
PLEASE READ THESE TERMS BEFORE DISTRIBUTING.

THE OggVorbis SOURCE CODE IS COPYRIGHT (C) 1994-2018 by the Xiph.Org Foundation https://www.xiph.org/

## License for example code
The code in the htdocs folder is provided as an example and separate licensing conditions may
apply to the respective artefacts.