::  POOR MAN'S DOS PROMPT BUILD SCRIPT.. make sure to delete the respective *.bc files before building 
::  existing *.bc files will not be recompiled. Unfortunately the script occasionally 
::  fails for no good reason - this must be the wonderful world of DOS/Win... ;-)

::  examples of windows bullshit bingo (i.e. the same compile works when started again..): 
::  1) error msg: "'_zxtune.html' is not recognized as an internal or external command, 
::  operable program or batch file." -> for some reason the last command line was arbitrarily cut in two..
::  2) The system cannot find the path specified.
::  3) WindowsError: [Error 5] Access is denied
::  4) cpp: error: CreateProcess: No such file or directory


setlocal enabledelayedexpansion

SET ERRORLEVEL
VERIFY > NUL


:: **** use the "-s WASM" switch to compile WebAssembly output. warning: the SINGLE_FILE approach does NOT currently work in Chrome 63.. ****
:: note: for older emscripten versions (which come without std::exception) you may need to add -DBLOODY_HACK

set "OPT2= -s WASM=1 -s ASSERTIONS=0 -Wcast-align -fno-strict-aliasing -s VERBOSE=0 -s FORCE_FILESYSTEM=1 -s SAFE_HEAP=0 -s DISABLE_EXCEPTION_CATCHING=0 -DNO_DEBUG_LOGS  -DBOOST_NO_RTTI -DBOOST_SYSTEM_NO_DEPRECATED -DNO_L10N -DBOOST_ERROR_CODE_HEADER_ONLY -Wno-pointer-sign -I. -I.. -I../include -I./include -I../3rdparty/zlib/ -I../3rdparty/z80ex/ -I../3rdparty/z80ex/include -I../3rdparty/curl/ -I../boost/ -I../src/  -Os -O3 --memory-init-file 0 "
set "OPT=%OPT2% -DBOOST_NO_STD_MIN_MAX" 

if not exist "built/thirdparty.bc" (
	call emcc.bat -DZ80EX_VERSION_STR='1.1.19pre1' -DZ80EX_API_REVISION=1 -DZ80EX_VERSION_MAJOR=1 -DZ80EX_VERSION_MINOR=19 -DZ80EX_RELEASE_TYPE=pre1 %OPT%  ../3rdparty/z80ex/z80ex.c ../3rdparty/lhasa/lib/crc16.c ../3rdparty/lhasa/lib/ext_header.c ../3rdparty/lhasa/lib/lha_arch_unix.c ../3rdparty/lhasa/lib/lha_arch_win32.c ../3rdparty/lhasa/lib/lha_decoder.c ../3rdparty/lhasa/lib/lha_endian.c ../3rdparty/lhasa/lib/lha_file_header.c ../3rdparty/lhasa/lib/lha_input_stream.c ../3rdparty/lhasa/lib/lha_basic_reader.c ../3rdparty/lhasa/lib/lha_reader.c ../3rdparty/lhasa/lib/macbinary.c ../3rdparty/lhasa/lib/null_decoder.c ../3rdparty/lhasa/lib/lh1_decoder.c ../3rdparty/lhasa/lib/lh5_decoder.c ../3rdparty/lhasa/lib/lh6_decoder.c ../3rdparty/lhasa/lib/lh7_decoder.c ../3rdparty/lhasa/lib/lz5_decoder.c ../3rdparty/lhasa/lib/lzs_decoder.c ../3rdparty/lhasa/lib/pm2_decoder.c ../3rdparty/zlib/adler32.c ../3rdparty/zlib/compress.c ../3rdparty/zlib/crc32.c ../3rdparty/zlib/gzio.c ../3rdparty/zlib/uncompr.c ../3rdparty/zlib/deflate.c ../3rdparty/zlib/trees.c ../3rdparty/zlib/zutil.c ../3rdparty/zlib/inflate.c ../3rdparty/zlib/infback.c ../3rdparty/zlib/inftrees.c ../3rdparty/zlib/inffast.c -r -o built/thirdparty.bc
	IF !ERRORLEVEL! NEQ 0 goto :END
)

