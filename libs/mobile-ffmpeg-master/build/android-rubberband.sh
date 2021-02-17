#!/bin/bash

if [[ -z ${ANDROID_NDK_ROOT} ]]; then
    echo -e "(*) ANDROID_NDK_ROOT not defined\n"
    exit 1
fi

if [[ -z ${ARCH} ]]; then
    echo -e "(*) ARCH not defined\n"
    exit 1
fi

if [[ -z ${API} ]]; then
    echo -e "(*) API not defined\n"
    exit 1
fi

if [[ -z ${BASEDIR} ]]; then
    echo -e "(*) BASEDIR not defined\n"
    exit 1
fi

# ENABLE COMMON FUNCTIONS
. ${BASEDIR}/build/android-common.sh

# PREPARE PATHS & DEFINE ${INSTALL_PKG_CONFIG_DIR}
LIB_NAME="rubberband"
set_toolchain_clang_paths ${LIB_NAME}

# PREPARING FLAGS
BUILD_HOST=$(get_build_host)
CFLAGS=$(get_cflags ${LIB_NAME})
CXXFLAGS=$(get_cxxflags ${LIB_NAME})
LDFLAGS=$(get_ldflags ${LIB_NAME})
export PKG_CONFIG_LIBDIR=${INSTALL_PKG_CONFIG_DIR}

cd ${BASEDIR}/src/${LIB_NAME} || exit 1

make distclean 2>/dev/null 1>/dev/null

# DISABLE OPTIONAL FEATURES MANUALLY, SINCE ./configure DOES NOT PROVIDE OPTIONS FOR THEM
rm -f ${BASEDIR}/src/${LIB_NAME}/configure.ac
cp ${BASEDIR}/tools/make/rubberband/configure.ac ${BASEDIR}/src/${LIB_NAME}/configure.ac
rm -f ${BASEDIR}/src/${LIB_NAME}/Makefile.in
cp ${BASEDIR}/tools/make/rubberband/Makefile.android.in ${BASEDIR}/src/${LIB_NAME}/Makefile.in

# FIX PACKAGE CONFIG FILE DEPENDENCIES
rm -f ${BASEDIR}/src/${LIB_NAME}/rubberband.pc.in
cp ${BASEDIR}/tools/make/rubberband/rubberband.pc.in ${BASEDIR}/src/${LIB_NAME}/rubberband.pc.in
${SED_INLINE} 's/%DEPENDENCIES%/sndfile, samplerate/g' ${BASEDIR}/src/${LIB_NAME}/rubberband.pc.in

# ALWAYS RECONFIGURE
autoreconf_library ${LIB_NAME}

# MANUALLY CREATE LIB DIRECTORY
mkdir -p ${BASEDIR}/src/${LIB_NAME}/lib || exit 1

./configure \
    --prefix=${BASEDIR}/prebuilt/android-$(get_target_build)/${LIB_NAME} \
    --host=${BUILD_HOST} || exit 1

make AR="$AR" -j$(get_cpu_count) || exit 1

make install || exit 1

# MANUALLY COPY PKG-CONFIG FILES
cp ./*.pc ${INSTALL_PKG_CONFIG_DIR} || exit 1
