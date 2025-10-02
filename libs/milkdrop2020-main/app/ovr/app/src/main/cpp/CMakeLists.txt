cmake_minimum_required(VERSION 3.4.1)


if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()


set(CMAKE_VERBOSE_MAKEFILE on)

set(COMMON_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../src)
set(EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../external)
set(HLSL2GLSL_SOURCE_DIR ${EXTERNAL_DIR}/hlsl2glslfork/hlslang)
set(IMGUI_SOURCE_DIR ${EXTERNAL_DIR}/imgui)
set(STB_SOURCE_DIR ${EXTERNAL_DIR}/stb)
set(OVRSDK_SOURCE_DIR ${EXTERNAL_DIR}/ovr_sdk_mobile_1.29.0)

set(CMAKE_CXX_FLAGS_DEBUG "-O2 -g -DANDROID_CONFIG_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DANDROID_CONFIG_RELEASE")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOCULUS=1 -std=c++11 -fno-rtti -fno-exceptions -Wall -Wno-reorder -Wno-deprecated-register -Wno-self-assign -Wno-c++11-narrowing -Wno-unused-function ")
set(OPENGL_LIB GLESv3)


set(HLSL2GLSL_SOURCE
  ${HLSL2GLSL_SOURCE_DIR}/OSDependent/Android/ossource.cpp
  ${HLSL2GLSL_SOURCE_DIR}/MachineIndependent/ConstantFolding.cpp
  ${HLSL2GLSL_SOURCE_DIR}/MachineIndependent/HLSL2GLSL.cpp
  ${HLSL2GLSL_SOURCE_DIR}/MachineIndependent/InfoSink.cpp
  ${HLSL2GLSL_SOURCE_DIR}/MachineIndependent/Initialize.cpp
  ${HLSL2GLSL_SOURCE_DIR}/MachineIndependent/Intermediate.cpp
  ${HLSL2GLSL_SOURCE_DIR}/MachineIndependent/IntermOut.cpp
  ${HLSL2GLSL_SOURCE_DIR}/MachineIndependent/IntermTraverse.cpp
  ${HLSL2GLSL_SOURCE_DIR}/MachineIndependent/ParseHelper.cpp
  ${HLSL2GLSL_SOURCE_DIR}/MachineIndependent/PoolAlloc.cpp
  ${HLSL2GLSL_SOURCE_DIR}/MachineIndependent/RemoveTree.cpp
  ${HLSL2GLSL_SOURCE_DIR}/MachineIndependent/SymbolTable.cpp
  ${HLSL2GLSL_SOURCE_DIR}/MachineIndependent/hlslang_tab.cpp
  ${HLSL2GLSL_SOURCE_DIR}/MachineIndependent/Gen_hlslang.cpp
  ${HLSL2GLSL_SOURCE_DIR}/MachineIndependent/preprocessor/mojoshader.cpp
  ${HLSL2GLSL_SOURCE_DIR}/MachineIndependent/preprocessor/mojoshader_common.cpp
  ${HLSL2GLSL_SOURCE_DIR}/MachineIndependent/preprocessor/mojoshader_lexer.cpp
  ${HLSL2GLSL_SOURCE_DIR}/MachineIndependent/preprocessor/mojoshader_preprocessor.cpp
  ${HLSL2GLSL_SOURCE_DIR}/MachineIndependent/preprocessor/sourceloc.cpp
  ${HLSL2GLSL_SOURCE_DIR}/GLSLCodeGen/glslCommon.cpp
  ${HLSL2GLSL_SOURCE_DIR}/GLSLCodeGen/glslFunction.cpp
  ${HLSL2GLSL_SOURCE_DIR}/GLSLCodeGen/glslOutput.cpp
  ${HLSL2GLSL_SOURCE_DIR}/GLSLCodeGen/glslStruct.cpp
  ${HLSL2GLSL_SOURCE_DIR}/GLSLCodeGen/glslSymbol.cpp
  ${HLSL2GLSL_SOURCE_DIR}/GLSLCodeGen/hlslCrossCompiler.cpp
  ${HLSL2GLSL_SOURCE_DIR}/GLSLCodeGen/hlslLinker.cpp
  ${HLSL2GLSL_SOURCE_DIR}/GLSLCodeGen/hlslSupportLib.cpp
  ${HLSL2GLSL_SOURCE_DIR}/GLSLCodeGen/propagateMutable.cpp
  ${HLSL2GLSL_SOURCE_DIR}/GLSLCodeGen/typeSamplers.cpp
    )