if not exist "built/binary.bc" (
	call emcc.bat  -std=c++11 %OPT2% ../src/binary/src/base64.cpp ../src/binary/src/compress.cpp ../src/binary/src/container.cpp ../src/binary/format/composite.cpp ../src/binary/format/expression.cpp ../src/binary/format/grammar.cpp ../src/binary/format/lexic_analysis.cpp ../src/binary/format/matching.cpp ../src/binary/format/scanning.cpp ../src/binary/format/static_expression.cpp ../src/binary/format/syntax.cpp  -r  -o built/binary.bc
	IF !ERRORLEVEL! NEQ 0 goto :END
)

if not exist "built/io.bc" (
	call emcc.bat -std=c++11 %OPT2% ../src/io/impl/parameters.cpp ../src/io/impl/template.cpp ../src/io/text/io.cpp ../src/io/providers/enumerator.cpp ../src/io/providers/file_provider.cpp ../src/io/providers/network_stub.cpp    -r  -o built/io.bc
	IF !ERRORLEVEL! NEQ 0 goto :END
)

if not exist "built/formats.bc" (
	call emcc.bat -std=c++11 %OPT2% ../src/formats/chiptune/builder_meta_stub.cpp  ../src/formats/chiptune/aym/globaltracker.cpp ../src/formats/chiptune/builder_pattern_stub.cpp ../src/formats/chiptune/metainfo.cpp ../src/formats/chiptune/aym/ym_vtx.cpp  ../src/formats/chiptune/aym/soundtrackerpro_compiled.cpp ../src/formats/chiptune/aym/soundtracker_compiled.cpp ../src/formats/chiptune/aym/soundtracker.cpp ../src/formats/chiptune/aym/soundtracker3.cpp ../src/formats/chiptune/aym/psg.cpp ../src/formats/chiptune/aym/protracker3_vortex.cpp ../src/formats/chiptune/aym/protracker3_compiled.cpp ../src/formats/chiptune/aym/protracker2.cpp ../src/formats/chiptune/aym/protracker1.cpp ../src/formats/chiptune/aym/prosoundmaker_compiled.cpp ../src/formats/chiptune/aym/prosoundcreator.cpp ../src/formats/chiptune/aym/fasttracker.cpp ../src/formats/chiptune/aym/ay.cpp ../src/formats/chiptune/aym/ascsoundmaster.cpp ../src/formats/chiptune/aym/turbosound.cpp ../src/formats/chiptune/aym/sqtracker_compiled.cpp   ../src/formats/chiptune/digital/sqdigitaltracker.cpp ../src/formats/chiptune/digital/sampletracker.cpp ../src/formats/chiptune/digital/prodigitracker.cpp ../src/formats/chiptune/digital/extremetracker1.cpp ../src/formats/chiptune/digital/digitalstudio.cpp ../src/formats/chiptune/digital/digitalmusicmaker.cpp ../src/formats/chiptune/digital/digital.cpp ../src/formats/chiptune/digital/chiptracker.cpp ../src/formats/chiptune/fm/tfc.cpp ../src/formats/chiptune/fm/tfd.cpp ../src/formats/chiptune/fm/tfmmusicmaker.cpp ../src/formats/chiptune/saa/etracker.cpp  ../src/formats/packed/zxzip_file.cpp ../src/formats/packed/zip_file.cpp ../src/formats/packed/z80.cpp ../src/formats/packed/turbolz.cpp ../src/formats/packed/trush.cpp ../src/formats/packed/teledisk.cpp ../src/formats/packed/sna128.cpp ../src/formats/packed/powerfullcodedecreaser6.cpp ../src/formats/packed/pack2.cpp ../src/formats/packed/mspack.cpp ../src/formats/packed/megalz.cpp ../src/formats/packed/lzs.cpp ../src/formats/packed/lzh.cpp ../src/formats/packed/image_utils.cpp ../src/formats/packed/hrust2.cpp ../src/formats/packed/hrust1.cpp ../src/formats/packed/hrum.cpp ../src/formats/packed/hobeta.cpp ../src/formats/packed/gamepacker.cpp ../src/formats/packed/fulldiskimage.cpp ../src/formats/packed/esvcruncher.cpp ../src/formats/packed/datasquieezer.cpp ../src/formats/packed/compressorcode4.cpp ../src/formats/packed/compiledstp.cpp ../src/formats/packed/compiledst3.cpp ../src/formats/packed/compiledptu13.cpp ../src/formats/packed/compiledpt2.cpp ../src/formats/packed/compiledasc.cpp ../src/formats/packed/codecruncher3.cpp ../src/formats/packed/charpres.cpp ../src/formats/text/packed.cpp ../src/formats/text/chiptune.cpp ../src/formats/text/archived.cpp ../src/formats/text/image.cpp  ../src/formats/image/lasercompact52.cpp ../src/formats/image/lasercompact40.cpp  ../src/formats/image/ascscreencrusher.cpp  ../src/formats/archived/hrip.cpp  ../src/formats/archived/multiay.cpp ../src/formats/archived/scl.cpp ../src/formats/archived/trd.cpp ../src/formats/archived/trdos_catalogue.cpp ../src/formats/archived/trdos_utils.cpp ../src/formats/archived/zip.cpp ../src/formats/archived/zxstate.cpp ../src/formats/archived/zxzip.cpp ../src/formats/archived/lha.cpp ../src/formats/packed/lha_file.cpp	-r  -o built/formats.bc
	IF !ERRORLEVEL! NEQ 0 goto :END
)

