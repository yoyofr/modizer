//
//  PlayerMenu.h
//  modizer
//
//  Created by Yohann Magnien David on 10/10/2025.
//

#include "ModizerConstants.h"

//--------------------------------------------------
// ImGui
//--------------------------------------------------
#include "../utils/imgui/imgui.h"
#include "../utils/imgui/backends/imgui_impl_ios.h"
#include "../utils/imgui/backends/imgui_impl_opengl3.h"


namespace PMenu {

void playerMenuInit();

void playerMenuShutdown();

int playerShowMenu(float ww,float hh,float glScaleFactor,float fadelev,float panX,float panY);

void playerMenuBack();

}
