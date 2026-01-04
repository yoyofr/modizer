# webNEZ

Copyright (C) 2018 Juergen Wothke

This is a JavaScript/WebAudio plugin of "NEZplug++" used to emulate music of various consoles: Famicom (.nes), 
Game Gear, Game Boy, MSX (.kss), ZX Spectrum (.ay), Hudson (.hes), etc. This plugin is designed to work with my generic WebAudio 
ScriptProcessor music player (see separate project). 

It supposedly emulates the following sound chips:

        Richo     NES-APU(RP2A03H)             [Famicom(NES)]
        Konami    VRC6(053328 VRC VI)          Castle vania 3, Madara, ...
        Konami    VRC7(053982 VRC VII)         Lagrange Point
        Nintendo  MMC5                         Just Breed, Yakuman Tengoku
        Richo     FDS(RP2C33A)                 Zelda(FDS), Dabada(FDS), ...
        Namco     NAMCOT106                    D.D.S.2, KingOfKings, ...
        Sunsoft   FME7                         Gimmick!

        GI        PSG(AY-3-8910)               [MSX] [ZX Spectrum]
        Yamaha    SSG(YM2149)                  [Amstrad CPC]
        Yamaha    FM-PAC(YM2413)               Aleste, Xak, ...
        Yamaha    MSX-AUDIO(Y8950)             Labyrinth, Xevious, ...
        Konami    SCC(051649 SCC-I2212)        Nemesis2, Space Manbow, ...
        Konami    SCC+(052539 SCC-I2312)       SD Snatcher, ...
        Konami    MajutushiDAC                 Majutushi

        TI        SMS-SNG(SN76496)             [SEGA-MKIII(SMS)]
        Yamaha    FM-UNIT(YM2413)              Shinobi, Tri Formation, ...
        Sega      GG-SNG                       [Game Gear]

        Richo     DMG-SYSTEM                   [Game Boy]

        Hudson    HES-PSG(HuC6280)             [PC-Engine(TG16)]



Support for these additional music formats has been added by means of converters:

        FAC SoundTracker (.mus/.sm1/.sm2) - not to be confused with the multitude of other MUS formats
		
		
A live demo of this program can be found here: http://www.wothke.ch/webNEZ/


## Credits
The project is based on NEZplug++ 0.9.4.8 + 3 + 24.00: http://offgao.net/program/nezplug++.html

## Known limitations
There seems to be some bug in the re-initialization logic: A .KSS song that plays perfectly fine 
when played directly makes noises when played after *specific* types of other songs.
Test case: 1) "battlestar galactica theme.kss" - 2 channel MSX stuff 2) then 
"4th Unit 4 - Zero, The (MSX)(1990)(Data West).kss" - 1 channel MSX stuff 3) back to 1) that song
no longer works. It seems some leftover garbage from the previous song is not properly 
reinitialized when loading a new song in some corner of the emulation..


## Howto build

You'll need Emscripten (http://kripken.github.io/emscripten-site/docs/getting_started/downloads.html). The make script 
is designed for use of emscripten version 1.37.29 (unless you want to create WebAssembly output, older versions might 
also still work).

The below instructions assume that the webNEZ project folder has been moved into the main emscripten 
installation folder (maybe not necessary) and that a command prompt has been opened within the 
project's "emscripten" sub-folder, and that the Emscripten environment vars have been previously 
set (run emsdk_env.bat).

The Web version is then built using the makeEmscripten.bat that can be found in this folder. The 
script will compile directly into the "emscripten/htdocs" example web folder, were it will create 
the backend_nez.js library. (To create a clean-build you have to delete any previously built libs in the 
'built' sub-folder!) The content of the "htdocs" can be tested by first copying it into some 
document folder of a web server. 


## Depencencies

The current version requires version >=1.2.1 of my https://bitbucket.org/wothke/webaudio-player/

This project comes without any music files, so you'll also have to get your own and place them
in the htdocs/music folder (you can configure them in the 'songs' list in index.html).


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
