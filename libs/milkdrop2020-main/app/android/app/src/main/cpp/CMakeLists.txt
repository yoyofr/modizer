cmake_minimum_required(VERSION 3.4.1)
project(mdp LANGUAGES C CXX)


if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()


set(CMAKE_VERBOSE_MAKEFILE on)

set(COMMON_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../src)
set(EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../external)
set(HLSL2GLSL_SOURCE_DIR ${EXTERNAL_DIR}/hlsl2glslfork/hlslang)
set(IMGUI_SOURCE_DIR ${EXTERNAL_DIR}/imgui)
set(STB_SOURCE_DIR ${EXTERNAL_DIR}/stb)


set(CMAKE_CXX_FLAGS_DEBUG "-g -DANDROID_CONFIG_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DANDROID_CONFIG_RELEASE")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fno-rtti -fno-exceptions -Wall -Wno-reorder -Wno-deprecated-register -Wno-self-assign -Wno-c++11-narrowing ")
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
        ${IMGUI_SOURCE_DIR}/imgui_tables.cpp
        ${COMMON_SOURCE_DIR}/imgui_support.cpp
        )

set(MDP_SOURCE
        ${COMMON_SOURCE_DIR}/matrix.cpp
        ${COMMON_SOURCE_DIR}/platform.cpp
        ${COMMON_SOURCE_DIR}/path.cpp
        ${COMMON_SOURCE_DIR}/ImageWriter.cpp
        ${COMMON_SOURCE_DIR}/ImageReader.cpp
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
      )


include_directories(${ANDROID_NDK}/sources/android/native_app_glue
        ${COMMON_SOURCE_DIR}
        ${COMMON_SOURCE_DIR}/script
        ${COMMON_SOURCE_DIR}/script/mdp-eel2
        ${HLSL2GLSL_SOURCE_DIR}
        ${HLSL2GLSL_SOURCE_DIR}/Include
        ${HLSL2GLSL_SOURCE_DIR}/MachineIndependent
        ${HLSL2GLSL_SOURCE_DIR}/OSDependent/Android
        ${STB_SOURCE_DIR}
    )

add_library(mdp SHARED
            ${MDP_SOURCE}
            ${HLSL2GLSL_SOURCE}
            ${IMGUI_SOURCE}
            android_main.cpp
          )

# Include libraries needed for lib
target_link_libraries(mdp
            ${OPENGL_LIB}
            android
            OpenSLES
            EGL
            log
            m)
