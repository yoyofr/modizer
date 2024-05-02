if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

project(pmdmini)

if(MSVC OR MINGW)
	cmake_minimum_required(VERSION 3.10)
endif()

if(UNIX)
	cmake_minimum_required(VERSION 3.0.2)
endif()

if(MSVC)
	find_package(SDL2 CONFIG REQUIRED)
	set(CMAKE_CXX_STANDARD 14)
	set(CMAKE_CXX_STANDARD_REQUIRED YES)
endif()

if(UNIX OR MINGW)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(SDL2 REQUIRED sdl2)
	set(CMAKE_CXX_STANDARD 14)
	set(CMAKE_CXX_STANDARD_REQUIRED YES)
	include(GNUInstallDirs OPTIONAL)
endif()

add_subdirectory(src)

file(GLOB SOURCES sdlplay.cpp)

include_directories(src src/ymfm src/pmdwin)

add_executable(pmdplay EXCLUDE_FROM_ALL ${SOURCES})

if(MSVC)
	target_link_libraries(pmdplay PRIVATE pmdmini SDL2::SDL2 SDL2::SDL2main)
endif()

if(UNIX OR MINGW)
	target_link_libraries(pmdplay PRIVATE pmdmini m ${SDL2_LDFLAGS})
	target_include_directories(pmdplay PUBLIC ${SDL2_INCLUDE_DIRS})
	target_compile_options(pmdplay PUBLIC ${SDL2_FLAGS} ${SDL2_FLAGS_OTHER})
	install(TARGETS pmdplay DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

