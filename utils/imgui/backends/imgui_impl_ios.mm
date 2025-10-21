#include "imgui.h"
#include "imgui_impl_ios.h"

#include "StopWatch.h"

extern float glScaleFactor;
static StopWatch g_timer;
static ImGuiIOSEvent currentEvent;

float font_size[4]={10,16,24,32};
ImFont  *font_menu[4];
ImFont  *font_tracker[FONT_TRACKER_NB][4];
ImFont  *font_trackerH[FONT_TRACKER_NB][4];

const char *font_trackerName[FONT_TRACKER_NB][2]={
    {"ModernDOS8x16","DOS"},
    {"amiga4ever","Amiga"},
    {"Commodore Rounded v1.2","C64"},
    {"Coda-Regular","Coda"},
    {"Roboto-Medium","Roboto"}
};

float font_trackerSize[FONT_TRACKER_NB][5]={
    {16.0,9.0,1.0,0.0,0.0},
    {16.0,17.0,0.82,0.0,2.0},
    {16.0,13.0,0.85,0.0,-1.0},
    {16.0,8.0,1.1,0.0,-2.0},
    {16.0,10.0,0.9,0.0,0.0},
};


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
    
//    font_body = io.Fonts->AddFontFromFileTTF([[[NSBundle mainBundle] pathForResource:@"Fonts/Roboto-Medium" ofType: @"ttf"] UTF8String], 24.0f*glScaleFactor, NULL, io.Fonts->GetGlyphRangesDefault());
//    IM_ASSERT(font_body != NULL);
    
    for (int j=0;j<4;j++) {
        float ft_size=font_size[j];
        font_menu[j]=io.Fonts->AddFontFromFileTTF([[[NSBundle mainBundle] pathForResource:@"Fonts/Roboto-Medium" ofType: @"ttf"] UTF8String], ft_size*glScaleFactor, NULL, io.Fonts->GetGlyphRangesDefault());
        IM_ASSERT(font_menu[j] != NULL);
    }
    
    
    for (int i=0;i<FONT_TRACKER_NB;i++) {
        for (int j=0;j<4;j++) {
            float ft_size=font_size[j];
            //NSLog(@"loading %s.ttf size: %f",font_trackerName[i][0],ft_size);
            
            ImFontConfig font_cfg = ImFontConfig();
            font_cfg.GlyphMinAdvanceX=font_trackerSize[i][1]*ft_size/16.0*glScaleFactor*font_trackerSize[i][2];
            font_cfg.GlyphMaxAdvanceX=font_trackerSize[i][1]*ft_size/16.0*glScaleFactor*font_trackerSize[i][2];
            
            font_tracker[i][j] = io.Fonts->AddFontFromFileTTF([[[NSBundle mainBundle] pathForResource:[NSString stringWithFormat:@"Fonts/%s",font_trackerName[i][0]] ofType: @"ttf"] UTF8String], font_trackerSize[i][0]*ft_size*glScaleFactor/16.0*font_trackerSize[i][2], &font_cfg, io.Fonts->GetGlyphRangesDefault());
            IM_ASSERT(font_tracker[i][j] != NULL);
            
            ImFontConfig font_cfgH = ImFontConfig();
            font_cfgH.GlyphMinAdvanceX=font_trackerSize[i][1]*ft_size/16.0*glScaleFactor*font_trackerSize[i][2];
            font_cfgH.GlyphMaxAdvanceX=font_trackerSize[i][1]*ft_size/16.0*glScaleFactor*font_trackerSize[i][2];
            
            font_trackerH[i][j] = io.Fonts->AddFontFromFileTTF([[[NSBundle mainBundle] pathForResource:[NSString stringWithFormat:@"Fonts/%s",font_trackerName[i][0]] ofType: @"ttf"] UTF8String], font_trackerSize[i][0]*ft_size*glScaleFactor/16.0*font_trackerSize[i][2]*1.3f, &font_cfg, io.Fonts->GetGlyphRangesDefault());
            IM_ASSERT(font_trackerH[i][j] != NULL);
        }
    }

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
//        NSLog(@"tap1 event: %d x %d",currentEvent.pos_x,currentEvent.pos_y);
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