if not exist "built/devices.bc" (
	call emcc.bat -std=c++11 %OPT2%  ../src/devices/aym/src/turbosound.cpp ../src/devices/aym/src/aym.cpp  ../src/devices/beeper/beeper.cpp ../src/devices/dac/dac.cpp ../src/devices/dac/sample.cpp  ../src/devices/fm/fm.cpp  ../src/devices/fm/tfm.cpp  ../src/devices/fm/Ym2203_Emu.cpp  ../src/devices/saa/saa.cpp  ../src/devices/z80/z80.cpp    -r  -o built/devices.bc
	IF !ERRORLEVEL! NEQ 0 goto :END
)

if not exist "built/core.bc" (
	call emcc.bat -std=c++11 %OPT2% ../src/core/plugins/players/ay/st3_supp.cpp   ../src/core/text/core.cpp ../src/core/text/plugins.cpp  ../src/core/plugins/players/ay/ts_supp.cpp  ../src/core/plugins/players/ay/pt1_supp.cpp  ../src/core/plugins/players/ay/pt2_supp.cpp  ../src/core/plugins/players/ay/gtr_supp.cpp ../src/core/conversion/aym.cpp ../src/core/properties/path.cpp  ../src/core/src/location_nested.cpp ../src/core/src/location_open.cpp ../src/core/plugins/players/ay/pt3_supp.cpp ../src/core/plugins/players/ay/ts_base.cpp  ../src/core/src/module_detect.cpp ../src/core/src/parameters.cpp ../src/core/plugins/players/ay/soundtracker.cpp  ../src/core/plugins/enumerator.cpp ../src/core/plugins/containers/container_supp_common.cpp   ../src/core/plugins/containers/plugins.cpp ../src/core/plugins/containers/raw_supp.cpp  ../src/core/plugins/containers/zdata_supp.cpp  register_plugins.cpp 	../src/core/plugins/players/analyzer.cpp ../src/core/plugins/players/iterator.cpp  ../src/core/plugins/players/module_properties.cpp ../src/core/plugins/players/plugin.cpp  ../src/core/plugins/players/streaming.cpp ../src/core/plugins/players/tracking.cpp  ../src/core/plugins/players/ay/asc_supp.cpp ../src/core/plugins/players/ay/ay_supp.cpp  ../src/core/plugins/players/ay/aym_base.cpp ../src/core/plugins/players/ay/aym_base_stream.cpp  ../src/core/plugins/players/ay/aym_base_track.cpp ../src/core/plugins/players/ay/aym_parameters.cpp ../src/core/plugins/players/ay/aym_plugin.cpp  ../src/core/plugins/players/ay/freq_tables.cpp ../src/core/plugins/players/ay/ftc_supp.cpp  ../src/core/plugins/players/ay/psc_supp.cpp ../src/core/plugins/players/ay/psg_supp.cpp  ../src/core/plugins/players/ay/psm_supp.cpp  ../src/core/plugins/players/ay/soundtrackerpro.cpp ../src/core/plugins/players/ay/sqt_supp.cpp  ../src/core/plugins/players/ay/st1_supp.cpp ../src/core/plugins/players/ay/stc_supp.cpp  ../src/core/plugins/players/ay/stp_supp.cpp ../src/core/plugins/players/ay/vortex_base.cpp  ../src/core/plugins/players/ay/vtx_supp.cpp  ../src/core/plugins/players/dac/chi_supp.cpp  ../src/core/plugins/players/dac/dac_base.cpp ../src/core/plugins/players/dac/dac_parameters.cpp  ../src/core/plugins/players/dac/dac_plugin.cpp ../src/core/plugins/players/dac/dmm_supp.cpp  ../src/core/plugins/players/dac/dst_supp.cpp ../src/core/plugins/players/dac/et1_supp.cpp  ../src/core/plugins/players/dac/pdt_supp.cpp ../src/core/plugins/players/dac/sqd_supp.cpp  ../src/core/plugins/players/dac/str_supp.cpp ../src/core/plugins/players/saa/cop_supp.cpp  ../src/core/plugins/players/saa/saa_base.cpp ../src/core/plugins/players/saa/saa_parameters.cpp ../src/core/plugins/players/tfm/tfc_supp.cpp ../src/core/plugins/players/tfm/tfd_supp.cpp  ../src/core/plugins/players/tfm/tfe_supp.cpp ../src/core/plugins/players/tfm/tfm_base.cpp  ../src/core/plugins/players/tfm/tfm_base_stream.cpp  ../src/core/plugins/players/tfm/tfm_base_track.cpp  ../src/core/plugins/players/tfm/tfm_parameters.cpp ../src/core/plugins/players/tfm/tfm_plugin.cpp ../src/sound/impl/gainer.cpp ../src/sound/impl/mixer.cpp ../src/sound/impl/mixer_params.cpp  ../src/sound/impl/parameters.cpp ../src/sound/impl/render_params.cpp -r  -o built/core.bc
	IF !ERRORLEVEL! NEQ 0 goto :END
)

