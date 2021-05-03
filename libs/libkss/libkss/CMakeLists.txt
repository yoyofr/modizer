cmake_minimum_required(VERSION 2.8)
project(libkss)

if(MSVC)
  set(CMAKE_C_FLAGS "/Ox /W3 /wd4996")
else()
  set(CMAKE_C_FLAGS "-O3 -Wall")
endif()

if(NOT TARGET emu2149) 
	add_subdirectory(modules/emu2149) 
endif()

if(NOT TARGET emu2212)
	add_subdirectory(modules/emu2212)
endif()

if(NOT TARGET emu2413)
	add_subdirectory(modules/emu2413)
endif()

if(NOT TARGET emu8950)
	add_subdirectory(modules/emu8950)
endif()

if(NOT TARGET emu76489)
	add_subdirectory(modules/emu76489)
endif()

if(NOT TARGET kmz80)
	add_subdirectory(modules/kmz80)
endif()

file(GLOB SOURCES src/*.c src/**/*.c)

include_directories(modules)

add_library(kss STATIC ${SOURCES})
target_link_libraries(kss emu2149 emu2212 emu2413 emu8950 emu76489 kmz80)

add_executable(kss2wav EXCLUDE_FROM_ALL src/kss2wav.c)
if(NOT CMAKE_COMPILER_IS_GNUCC)
	target_link_libraries(kss2wav kss)
else()
	target_link_libraries(kss2wav kss m)
endif()

