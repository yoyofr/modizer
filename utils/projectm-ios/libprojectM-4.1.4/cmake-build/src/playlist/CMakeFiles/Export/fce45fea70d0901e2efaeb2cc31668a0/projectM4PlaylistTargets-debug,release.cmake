#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug,Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libprojectM::playlist" for configuration "Debug,Release"
set_property(TARGET libprojectM::playlist APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG,RELEASE)
set_target_properties(libprojectM::playlist PROPERTIES
  IMPORTED_LOCATION_DEBUG,RELEASE "${_IMPORT_PREFIX}/lib/libprojectM-4-playlist.4.1.4.dylib"
  IMPORTED_SONAME_DEBUG,RELEASE "@rpath/libprojectM-4-playlist.4.dylib"
  )

list(APPEND _cmake_import_check_targets libprojectM::playlist )
list(APPEND _cmake_import_check_files_for_libprojectM::playlist "${_IMPORT_PREFIX}/lib/libprojectM-4-playlist.4.1.4.dylib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
