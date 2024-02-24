set(libbinio_la_SOURCES binio.cpp binfile.cpp binwrap.cpp binstr.cpp)
configure_file(binio.h.in ${CMAKE_SOURCE_DIR}/src/binio.h)
set(pkginclude_HEADERS binio.h binfile.h binwrap.h binstr.h)
add_library(libbinio ${libbinio_la_SOURCES} ${pkginclude_HEADERS})
