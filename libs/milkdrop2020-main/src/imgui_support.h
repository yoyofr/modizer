
#pragma once

#include <string>
#include <functional>
#include <memory>

#include "render/MDcontext.h"
#include "FontAwesome.h"
#include "keycode.h"
#include "../external/imgui/imgui.h"


extern void     ImGuiSupport_Init(render::ContextPtr context, std::string assetDir);
extern void     ImGuiSupport_Shutdown();
extern void     ImGuiSupport_NewFrame();
extern void     ImGuiSupport_Render();



namespace ImGui
{
    template<typename T>
    inline bool RadioButtonT(const char* label, T* v, T v_button)
    {
        const bool pressed = RadioButton(label, *v == v_button);
        if (pressed)
        *v = v_button;
        return pressed;
    }
    


    void PlotHistogram(const char* label, std::function<float (int idx)> value_func, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size);

    

}




inline ImVec2 operator+(const ImVec2 &a, const ImVec2 & b)
{
    return ImVec2(a.x + b.x, a.y + b.y);
}

inline ImVec2 operator-(const ImVec2 & a, const ImVec2 & b)
{
    return ImVec2(a.x - b.x, a.y - b.y);
}

inline ImVec2 operator*(const ImVec2 & a, const ImVec2 & b)
{
    return ImVec2(a.x * b.x, a.y * b.y);
}

inline ImVec2 operator/(const ImVec2 & a, const ImVec2 & b)
{
    return ImVec2(a.x / b.x, a.y / b.y);
}

inline ImVec2 operator*(const ImVec2 &a, float s)
{
    return ImVec2(a.x * s, a.y * s);
}

