::  POOR MAN'S DOS PROMPT BUILD SCRIPT.. make sure to delete the respective built/*.bc files before building!
::  existing *.bc files will not be recompiled. 

setlocal enabledelayedexpansion

SET ERRORLEVEL
VERIFY > NUL

:: **** use the "-s WASM" switch to compile WebAssembly output. warning: the SINGLE_FILE approach does NOT currently work in Chrome 63.. ****  
set "OPT= -s WASM=1  -s DEMANGLE_SUPPORT=0 -s SAFE_HEAP=0 -s VERBOSE=0 -s ASSERTIONS=0 -s FORCE_FILESYSTEM=1 -s TOTAL_MEMORY=67108864 -Wno-pointer-sign -Dstricmp=strcasecmp -I. -I../src/ -I../src/vorbis/ -I../src/pxtone/ -I../src/organya/  -Os -O3 "

if not exist "built/vorbis.bc" (
	call emcc.bat %OPT% ../src/vorbis/mdct.c ../src/vorbis/smallft.c ../src/vorbis/block.c ../src/vorbis/envelope.c ../src/vorbis/window.c ../src/vorbis/lsp.c ../src/vorbis/lpc.c ../src/vorbis/analysis.c ../src/vorbis/synthesis.c ../src/vorbis/psy.c ../src/vorbis/info.c ../src/vorbis/floor1.c ../src/vorbis/floor0.c ../src/vorbis/res0.c ../src/vorbis/mapping0.c ../src/vorbis/registry.c ../src/vorbis/codebook.c ../src/vorbis/sharedbook.c ../src/vorbis/lookup.c ../src/vorbis/bitrate.c -o built/vorbis.bc
	IF !ERRORLEVEL! NEQ 0 goto :END
)

if not exist "built/px.bc" (
	call emcc.bat %OPT% ../src/pxtone/pxtnDelay.cpp ../src/pxtone/pxtnDescriptor.cpp ../src/pxtone/pxtnError.cpp ../src/pxtone/pxtnEvelist.cpp ../src/pxtone/pxtnMaster.cpp ../src/pxtone/pxtnMem.cpp ../src/pxtone/pxtnOverDrive.cpp ../src/pxtone/pxtnPulse_Frequency.cpp ../src/pxtone/pxtnPulse_Noise.cpp ../src/pxtone/pxtnPulse_NoiseBuilder.cpp ../src/pxtone/pxtnPulse_Oggv.cpp ../src/pxtone/pxtnPulse_Oscillator.cpp ../src/pxtone/pxtnPulse_PCM.cpp ../src/pxtone/pxtnService.cpp ../src/pxtone/pxtnService_moo.cpp ../src/pxtone/pxtnText.cpp ../src/pxtone/pxtnUnit.cpp ../src/pxtone/pxtnWoice.cpp ../src/pxtone/pxtnWoice_io.cpp ../src/pxtone/pxtnWoicePTV.cpp ../src/pxtone/pxtoneNoise.cpp  -o built/px.bc
	IF !ERRORLEVEL! NEQ 0 goto :END
)

if not exist "built/org.bc" (
	call emcc.bat %OPT% organya.c  -o built/org.bc
	IF !ERRORLEVEL! NEQ 0 goto :END
)
emcc.bat %OPT% --closure 1 --llvm-lto 1 --memory-init-file 0 built/org.bc built/vorbis.bc built/px.bc MetaInfoHelper.cpp Adapter.cpp -s EXPORTED_FUNCTIONS="['_emu_load_file','_emu_teardown','_emu_set_subsong','_emu_get_track_info','_emu_get_audio_buffer','_emu_get_audio_buffer_length','_emu_compute_audio_samples','_emu_get_current_position','_emu_seek_position','_emu_get_max_position', '_malloc', '_free']" -o htdocs/pixel.js -s SINGLE_FILE=0 -s EXTRA_EXPORTED_RUNTIME_METHODS="[ 'ccall', 'Pointer_stringify', 'UTF8ToString', 'getValue']"  -s BINARYEN_ASYNC_COMPILATION=1 -s BINARYEN_TRAP_MODE='clamp' && copy /b shell-pre.js + htdocs\pixel.js + shell-post.js htdocs\pixel3.js && del htdocs\pixel.js && copy /b htdocs\pixel3.js + pixel_adapter.js htdocs\backend_pixel.js && del htdocs\pixel3.js