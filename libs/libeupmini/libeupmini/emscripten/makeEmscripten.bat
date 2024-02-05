::  POOR MAN'S DOS PROMPT BUILD SCRIPT.. make sure to delete the respective built/*.bc files before building!
::  existing *.bc files will not be recompiled. 

setlocal enabledelayedexpansion

SET ERRORLEVEL
VERIFY > NUL

:: **** use the "-s WASM" switch to compile WebAssembly output. warning: the SINGLE_FILE approach does NOT currently work in Chrome 63.. ****
set "OPT= -s WASM=1 -s DEMANGLE_SUPPORT=0 -s ASSERTIONS=1 -s VERBOSE=1 -s FORCE_FILESYSTEM=1 -DSNDDEV_SELECT -DSNDDEV_YM2612 -DEMSCRIPTEN -DNO_DEBUG_LOGS -DHAVE_LIMITS_H -DHAVE_STDINT_H -Wcast-align -fno-strict-aliasing -s SAFE_HEAP=1 -s DISABLE_EXCEPTION_CATCHING=0 -Wno-pointer-sign -I. -I.. -I../mame -I../Nuked-OPN2 -I../eupmini  -Os -O2 "

:: note: this "unnessearily" also compiles the unused old FM emulation (as well as Nuked-OPN2) - optimizer will ditch that unused code and but makes it more convenient to switch
call emcc.bat %OPT% -s TOTAL_MEMORY=67108864 --closure 1 --llvm-lto 1  --memory-init-file 0  --js-library callback.js   ../Nuked-OPN2/ym3438.c  ../mame/fmopn.c  ../eupmini/opn2.c ../eupmini/TownsFmEmulator2.cpp ../eupmini/TownsFmEmulator.cpp ../eupmini/eupplayer.cpp ../eupmini/eupplayer_townsEmulator.cpp ../eupmini/sintbl.cpp MetaInfoHelper.cpp Adapter.cpp   -s EXPORTED_FUNCTIONS="[ '_emu_load_file','_emu_teardown','_emu_get_current_position','_emu_seek_position','_emu_get_max_position','_emu_set_subsong','_emu_get_track_info','_emu_get_sample_rate','_emu_get_audio_buffer','_emu_get_audio_buffer_length','_emu_compute_audio_samples', '_malloc', '_free']"  -o htdocs/eup.js  -s SINGLE_FILE=0 -s EXTRA_EXPORTED_RUNTIME_METHODS="['ccall', 'Pointer_stringify', 'UTF8ToString']"  -s BINARYEN_ASYNC_COMPILATION=1 -s BINARYEN_TRAP_MODE='clamp' && copy /b shell-pre.js + htdocs\eup.js + shell-post.js htdocs\web_eup3.js && del htdocs\eup.wasm.pre && del htdocs\eup.js && copy /b htdocs\web_eup3.js + eup_adapter.js htdocs\backend_eup.js && del htdocs\web_eup3.js
:END

