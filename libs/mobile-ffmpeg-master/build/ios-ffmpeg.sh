#!/bin/bash

if [[ -z ${ARCH} ]]; then
    echo -e "(*) ARCH not defined\n"
    exit 1
fi

if [[ -z ${TARGET_SDK} ]]; then
    echo -e "(*) TARGET_SDK not defined\n"
    exit 1
fi

if [[ -z ${SDK_PATH} ]]; then
    echo -e "(*) SDK_PATH not defined\n"
    exit 1
fi

if [[ -z ${BASEDIR} ]]; then
    echo -e "(*) BASEDIR not defined\n"
    exit 1
fi

if ! [ -x "$(command -v pkg-config)" ]; then
    echo -e "(*) pkg-config command not found\n"
    exit 1
fi

# ENABLE COMMON FUNCTIONS
if [[ ${APPLE_TVOS_BUILD} -eq 1 ]]; then
    . ${BASEDIR}/build/tvos-common.sh
else
    . ${BASEDIR}/build/ios-common.sh
fi

# PREPARE PATHS & DEFINE ${INSTALL_PKG_CONFIG_DIR}
LIB_NAME="ffmpeg"
set_toolchain_clang_paths ${LIB_NAME}

# PREPARING FLAGS
BUILD_HOST=$(get_build_host)
FFMPEG_CFLAGS=""
FFMPEG_LDFLAGS=""
export PKG_CONFIG_LIBDIR="${INSTALL_PKG_CONFIG_DIR}"

TARGET_CPU=""
TARGET_ARCH=""
BITCODE_FLAGS=""
NEON_FLAG=""
ARCH_OPTIONS="--enable-asm"
case ${ARCH} in
    armv7)
        TARGET_CPU="armv7"
        TARGET_ARCH="armv7"
        NEON_FLAG="	--enable-neon"
        BITCODE_FLAGS="-fembed-bitcode -Wc,-fembed-bitcode"
    ;;
    armv7s)
        TARGET_CPU="armv7s"
        TARGET_ARCH="armv7s"
        NEON_FLAG="	--enable-neon"
        BITCODE_FLAGS="-fembed-bitcode -Wc,-fembed-bitcode"
    ;;
    arm64)
        TARGET_CPU="armv8"
        TARGET_ARCH="aarch64"
        NEON_FLAG="	--enable-neon"
        BITCODE_FLAGS="-fembed-bitcode -Wc,-fembed-bitcode"
    ;;
    arm64e)
        TARGET_CPU="armv8.3-a"
        TARGET_ARCH="aarch64"
        NEON_FLAG="	--enable-neon"
        BITCODE_FLAGS="-fembed-bitcode -Wc,-fembed-bitcode"
    ;;
    i386)
        TARGET_CPU="i386"
        TARGET_ARCH="i386"
        NEON_FLAG="	--disable-neon"
        BITCODE_FLAGS=""
    ;;
    x86-64)
        ARCH_OPTIONS="--disable-asm"
        TARGET_CPU="x86_64"
        TARGET_ARCH="x86_64"
        NEON_FLAG="	--disable-neon"
        BITCODE_FLAGS=""
    ;;
    x86-64-mac-catalyst)
        ARCH_OPTIONS="--disable-asm"
        TARGET_CPU="x86_64"
        TARGET_ARCH="x86_64"
        NEON_FLAG="	--disable-neon"
        BITCODE_FLAGS="-fembed-bitcode -Wc,-fembed-bitcode"
    ;;
esac