set(IMGUI_SOURCE
        ${IMGUI_SOURCE_DIR}/imgui.cpp
        ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
        ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp
        ${IMGUI_SOURCE_DIR}/imgui_demo.cpp
        ${COMMON_SOURCE_DIR}/imgui_support.cpp
        )

set(MDP_SOURCE
        ${COMMON_SOURCE_DIR}/matrix.cpp
        ${COMMON_SOURCE_DIR}/platform.cpp
        ${COMMON_SOURCE_DIR}/path.cpp
        ${COMMON_SOURCE_DIR}/UnitTests.cpp
        ${COMMON_SOURCE_DIR}/ImageWriter.cpp
        ${COMMON_SOURCE_DIR}/VizController.cpp
        ${COMMON_SOURCE_DIR}/TProfiler.cpp
        ${COMMON_SOURCE_DIR}/vis_milk2/fft.cpp
        ${COMMON_SOURCE_DIR}/vis_milk2/milkdropfs.cpp
        ${COMMON_SOURCE_DIR}/vis_milk2/plugin.cpp
        ${COMMON_SOURCE_DIR}/vis_milk2/state.cpp
        ${COMMON_SOURCE_DIR}/vis_milk2/AudioAnalyzer.cpp
        ${COMMON_SOURCE_DIR}/audio/RawFileAudioSource.cpp
        ${COMMON_SOURCE_DIR}/audio/WavFileAudioSource.cpp
        ${COMMON_SOURCE_DIR}/audio/SLESAudioSource.cpp
        ${COMMON_SOURCE_DIR}/audio/NullAudioSource.cpp
        ${COMMON_SOURCE_DIR}/render/context.cpp
        ${COMMON_SOURCE_DIR}/render/context_null.cpp
        ${COMMON_SOURCE_DIR}/render/context_gles.cpp
        ${COMMON_SOURCE_DIR}/render/DrawBuffer.cpp
        ${COMMON_SOURCE_DIR}/script/script-tests.cpp
        ${COMMON_SOURCE_DIR}/script/mdp-eel2/script-compiler.cpp
        ${COMMON_SOURCE_DIR}/script/mdp-eel2/script-caltab.cpp
        ${COMMON_SOURCE_DIR}/script/mdp-eel2/script-preprocessor.cpp
        ${COMMON_SOURCE_DIR}/script/mdp-eel2/script-eval.cpp
        ${COMMON_SOURCE_DIR}/script/mdp-eel2/script-lextab.cpp
        ${COMMON_SOURCE_DIR}/script/mdp-eel2/script-context.cpp
        ${COMMON_SOURCE_DIR}/script/mdp-eel2/script-functype.cpp
        ${COMMON_SOURCE_DIR}/script/mdp-eel2/script-expression.cpp
        ${COMMON_SOURCE_DIR}/script/mdp-eel2/script-yylex.cpp
        ${COMMON_SOURCE_DIR}/script/mdp-eel2/script-kernel.cpp
        ${COMMON_SOURCE_DIR}/script/mdp-eel2/script-kernel-generated.cpp
      )


include_directories(${ANDROID_NDK}/sources/android/native_app_glue
        ${COMMON_SOURCE_DIR}
        ${COMMON_SOURCE_DIR}/script
        ${COMMON_SOURCE_DIR}/script/mdp-eel2
        ${HLSL2GLSL_SOURCE_DIR}
    ${HLSL2GLSL_SOURCE_DIR}/Include
    ${HLSL2GLSL_SOURCE_DIR}/MachineIndependent
    ${HLSL2GLSL_SOURCE_DIR}/OSDependent/Android
        ${OVRSDK_SOURCE_DIR}/VrApi/Include
        ${STB_SOURCE_DIR}
    )


add_library(lib_vrapi SHARED IMPORTED )
set_target_properties(lib_vrapi
        PROPERTIES IMPORTED_LOCATION
        # ${OVRSDK_SOURCE_DIR}/VrApi/Libs/Android/${ANDROID_ABI}/Debug/libvrapi.so
        ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}/libvrapi.so
        )


add_library(mdp SHARED
            VrMilkDrop_NativeActivity.cpp
           # android_main.cpp
            android_textureloader.cpp
            ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
            ${MDP_SOURCE}
            ${HLSL2GLSL_SOURCE}
            ${IMGUI_SOURCE}
          )

set_target_properties( mdp PROPERTIES LINK_FLAGS "-u ANativeActivity_onCreate" )


# Include libraries needed for lib
target_link_libraries(mdp
            android
            ${OPENGL_LIB}
            OpenSLES
            lib_vrapi
            EGL
            log
            m)
