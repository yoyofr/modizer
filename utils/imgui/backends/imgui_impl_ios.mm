#include "imgui.h"
#include "imgui_impl_ios.h"

#include "StopWatch.h"

static StopWatch g_timer;
static ImGuiIOSEvent currentEvent;

ImFont  *font_body;

// Functions
bool ImGui_ImplIOS_Init()
{
    ImGuiIO& io = ImGui::GetIO();

    // Setup backend capabilities flags
    //io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;           // We can honor GetMouseCursor() values (optional)
    //io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
    //io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;    // We can create multi-viewports on the Platform side (optional)
    //io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can set io.MouseHoveredViewport correctly (optional, not easy)
    //io.BackendFlags |= ImGuiConfigFlags_IsTouchScreen;
    io.BackendPlatformName = "imgui_impl_ios";
    
    g_timer.Restart();
    
    currentEvent.event_type=IMGUI_IOS_Event_None;
    
    ;
    ImFont* font_body = io.Fonts->AddFontFromFileTTF([[[NSBundle mainBundle] pathForResource:@"Roboto-Medium" ofType: @"ttf"] UTF8String], 24.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    IM_ASSERT(font_body != NULL);

    return true;
}

void ImGui_ImplIOS_Shutdown()
{
}

void ImGui_ImplIOS_UpdateEvent(ImGuiIOSEvent *event)
{
    if (event) {
        currentEvent=*event;
    }
    // Update buttons
    ImGuiIO& io = ImGui::GetIO();
    //io.WantCaptureMouse=true;
    
    if (currentEvent.event_type==IMGUI_IOS_Event_Tap_1) {
        //NSLog(@"tap1 event: %d x %d",currentEvent.pos_x,currentEvent.pos_y);
        //io.MouseDown[0] = 1;
        //io.MousePos = ImVec2((float)currentEvent.pos_x, (float)currentEvent.pos_y);
        
        io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);//TouchScreen);
        io.AddMousePosEvent((float)(currentEvent.pos_x), (float)(currentEvent.pos_y));
        
        io.AddMouseButtonEvent(0, true);
    } else {
        
        io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);//TouchScreen);
        //Do not reset position if button hasn't been released yet, in order to let ImGui process the event first
        //NSLog(@"release tap");
        
//        if (io.MouseDown[0]==0) {
//            //io.MousePos = ImVec2((float)0, (float)0);
//            io.AddMousePosEvent((float)(0), (float)(0));
//        }
        
        //io.MouseDown[0] = 0;
        io.AddMouseButtonEvent(0, false);
        io.AddMousePosEvent((float)(-1), (float)(-1));
    }
}

void ImGui_ImplIOS_ResetTapPos() {
    // Update buttons
    ImGuiIO& io = ImGui::GetIO();
    io.WantCaptureMouse=true;
    
    currentEvent.event_type=IMGUI_IOS_Event_None;
    io.MousePos = ImVec2((float)0, (float)0);
    io.MouseDown[0] = 0;
}


void ImGui_ImplIOS_NewFrame(float w,float h,float scale,ImGuiIOSEvent *event)
{
    ImGuiIO &io = ImGui::GetIO();

    io.DisplaySize.x = w / scale;
    io.DisplaySize.y = h / scale ;
    io.DisplayFramebufferScale = ImVec2(scale, scale);
    
    io.DeltaTime = g_timer.GetElapsedSeconds();
    g_timer.Restart();
    
    ImGui_ImplIOS_UpdateEvent(event);
}