library=1
while [[ ${library} -le 49 ]]
do
    if [[ ${!library} -eq 1 ]]; then
        ENABLED_LIBRARY=$(get_library_name $((library - 1)))

        echo -e "INFO: Enabling library ${ENABLED_LIBRARY}" 1>>${BASEDIR}/build.log 2>&1

        case ${ENABLED_LIBRARY} in
            chromaprint)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags libchromaprint)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static libchromaprint)"
                CONFIGURE_POSTFIX+=" --enable-chromaprint"
            ;;
            fontconfig)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags fontconfig)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static fontconfig)"
                CONFIGURE_POSTFIX+=" --enable-libfontconfig"
            ;;
            freetype)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags freetype2)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static freetype2)"
                CONFIGURE_POSTFIX+=" --enable-libfreetype"
            ;;
            fribidi)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags fribidi)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static fribidi)"
                CONFIGURE_POSTFIX+=" --enable-libfribidi"
            ;;
            gmp)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags gmp)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static gmp)"
                CONFIGURE_POSTFIX+=" --enable-gmp"
            ;;
            gnutls)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags gnutls)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static gnutls)"
                CONFIGURE_POSTFIX+=" --enable-gnutls"
            ;;
            kvazaar)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags kvazaar)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static kvazaar)"
                CONFIGURE_POSTFIX+=" --enable-libkvazaar"
            ;;
            lame)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags libmp3lame)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static libmp3lame)"
                CONFIGURE_POSTFIX+=" --enable-libmp3lame"
            ;;
            libaom)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags aom)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static aom)"
                CONFIGURE_POSTFIX+=" --enable-libaom"
            ;;
            libass)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags libass)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static libass)"
                CONFIGURE_POSTFIX+=" --enable-libass"
            ;;
            libilbc)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags libilbc)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static libilbc)"
                CONFIGURE_POSTFIX+=" --enable-libilbc"
            ;;
            libtheora)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags theora)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static theora)"
                CONFIGURE_POSTFIX+=" --enable-libtheora"
            ;;
            libvidstab)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags vidstab)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static vidstab)"
                CONFIGURE_POSTFIX+=" --enable-libvidstab --enable-gpl"
            ;;
            libvorbis)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags vorbis)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static vorbis)"
                CONFIGURE_POSTFIX+=" --enable-libvorbis"
            ;;
            libvpx)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags vpx)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static vpx)"
                CONFIGURE_POSTFIX+=" --enable-libvpx"
            ;;
            libwebp)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags libwebp)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static libwebp)"
                CONFIGURE_POSTFIX+=" --enable-libwebp"
            ;;
            libxml2)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags libxml-2.0)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static libxml-2.0)"
                CONFIGURE_POSTFIX+=" --enable-libxml2"
            ;;
            opencore-amr)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags opencore-amrnb)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static opencore-amrnb)"
                CONFIGURE_POSTFIX+=" --enable-libopencore-amrnb"
            ;;
            openh264)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags openh264)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static openh264)"
                CONFIGURE_POSTFIX+=" --enable-libopenh264"
            ;;
            opus)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags opus)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static opus)"
                CONFIGURE_POSTFIX+=" --enable-libopus"
            ;;
            rubberband)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags rubberband)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static rubberband)"
                FFMPEG_LDFLAGS+=" -framework Accelerate"
                CONFIGURE_POSTFIX+=" --enable-librubberband --enable-gpl"
            ;;
            sdl)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags sdl2)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static sdl2)"
                CONFIGURE_POSTFIX+=" --enable-sdl2"
            ;;
            shine)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags shine)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static shine)"
                CONFIGURE_POSTFIX+=" --enable-libshine"
            ;;
            snappy)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags snappy)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static snappy)"
                CONFIGURE_POSTFIX+=" --enable-libsnappy"
            ;;
            soxr)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags soxr)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static soxr)"
                CONFIGURE_POSTFIX+=" --enable-libsoxr"
            ;;
            speex)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags speex)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static speex)"
                CONFIGURE_POSTFIX+=" --enable-libspeex"
            ;;
            tesseract)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags tesseract)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static tesseract)"
                FFMPEG_CFLAGS+=" $(pkg-config --cflags giflib)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static giflib)"
                CONFIGURE_POSTFIX+=" --enable-libtesseract"
            ;;
            twolame)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags twolame)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static twolame)"
                CONFIGURE_POSTFIX+=" --enable-libtwolame"
            ;;
            vo-amrwbenc)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags vo-amrwbenc)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static vo-amrwbenc)"
                CONFIGURE_POSTFIX+=" --enable-libvo-amrwbenc"
            ;;
            wavpack)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags wavpack)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static wavpack)"
                CONFIGURE_POSTFIX+=" --enable-libwavpack"
            ;;
            x264)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags x264)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static x264)"
                CONFIGURE_POSTFIX+=" --enable-libx264 --enable-gpl"
            ;;
            x265)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags x265)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static x265)"
                CONFIGURE_POSTFIX+=" --enable-libx265 --enable-gpl"
            ;;
            xvidcore)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags xvidcore)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static xvidcore)"
                CONFIGURE_POSTFIX+=" --enable-libxvid --enable-gpl"
            ;;
            expat)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags expat)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static expat)"
            ;;
            libogg)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags ogg)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static ogg)"
            ;;
            libpng)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags libpng)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static libpng)"
            ;;
            nettle)
                FFMPEG_CFLAGS+=" $(pkg-config --cflags nettle)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static nettle)"
                FFMPEG_CFLAGS+=" $(pkg-config --cflags hogweed)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static hogweed)"
            ;;
            ios-*|tvos-*)

                # BUILT-IN LIBRARIES SHARE INCLUDE AND LIB DIRECTORIES
                # INCLUDING ONLY ONE OF THEM IS ENOUGH
                FFMPEG_CFLAGS+=" $(pkg-config --cflags zlib)"
                FFMPEG_LDFLAGS+=" $(pkg-config --libs --static zlib)"

                case ${ENABLED_LIBRARY} in
                    *-audiotoolbox)
                        CONFIGURE_POSTFIX+=" --enable-audiotoolbox"
                    ;;
                    *-avfoundation)
                        CONFIGURE_POSTFIX+=" --enable-avfoundation"
                    ;;
                    *-bzip2)
                        CONFIGURE_POSTFIX+=" --enable-bzlib"
                    ;;
                    *-videotoolbox)
                        CONFIGURE_POSTFIX+=" --enable-videotoolbox"
                    ;;
                    *-zlib)
                        CONFIGURE_POSTFIX+=" --enable-zlib"
                    ;;
                    *-libiconv)
                        CONFIGURE_POSTFIX+=" --enable-iconv"
                    ;;
                esac
            ;;
        esac
    else

        # THE FOLLOWING LIBRARIES SHOULD BE EXPLICITLY DISABLED TO PREVENT AUTODETECT
        # NOTE THAT IDS MUST BE +1 OF THE INDEX VALUE
        if [[ ${library} -eq 30 ]]; then
            CONFIGURE_POSTFIX+=" --disable-sdl2"
        elif [[ ${library} -eq 44 ]]; then
            CONFIGURE_POSTFIX+=" --disable-zlib"
        elif [[ ${library} -eq 45 ]]; then
            CONFIGURE_POSTFIX+=" --disable-audiotoolbox"
        elif [[ ${library} -eq 46 ]]; then
            CONFIGURE_POSTFIX+=" --disable-bzlib"
        elif [[ ${library} -eq 47 ]]; then
            CONFIGURE_POSTFIX+=" --disable-videotoolbox"
        elif [[ ${library} -eq 48 ]]; then
            CONFIGURE_POSTFIX+=" --disable-avfoundation"
        elif [[ ${library} -eq 49 ]]; then
            CONFIGURE_POSTFIX+=" --disable-iconv"
        fi
    fi

    ((library++))
