#include <stdio.h>
#include <stdint.h>

#include "platform.h"
#include "imgui_support.h"

using namespace render;

static render::ContextPtr g_context;

static render::TexturePtr      g_FontTexture;
static render::ShaderPtr       g_FixedShader;


static StopWatch g_timer;


ImFont *g_font_awesome;


// Functions



void     ImGuiSupport_NewFrame()
{
    ImGuiIO &io = ImGui::GetIO();

    if (g_context)
    {
        Size2D size = g_context->GetDisplaySize();
        float scale  = g_context->GetDisplayScale();

        io.DisplaySize.x = size.width  / scale;
        io.DisplaySize.y = size.height / scale;
        
        io.DisplayFramebufferScale = ImVec2(scale, scale);
    }

    
    io.DeltaTime = g_timer.GetElapsedSeconds();
    g_timer.Restart();

    ImGui::NewFrame();


}


static void ImGui_ImplRender_SetupRenderState(render::ContextPtr context, ImDrawData* draw_data)
{
    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill

    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    
    // setup viewport
    context->SetViewport(0, 0, fb_width, fb_height);
    context->SetBlend(BLEND_SRCALPHA, BLEND_INVSRCALPHA);
    
    
    float L = draw_data->DisplayPos.x;
	float T = draw_data->DisplayPos.y;
	float R = L + draw_data->DisplaySize.x;
    float B = T + draw_data->DisplaySize.y;

    Matrix44 ortho_projection = MatrixOrthoLH(L, R, B, T, 0, 1.0f);
    context->SetTransform(ortho_projection);
    context->SetShader(g_FixedShader);
    
}


static void ConvertVertexData(Vertex *dest, const ImDrawVert *src, int count)
{
    for (int i=0; i < count; i++)
    {
        const auto &sv = src[i];
        Vertex &v = dest[i];
        
        v.x = sv.pos.x;
        v.y = sv.pos.y;
        v.z = 0.0f;
        
        v.Diffuse = sv.col;

        v.tu = sv.uv.x;
        v.tv = sv.uv.y;
        v.rad = v.ang = 0.0f;
    }
}


static void ConvertVertexData(std::vector<Vertex> &ov, const ImVector<ImDrawVert> &src)
{
    ov.resize(src.Size);
    ConvertVertexData(ov.data(), src.Data, src.Size);
}


// (this used to be set in io.RenderDrawListsFn and called by ImGui::Render(), but you can now call this directly from your main loop)
// Note that this implementation is little overcomplicated because we are saving/setting up/restoring every OpenGL state explicitly, in order to be able to run within any OpenGL engine that doesn't do so.
static void    ImGui_ImplRender_RenderDrawData(render::ContextPtr context, ImDrawData* draw_data)
{
    PROFILE_FUNCTION_CAT("ui");

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(draw_data->DisplaySize.x * clip_scale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * clip_scale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;

    // Setup desired GL state
    ImGui_ImplRender_SetupRenderState(context, draw_data);

    auto sampler = g_FixedShader->GetSampler(0);
    
    std::vector<Vertex> vertexData;
    int max = 0;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        max = std::max(max, cmd_list->VtxBuffer.Size);
    }
    vertexData.reserve(max);

    
    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        
    
        ConvertVertexData(vertexData, cmd_list->VtxBuffer);
        context->UploadVertexData(vertexData);
//        context->UploadVertexData(cmd_list->VtxBuffer.Size, (UIVertex *)cmd_list->VtxBuffer.Data);
        context->UploadIndexData(cmd_list->IdxBuffer.Size, cmd_list->IdxBuffer.Data);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != NULL)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplRender_SetupRenderState(context, draw_data);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec4 clip_rect;
                clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

                if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
                {
                    // Apply scissor/clipping rectangle
                    context->SetScissorRect(
                                            (int)clip_rect.x,
                                            (int)clip_rect.y,
                                            (int)(clip_rect.z - clip_rect.x),
                                            (int)(clip_rect.w - clip_rect.y)
                                            );

                    // Bind texture, Draw
                    TexturePtr tex = pcmd->TextureId ? pcmd->TextureId->GetSharedPtr() : nullptr;
                    if (sampler)
                        sampler->SetTexture( tex, SAMPLER_WRAP, SAMPLER_LINEAR);
                    context->DrawIndexed(PRIMTYPE_TRIANGLELIST, pcmd->IdxOffset, pcmd->ElemCount);
                }
            }
        }
    }
    
    context->SetScissorDisable();

}

