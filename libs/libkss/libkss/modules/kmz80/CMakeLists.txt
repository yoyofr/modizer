cmake_minimum_required(VERSION 2.8)

if(MSVC)
  set(CMAKE_C_FLAGS "/Ox /W3 /wd4996")
else()
  set(CMAKE_C_FLAGS "-O3 -Wall")
endif()

add_library(kmz80 STATIC 
  kmdmg.c
  kmevent.c
  kmr800.c
  kmz80.c
  kmz80c.c
  kmz80t.c
)