done

# ALWAYS BUILD STATIC LIBRARIES
BUILD_LIBRARY_OPTIONS="--enable-static --disable-shared";

# OPTIMIZE FOR SPEED INSTEAD OF SIZE
if [[ -z ${MOBILE_FFMPEG_OPTIMIZED_FOR_SPEED} ]]; then
    SIZE_OPTIONS="--enable-small";
else
    SIZE_OPTIONS="";
fi

# SET DEBUG OPTIONS
if [[ -z ${MOBILE_FFMPEG_DEBUG} ]]; then
    DEBUG_OPTIONS="--disable-debug";
else
    DEBUG_OPTIONS="--enable-debug --disable-stripping";
fi

# CFLAGS PARTS
ARCH_CFLAGS=$(get_arch_specific_cflags);
APP_CFLAGS=$(get_app_specific_cflags ${LIB_NAME});
COMMON_CFLAGS=$(get_common_cflags);
if [[ -z ${MOBILE_FFMPEG_DEBUG} ]]; then
    OPTIMIZATION_CFLAGS="$(get_size_optimization_cflags ${LIB_NAME})"
else
    OPTIMIZATION_CFLAGS="${MOBILE_FFMPEG_DEBUG}"
fi
MIN_VERSION_CFLAGS=$(get_min_version_cflags);
COMMON_INCLUDES=$(get_common_includes);

# LDFLAGS PARTS
ARCH_LDFLAGS=$(get_arch_specific_ldflags);
if [[ -z ${MOBILE_FFMPEG_DEBUG} ]]; then
    OPTIMIZATION_FLAGS="$(get_size_optimization_ldflags ${LIB_NAME})"
else
    OPTIMIZATION_FLAGS="${MOBILE_FFMPEG_DEBUG}"
fi
LINKED_LIBRARIES=$(get_common_linked_libraries);
COMMON_LDFLAGS=$(get_common_ldflags);

# REORDERED FLAGS
export CFLAGS="${ARCH_CFLAGS} ${APP_CFLAGS} ${COMMON_CFLAGS} ${OPTIMIZATION_CFLAGS} ${MIN_VERSION_CFLAGS}${FFMPEG_CFLAGS} ${COMMON_INCLUDES}"
export CXXFLAGS=$(get_cxxflags ${LIB_NAME})
export LDFLAGS="${ARCH_LDFLAGS}${FFMPEG_LDFLAGS} ${LINKED_LIBRARIES} ${COMMON_LDFLAGS} ${BITCODE_FLAGS} ${OPTIMIZATION_FLAGS}"