static void ImGui_ImplRender_CreateFontsTexture(render::ContextPtr context)
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    
    auto fmt = context->IsSupported(PixelFormat::BGRA8Unorm) ? PixelFormat::BGRA8Unorm : PixelFormat::RGBA8Unorm;
    g_FontTexture = context->CreateTexture("imgui_font", width, height, fmt, pixels);
    
    // Store our identifier
    io.Fonts->TexID = g_FontTexture.get();
//    io.Fonts->ClearTexData();
}

static ShaderPtr LoadShaderFromFile(render::ContextPtr context, std::string rootDir, std::string path)
{
    std::string code;
    if (!FileReadAllText(path, code))
    {
        return nullptr;
    }
    
    auto shader = context->CreateShader("ui");
    shader->CompileAndLink({
        ShaderSource{ShaderType::Vertex,     path, code, "VS", "vs_1_1", "hlsl"},
        ShaderSource{ShaderType::Fragment,   path, code, "PS", "ps_3_0", "hlsl"}
    });
    return shader;
}

static void    ImGui_ImplRender_Init(render::ContextPtr context, std::string assetDir)
{
    g_context = context;

    // Setup back-end capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_custom_render";
    
    ImGui_ImplRender_CreateFontsTexture(context);
    
    std::string rootDir = PathCombine(assetDir, "shaders");;
    std::string shaderPath = PathCombine(rootDir, "imgui.fx");
    g_FixedShader = LoadShaderFromFile(context, rootDir, shaderPath);
        
}

static void    ImGui_ImplRender_Shutdown()
{
    ImGuiIO& io = ImGui::GetIO();
    
    io.Fonts->TexID = nullptr;
    
    g_FontTexture = nullptr;
    g_FixedShader = nullptr;
    g_context = nullptr;

}

static ImFont*  ImGuiSupport_AddFontFromFile(const std::string &path, float size_pixels, const ImFontConfig* font_cfg = NULL, const ImWchar* glyph_ranges = NULL)
{
    PROFILE_FUNCTION_CAT("ui");

//    IMGUI_API ImFont*           AddFontFromMemoryTTF(void* font_data, int font_size, float size_pixels, const ImFontConfig* font_cfg = NULL, const ImWchar* glyph_ranges = NULL); // Note: Transfer ownership of 'ttf_data' to ImFontAtlas! Will be deleted after destruction of the atlas. Set font_cfg->FontDataOwnedByAtlas=false to keep ownership of your data and it won't be freed.

    std::vector<uint8_t> data;
    if (!FileReadAllBytes(path, data))
    {
        return nullptr;
    }
    
    void * font_data = ImGui::MemAlloc(data.size());
    memcpy(font_data, data.data(), data.size());
    
    
    std::string name = PathGetFileName(path);
    
    ImFontConfig cfg = *font_cfg;
    strncpy(cfg.Name, name.c_str(), sizeof(cfg.Name));
    
    ImGuiIO& io = ImGui::GetIO();
    return io.Fonts->AddFontFromMemoryTTF(font_data, (int)data.size(), size_pixels, &cfg, glyph_ranges);
}

