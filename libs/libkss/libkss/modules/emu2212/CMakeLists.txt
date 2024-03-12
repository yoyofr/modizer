cmake_minimum_required(VERSION 3.0)

if(MSVC)
  set(CMAKE_C_FLAGS "/Ox /W3 /wd4996")
else()
  set(CMAKE_C_FLAGS "-O3 -Wall")
endif()

add_library(emu2212 STATIC emu2212.c)
