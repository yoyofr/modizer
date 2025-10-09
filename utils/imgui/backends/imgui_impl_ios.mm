#include "imgui.h"
#include "imgui_impl_ios.h"

#include "StopWatch.h"

@class ImFocusObserver;

static StopWatch g_timer;

// Functions
bool ImGui_ImplIOS_Init()
{
    ImGuiIO& io = ImGui::GetIO();

    // Setup backend capabilities flags
    //io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;           // We can honor GetMouseCursor() values (optional)
    //io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
    //io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;    // We can create multi-viewports on the Platform side (optional)
    //io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can set io.MouseHoveredViewport correctly (optional, not easy)
    io.BackendPlatformName = "imgui_impl_ios";
    
    g_timer.Restart();

    return true;
}

void ImGui_ImplIOS_Shutdown()
{
}


void ImGui_ImplIOS_NewFrame(float w,float h,float scale)
{
    ImGuiIO &io = ImGui::GetIO();

    io.DisplaySize.x = w / scale;
    io.DisplaySize.y = h / scale ;
    io.DisplayFramebufferScale = ImVec2(scale, scale);
    
    io.DeltaTime = g_timer.GetElapsedSeconds();
    g_timer.Restart();
}

bool ImGui_ImplIOS_HandleEvent()
{
    ImGuiIO& io = ImGui::GetIO();
    return false;
}
