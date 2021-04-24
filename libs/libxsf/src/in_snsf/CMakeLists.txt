set(HEADERS
	snes9x/65c816.h
	snes9x/apu/apu.h
	snes9x/apu/bapu/dsp/blargg_common.h
	snes9x/apu/bapu/dsp/blargg_config.h
	snes9x/apu/bapu/dsp/blargg_endian.h
	snes9x/apu/bapu/dsp/sdsp.hpp
	snes9x/apu/bapu/dsp/SPC_DSP.h
	snes9x/apu/bapu/smp/smp.hpp
	snes9x/apu/bapu/snes/snes.hpp
	snes9x/apu/resampler.h
	snes9x/cpuaddr.h
	snes9x/cpuexec.h
	snes9x/cpumacro.h
	snes9x/dma.h
	snes9x/getset.h
	snes9x/memmap.h
	snes9x/port.h
	snes9x/ppu.h
	snes9x/sdd1.h
	snes9x/snes9x.h
	XSFConfig_SNSF.h
	XSFConfigDialog_SNSF.h)
set(SNES9X_SOURCES
	snes9x/apu/apu.cpp
	snes9x/apu/bapu/dsp/sdsp.cpp
	snes9x/apu/bapu/dsp/SPC_DSP.cpp
	snes9x/apu/bapu/smp/algorithms.cpp
	snes9x/apu/bapu/smp/core.cpp
	snes9x/apu/bapu/smp/iplrom.cpp
	snes9x/apu/bapu/smp/memory.cpp
	snes9x/apu/bapu/smp/smp.cpp
	snes9x/apu/bapu/smp/timing.cpp
	snes9x/cpu.cpp
	snes9x/cpuexec.cpp
	snes9x/cpuops.cpp
	snes9x/dma.cpp
	snes9x/globals.cpp
	snes9x/memmap.cpp
	snes9x/ppu.cpp
	snes9x/sdd1.cpp)
set(SOURCES
	${SNES9X_SOURCES}
	XSFConfig_SNSF.cpp
	XSFConfigDialog_SNSF.cpp
	XSFPlayer_SNSF.cpp)

set(SNES9X_COMPILE_OPTIONS
	$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:-Wno-implicit-fallthrough -Wno-shift-negative-value>)

set_source_files_properties(${SNES9X_SOURCES} PROPERTIES
	COMPILE_OPTIONS "${SNES9X_COMPILE_OPTIONS}")

add_library(in_snsf SHARED ${HEADERS} ${SOURCES})
set_target_properties(in_snsf PROPERTIES PREFIX "")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${SOURCES})
target_compile_options(in_snsf PUBLIC
	$<$<CXX_COMPILER_ID:MSVC>:/wd4127 /wd4245 /wd4389 /wd6385 /wd6386 /wd26439 /wd26453 /wd26495 /wd26812 /wd26819>)
target_link_libraries(in_snsf
	in_xsf_framework)