echo -n -e "\n${LIB_NAME}: "

# DOWNLOAD LIBRARY
DOWNLOAD_RESULT=$(download_library_source ${LIB_NAME})
if [[ ${DOWNLOAD_RESULT} -ne 0 ]]; then
    exit 1
fi

cd ${BASEDIR}/src/${LIB_NAME} || exit 1

if [[ -z ${NO_WORKSPACE_CLEANUP_ffmpeg} ]]; then
    echo -e "INFO: Cleaning workspace for ${LIB_NAME}" 1>>${BASEDIR}/build.log 2>&1
    make distclean 2>/dev/null 1>/dev/null
fi

########################### CUSTOMIZATIONS #######################

# 1. Workaround to prevent adding of -mdynamic-no-pic flag
${SED_INLINE} 's/check_cflags -mdynamic-no-pic && add_asflags -mdynamic-no-pic;/check_cflags -mdynamic-no-pic;/g' ./configure 1>>${BASEDIR}/build.log 2>&1

# 2. Workaround for videotoolbox on mac catalyst
if [ ${ARCH} == "x86-64-mac-catalyst" ]; then
    ${SED_INLINE} 's/    CFDictionarySetValue(buffer_attributes\, kCVPixelBufferOpenGLESCompatibilityKey/   \/\/ CFDictionarySetValue(buffer_attributes\, kCVPixelBufferOpenGLESCompatibilityKey/g' ${BASEDIR}/src/${LIB_NAME}/libavcodec/videotoolbox.c
else
    ${SED_INLINE} 's/   \/\/ CFDictionarySetValue(buffer_attributes\, kCVPixelBufferOpenGLESCompatibilityKey/    CFDictionarySetValue(buffer_attributes\, kCVPixelBufferOpenGLESCompatibilityKey/g' ${BASEDIR}/src/${LIB_NAME}/libavcodec/videotoolbox.c
fi

# 3. Use thread local log level
${SED_INLINE} 's/static int av_log_level/__thread int av_log_level/g' ${BASEDIR}/src/${LIB_NAME}/libavutil/log.c 1>>${BASEDIR}/build.log 2>&1

###################################################################

./configure \
    --sysroot=${SDK_PATH} \
    --prefix=${BASEDIR}/prebuilt/$(get_target_build_directory)/${LIB_NAME} \
    --enable-version3 \
    --arch="${TARGET_ARCH}" \
    --cpu="${TARGET_CPU}" \
    --target-os=darwin \
    --ar="${AR}" \
    --cc="${CC}" \
    --cxx="${CXX}" \
    --as="${AS}" \
    --ranlib="${RANLIB}" \
    --strip="${STRIP}" \
    ${NEON_FLAG} \
    --enable-cross-compile \
    --enable-pic \
    ${ARCH_OPTIONS} \
    --enable-inline-asm \
    --enable-optimizations \
    --enable-swscale \
    ${BUILD_LIBRARY_OPTIONS} \
    ${SIZE_OPTIONS}  \
    --disable-v4l2-m2m \
    --disable-outdev=v4l2 \
    --disable-outdev=fbdev \
    --disable-outdev=audiotoolbox \
    --disable-indev=v4l2 \
    --disable-indev=fbdev \
    --disable-openssl \
    --disable-xmm-clobber-test \
    ${DEBUG_OPTIONS} \
    --disable-neon-clobber-test \
    --disable-programs \
    --disable-postproc \
    --disable-doc \
    --disable-htmlpages \
    --disable-manpages \
    --disable-podpages \
    --disable-txtpages \
    --disable-sndio \
    --disable-schannel \
    --disable-securetransport \
    --disable-xlib \
    --disable-cuda \
    --disable-cuvid \
    --disable-nvenc \
    --disable-vaapi \
    --disable-vdpau \
    --disable-appkit \
    --disable-alsa \
    --disable-cuda \
    --disable-cuvid \
    --disable-nvenc \
    --disable-vaapi \
    --disable-vdpau \
    ${CONFIGURE_POSTFIX} 1>>${BASEDIR}/build.log 2>&1

if [ $? -ne 0 ]; then
    echo "failed"
    exit 1
fi

if [[ -z ${NO_OUTPUT_REDIRECTION} ]]; then
    make -j$(get_cpu_count) 1>>${BASEDIR}/build.log 2>&1

    if [ $? -ne 0 ]; then
        echo "failed"
        exit 1
    fi
