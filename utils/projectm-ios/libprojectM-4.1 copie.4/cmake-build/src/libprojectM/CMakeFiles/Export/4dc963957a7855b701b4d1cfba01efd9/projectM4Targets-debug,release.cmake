#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug,Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libprojectM::projectM" for configuration "Debug,Release"
set_property(TARGET libprojectM::projectM APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG,RELEASE)
set_target_properties(libprojectM::projectM PROPERTIES
  IMPORTED_LOCATION_DEBUG,RELEASE "${_IMPORT_PREFIX}/lib/libprojectM-4.4.1.4.dylib"
  IMPORTED_SONAME_DEBUG,RELEASE "@rpath/libprojectM-4.4.dylib"
  )

list(APPEND _cmake_import_check_targets libprojectM::projectM )
list(APPEND _cmake_import_check_files_for_libprojectM::projectM "${_IMPORT_PREFIX}/lib/libprojectM-4.4.1.4.dylib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