if not exist "built/core_packed.bc" (
	call emcc.bat -std=c++11 %OPT2% ../src/core/plugins/archives/full/archive_plugins.cpp ../src/core/plugins/archives/archive_supp_common.cpp  ../src/core/plugins/archives/plugins.cpp  -r  -o built/core_packed.bc
	IF !ERRORLEVEL! NEQ 0 goto :END
)

if not exist "built/other.bc" (
	call emcc.bat -std=c++11 %OPT2% ../src/strings/text/time_format.cpp ../src/l10n/stub/stub.cpp ../src/strings/src/format.cpp ../src/strings/src/template.cpp ../src/tools/src/crc.cpp ../src/tools/text/tools.cpp ../src/tools/src/error.cpp ../src/tools/src/progress_callback.cpp ../src/tools/src/range_checker.cpp  ../src/debug/src/log.cpp ../src/analysis/src/result.cpp  ../src/analysis/src/path.cpp  ../src/analysis/src/scanner.cpp  ../src/parameters/src/container.cpp ../src/parameters/src/convert.cpp  ../src/parameters/src/merged_accessor.cpp ../src/parameters/src/serialize.cpp  ../src/parameters/src/tracking.cpp   -r  -o built/other.bc
	IF !ERRORLEVEL! NEQ 0 goto :END
)

if not exist "built/adapter.bc" (
	call emcc.bat  -std=c++11 %OPT2%   Spectre.cpp MetaInfoHelper.cpp Adapter.cpp    -r  -o built/adapter.bc
	IF !ERRORLEVEL! NEQ 0 goto :END
)

call emcc.bat %OPT% -s TOTAL_MEMORY=33554432  --closure 1 --llvm-lto 1 built/thirdparty.bc built/binary.bc built/io.bc built/formats.bc  built/devices.bc built/core.bc built/core_packed.bc built/other.bc built/adapter.bc  -s EXPORTED_FUNCTIONS="['_emu_load_file','_emu_get_sample_rate','_emu_get_current_position','_emu_get_max_position','_emu_seek_position','_emu_teardown','_emu_set_subsong','_emu_get_track_info','_emu_get_audio_buffer','_emu_get_audio_buffer_length','_emu_compute_audio_samples', '_malloc', '_free']"  -o htdocs/zxtune.js -s SINGLE_FILE=0 -s EXTRA_EXPORTED_RUNTIME_METHODS="['ccall', 'Pointer_stringify']"  -s BINARYEN_ASYNC_COMPILATION=1 -s BINARYEN_TRAP_MODE='clamp' && copy /b shell-pre.js + htdocs\zxtune.js + shell-post.js htdocs\web_zxtune3.js && del htdocs\zxtune.js && copy /b htdocs\web_zxtune3.js + zxtune_adapter.js htdocs\backend_zxtune.js && del htdocs\web_zxtune3.js

:END