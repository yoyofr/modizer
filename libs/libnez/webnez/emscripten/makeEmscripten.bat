::  POOR MAN'S DOS PROMPT BUILD SCRIPT.. make sure to delete the respective built/*.bc files before building!
::  existing *.bc files will not be recompiled. 

setlocal enabledelayedexpansion

SET ERRORLEVEL
VERIFY > NUL

:: **** use the "-s WASM" switch to compile WebAssembly output. warning: the SINGLE_FILE approach does NOT currently work in Chrome 63.. ****
set "OPT= -s WASM=1 -s ASSERTIONS=1 -s VERBOSE=0 -s FORCE_FILESYSTEM=1  -DNO_DEBUG_LOGS -DHAVE_LIMITS_H -DHAVE_STDINT_H -Wcast-align -fno-strict-aliasing -s SAFE_HEAP=0 -s DISABLE_EXCEPTION_CATCHING=0 -Wno-pointer-sign -I. -I.. -I../zlib -I../src -I../src/zlib -I../src/format -I../src/device -I../src/cpu -I../src/sdlplay -Os -O3 "
if not exist "built/zlib.bc" (
	call emcc.bat %OPT% ../src/zlib/nez.c ../src/zlib/memzip.c ../zlib/adler32.c ../zlib/compress.c ../zlib/crc32.c ../zlib/gzio.c ../zlib/uncompr.c ../zlib/deflate.c ../zlib/trees.c ../zlib/zutil.c ../zlib/inflate.c ../zlib/infback.c ../zlib/inftrees.c ../zlib/inffast.c  -o built/zlib.bc
	IF !ERRORLEVEL! NEQ 0 goto :END
)
if not exist "built/cpu.bc" (
	call emcc.bat  %OPT% ../src/cpu/kmz80/kmz80t.c ../src/cpu/kmz80/kmz80c.c ../src/cpu/kmz80/kmz80.c ../src/cpu/kmz80/kmevent.c ../src/cpu/kmz80/kmdmg.c -o built/cpu.bc
	IF !ERRORLEVEL! NEQ 0 goto :END
)
if not exist "built/device.bc" (
	call emcc.bat   %OPT%   ../src/device/opl/s_deltat.c ../src/device/opl/s_opltbl.c ../src/device/opl/s_opl.c  ../src/device/nes/s_vrc7.c ../src/device/nes/s_vrc6.c ../src/device/s_sng.c ../src/device/s_scc.c ../src/device/   s_psg.c ../src/device/nes/s_n106.c ../src/device/nes/s_mmc5.c ../src/device/s_logtbl.c ../src/device/s_hesad.c ../src/device/s_hes.c ../src/device/nes/s_fme7.c ../src/device/nes/s_fds3.c ../src/device/nes/s_fds2.c ../src/device/nes/s_fds1.c ../src/device/nes/s_fds.c ../src/device/s_dmg.c ../src/device/nes/s_apu.c ../src/device/nes/logtable.c -o built/device.bc
	IF !ERRORLEVEL! NEQ 0 goto :END
)
if not exist "built/format.bc" (
	call emcc.bat   %OPT% ../src/format/songinfo.c ../src/format/nsf6502.c  ../src/format/m_nsd.c ../src/format/m_sgc.c ../src/format/m_zxay.c ../src/format/m_nsf.c ../src/format/m_kss.c ../src/format/m_hes.c ../src/format/m_gbr.c ../src/format/audiosys.c ../src/format/handler.c -o built/format.bc
	IF !ERRORLEVEL! NEQ 0 goto :END
)
call emcc.bat %OPT% -s TOTAL_MEMORY=67108864 --closure 0 --llvm-lto 0  --memory-init-file 0  --js-library callback.js built/zlib.bc  built/cpu.bc built/device.bc built/format.bc  ../src/mbm2kss/mbm2kssx.c ../src/format/nezplug.c MetaInfoHelper.cpp Adapter.cpp   -s EXPORTED_FUNCTIONS="[ '_emu_load_file','_emu_set_loop','_emu_teardown','_emu_get_current_position','_emu_seek_position','_emu_get_max_position','_emu_set_subsong','_emu_get_track_info','_emu_get_sample_rate','_emu_get_audio_buffer','_emu_get_audio_buffer_length','_emu_compute_audio_samples', '_malloc', '_free']"  -o htdocs/nez.js  -s SINGLE_FILE=0 -s EXTRA_EXPORTED_RUNTIME_METHODS="['ccall', 'Pointer_stringify', 'UTF8ToString']"  -s BINARYEN_ASYNC_COMPILATION=1 -s BINARYEN_TRAP_MODE='clamp' && copy /b shell-pre.js + htdocs\nez.js + shell-post.js htdocs\web_nez3.js && del htdocs\nez.js && copy /b htdocs\web_nez3.js + sng_converter.js + mus_converter.js + nez_adapter.js htdocs\backend_nez.js && del htdocs\web_nez3.js
:END

