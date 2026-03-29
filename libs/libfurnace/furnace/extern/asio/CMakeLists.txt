cmake_minimum_required(VERSION 3.0...3.20)
project(ASIOSDK)

option(BUILD_SAMPLE "Build a host example" OFF)

if (NOT WIN32)
  message(FATAL_ERROR "ASIO only supports Windows!")
endif()

set(COMMON_SOURCES
common/asio.cpp
common/asio.h
common/asiosys.h
common/combase.cpp
common/combase.h
common/debugmessage.cpp
common/dllentry.cpp
common/iasiodrv.h
common/register.cpp
common/wxdebug.h
)

set(HOST_SOURCES
host/ASIOConvertSamples.cpp
host/ASIOConvertSamples.h
host/asiodrivers.cpp
host/asiodrivers.h
host/ginclude.h
host/pc/asiolist.cpp
host/pc/asiolist.h
)

add_library(ASIO STATIC ${COMMON_SOURCES} ${HOST_SOURCES})
target_include_directories(ASIO
  PUBLIC
    common
    host host/pc
)

add_library(ASIO::ASIO ALIAS ASIO)

# TODO: improve include dir locating
if (BUILD_SAMPLE)
  add_executable(hostsample host/sample/hostsample.cpp)
  target_link_libraries(hostsample ASIO odbc32 odbccp32 winmm -static)
  target_include_directories(hostsample PRIVATE common host host/pc)
endif()