else
    echo -e "started\n"
    make -j$(get_cpu_count) 1>>${BASEDIR}/build.log 2>&1

    if [ $? -ne 0 ]; then
        echo -n -e "\n${LIB_NAME}: failed\n"
        exit 1
    else
        echo -n -e "\n${LIB_NAME}: "
    fi
fi

rm -rf ${BASEDIR}/prebuilt/$(get_target_build_directory)/${LIB_NAME} 1>>${BASEDIR}/build.log 2>&1
make install 1>>${BASEDIR}/build.log 2>&1

if [ $? -ne 0 ]; then
    echo "failed"
    exit 1
fi

# MANUALLY ADD REQUIRED HEADERS
mkdir -p ${BASEDIR}/prebuilt/$(get_target_build_directory)/ffmpeg/include/libavutil/x86
mkdir -p ${BASEDIR}/prebuilt/$(get_target_build_directory)/ffmpeg/include/libavutil/arm
mkdir -p ${BASEDIR}/prebuilt/$(get_target_build_directory)/ffmpeg/include/libavutil/aarch64
mkdir -p ${BASEDIR}/prebuilt/$(get_target_build_directory)/ffmpeg/include/libavcodec/x86
mkdir -p ${BASEDIR}/prebuilt/$(get_target_build_directory)/ffmpeg/include/libavcodec/arm
cp -f ${BASEDIR}/src/ffmpeg/config.h ${BASEDIR}/prebuilt/$(get_target_build_directory)/ffmpeg/include
cp -f ${BASEDIR}/src/ffmpeg/libavcodec/mathops.h ${BASEDIR}/prebuilt/$(get_target_build_directory)/ffmpeg/include/libavcodec
cp -f ${BASEDIR}/src/ffmpeg/libavcodec/x86/mathops.h ${BASEDIR}/prebuilt/$(get_target_build_directory)/ffmpeg/include/libavcodec/x86
cp -f ${BASEDIR}/src/ffmpeg/libavcodec/arm/mathops.h ${BASEDIR}/prebuilt/$(get_target_build_directory)/ffmpeg/include/libavcodec/arm
cp -f ${BASEDIR}/src/ffmpeg/libavformat/network.h ${BASEDIR}/prebuilt/$(get_target_build_directory)/ffmpeg/include/libavformat
cp -f ${BASEDIR}/src/ffmpeg/libavformat/os_support.h ${BASEDIR}/prebuilt/$(get_target_build_directory)/ffmpeg/include/libavformat
cp -f ${BASEDIR}/src/ffmpeg/libavformat/url.h ${BASEDIR}/prebuilt/$(get_target_build_directory)/ffmpeg/include/libavformat
cp -f ${BASEDIR}/src/ffmpeg/libavutil/internal.h ${BASEDIR}/prebuilt/$(get_target_build_directory)/ffmpeg/include/libavutil
cp -f ${BASEDIR}/src/ffmpeg/libavutil/libm.h ${BASEDIR}/prebuilt/$(get_target_build_directory)/ffmpeg/include/libavutil
cp -f ${BASEDIR}/src/ffmpeg/libavutil/reverse.h ${BASEDIR}/prebuilt/$(get_target_build_directory)/ffmpeg/include/libavutil
cp -f ${BASEDIR}/src/ffmpeg/libavutil/thread.h ${BASEDIR}/prebuilt/$(get_target_build_directory)/ffmpeg/include/libavutil
cp -f ${BASEDIR}/src/ffmpeg/libavutil/timer.h ${BASEDIR}/prebuilt/$(get_target_build_directory)/ffmpeg/include/libavutil
cp -f ${BASEDIR}/src/ffmpeg/libavutil/x86/asm.h ${BASEDIR}/prebuilt/$(get_target_build_directory)/ffmpeg/include/libavutil/x86
cp -f ${BASEDIR}/src/ffmpeg/libavutil/x86/timer.h ${BASEDIR}/prebuilt/$(get_target_build_directory)/ffmpeg/include/libavutil/x86
cp -f ${BASEDIR}/src/ffmpeg/libavutil/arm/timer.h ${BASEDIR}/prebuilt/$(get_target_build_directory)/ffmpeg/include/libavutil/arm
cp -f ${BASEDIR}/src/ffmpeg/libavutil/aarch64/timer.h ${BASEDIR}/prebuilt/$(get_target_build_directory)/ffmpeg/include/libavutil/aarch64
cp -f ${BASEDIR}/src/ffmpeg/libavutil/x86/emms.h ${BASEDIR}/prebuilt/$(get_target_build_directory)/ffmpeg/include/libavutil/x86

if [ $? -eq 0 ]; then
    echo "ok"
else
    echo "failed"
    exit 1
fi
