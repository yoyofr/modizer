cmake_minimum_required(VERSION 3.13)

project(libbinio)

include(CheckTypeSize)
check_type_size("long long" SIZEOF_LONG_LONG)
if(HAVE_SIZEOF_LONG_LONG)
    set(TYPE_INT "long long")
else()
    set(TYPE_INT "long")
endif()
check_type_size("long double" SIZEOF_LONG_DOUBLE)
if(HAVE_SIZEOF_LONG_DOUBLE)
    set(TYPE_FLOAT "long double")
else()
    set(TYPE_FLOAT "double")
endif()

# TODO: check library exists
set(ENABLE_IOSTREAM 1)
set(ENABLE_STRING 1)
set(ISO_STDLIB 1)
set(WITH_MATH 1)

add_subdirectory(src)
