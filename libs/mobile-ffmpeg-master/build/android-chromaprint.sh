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
LIB_NAME="chromaprint"
set_toolchain_clang_paths ${LIB_NAME}

# PREPARING FLAGS
BUILD_HOST=$(get_build_host)
CFLAGS=$(get_cflags ${LIB_NAME})
CXXFLAGS=$(get_cxxflags ${LIB_NAME})
LDFLAGS=$(get_ldflags ${LIB_NAME})

cd ${BASEDIR}/src/${LIB_NAME} || exit 1

if [ -d "build" ]; then
    rm -rf build
fi

mkdir build;
cd build

cmake -Wno-dev \
    -DCMAKE_VERBOSE_MAKEFILE=0 \
    -DCMAKE_C_FLAGS="${CFLAGS}" \
    -DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
    -DCMAKE_EXE_LINKER_FLAGS="${LDFLAGS}" \
    -DCMAKE_SYSROOT="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/sysroot" \
    -DCMAKE_FIND_ROOT_PATH="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/sysroot" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${BASEDIR}/prebuilt/android-$(get_target_build)/${LIB_NAME}" \
    -DCMAKE_SYSTEM_NAME=Generic \
    -DCMAKE_C_COMPILER="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/bin/$CC" \
    -DCMAKE_LINKER="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/bin/$LD" \
    -DCMAKE_AR="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/bin/$AR" \
    -DCMAKE_AS="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/bin/$AS" \
    -DCMAKE_SYSTEM_PROCESSOR=$(get_cmake_target_processor) \
    -DCMAKE_POSITION_INDEPENDENT_CODE=1 \
    -DFFT_LIB=kissfft \
    -DBUILD_SHARED_LIBS=0 .. || exit 1

make -j$(get_cpu_count) || exit 1

# CREATE PACKAGE CONFIG MANUALLY
create_chromaprint_package_config "1.5.0"

make install || exit 1
