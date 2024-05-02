if(MSVC)
	cmake_minimum_required(VERSION 3.10)
endif()

if(UNIX OR MINGW)
	cmake_minimum_required(VERSION 3.0)
endif()


if(MSVC)
	set(CMAKE_CXX_STANDARD 14)
	set(CMAKE_CXX_STANDARD_REQUIRED YES)
endif()
if(UNIX OR MINGW)
	set(CMAKE_CXX_STANDARD 14)
	set(CMAKE_CXX_STANDARD_REQUIRED YES)
endif()

include_directories(./.. ymfm pmdwin)

file(GLOB SOURCES *.cpp ymfm/*.cpp pmdwin/*.cpp)

add_library(pmdmini STATIC ${SOURCES})
