::  POOR MAN'S DOS PROMPT BUILD SCRIPT.. make sure to delete the respective built/*.bc files before building!
::  existing *.bc files will not be recompiled. 

:: EMSCRIPTEN does no allow use of the features that might elsewhere be activated using -DNEW_DYNAREC -DDYNAREC -DARCH_MIN_SSE2
:: the respective "dynarec" specific files have therefore completely been trown out of this project..
setlocal enabledelayedexpansion

SET ERRORLEVEL
VERIFY > NUL

:: **** use the "-s WASM" switch to compile WebAssembly output. warning: the SINGLE_FILE approach does NOT currently work in Chrome 63.. ****
set "OPT= -s WASM=1 -s ASSERTIONS=0 -s FORCE_FILESYSTEM=1 -Wcast-align -fno-strict-aliasing -s VERBOSE=0 -s SAFE_HEAP=0 -s DISABLE_EXCEPTION_CATCHING=0 -DHAVE_STDINT_H -DNO_DEBUG_LOGS -Wno-pointer-sign -I. -I.. -I../vio2sf/desmume/ -I../psflib -I../zlib  -Os -O3 "
set "OPT2=  -DHAVE_CRC32 -DDISABLE_THREADING %OPT%"
if not exist "built/extra.bc" (
	call emcc.bat %OPT% ../psflib/psflib.c ../psflib/psf2fs.c ../zlib/adler32.c ../zlib/compress.c ../zlib/crc32.c ../zlib/gzio.c ../zlib/uncompr.c ../zlib/deflate.c ../zlib/trees.c ../zlib/zutil.c ../zlib/inflate.c ../zlib/infback.c ../zlib/inftrees.c ../zlib/inffast.c -o built/extra.bc
	IF !ERRORLEVEL! NEQ 0 goto :END
)

if not exist "built/core.bc" (
	call emcc.bat %OPT2%  ../vio2sf/desmume/arm_instructions.c ../vio2sf/desmume/armcpu.c ../vio2sf/desmume/barray.c ../vio2sf/desmume/bios.c ../vio2sf/desmume/cp15.c ../vio2sf/desmume/FIFO.c ../vio2sf/desmume/GPU.c ../vio2sf/desmume/isqrt.c ../vio2sf/desmume/matrix.c ../vio2sf/desmume/mc.c ../vio2sf/desmume/MMU.c ../vio2sf/desmume/NDSSystem.c ../vio2sf/desmume/resampler.c ../vio2sf/desmume/state.c ../vio2sf/desmume/thumb_instructions.c ../vio2sf/desmume/SPU.cpp -o built/core.bc
	IF !ERRORLEVEL! NEQ 0 goto :END
)

:: note: emulator seems to be a memory hog.. 128MB wasn't enough for many songs..
call emcc.bat %OPT2%  -s DEMANGLE_SUPPORT=0 -s ALLOW_MEMORY_GROWTH=1 --closure 1 --llvm-lto 1  -s TOTAL_MEMORY=268435456 --memory-init-file 0  built/extra.bc built/core.bc twosfplug.cpp MetaInfoHelper.cpp Adapter.cpp --js-library callback.js  -s EXPORTED_FUNCTIONS="['_emu_load_file','_emu_set_boost','_emu_teardown','_emu_get_current_position','_emu_seek_position','_emu_get_max_position','_emu_set_subsong','_emu_get_track_info','_emu_get_sample_rate','_emu_get_audio_buffer','_emu_get_audio_buffer_length','_emu_compute_audio_samples', '_malloc', '_free']"  -o htdocs/2sf.js  -s SINGLE_FILE=0 -s EXTRA_EXPORTED_RUNTIME_METHODS="['ccall', 'Pointer_stringify', 'UTF8ToString']"  -s BINARYEN_ASYNC_COMPILATION=1 -s BINARYEN_TRAP_MODE='clamp' && copy /b shell-pre.js + htdocs\2sf.js + shell-post.js htdocs\web_2sf3.js && del htdocs\2sf.js && copy /b htdocs\web_2sf3.js + 2sf_adapter.js htdocs\backend_ds.js && del htdocs\web_2sf3.js
:END