void     ImGuiSupport_Init(render::ContextPtr context, std::string assetDir)
{
    PROFILE_FUNCTION_CAT("ui");

    
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    ImGuiIO& io = ImGui::GetIO();

//    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Keyboard Controls
//    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;     // Enable Keyboard Controls
//
//    io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
//    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;


#ifdef EMSCRIPTEN
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    //    io.IniFilename = NULL;
#endif
    
    
    io.KeyMap[ImGuiKey_Tab] = KEYCODE_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = KEYCODE_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = KEYCODE_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = KEYCODE_UP;
    io.KeyMap[ImGuiKey_DownArrow] = KEYCODE_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = KEYCODE_PAGEUP;
    io.KeyMap[ImGuiKey_PageDown] = KEYCODE_PAGEDOWN;
    io.KeyMap[ImGuiKey_Home] = KEYCODE_HOME;
    io.KeyMap[ImGuiKey_End] = KEYCODE_END;
    io.KeyMap[ImGuiKey_Insert] = KEYCODE_INSERT;
    io.KeyMap[ImGuiKey_Delete] = KEYCODE_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = KEYCODE_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = KEYCODE_SPACE;
    io.KeyMap[ImGuiKey_Enter] = KEYCODE_RETURN;
    io.KeyMap[ImGuiKey_Escape] = KEYCODE_ESCAPE;
    io.KeyMap[ImGuiKey_A] = KEYCODE_A;
    io.KeyMap[ImGuiKey_C] = KEYCODE_C;
    io.KeyMap[ImGuiKey_V] = KEYCODE_V;
    io.KeyMap[ImGuiKey_X] = KEYCODE_X;
    io.KeyMap[ImGuiKey_Y] = KEYCODE_Y;
    io.KeyMap[ImGuiKey_Z] = KEYCODE_Z;
    
    
    // Setup Dear ImGui style
        ImGui::StyleColorsDark();
//    ImGui::StyleColorsClassic();    
//    SetupImGuiStyle(true, 1.0f);
//    ImGuiStyleColor_OSX();
//    ImGuiStyleColor_Custom1();
    
    
    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'misc/fonts/README.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    

#if  1
    {
        std::string fontName;
        float fontSize;
        //
        
//        fontName = "Roboto-Regular.ttf"; fontSize = 12;
        
    //        fontName = "ProggyTiny.ttf";  fontSize = 12;
    //    fontName = "ProggyClean.ttf"; fontSize = 18;
        fontName = "Cousine-Regular.ttf"; fontSize = 14;
        std::string fontPath = PathCombine(PathCombine(assetDir, "fonts"), fontName);
        
        ImFontConfig font_config;
        font_config.PixelSnapH = false;
        font_config.OversampleH = 2;
        font_config.OversampleV = 1;
        
        ImGuiSupport_AddFontFromFile(fontPath, fontSize, &font_config);
        
    }
#else
    io.Fonts->AddFontDefault();
#endif
    
    {
        std::string fontName = "FontAwesome.ttf";

        float fontSize = 32.0f;
        std::string fontPath = PathCombine(PathCombine(assetDir, "fonts"), fontName);

        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        icons_config.OversampleH = 1;
        icons_config.OversampleV = 1;
        
        static const ImWchar ranges[] =
          {
              ICON_MIN_FA, ICON_MAX_FA,
              0,
          };
        g_font_awesome = ImGuiSupport_AddFontFromFile(fontPath, fontSize, &icons_config, ranges);
    }

    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);

    //IM_ASSERT(font != NULL);
    
    // Setup Platform/Renderer bindings
    ImGui_ImplRender_Init( context, assetDir);
    


}

void    ImGuiSupport_Render()
{
    
    PROFILE_FUNCTION_CAT("ui");
    ImGui::Render();
    ImGui_ImplRender_RenderDrawData(g_context, ImGui::GetDrawData() );
}

void     ImGuiSupport_Shutdown()
{
    ImGui_ImplRender_Shutdown();
    ImGui::DestroyContext();
}


namespace ImGui {


    static  float _values_getter(void* data, int idx)
    {
        auto func = (std::function<float (int idx)> *)data;
        return (*func)(idx);
    }


    void PlotHistogram(const char* label, std::function<float (int idx)> value_func, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size)
    {
        ImGui::PlotHistogram(label, _values_getter, &value_func, values_count, values_offset, overlay_text, scale_min, scale_max, graph_size);
    }


}
