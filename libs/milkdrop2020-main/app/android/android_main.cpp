/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <jni.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <android/log.h>
#include "platform.h"


#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <android/asset_manager_jni.h>

//#include <EGL/egl.h>

// Include the latest possible header file( GL version header )
#if __ANDROID_API__ >= 24
#include <GLES3/gl32.h>
#elif __ANDROID_API__ >= 21
#include <GLES3/gl31.h>
#else
#include <GLES3/gl3.h>
#endif



#include "../external/imgui/imgui.h"
#include "render/context.h"
#include "render/context_gles.h"
#include "VizController.h"
#include "audio/IAudioSource.h"

using namespace render;

static ContextPtr context;
static VizControllerPtr vizController;


extern "C" {
    JNIEXPORT void JNICALL Java_com_android_milkdrop2_MilkDropLib_setAssetManager(JNIEnv* env, jobject obj, jobject assetManager);
    JNIEXPORT void JNICALL Java_com_android_milkdrop2_MilkDropLib_init(JNIEnv* env, jobject obj);
    JNIEXPORT void JNICALL Java_com_android_milkdrop2_MilkDropLib_onDrawFrame(JNIEnv* env, jobject obj, jint width, jint height, jfloat density);
    JNIEXPORT void JNICALL Java_com_android_milkdrop2_MilkDropLib_onTouch(JNIEnv* env, jobject obj, jint eventType, jfloat width, jfloat height);

};



static void printGlString(const char* name, GLenum s) {
    const char* v = (const char*)glGetString(s);
    LogPrint("GL %s: %s\n", name, v);
}

AAssetManager *g_assetManager;

static int g_width, g_height;
static float g_dpi = 1.0f;

static bool g_MousePressed = false;
static bool g_MouseReleased = false;
//static ImVec2 g_MousePos(0,0);

JNIEXPORT void JNICALL
Java_com_android_milkdrop2_MilkDropLib_setAssetManager(JNIEnv* env, jobject _this, jobject _assetManager)
{
    g_assetManager = AAssetManager_fromJava(env, _assetManager);
}

JNIEXPORT void JNICALL
Java_com_android_milkdrop2_MilkDropLib_init(JNIEnv* env, jobject _this)
{
#if ANDROID_CONFIG_DEBUG
    LogPrint("ANDROID_CONFIG_DEBUG\n");
#elif ANDROID_CONFIG_RELEASE
    LogPrint("ANDROID_CONFIG_RELEASE\n");
#endif

    printGlString("Version", GL_VERSION);
    printGlString("Vendor", GL_VENDOR);
    printGlString("Renderer", GL_RENDERER);
    printGlString("Extensions", GL_EXTENSIONS);

    

    context = render::gles::GLCreateContext();

    
    int vpt[4];
    glGetIntegerv( GL_VIEWPORT, vpt );
    LogPrint("viewport %dx%d\n", vpt[2], vpt[3]);

    g_width = vpt[2];
    g_height = vpt[3];
    g_dpi = 3.5f;

    render::DisplayInfo info = { .size= Size2D(g_width, g_height),
            .format= render::PixelFormat::RGBA8Unorm,
            .refreshRate = 60.0f,
            .scale = g_dpi,
            .samples = 1,
            .maxEDR = 1
    };

    context->SetDisplayInfo(info);


    std::string resourceDir = "";

    std::string userDir = "";
    
    vizController = CreateVizController(context, resourceDir, userDir);
}

JNIEXPORT void JNICALL
Java_com_android_milkdrop2_MilkDropLib_onDrawFrame(JNIEnv* env, jobject obj, jint width, jint height, jfloat density)
{
    ImGuiIO& io = ImGui::GetIO();

    g_width = width;
    g_height = height;
    g_dpi = density;
    context->SetDisplayInfo( { .size= Size2D(g_width, g_height),
                                     .format= render::PixelFormat::RGBA8Unorm,
                                     .refreshRate = 60.0f,
                                     .scale = g_dpi,
                                     .samples = 1,
                                     .maxEDR = 1
                             });

//    io.MousePos = g_MousePos;
  //  io.MouseDown[0] = g_MousePressed[0];

    if (g_MousePressed) {
        io.MouseDown[0] = true;
        g_MousePressed = false;
    } else if (g_MouseReleased) {
        io.MouseDown[0] = false;
        g_MouseReleased = false;
    }

    if (vizController)
    {
        vizController->Render(0, 1);
    }

}

JNIEXPORT void JNICALL
Java_com_android_milkdrop2_MilkDropLib_onTouch(JNIEnv* env, jobject obj, jint eventType, jfloat x, jfloat y)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2(x / g_dpi, y  / g_dpi);

//    io.MouseClicked[0] = false;
    switch (eventType)
    {
        case 0:
            g_MousePressed = true;
            break;
        case 1:
            g_MouseReleased = true;
            break;
        case 2:
            g_MousePressed = true;
            break;
    }

  //  LogPrint("onTouch %d %.f %.f\n", eventType, x, y);
}
