if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

project(eupmini)

if(MSVC OR MSYS)
	cmake_minimum_required(VERSION 3.10)
endif()

if(UNIX)
	cmake_minimum_required(VERSION 3.0)
endif()

#add_compile_definitions(PACKAGE=\"eupplayer\" VERSION=\"0.008\" STDC_HEADERS=1 HAVE_UNISTD_H=1 EUPPLAYER_LITTLE_ENDIAN=1)
add_definitions(-DPACKAGE=\"eupplayer\" -DVERSION=\"0.008\" -DSTDC_HEADERS=1 -DHAVE_UNISTD_H=1 -DEUPPLAYER_LITTLE_ENDIAN=1)

if(MSVC)
	add_definitions(-DSDL_MAIN_HANDLED=1)
        find_package(SDL2 CONFIG REQUIRED)
	set(CMAKE_CXX_STANDARD 11)
	set(CMAKE_CXX_STANDARD_REQUIRED YES)
endif()
if(UNIX OR MSYS)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(SDL2 REQUIRED sdl2)
	set(CMAKE_CXX_STANDARD 11)
	set(CMAKE_CXX_STANDARD_REQUIRED YES)
endif()

file(GLOB SOURCES eupplay.cpp eupplayer.cpp eupplayer_townsEmulator.cpp eupplayer_townsIinternal.cpp sintbl.cpp)

add_executable(eupplay EXCLUDE_FROM_ALL ${SOURCES})

if(MSVC)
	target_link_libraries(eupplay PRIVATE SDL2::SDL2 SDL2::SDL2main)
endif()

if(UNIX OR MSYS)
	target_link_libraries(eupplay PRIVATE stdc++ ${SDL2_LIBRARIES})
	target_include_directories(eupplay PUBLIC ${SDL2_INCLUDE_DIRS})
	target_compile_options(eupplay PUBLIC ${SDL2_FLAGS_OTHERS})
endif()
