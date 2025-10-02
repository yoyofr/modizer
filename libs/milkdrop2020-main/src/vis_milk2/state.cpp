/*
  LICENSE
  -------
Copyright 2005-2013 Nullsoft, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer. 

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 

  * Neither the name of Nullsoft nor the names of its contributors may be used to 
    endorse or promote products derived from this software without specific prior written permission. 
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "platform.h"
#include "state.h"
#include "plugin.h"
#include <map>
#include <iostream>
#define FRAND frand()



float CState::frand()
{
    return m_plugin->frand();
}


// These are intended to replace GetPrivateProfileInt/FloatString, which are very slow
//  for large files (they always start from the top).  (really slow - some preset loads 
//  were taking 90 ms because of these!)
// The trick here is that we assume the file will be ordered correctly.  If not - if 
//  the next line doesn't have the expected token - we rescan from the top.  If the line
//  is never found, we use the default value, and leave MyGetPos untouched.


class PresetWriter
{
public:
    PresetWriter()
    {
        
    }
    
    
    void SetPrefix(std::string prefix) {m_prefix = prefix;}

    void Write(const char *name, int v);
    void Write(const char *name, float v);
    void Write(const char *name, float v, int digits);
    void Write(const char *name, bool v);
    void Write(const char *name, CBlendableFloat &v, int digits = 3);
    void WriteHeader(const char *name);
    void WriteCode(const char *name, const std::string &code, bool prefixLines = false);
    
    std::string GetString() const
    {
        return m_stream.str();
    }

private:
    std::string       m_prefix;
    std::stringstream m_stream;
};


void PresetWriter::WriteHeader(const char *name)
{
    m_stream << name << std::endl;
}

void PresetWriter::Write(const char *name, int v)
{
    m_stream << m_prefix << name << "=" << v << std::endl;
}
void PresetWriter::Write(const char *name, float v)
{
    Write(name, v, 3);
}

void PresetWriter::Write(const char *name, CBlendableFloat &v, int digits)
{
    Write(name, v.eval(-1), digits);
}

void PresetWriter::Write(const char *name, float v, int digits)
{
    char buffer[256];
    if (digits == 5)
        snprintf(buffer, sizeof(buffer), "%.5f", v);
    else
        snprintf(buffer, sizeof(buffer), "%.3f", v);

    m_stream << m_prefix << name << "=" << buffer << std::endl;
}

void PresetWriter::Write(const char *name, bool v)
{
    m_stream << m_prefix << name << "=" << (v ? 1 : 0) << std::endl;
}

void PresetWriter::WriteCode(const char *name, const std::string &code, bool prefixLines)
{
    int line_number = 1;
    std::string line_str;
    
    for (auto c : code)
    {
        if (c != '\n')
        {
            line_str += c;
        }
        else
        {
            m_stream << m_prefix << name << line_number << '=';
            
            if (prefixLines)
            {
                m_stream << '`';
            }
            m_stream << line_str;
            m_stream << std::endl;

            line_str.clear();
            line_number++;
        }
    }
}



class PresetReader
{
public:
	PresetReader()
	{
	}
    
    
    void Parse(const std::string &text);

    bool Load(std::string path);
	std::string ReadCode(const char* prefix);

    void SetPrefix(std::string prefix) {m_prefix = prefix;}
    void  Serialize(const char* szVarName, bool   &v);
    void  Serialize(const char* szVarName, int   &v);
    void  Serialize(const char* szVarName, float &v);
    void  Serialize(const char* szVarName, CBlendableFloat &v);

    int   ReadInt(const char* szVarName, int   def);

private:
    float ReadFloat(const char* szVarName, float def);

    bool _GetLineByName(const char* szVarName, std::string &value);
    std::string m_prefix;
    std::map<std::string, std::string> m_varmap;
};

static void SplitLines(const std::string &text, std::vector<std::string> &lines)
{
    auto pos = text.begin();
    auto begin = pos;
    
    while (pos != text.end())
    {
        char c = *pos++;
        
        if (c == '\n')
        {
            std::string line(begin, pos - 1);
            if (!line.empty()) {
                lines.push_back(line);
            }
            
            begin = pos;
        }
    }

    // add last line
    if (begin != text.end())
    {
        std::string line(begin, pos);
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
}

void PresetReader::Parse(const std::string &text)
{
    std::vector<std::string> lines;
    SplitLines(text, lines);
    for (auto &line : lines)
    {
        //        printf("line: %s\n", line.c_str());
        auto pos = line.begin();
        while (pos != line.end())
        {
            char c = *pos++;
            if (c == '=' || c == ' ')
            {
                std::string name(line.begin(), pos - 1);
                
                // skip backquotes in value
                if (pos != line.end() && *pos == '`') {
                    pos++;
                }
                
                std::string value(pos, line.end());
                if (!name.empty()) {
//                    printf("var: '%s'='%s'\n", name.c_str(), value.c_str());
                    m_varmap[name] = value;
                }
                
                break;
            }
        }
    }

}
bool PresetReader::Load(std::string path)
{
    std::string text;
    if (!FileReadAllText(path, text))
        return false;
    Parse(text);
    return true;
}


bool PresetReader::_GetLineByName(const char* szVarName, std::string &value)
{
    // lines in the file look like this:  szVarName=szRet
    //                               OR:  szVarName szRet
    // the part of the line after the '=' sign (or space) goes into szRet.  
    // szVarName can't have any spaces in it.
    
    std::string name = m_prefix;
    name += szVarName;

    auto it = m_varmap.find(name);
    if (it == m_varmap.end()) {
        value.clear();
        return false;
    }
    
    value = it->second;
    return true;
}

int PresetReader::ReadInt   (const char* szVarName, int   def)
{
    std::string buf;
    if (!_GetLineByName(szVarName, buf))
        return def;
    int ret;
    if (sscanf(buf.c_str(), "%d", &ret)==1)
    {
        return ret;
    }
    return def;
}

float PresetReader::ReadFloat (const char* szVarName, float def)
{
    std::string buf;
    if (!_GetLineByName(szVarName, buf))
        return def;
    float ret;
	if (sscanf(buf.c_str(), "%f", &ret)==1)
	{
        return ret;
	}
	return def;
}


void  PresetReader::Serialize(const char* szVarName, bool   &v)
{
    v = ReadInt(szVarName, v ? 1 : 0) != 0;
}
void  PresetReader::Serialize(const char* szVarName, int   &v)
{
    v = ReadInt(szVarName, v);
}
void  PresetReader::Serialize(const char* szVarName, float &v)
{
    v = ReadFloat(szVarName, v);
}

void  PresetReader::Serialize(const char* szVarName, CBlendableFloat &v)
{
    v = ReadFloat(szVarName, v.eval(-1));
}



std::string PresetReader::ReadCode(const char* prefix)
{
	std::string o;

	// read in & compile arbitrary expressions
	char szLineName[32];
    std::string szLine;

	int line = 1;
    for (;;)
	{
		sprintf(szLineName, "%s%d", prefix, line);
        
        if (!_GetLineByName(szLineName, szLine))
		{
            break;
		}
		else
		{
            if (!szLine.empty() && szLine[0] == '`') {
                szLine.erase(szLine.begin() + 0 );
            }
            o += szLine;
			o += '\n';
		}

		line++;
	}
	return o;
}




CState::CState(CPlugin *plugin)
    :m_plugin(plugin)
{
	//Default();
    
    m_waves.clear();
    for (int i=0; i<MAX_CUSTOM_WAVES; i++)
    {
        m_waves.push_back(new CWave(i));
    }
    
    m_shapes.clear();
    for (int i=0; i<MAX_CUSTOM_SHAPES; i++)
    {
        m_shapes.push_back(new CShape(i));
    }
    
    InstanceCounter<CState>::Increment();
}

CState::~CState()
{
    InstanceCounter<CState>::Decrement();

	FreeScriptObjects();
    
    
    for (auto wave : m_waves)
    {
        delete wave;
    }
    
    for (auto shape : m_shapes)
    {
        delete shape;
    }
}

class ImGuiSerializer
{
public:
    void Serialize(const char *name, CBlendableFloat &f)
    {
        ImGui::InputFloat(name, f.GetPtr());
    }
    
    void Serialize(const char *name, int &v)
    {
        ImGui::InputInt(name, &v);
    }
    
    void Serialize(const char *name, float &f)
    {
        ImGui::InputFloat(name, &f);
    }
    
    void Serialize(const char *name, bool &b)
    {
        ImGui::Checkbox(name, &b);
    }
};


void CWave::ShowDebugUI()
{
    
     if (ImGui::BeginTabBar(GetName().c_str(), ImGuiTabBarFlags_None))
    {
        if (ImGui::BeginTabItem("State"))
        {
            ImGuiSerializer s;

            s.Serialize("enabled", enabled);
            s.Serialize("samples", samples);
            s.Serialize("sep", sep);
            
            s.Serialize("scaling", scaling);
            s.Serialize("smoothing", smoothing);
            ImGui::InputFloat4("rgba", &rgba.r);
            
            s.Serialize("bSpectrum", bSpectrum);
            s.Serialize("bUseDots", bUseDots);
            s.Serialize("bDrawThick", bDrawThick);
            s.Serialize("bAdditive", bAdditive);

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("PerFrame"))
        {
            m_perframe_context->DebugUI();
                   ImGui::Separator();
            m_perframe_init->DebugUI();
                   ImGui::Separator();
            m_perframe_expression->DebugUI();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("PerPoint"))
        {
            m_perpoint_context->DebugUI();
                   ImGui::Separator();
            m_perpoint_expression->DebugUI();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    

}
    
void CShape::ShowDebugUI()
{
    if (ImGui::BeginTabBar(GetName().c_str(), ImGuiTabBarFlags_None))
      {
          if (ImGui::BeginTabItem("State"))
          {
              ImGuiSerializer s;
                    s.Serialize("enabled", enabled);
                    s.Serialize("sides", sides);
                    s.Serialize("additive", additive);
                    s.Serialize("thickOutline", thickOutline);
                    s.Serialize("textured", textured);
                    s.Serialize("instances", instances);

                    s.Serialize("tex_zoom", tex_zoom);
                    s.Serialize("tex_ang", tex_ang);

                    ImGui::InputFloat2("xy", &x);
                    s.Serialize("rad", rad);
                    s.Serialize("ang", ang);
                    
                //    ImGui::InputFloat("r", &r);
                //    ImGui::InputFloat("g", &g);
                //    ImGui::InputFloat("b", &b);
                //    ImGui::InputFloat("a", &a);
                    
                    ImGui::InputFloat4("rgba", &rgba.r);
                    ImGui::InputFloat4("rgba2", &rgba2.r);
                    ImGui::InputFloat4("border", &border.r);
                    ImGui::EndTabItem();
          }
          
          if (ImGui::BeginTabItem("PerFrame"))
               {
                   m_perframe_context->DebugUI();
                          ImGui::Separator();
                   m_perframe_init->DebugUI();
                          ImGui::Separator();
                   m_perframe_expression->DebugUI();
                   ImGui::EndTabItem();
               }

      
          ImGui::EndTabBar();
      }



}


void CState::ShowDebugUI( CStateDebugPanel mode)
{
    ImGuiSerializer pr;
    
    // general:
    if (mode == CStateDebugPanel::General)
    {
        pr.Serialize("fRating",m_fRating);
        pr.Serialize("fDecay",m_fDecay);
        pr.Serialize("fGammaAdj" ,m_fGammaAdj);
        pr.Serialize("fVideoEchoZoom",m_fVideoEchoZoom);
        pr.Serialize("fVideoEchoAlpha",m_fVideoEchoAlpha);
        pr.Serialize("nVideoEchoOrientation",m_nVideoEchoOrientation);
        pr.Serialize("bRedBlueStereo", m_bRedBlueStereo);
        pr.Serialize("bBrighten",m_bBrighten    );
        pr.Serialize("bDarken"  ,m_bDarken    );
        pr.Serialize("bSolarize",m_bSolarize    );
        pr.Serialize("bInvert"  ,m_bInvert    );
        pr.Serialize("fShader",m_fShader);
        pr.Serialize("b1n",    m_fBlur1Min);
        pr.Serialize("b2n",    m_fBlur2Min);
        pr.Serialize("b3n",    m_fBlur3Min);
        pr.Serialize("b1x",    m_fBlur1Max);
        pr.Serialize("b2x",    m_fBlur2Max);
        pr.Serialize("b3x",    m_fBlur3Max);
        pr.Serialize("b1ed",   m_fBlur1EdgeDarken);
    }
    
    // wave:
    if (mode == CStateDebugPanel::Wave)
    {
        pr.Serialize("nWaveMode",m_nWaveMode);
        pr.Serialize("bAdditiveWaves",m_bAdditiveWaves);
        pr.Serialize("bWaveDots",m_bWaveDots);
        pr.Serialize("bWaveThick",m_bWaveThick);
        pr.Serialize("bModWaveAlphaByVolume",m_bModWaveAlphaByVolume);
        pr.Serialize("bMaximizeWaveColor" ,m_bMaximizeWaveColor);
        pr.Serialize("fWaveAlpha",m_fWaveAlpha);
        pr.Serialize("fWaveScale",m_fWaveScale);
        pr.Serialize("fWaveSmoothing",m_fWaveSmoothing);
        pr.Serialize("fWaveParam",m_fWaveParam);
        pr.Serialize("fModWaveAlphaStart",m_fModWaveAlphaStart);
        pr.Serialize("fModWaveAlphaEnd",m_fModWaveAlphaEnd);
        pr.Serialize("wave_r",m_fWaveR);
        pr.Serialize("wave_g",m_fWaveG);
        pr.Serialize("wave_b",m_fWaveB);
        pr.Serialize("wave_x",m_fWaveX);
        pr.Serialize("wave_y",m_fWaveY);
        pr.Serialize("nMotionVectorsX",  m_fMvX);
        pr.Serialize("nMotionVectorsY",  m_fMvY);
        pr.Serialize("mv_dx",  m_fMvDX);
        pr.Serialize("mv_dy",  m_fMvDY);
        pr.Serialize("mv_l",   m_fMvL);
        pr.Serialize("mv_r",   m_fMvR);
        pr.Serialize("mv_g",   m_fMvG);
        pr.Serialize("mv_b",   m_fMvB);
        //        m_fMvA                = (pr.ReadInt ("bMotionVectorsOn",false) == 0) ? 0.0f : 1.0f; // for backwards compatibility
        pr.Serialize("mv_a",   m_fMvA);
    }
    
    // motion:
    if (mode == CStateDebugPanel::Motion)
    {
        pr.Serialize("zoom",m_fZoom);
        pr.Serialize("rot",m_fRot);
        pr.Serialize("cx",m_fRotCX);
        pr.Serialize("cy",m_fRotCY);
        pr.Serialize("dx",m_fXPush);
        pr.Serialize("dy",m_fYPush);
        pr.Serialize("warp",m_fWarpAmount);
        pr.Serialize("sx",m_fStretchX);
        pr.Serialize("sy",m_fStretchY);
        pr.Serialize("bTexWrap", m_bTexWrap);
        pr.Serialize("bDarkenCenter", m_bDarkenCenter);
        pr.Serialize("fWarpAnimSpeed",m_fWarpAnimSpeed);
        pr.Serialize("fWarpScale",m_fWarpScale);
        pr.Serialize("fZoomExponent",m_fZoomExponent);
        pr.Serialize("ob_size",m_fOuterBorderSize);
        pr.Serialize("ob_r",   m_fOuterBorderR);
        pr.Serialize("ob_g",   m_fOuterBorderG);
        pr.Serialize("ob_b",   m_fOuterBorderB);
        pr.Serialize("ob_a",   m_fOuterBorderA);
        pr.Serialize("ib_size",m_fInnerBorderSize);
        pr.Serialize("ib_r",   m_fInnerBorderR);
        pr.Serialize("ib_g",   m_fInnerBorderG);
        pr.Serialize("ib_b",   m_fInnerBorderB);
        pr.Serialize("ib_a",   m_fInnerBorderA);
//        m_szPerFrameInit = pr.ReadCode("per_frame_init_");
//        m_szPerFrameExpr = pr.ReadCode("per_frame_");
//        m_szPerPixelExpr = pr.ReadCode("per_pixel_");
    }


    
}


void CState::DebugUI(bool *open)
{
    if (ImGui::Begin("Preset", open))
    {
        ImGui::BeginChild("PresetComponents", ImVec2(150, 0), true);
        
        static CStateDebugPanel selected = CStateDebugPanel::None;

        if (ImGui::TreeNode("Preset"))
        {
            if (ImGui::Selectable("state", selected == CStateDebugPanel::State))  selected = CStateDebugPanel::State;
            if (ImGui::Selectable("general", selected == CStateDebugPanel::General))  selected = CStateDebugPanel::General;
            if (ImGui::Selectable("motion", selected == CStateDebugPanel::Motion))  selected = CStateDebugPanel::Motion;
            if (ImGui::Selectable("wave", selected == CStateDebugPanel::Wave))  selected = CStateDebugPanel::Wave;
            ImGui::TreePop();

        }

        
        if (ImGui::TreeNode("Shader"))
        {
            if (ImGui::Selectable("warp", selected == CStateDebugPanel::ShaderWarp))  selected = CStateDebugPanel::ShaderWarp;
            if (ImGui::Selectable("comp", selected == CStateDebugPanel::ShaderComp))  selected = CStateDebugPanel::ShaderComp;
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Custom Waves"))
        {
            if (ImGui::Selectable("wave0", selected == CStateDebugPanel::Wave0)) selected = CStateDebugPanel::Wave0;
            if (ImGui::Selectable("wave1", selected == CStateDebugPanel::Wave1)) selected = CStateDebugPanel::Wave1;
            if (ImGui::Selectable("wave2", selected == CStateDebugPanel::Wave2)) selected = CStateDebugPanel::Wave2;
            if (ImGui::Selectable("wave3", selected == CStateDebugPanel::Wave3)) selected = CStateDebugPanel::Wave3;
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Custom Shapes"))
        {
            if (ImGui::Selectable("shape0", selected == CStateDebugPanel::Shape0)) selected = CStateDebugPanel::Shape0;
            if (ImGui::Selectable("shape1", selected == CStateDebugPanel::Shape1)) selected = CStateDebugPanel::Shape1;
            if (ImGui::Selectable("shape2", selected == CStateDebugPanel::Shape2)) selected = CStateDebugPanel::Shape2;
            if (ImGui::Selectable("shape3", selected == CStateDebugPanel::Shape3)) selected = CStateDebugPanel::Shape3;
            ImGui::TreePop();
        }
        
        ImGui::EndChild();
        
        
        ImGui::SameLine();
        
        
        ImGui::BeginGroup();
        ImGui::BeginChild("item view", ImVec2(0, 0)); // Leave room for 1 line below us
        
        
        switch (selected)
        {
            case CStateDebugPanel::State:
            case CStateDebugPanel::General:
            case CStateDebugPanel::Wave:
            case CStateDebugPanel::Motion:
            {
                ShowDebugUI(selected);
                break;
            }
                
            case CStateDebugPanel::ShaderWarp:
            {
//                if (m_shader_warp)
                {
                    
                    
                    ImGui::TextWrapped("%s", m_szWarpShadersText.c_str());
                    
//                    char buffer[64  * 1024];
//                    strcpy(buffer, m_szWarpShadersText.c_str());
//                    ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
//                    if (ImGui::InputTextMultiline("shader-warp", buffer, sizeof(buffer),
//                                              ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16),
//                                              flags))
//                    {
//                        //m_szCompShadersText = buffer;
//                    }
                    
                    //m_shader_warp
                }
                break;
            }
            case CStateDebugPanel::ShaderComp:
            {
                ImGui::TextWrapped("%s", m_szCompShadersText.c_str());

            }
                break;

            case CStateDebugPanel::Wave0:
            case CStateDebugPanel::Wave1:
            case CStateDebugPanel::Wave2:
            case CStateDebugPanel::Wave3:
            {
                int i  = (int)selected - (int)CStateDebugPanel::Wave0;
                auto wave = m_waves[i];
                wave->ShowDebugUI();
                break;
            }

            case CStateDebugPanel::Shape0:
            case CStateDebugPanel::Shape1:
            case CStateDebugPanel::Shape2:
            case CStateDebugPanel::Shape3:
            {
                int i  = (int)selected - (int)CStateDebugPanel::Shape0;
                auto shape = m_shapes[i];
                shape->ShowDebugUI();
                break;
            }
            default:
                break;
        }
     
        
        ImGui::EndChild();
        ImGui::EndGroup();
    }
    
    ImGui::End();
}

#if 0
void CState::DebugUI()
{
    static bool _open = false;
    if (ImGui::Begin("State", &_open))
    {
        std::vector< Script::IKernelPtr > kernels;
        
        kernels.push_back( m_perframe_init );
        kernels.push_back( m_perframe_expression );
        kernels.push_back(  m_pervertex_expression );

        for (int i=0; i < MAX_CUSTOM_WAVES; i++)
        {
            kernels.push_back( m_wave[i].m_perframe_init  );
            kernels.push_back( m_wave[i].m_perframe_expression  );
            kernels.push_back( m_wave[i].m_perpoint_expression  );
        }

        
        for (int i=0; i < MAX_CUSTOM_SHAPES; i++)
        {
            kernels.push_back( m_shape[i].m_perframe_init  );
            kernels.push_back( m_shape[i].m_perframe_expression);
        }
        
        static int selected = 0;
        ImGui::BeginChild("Kernels", ImVec2(150, 0), true);
        for (int i = 0; i < (int) kernels.size(); i++)
        {
            auto k = kernels[i];
            if (ImGui::Selectable(k->GetName().c_str(), selected == i,
                                 k->IsEmpty() ? ImGuiSelectableFlags_Disabled : 0
                                  ))
            {
                selected = i;
            }
        }
        ImGui::EndChild();
        
        
        ImGui::SameLine();
        
        // right
        ImGui::BeginGroup();
        ImGui::BeginChild("item view", ImVec2(0, 0)); // Leave room for 1 line below us
        
        auto k = kernels[selected];
        
//        ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us
//        ImGui::Text("Kernel: %s", k->GetName().c_str());
        
        ImGui::Separator();
        if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Source"))
            {
                ImGui::TextWrapped("%s\n", k->GetSource().c_str());
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Disassembly"))
            {
                ImGui::TextWrapped("%s\n", k->GetDisassembly().c_str());
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        
        ImGui::EndChild();
//        if (ImGui::Button("Revert")) {}
//        ImGui::SameLine();
//        if (ImGui::Button("Save")) {}
        ImGui::EndGroup();
        
        
        
//        ImGui::TextWrapped("Preset:        %s\n", m_pState->GetDescription()   );
//
//        ImGui::Separator();
//
//        ImGui::Text("Progress: ");
//        ImGui::SameLine();
//        ImGui::ProgressBar( GetPresetProgress(), ImVec2(100, 10) );
//
//        ImGui::Separator();
//
        //            ImGui::LabelText("RenderTime", "%3.2fms", m_renderFrameTime );
        
        
        
//        ImGui::Separator();
        
        //   auto stats = m_scriptEngine->GetStats();
        
        //            ImGui::InvisibleButton("table", ImVec2(300, 1));
        //            ImGui::Columns(2);
        //
        //                ImGui::SetColumnOffset(0, 0);
        //                ImGui::SetColumnOffset(1, 100);
        //
        //                ImGui::Text("Script compile count:");
        //                ImGui::NextColumn();
        //                ImGui::Text("%d",  stats->compileCount );
        //                ImGui::NextColumn();
        //
        //                ImGui::Text("Script compile time:");
        //                ImGui::NextColumn();
        //                ImGui::Text("%3.2fms\n", stats->compileTime );
        //                ImGui::NextColumn();
        //            ImGui::Columns(1);
        
//        ImGui::Text("Debug: %s\n",  PlatformIsDebug() ? "true" : "false" );
//        ImGui::Text("Platform: %s\n", PlatformGetName() );
//
//        ImGui::Separator();
///Users/iaddis/dev/milkdrop2/app/OSX/VisualizerGLView.mm
        
        
        
//        ImGui::Text("RenderTime:        %3.2fms\n", m_renderFrameTime );
        //            ImGui::Text("Script compile count: %d\n",  stats->compileCount );
        //            ImGui::Text("Script compile time:  %3.2fms\n", stats->compileTime );
        //            ImGui::Text("Script eval count: %d\n",  stats->evalCount );
        //            ImGui::Text("Script eval time:  %3.2fms\n", stats->evalTime );
        
//
//        ImGui::Text("Shader comp compile time:  %3.2fms\n", m_pState->m_shader_comp.compileTime );
//        ImGui::Text("Shader warp compile time:  %3.2fms\n", m_pState->m_shader_warp.compileTime );
//
//
  
        
    }
    
    ImGui::End();
        
        
        
}
#endif


void CState::Dump()
{
    m_perframe_context->DumpStats();
    m_perframe_expression->DumpStats(true);
    
    m_pervertex_context->DumpStats();
    m_pervertex_expression->DumpStats(true);
    m_pervertex_buffer->DumpStats();
    
    for (auto wave : m_waves)
    {
        if (wave->enabled)
        {

            wave->m_perframe_context->DumpStats();
            
            wave->m_perframe_init->DumpStats(true);
            wave->m_perframe_expression->DumpStats(true);

//            wave->m_perpoint_init=->DumpStats(true);
            wave->m_perpoint_context->DumpStats();
            wave->m_perpoint_expression->DumpStats(true);
            wave->m_perpoint_buffer->DumpStats();
            
        }
    }
    
    
    for (auto shape : m_shapes)
    {
        if (shape->enabled)
        {
            shape->m_perframe_init->DumpStats(true);
            shape->m_perframe_expression->DumpStats(true);
        
        }
    }
}


void RegisterStruct(Script::IContextPtr context, CommonVars &common)
{
    common.RegisterStruct(context);
}

void CShape::RegisterVars()
{
    m_perframe_context= Script::mdpx::CreateContext(name + "_perframe");
    RegisterStruct(m_perframe_context, var_pf_common);
    m_perframe_context->RegVars("q", 1, NUM_Q_VAR, var_pf_q);
    m_perframe_context->RegVars("t", 1, NUM_T_VAR, var_pf_t);
    m_perframe_context->RegVar("x", var_pf_x, Script::VarUsage::Parameter);         // i/o
    m_perframe_context->RegVar("y", var_pf_y, Script::VarUsage::Parameter);         // i/o
    m_perframe_context->RegVar("rad", var_pf_rad, Script::VarUsage::Parameter);         // i/o
    m_perframe_context->RegVar("ang", var_pf_ang, Script::VarUsage::Parameter);         // i/o
    m_perframe_context->RegVar("tex_ang", var_pf_tex_ang, Script::VarUsage::Parameter);         // i/o
    m_perframe_context->RegVar("tex_zoom", var_pf_tex_zoom, Script::VarUsage::Parameter);         // i/o
    m_perframe_context->RegVar("sides", var_pf_sides, Script::VarUsage::Parameter);         // i/o
    m_perframe_context->RegVar("textured", var_pf_textured, Script::VarUsage::Parameter);         // i/o
    m_perframe_context->RegVar("instance", var_pf_instance, Script::VarUsage::Parameter);         // i/o
    m_perframe_context->RegVar("num_inst", var_pf_instances, Script::VarUsage::Parameter);         // i/o
    m_perframe_context->RegVar("additive", var_pf_additive, Script::VarUsage::Parameter);         // i/o
    m_perframe_context->RegVar("thick", var_pf_thick, Script::VarUsage::Parameter);         // i/o
    m_perframe_context->RegVar("r", var_pf_rgba.r, Script::VarUsage::Parameter);         // i/o
    m_perframe_context->RegVar("g", var_pf_rgba.g, Script::VarUsage::Parameter);         // i/o
    m_perframe_context->RegVar("b", var_pf_rgba.b, Script::VarUsage::Parameter);         // i/o
    m_perframe_context->RegVar("a", var_pf_rgba.a, Script::VarUsage::Parameter);         // i/o
    m_perframe_context->RegVar("r2", var_pf_rgba2.r, Script::VarUsage::Parameter);         // i/o
    m_perframe_context->RegVar("g2", var_pf_rgba2.g, Script::VarUsage::Parameter);         // i/o
    m_perframe_context->RegVar("b2", var_pf_rgba2.b, Script::VarUsage::Parameter);         // i/o
    m_perframe_context->RegVar("a2", var_pf_rgba2.a, Script::VarUsage::Parameter);         // i/o
    m_perframe_context->RegVar("border_", var_pf_border, Script::VarUsage::Parameter);         // i/o

}
void CWave::RegisterVars()
{
    m_perframe_context = Script::mdpx::CreateContext( name + "_perframe");
    RegisterStruct(m_perframe_context, var_pf_common);
    m_perframe_context->RegVars("q", 1, NUM_Q_VAR, var_pf_q);
    m_perframe_context->RegVars("t", 1, NUM_T_VAR, var_pf_t);
    m_perframe_context->RegVar("", var_pf_rgba);         // i/o
    m_perframe_context->RegVar("samples", var_pf_samples);   // i/o
    
    m_perpoint_context = Script::mdpx::CreateContext(name + "_perpoint");
    RegisterStruct(m_perpoint_context, var_pp_common);
    m_perpoint_context->RegVars("q", 1, NUM_Q_VAR, var_pp_q);
    m_perpoint_context->RegVars("t", 1, NUM_T_VAR, var_pp_t);
    m_perpoint_context->RegVar("sample", var_pp_sample, Script::VarUsage::Parameter);    // i
    m_perpoint_context->RegVar("value1", var_pp_value1, Script::VarUsage::Parameter);    // i
    m_perpoint_context->RegVar("value2", var_pp_value2, Script::VarUsage::Parameter);    // i
    m_perpoint_context->RegVar("x", var_pp_x, Script::VarUsage::Parameter);         // i/o
    m_perpoint_context->RegVar("y", var_pp_y, Script::VarUsage::Parameter);         // i/o
    m_perpoint_context->RegVar("", var_pp_rgba, Script::VarUsage::Parameter);         // i/o

}
//--------------------------------------------------------------------------------

void CState::RegisterBuiltInVariables()
{
    {
        m_perframe_context = Script::mdpx::CreateContext("perframe");
        RegisterStruct(m_perframe_context, var_pf_common);
        m_perframe_context->RegVar("zoom", var_pf_zoom);		// i/o
	    m_perframe_context->RegVar("zoomexp", var_pf_zoomexp);	// i/o
	    m_perframe_context->RegVar("rot", var_pf_rot);		// i/o
	    m_perframe_context->RegVar("warp", var_pf_warp);		// i/o
	    m_perframe_context->RegVar("c", var_pf_cxy);		// i/o
	    m_perframe_context->RegVar("d", var_pf_dxy);		// i/o
	    m_perframe_context->RegVar("s", var_pf_sxy);		// i/o
	    m_perframe_context->RegVar("decay", var_pf_decay);
	    m_perframe_context->RegVar("wave_", var_pf_wave_rgba);
	    m_perframe_context->RegVar("wave_x", var_pf_wave_x);
	    m_perframe_context->RegVar("wave_y", var_pf_wave_y);
	    m_perframe_context->RegVar("wave_mystery", var_pf_wave_mystery);
	    m_perframe_context->RegVar("wave_mode", var_pf_wave_mode);
        m_perframe_context->RegVars("q", 1, NUM_Q_VAR, var_pf_q);
	    m_perframe_context->RegVar("ob_size", var_pf_ob_size);
	    m_perframe_context->RegVar("ob_", var_pf_ob_rgba);
	    m_perframe_context->RegVar("ib_size", var_pf_ib_size);
	    m_perframe_context->RegVar("ib_", var_pf_ib_rgba);
	    m_perframe_context->RegVar("mv_", var_pf_mv_xy);
	    m_perframe_context->RegVar("mv_d", var_pf_mv_dxy);
	    m_perframe_context->RegVar("mv_l", var_pf_mv_l);
	    m_perframe_context->RegVar("mv_", var_pf_mv_rgba);
	    m_perframe_context->RegVar("monitor", var_pf_monitor);
	    m_perframe_context->RegVar("echo_zoom", var_pf_echo_zoom);
	    m_perframe_context->RegVar("echo_alpha", var_pf_echo_alpha);
	    m_perframe_context->RegVar("echo_orient", var_pf_echo_orient);
        m_perframe_context->RegVar("wave_usedots", var_pf_wave_usedots);
        m_perframe_context->RegVar("wave_thick", var_pf_wave_thick);
        m_perframe_context->RegVar("wave_additive", var_pf_wave_additive);
        m_perframe_context->RegVar("wave_brighten", var_pf_wave_brighten);
        m_perframe_context->RegVar("darken_center", var_pf_darken_center);
        m_perframe_context->RegVar("gamma", var_pf_gamma);
        m_perframe_context->RegVar("wrap", var_pf_wrap);
        m_perframe_context->RegVar("invert", var_pf_invert);
        m_perframe_context->RegVar("brighten", var_pf_brighten);
        m_perframe_context->RegVar("darken", var_pf_darken);
        m_perframe_context->RegVar("solarize", var_pf_solarize);
        m_perframe_context->RegVar("blur1_min", var_pf_blur1min);
        m_perframe_context->RegVar("blur2_min", var_pf_blur2min);
        m_perframe_context->RegVar("blur3_min", var_pf_blur3min);
        m_perframe_context->RegVar("blur1_max", var_pf_blur1max);
        m_perframe_context->RegVar("blur2_max", var_pf_blur2max);
        m_perframe_context->RegVar("blur3_max", var_pf_blur3max);
        m_perframe_context->RegVar("blur1_edge_darken", var_pf_blur1_edge_darken);

	    // this is the list of variables that can be used for a PER-VERTEX calculation:
	    // ('vertex' meaning a vertex on the mesh) (as opposed to a once-per-frame calculation)

        m_pervertex_context = Script::mdpx::CreateContext("pervertex");
        RegisterStruct(m_pervertex_context, var_pv_common);
        m_pervertex_context->RegVar("zoom", var_pv_zoom, Script::VarUsage::Parameter);		// i/o
	    m_pervertex_context->RegVar("zoomexp", var_pv_zoomexp, Script::VarUsage::Parameter);	// i/o
	    m_pervertex_context->RegVar("rot", var_pv_rot, Script::VarUsage::Parameter);		// i/o
	    m_pervertex_context->RegVar("warp", var_pv_warp, Script::VarUsage::Parameter);		// i/o
	    m_pervertex_context->RegVar("c", var_pv_cxy, Script::VarUsage::Parameter);		// i/o
	    m_pervertex_context->RegVar("d", var_pv_dxy, Script::VarUsage::Parameter);		// i/o
	    m_pervertex_context->RegVar("s", var_pv_sxy, Script::VarUsage::Parameter);		// i/o
	    m_pervertex_context->RegVar("", var_pv_xy, Script::VarUsage::Parameter);			// i
	    m_pervertex_context->RegVar("rad", var_pv_rad, Script::VarUsage::Parameter);		// i
	    m_pervertex_context->RegVar("ang", var_pv_ang, Script::VarUsage::Parameter);		// i
        m_pervertex_context->RegVars("q", 1, NUM_Q_VAR, var_pv_q);
    }

    for (auto wave : m_waves)
    {
        wave->RegisterVars();
    }

    for (auto shape : m_shapes)
    {
        shape->RegisterVars();
    }
}

void CState::Default()
{
	// DON'T FORGET TO ADD NEW VARIABLES TO BLEND FUNCTION, IMPORT, and EXPORT AS WELL!!!!!!!!

    m_name.clear();

    m_nMinPSVersion   = 0;     
    m_nMaxPSVersion   = 0;     

	m_shader_warp = nullptr;
	m_shader_comp = nullptr;

    // general:
    {
        m_fRating				= 3.0f;
	    m_fDecay				= 0.98f;	// 1.0 = none, 0.95 = heavy decay
	    m_fGammaAdj				= 2.0f;		// 1.0 = reg; +2.0 = double, +3.0 = triple...
	    m_fVideoEchoZoom		= 2.0f;
	    m_fVideoEchoAlpha		= 0.0f;
	    m_nVideoEchoOrientation	= 0;		// 0-3
	    m_bRedBlueStereo		= false;
	    m_bBrighten				= false;
	    m_bDarken				= false;
	    m_bTexWrap				= true;
	    m_bDarkenCenter			= false;
	    m_bSolarize				= false;
	    m_bInvert				= false;
	    m_fShader				= 0.0f;
	    m_fBlur1Min             = 0.0f;
	    m_fBlur2Min             = 0.0f;
	    m_fBlur3Min             = 0.0f;
	    m_fBlur1Max             = 1.0f;
	    m_fBlur2Max             = 1.0f;
	    m_fBlur3Max             = 1.0f;
	    m_fBlur1EdgeDarken      = 0.25f;
    }

 
    // wave:
    {
	    m_nWaveMode				= 0;
	     m_nOldWaveMode			= -1;
	    m_bAdditiveWaves		= false;
	    m_bWaveDots				= false;
	    m_bWaveThick            = false;
        m_fWaveAlpha			= 0.8f;
	    m_fWaveScale			= 1.0f;
	    m_fWaveSmoothing		= 0.75f;	// 0 = no smoothing, 0.9 = HEAVY smoothing
	    m_fWaveParam			= 0.0f;
	    m_bModWaveAlphaByVolume = false;
	    m_fModWaveAlphaStart	= 0.75f;		// when relative volume hits this level, alpha -> 0
	    m_fModWaveAlphaEnd		= 0.95f;		// when relative volume hits this level, alpha -> 1
	    m_fWaveR		= 1.0f;
	    m_fWaveG		= 1.0f;
	    m_fWaveB		= 1.0f;
	    m_fWaveX		= 0.5f;
	    m_fWaveY		= 0.5f;
	    m_bMaximizeWaveColor	= true;
	    m_fMvX				  	= 12.0f;
	    m_fMvY					= 9.0f;
	    m_fMvDX                 = 0.0f;
	    m_fMvDY                 = 0.0f;
	    m_fMvL				  	= 0.9f;
	    m_fMvR                  = 1.0f;
	    m_fMvG                  = 1.0f;
	    m_fMvB                  = 1.0f;
	    m_fMvA                  = 1.0f;

    }

    // motion:
    {
        m_fWarpAnimSpeed		= 1.0f;		// additional timescaling for warp animation
	    m_fWarpScale			= 1.0f;
	    m_fZoomExponent			= 1.0f;
	    m_fZoom			= 1.0f;
	    m_fRot 			= 0.0f;
	    m_fRotCX		= 0.5f;
	    m_fRotCY		= 0.5f;
	    m_fXPush		= 0.0f;
	    m_fYPush		= 0.0f;
	    m_fWarpAmount	= 1.0f;
	    m_fStretchX     = 1.0f;
	    m_fStretchY     = 1.0f;
	    m_fOuterBorderSize = 0.01f;
	    m_fOuterBorderR	= 0.0f;
	    m_fOuterBorderG	= 0.0f;
	    m_fOuterBorderB	= 0.0f;
	    m_fOuterBorderA	= 0.0f;
	    m_fInnerBorderSize = 0.01f;
	    m_fInnerBorderR	= 0.25f;
	    m_fInnerBorderG	= 0.25f;
	    m_fInnerBorderB	= 0.25f;
	    m_fInnerBorderA	= 0.0f;

        // clear all code strings:
        m_szPerFrameInit.clear();
        m_szPerFrameExpr.clear();
        m_szPerPixelExpr.clear();
    }

	// DON'T FORGET TO ADD NEW VARIABLES TO BLEND FUNCTION, IMPORT, and EXPORT AS WELL!!!!!!!!
	// ALSO BE SURE TO REGISTER THEM ON THE MAIN MENU (SEE MILKDROP.CPP)

    // warp shader
    {
        m_szWarpShadersText.clear();
        m_nWarpPSVersion   = 0;     
    }
    
    // comp shader
    {
        m_szCompShadersText.clear();
        m_nCompPSVersion   = 0;     
    }

    RandomizePresetVars();

	FreeScriptObjects();
}

void CState::StartBlendFrom(CState *s_from, float fAnimTime, float fTimespan)
{
	CState *s_to = this;

	// bools, ints, and strings instantly change

	s_to->m_fVideoEchoAlphaOld		 = s_from->m_fVideoEchoAlpha.eval(-1);
	s_to->m_nVideoEchoOrientationOld = s_from->m_nVideoEchoOrientation;
	s_to->m_nOldWaveMode			 = s_from->m_nWaveMode;

	/*
	s_to->m_fVideoEchoAlphaOld		 = s_from->m_fVideoEchoAlpha.eval(-1);
	s_to->m_nVideoEchoOrientationOld = s_from->m_nVideoEchoOrientation;

	s_to->m_nOldWaveMode			= s_from->m_nWaveMode;
	s_to->m_nWaveMode				= s_from->m_nWaveMode;
	s_to->m_bAdditiveWaves			= s_from->m_bAdditiveWaves;
	s_to->m_nVideoEchoOrientation	= s_from->m_nVideoEchoOrientation;
	s_to->m_fWarpAnimSpeed			= s_from->m_fWarpAnimSpeed;	// would req. 10 phase-matches to blend this one!!!
	s_to->m_bWaveDots				= s_from->m_bWaveDots;
	s_to->m_bWaveThick				= s_from->m_bWaveThick;
	s_to->m_bModWaveAlphaByVolume	= s_from->m_bModWaveAlphaByVolume;
	s_to->m_bMaximizeWaveColor		= s_from->m_bMaximizeWaveColor;
	s_to->m_bTexWrap				= s_from->m_bTexWrap;			
	s_to->m_bDarkenCenter			= s_from->m_bDarkenCenter;
	s_to->m_bRedBlueStereo			= s_from->m_bRedBlueStereo;
	s_to->m_bBrighten				= s_from->m_bBrighten;
	s_to->m_bDarken					= s_from->m_bDarken;
	s_to->m_bSolarize				= s_from->m_bSolarize;
	s_to->m_bInvert					= s_from->m_bInvert;
	s_to->m_fRating					= s_from->m_fRating;
	*/

	// expr. eval. also copies over immediately (replaces prev.)
	
	/*
	//for (int e=0; e<MAX_EVALS; e++)
	{
		char szTemp[MAX_BIGSTRING_LEN];

		lstrcpy(szTemp, m_szPerFrameExpr);
		lstrcpy(m_szPerFrameExpr, s_to->m_szPerFrameExpr);
		lstrcpy(s_to->m_szPerFrameExpr, szTemp);

		lstrcpy(szTemp, m_szPerPixelExpr);
		lstrcpy(m_szPerPixelExpr, s_to->m_szPerPixelExpr);
		lstrcpy(s_to->m_szPerPixelExpr, szTemp);

		lstrcpy(szTemp, m_szPerFrameInit);
		lstrcpy(m_szPerFrameInit, s_to->m_szPerFrameInit);
		lstrcpy(s_to->m_szPerFrameInit, szTemp);
	}
	RecompileExpressions();
	s_to->RecompileExpressions();

	lstrcpy(m_szDesc,    s_to->m_szDesc);
	//lstrcpy(m_szSection, s_to->m_szSection);
	*/
	
	// CBlendableFloats & SuperValues blend over time 
	m_fGammaAdj      .StartBlendFrom(&s_from->m_fGammaAdj      , fAnimTime, fTimespan);
	m_fVideoEchoZoom .StartBlendFrom(&s_from->m_fVideoEchoZoom , fAnimTime, fTimespan);
	m_fVideoEchoAlpha.StartBlendFrom(&s_from->m_fVideoEchoAlpha, fAnimTime, fTimespan);
	m_fDecay         .StartBlendFrom(&s_from->m_fDecay         , fAnimTime, fTimespan);
	m_fWaveAlpha     .StartBlendFrom(&s_from->m_fWaveAlpha     , fAnimTime, fTimespan);
	m_fWaveScale     .StartBlendFrom(&s_from->m_fWaveScale     , fAnimTime, fTimespan);
	m_fWaveSmoothing .StartBlendFrom(&s_from->m_fWaveSmoothing , fAnimTime, fTimespan);
	m_fWaveParam     .StartBlendFrom(&s_from->m_fWaveParam     , fAnimTime, fTimespan);
	m_fWarpScale     .StartBlendFrom(&s_from->m_fWarpScale     , fAnimTime, fTimespan);
	m_fZoomExponent  .StartBlendFrom(&s_from->m_fZoomExponent  , fAnimTime, fTimespan);
	m_fShader        .StartBlendFrom(&s_from->m_fShader        , fAnimTime, fTimespan);
	m_fModWaveAlphaStart.StartBlendFrom(&s_from->m_fModWaveAlphaStart, fAnimTime, fTimespan);
	m_fModWaveAlphaEnd  .StartBlendFrom(&s_from->m_fModWaveAlphaEnd, fAnimTime, fTimespan);

	m_fZoom		.StartBlendFrom(&s_from->m_fZoom		, fAnimTime, fTimespan);
	m_fRot 		.StartBlendFrom(&s_from->m_fRot 		, fAnimTime, fTimespan);
	m_fRotCX	.StartBlendFrom(&s_from->m_fRotCX		, fAnimTime, fTimespan);
	m_fRotCY	.StartBlendFrom(&s_from->m_fRotCY		, fAnimTime, fTimespan);
	m_fXPush	.StartBlendFrom(&s_from->m_fXPush		, fAnimTime, fTimespan);
	m_fYPush	.StartBlendFrom(&s_from->m_fYPush		, fAnimTime, fTimespan);
	m_fWarpAmount.StartBlendFrom(&s_from->m_fWarpAmount,fAnimTime, fTimespan);
	m_fStretchX .StartBlendFrom(&s_from->m_fStretchX	, fAnimTime, fTimespan);
	m_fStretchY .StartBlendFrom(&s_from->m_fStretchY	, fAnimTime, fTimespan);
	m_fWaveR	.StartBlendFrom(&s_from->m_fWaveR		, fAnimTime, fTimespan);
	m_fWaveG	.StartBlendFrom(&s_from->m_fWaveG		, fAnimTime, fTimespan);
	m_fWaveB	.StartBlendFrom(&s_from->m_fWaveB		, fAnimTime, fTimespan);
	m_fWaveX	.StartBlendFrom(&s_from->m_fWaveX		, fAnimTime, fTimespan);
	m_fWaveY	.StartBlendFrom(&s_from->m_fWaveY		, fAnimTime, fTimespan);
	m_fOuterBorderSize	.StartBlendFrom(&s_from->m_fOuterBorderSize	, fAnimTime, fTimespan);
	m_fOuterBorderR		.StartBlendFrom(&s_from->m_fOuterBorderR	, fAnimTime, fTimespan);
	m_fOuterBorderG		.StartBlendFrom(&s_from->m_fOuterBorderG	, fAnimTime, fTimespan);
	m_fOuterBorderB		.StartBlendFrom(&s_from->m_fOuterBorderB	, fAnimTime, fTimespan);
	m_fOuterBorderA		.StartBlendFrom(&s_from->m_fOuterBorderA	, fAnimTime, fTimespan);
	m_fInnerBorderSize	.StartBlendFrom(&s_from->m_fInnerBorderSize	, fAnimTime, fTimespan);
	m_fInnerBorderR		.StartBlendFrom(&s_from->m_fInnerBorderR	, fAnimTime, fTimespan);
	m_fInnerBorderG		.StartBlendFrom(&s_from->m_fInnerBorderG	, fAnimTime, fTimespan);
	m_fInnerBorderB		.StartBlendFrom(&s_from->m_fInnerBorderB	, fAnimTime, fTimespan);
	m_fInnerBorderA		.StartBlendFrom(&s_from->m_fInnerBorderA	, fAnimTime, fTimespan);
	m_fMvX				.StartBlendFrom(&s_from->m_fMvX				, fAnimTime, fTimespan);
	m_fMvY				.StartBlendFrom(&s_from->m_fMvY				, fAnimTime, fTimespan);
	m_fMvDX				.StartBlendFrom(&s_from->m_fMvDX			, fAnimTime, fTimespan);
	m_fMvDY				.StartBlendFrom(&s_from->m_fMvDY			, fAnimTime, fTimespan);
	m_fMvL				.StartBlendFrom(&s_from->m_fMvL				, fAnimTime, fTimespan);
	m_fMvR				.StartBlendFrom(&s_from->m_fMvR				, fAnimTime, fTimespan);
	m_fMvG				.StartBlendFrom(&s_from->m_fMvG				, fAnimTime, fTimespan);
	m_fMvB				.StartBlendFrom(&s_from->m_fMvB				, fAnimTime, fTimespan);
	m_fMvA				.StartBlendFrom(&s_from->m_fMvA				, fAnimTime, fTimespan);
    m_fBlur1Min         .StartBlendFrom(&s_from->m_fBlur1Min        , fAnimTime, fTimespan);
    m_fBlur2Min         .StartBlendFrom(&s_from->m_fBlur2Min        , fAnimTime, fTimespan);
    m_fBlur3Min         .StartBlendFrom(&s_from->m_fBlur3Min        , fAnimTime, fTimespan);
    m_fBlur1Max         .StartBlendFrom(&s_from->m_fBlur1Max        , fAnimTime, fTimespan);
    m_fBlur2Max         .StartBlendFrom(&s_from->m_fBlur2Max        , fAnimTime, fTimespan);
    m_fBlur3Max         .StartBlendFrom(&s_from->m_fBlur3Max        , fAnimTime, fTimespan);
    m_fBlur1EdgeDarken  .StartBlendFrom(&s_from->m_fBlur1EdgeDarken , fAnimTime, fTimespan);

	// if motion vectors were transparent before, don't morph the # in X and Y - just
	// start in the right place, and fade them in.
	bool bOldStateTransparent = (s_from->m_fMvA.eval(-1) < 0.001f);
	bool bNewStateTransparent = (s_to->m_fMvA.eval(-1) < 0.001f);
	if (!bOldStateTransparent && bNewStateTransparent)
	{
		s_from->m_fMvX = s_to->m_fMvX.eval(fAnimTime);
		s_from->m_fMvY = s_to->m_fMvY.eval(fAnimTime);
		s_from->m_fMvDX = s_to->m_fMvDX.eval(fAnimTime);
		s_from->m_fMvDY = s_to->m_fMvDY.eval(fAnimTime);
		s_from->m_fMvL = s_to->m_fMvL.eval(fAnimTime);
		s_from->m_fMvR = s_to->m_fMvR.eval(fAnimTime);
		s_from->m_fMvG = s_to->m_fMvG.eval(fAnimTime);
		s_from->m_fMvB = s_to->m_fMvB.eval(fAnimTime);
	}
	if (bNewStateTransparent && !bOldStateTransparent)
	{
		s_to->m_fMvX = s_from->m_fMvX.eval(fAnimTime);
		s_to->m_fMvY = s_from->m_fMvY.eval(fAnimTime);
		s_to->m_fMvDX = s_from->m_fMvDX.eval(fAnimTime);
		s_to->m_fMvDY = s_from->m_fMvDY.eval(fAnimTime);
		s_to->m_fMvL = s_from->m_fMvL.eval(fAnimTime);
		s_to->m_fMvR = s_from->m_fMvR.eval(fAnimTime);
		s_to->m_fMvG = s_from->m_fMvG.eval(fAnimTime);
		s_to->m_fMvB = s_from->m_fMvB.eval(fAnimTime);
	}

}

void WriteCode(FILE* fOut, int i, std::string pStr, const char* prefix, bool bPrependApostrophe = false)
{
	char szLineName[32];
	int line = 1;
	int start_pos = 0;
	int char_pos = 0;

	while (pStr[start_pos] != 0)
	{
		while (	pStr[char_pos] != 0 &&
				pStr[char_pos] != '\n')
			char_pos++;

		sprintf(szLineName, "%s%d", prefix, line);

		char ch = pStr[char_pos];
		pStr[char_pos] = 0;
		//if (!WritePrivateProfileString(szSectionName,szLineName,&pStr[start_pos],szIniFile)) return false;
        fprintf(fOut, "%s=%s%s\n", szLineName, bPrependApostrophe ? "`" : "", &pStr[start_pos]);
		pStr[char_pos] = ch;

		if (pStr[char_pos] != 0) char_pos++;
		start_pos = char_pos;
		line++;
	}
}

void CState::Export(PresetWriter &pw)
{
    
    // IMPORTANT: THESE MUST BE THE FIRST TWO LINES.  Otherwise it is assumed to be a MilkDrop 1-era preset.
    if (m_nMaxPSVersion > 0)
    {
        pw.Write("MILKDROP_PRESET_VERSION", CUR_MILKDROP_PRESET_VERSION);
        pw.Write("PSVERSION"     ,m_nMaxPSVersion);  // the max
        pw.Write("PSVERSION_WARP",m_nWarpPSVersion);
        pw.Write("PSVERSION_COMP",m_nCompPSVersion);
    }
    
    // just for backwards compatibility; MilkDrop 1 can read MilkDrop 2 presets, minus the new features.
    // (...this section name allows the GetPrivateProfile*() functions to still work on milkdrop 1)
    pw.WriteHeader("[preset00]");
    
    pw.Write( "fRating",                m_fRating);
    pw.Write( "fGammaAdj",              m_fGammaAdj);
    pw.Write( "fDecay",                 m_fDecay);
    pw.Write( "fVideoEchoZoom",         m_fVideoEchoZoom);
    pw.Write( "fVideoEchoAlpha",        m_fVideoEchoAlpha);
    pw.Write( "nVideoEchoOrientation",  m_nVideoEchoOrientation);
    
    pw.Write( "nWaveMode",              m_nWaveMode);
    pw.Write( "bAdditiveWaves",         m_bAdditiveWaves);
    pw.Write( "bWaveDots",              m_bWaveDots);
    pw.Write( "bWaveThick",             m_bWaveThick);
    pw.Write( "bModWaveAlphaByVolume",  m_bModWaveAlphaByVolume);
    pw.Write( "bMaximizeWaveColor",     m_bMaximizeWaveColor);
    pw.Write( "bTexWrap",               m_bTexWrap            );
    pw.Write( "bDarkenCenter",          m_bDarkenCenter        );
    pw.Write( "bRedBlueStereo",         m_bRedBlueStereo     );
    pw.Write( "bBrighten",              m_bBrighten            );
    pw.Write( "bDarken",                m_bDarken            );
    pw.Write( "bSolarize",              m_bSolarize            );
    pw.Write( "bInvert",                m_bInvert            );
    
    pw.Write( "fWaveAlpha",             m_fWaveAlpha);
    pw.Write( "fWaveScale",             m_fWaveScale);
    pw.Write( "fWaveSmoothing",         m_fWaveSmoothing);
    pw.Write( "fWaveParam",             m_fWaveParam);
    pw.Write( "fModWaveAlphaStart",     m_fModWaveAlphaStart);
    pw.Write( "fModWaveAlphaEnd",       m_fModWaveAlphaEnd);
    pw.Write( "fWarpAnimSpeed",         m_fWarpAnimSpeed);
    pw.Write( "fWarpScale",             m_fWarpScale);
    pw.Write( "fZoomExponent",          m_fZoomExponent, 5);
    pw.Write( "fShader",                m_fShader);
    
    pw.Write( "zoom",                   m_fZoom      , 5);
    pw.Write( "rot",                    m_fRot       , 5);
    pw.Write( "cx",                     m_fRotCX     );
    pw.Write( "cy",                     m_fRotCY     );
    pw.Write( "dx",                     m_fXPush     , 5);
    pw.Write( "dy",                     m_fYPush     , 5);
    pw.Write( "warp",                   m_fWarpAmount, 5);
    pw.Write( "sx",                     m_fStretchX  , 5);
    pw.Write( "sy",                     m_fStretchY  , 5);
    pw.Write( "wave_r",                 m_fWaveR     );
    pw.Write( "wave_g",                 m_fWaveG     );
    pw.Write( "wave_b",                 m_fWaveB     );
    pw.Write( "wave_x",                 m_fWaveX     );
    pw.Write( "wave_y",                 m_fWaveY     );
    
    pw.Write( "ob_size",             m_fOuterBorderSize);
    pw.Write( "ob_r",                m_fOuterBorderR);
    pw.Write( "ob_g",                m_fOuterBorderG);
    pw.Write( "ob_b",                m_fOuterBorderB);
    pw.Write( "ob_a",                m_fOuterBorderA);
    pw.Write( "ib_size",             m_fInnerBorderSize);
    pw.Write( "ib_r",                m_fInnerBorderR);
    pw.Write( "ib_g",                m_fInnerBorderG);
    pw.Write( "ib_b",                m_fInnerBorderB);
    pw.Write( "ib_a",                m_fInnerBorderA);
    pw.Write( "nMotionVectorsX",     m_fMvX);
    pw.Write( "nMotionVectorsY",     m_fMvY);
    pw.Write( "mv_dx",               m_fMvDX);
    pw.Write( "mv_dy",               m_fMvDY);
    pw.Write( "mv_l",                m_fMvL);
    pw.Write( "mv_r",                m_fMvR);
    pw.Write( "mv_g",                m_fMvG);
    pw.Write( "mv_b",                m_fMvB);
    pw.Write( "mv_a",                m_fMvA);
    pw.Write( "b1n",                 m_fBlur1Min);
    pw.Write( "b2n",                 m_fBlur2Min);
    pw.Write( "b3n",                 m_fBlur3Min);
    pw.Write( "b1x",                 m_fBlur1Max);
    pw.Write( "b2x",                 m_fBlur2Max);
    pw.Write( "b3x",                 m_fBlur3Max);
    pw.Write( "b1ed",                m_fBlur1EdgeDarken);
    
    for (auto wave : m_waves)
        wave->Export(pw);
    
    for (auto shape : m_shapes)
        shape->Export(pw);
    
    // write out arbitrary expressions, one line at a time
    pw.WriteCode("per_frame_init_", m_szPerFrameInit );
    pw.WriteCode("per_frame_", m_szPerFrameExpr);
    pw.WriteCode("per_pixel_", m_szPerPixelExpr );
    if (m_nWarpPSVersion >= MD2_PS_2_0)
        pw.WriteCode("warp_", m_szWarpShadersText, true);
    if (m_nCompPSVersion >= MD2_PS_2_0)
        pw.WriteCode("comp_", m_szCompShadersText, true);
    
}


bool CState::Export(std::string szIniFile)
{
#if 0
    PresetWriter pw;
    Export(pw);
    return FileWriteAllText(szIniFile, pw.GetString());
#else

	FILE *fOut = fopen(szIniFile.c_str(), "wt");
	if (!fOut) return false;

    // IMPORTANT: THESE MUST BE THE FIRST TWO LINES.  Otherwise it is assumed to be a MilkDrop 1-era preset.
    if (m_nMaxPSVersion > 0)
    {
        fprintf(fOut, "MILKDROP_PRESET_VERSION=%d\n", CUR_MILKDROP_PRESET_VERSION);
        fprintf(fOut, "PSVERSION=%d\n"     ,m_nMaxPSVersion);  // the max
        fprintf(fOut, "PSVERSION_WARP=%d\n",m_nWarpPSVersion);
        fprintf(fOut, "PSVERSION_COMP=%d\n",m_nCompPSVersion);
    }
    
    // just for backwards compatibility; MilkDrop 1 can read MilkDrop 2 presets, minus the new features.
    // (...this section name allows the GetPrivateProfile*() functions to still work on milkdrop 1)
	fprintf(fOut, "[preset00]\n");    

	fprintf(fOut, "%s=%.3f\n", "fRating",                m_fRating);         
	fprintf(fOut, "%s=%.3f\n", "fGammaAdj",              m_fGammaAdj.eval(-1));         
	fprintf(fOut, "%s=%.3f\n", "fDecay",                 m_fDecay.eval(-1));            
	fprintf(fOut, "%s=%.3f\n", "fVideoEchoZoom",         m_fVideoEchoZoom.eval(-1));    
	fprintf(fOut, "%s=%.3f\n", "fVideoEchoAlpha",        m_fVideoEchoAlpha.eval(-1));   
	fprintf(fOut, "%s=%d\n", "nVideoEchoOrientation",  m_nVideoEchoOrientation);      

	fprintf(fOut, "%s=%d\n", "nWaveMode",              m_nWaveMode);                  
	fprintf(fOut, "%s=%d\n", "bAdditiveWaves",         m_bAdditiveWaves);             
	fprintf(fOut, "%s=%d\n", "bWaveDots",              m_bWaveDots);                  
	fprintf(fOut, "%s=%d\n", "bWaveThick",             m_bWaveThick);                  
	fprintf(fOut, "%s=%d\n", "bModWaveAlphaByVolume",  m_bModWaveAlphaByVolume);      
	fprintf(fOut, "%s=%d\n", "bMaximizeWaveColor",     m_bMaximizeWaveColor);         
	fprintf(fOut, "%s=%d\n", "bTexWrap",               m_bTexWrap			);         
	fprintf(fOut, "%s=%d\n", "bDarkenCenter",          m_bDarkenCenter		);         
	fprintf(fOut, "%s=%d\n", "bRedBlueStereo",         m_bRedBlueStereo     );
	fprintf(fOut, "%s=%d\n", "bBrighten",              m_bBrighten			);         
	fprintf(fOut, "%s=%d\n", "bDarken",                m_bDarken			);         
	fprintf(fOut, "%s=%d\n", "bSolarize",              m_bSolarize			);         
	fprintf(fOut, "%s=%d\n", "bInvert",                m_bInvert			);         

	fprintf(fOut, "%s=%.3f\n", "fWaveAlpha",             m_fWaveAlpha.eval(-1)); 		  
	fprintf(fOut, "%s=%.3f\n", "fWaveScale",             m_fWaveScale.eval(-1));        
	fprintf(fOut, "%s=%.3f\n", "fWaveSmoothing",         m_fWaveSmoothing.eval(-1));    
	fprintf(fOut, "%s=%.3f\n", "fWaveParam",             m_fWaveParam.eval(-1));        
	fprintf(fOut, "%s=%.3f\n", "fModWaveAlphaStart",     m_fModWaveAlphaStart.eval(-1));
	fprintf(fOut, "%s=%.3f\n", "fModWaveAlphaEnd",       m_fModWaveAlphaEnd.eval(-1));  
	fprintf(fOut, "%s=%.3f\n", "fWarpAnimSpeed",         m_fWarpAnimSpeed);             
	fprintf(fOut, "%s=%.3f\n", "fWarpScale",             m_fWarpScale.eval(-1));        
	fprintf(fOut, "%s=%.5f\n", "fZoomExponent",          m_fZoomExponent.eval(-1));     
	fprintf(fOut, "%s=%.3f\n", "fShader",                m_fShader.eval(-1));           

	fprintf(fOut, "%s=%.5f\n", "zoom",                   m_fZoom      .eval(-1));       
	fprintf(fOut, "%s=%.5f\n", "rot",                    m_fRot       .eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "cx",                     m_fRotCX     .eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "cy",                     m_fRotCY     .eval(-1));       
	fprintf(fOut, "%s=%.5f\n", "dx",                     m_fXPush     .eval(-1));       
	fprintf(fOut, "%s=%.5f\n", "dy",                     m_fYPush     .eval(-1));       
	fprintf(fOut, "%s=%.5f\n", "warp",                   m_fWarpAmount.eval(-1));       
	fprintf(fOut, "%s=%.5f\n", "sx",                     m_fStretchX  .eval(-1));       
	fprintf(fOut, "%s=%.5f\n", "sy",                     m_fStretchY  .eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "wave_r",                 m_fWaveR     .eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "wave_g",                 m_fWaveG     .eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "wave_b",                 m_fWaveB     .eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "wave_x",                 m_fWaveX     .eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "wave_y",                 m_fWaveY     .eval(-1));       

	fprintf(fOut, "%s=%.3f\n", "ob_size",             m_fOuterBorderSize.eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "ob_r",                m_fOuterBorderR.eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "ob_g",                m_fOuterBorderG.eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "ob_b",                m_fOuterBorderB.eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "ob_a",                m_fOuterBorderA.eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "ib_size",             m_fInnerBorderSize.eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "ib_r",                m_fInnerBorderR.eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "ib_g",                m_fInnerBorderG.eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "ib_b",                m_fInnerBorderB.eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "ib_a",                m_fInnerBorderA.eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "nMotionVectorsX",     m_fMvX.eval(-1));         
	fprintf(fOut, "%s=%.3f\n", "nMotionVectorsY",     m_fMvY.eval(-1));         
	fprintf(fOut, "%s=%.3f\n", "mv_dx",               m_fMvDX.eval(-1));         
	fprintf(fOut, "%s=%.3f\n", "mv_dy",               m_fMvDY.eval(-1));         
	fprintf(fOut, "%s=%.3f\n", "mv_l",                m_fMvL.eval(-1));         
	fprintf(fOut, "%s=%.3f\n", "mv_r",                m_fMvR.eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "mv_g",                m_fMvG.eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "mv_b",                m_fMvB.eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "mv_a",                m_fMvA.eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "b1n",                 m_fBlur1Min.eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "b2n",                 m_fBlur2Min.eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "b3n",                 m_fBlur3Min.eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "b1x",                 m_fBlur1Max.eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "b2x",                 m_fBlur2Max.eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "b3x",                 m_fBlur3Max.eval(-1));       
	fprintf(fOut, "%s=%.3f\n", "b1ed",                m_fBlur1EdgeDarken.eval(-1));       

    for (auto wave : m_waves)
        wave->Export(fOut, "dummy_filename");

    for (auto shape : m_shapes)
        shape->Export(fOut, "dummy_filename");

	// write out arbitrary expressions, one line at a time
    int i = 0;
    WriteCode(fOut, i, m_szPerFrameInit, "per_frame_init_");
    WriteCode(fOut, i, m_szPerFrameExpr, "per_frame_"); 
    WriteCode(fOut, i, m_szPerPixelExpr, "per_pixel_"); 
    if (m_nWarpPSVersion >= MD2_PS_2_0)
        WriteCode(fOut, i, m_szWarpShadersText, "warp_", true);
    if (m_nCompPSVersion >= MD2_PS_2_0)
        WriteCode(fOut, i, m_szCompShadersText, "comp_", true);

	fclose(fOut);

	return true;
#endif
}

int  CWave::Export(FILE* fOut, const char *szFile)
{
    int i = index;
    FILE* f2 = fOut;
    if (!fOut)
    {
	    f2 = fopen(szFile, "wt");
        if (!f2) return 0;
    }

    fprintf(f2, "wavecode_%d_%s=%d\n", i, "enabled",    enabled ? 1 : 0);
	fprintf(f2, "wavecode_%d_%s=%d\n", i, "samples",    samples);
	fprintf(f2, "wavecode_%d_%s=%d\n", i, "sep",        sep    );
	fprintf(f2, "wavecode_%d_%s=%d\n", i, "bSpectrum",  bSpectrum);
	fprintf(f2, "wavecode_%d_%s=%d\n", i, "bUseDots",   bUseDots);
	fprintf(f2, "wavecode_%d_%s=%d\n", i, "bDrawThick", bDrawThick);
	fprintf(f2, "wavecode_%d_%s=%d\n", i, "bAdditive",  bAdditive);
	fprintf(f2, "wavecode_%d_%s=%.5f\n", i, "scaling",    scaling);
	fprintf(f2, "wavecode_%d_%s=%.5f\n", i, "smoothing",  smoothing);
	fprintf(f2, "wavecode_%d_%s=%.3f\n", i, "r",          rgba.r);
	fprintf(f2, "wavecode_%d_%s=%.3f\n", i, "g",          rgba.g);
	fprintf(f2, "wavecode_%d_%s=%.3f\n", i, "b",          rgba.b);
	fprintf(f2, "wavecode_%d_%s=%.3f\n", i, "a",          rgba.a);

    // READ THE CODE IN
    char prefix[64];
    sprintf(prefix, "wave_%d_init",      i); WriteCode(f2, i, m_szInit,     prefix);
    sprintf(prefix, "wave_%d_per_frame", i); WriteCode(f2, i, m_szPerFrame, prefix);
    sprintf(prefix, "wave_%d_per_point", i); WriteCode(f2, i, m_szPerPoint, prefix);

    if (!fOut)
	    fclose(f2); // [sic]

    return 1;
}


void  CWave::Export(PresetWriter &pw)
{
    pw.SetPrefix( std::string("wavecode_") + std::to_string(index) + "_"  );
    
    pw.Write("enabled",    enabled ? 1 : 0);
    pw.Write("samples",    samples);
    pw.Write("sep",        sep    );
    pw.Write("bSpectrum",  bSpectrum);
    pw.Write("bUseDots",   bUseDots);
    pw.Write("bDrawThick", bDrawThick);
    pw.Write("bAdditive",  bAdditive);
    pw.Write("scaling",    scaling, 5);
    pw.Write("smoothing",  smoothing, 5);
    pw.Write("r",          rgba.r);
    pw.Write("g",          rgba.g);
    pw.Write("b",          rgba.b);
    pw.Write("a",          rgba.a);

    pw.SetPrefix( std::string("wave_") + std::to_string(index) + "_"  );

    pw.WriteCode("init", m_szInit);
    pw.WriteCode("per_frame", m_szPerFrame);
    pw.WriteCode("per_point", m_szPerPoint);

    pw.SetPrefix("");
}



int  CShape::Export(FILE* fOut, const char *szFile)
{
    int i = index;

    FILE* f2 = fOut;
    if (!fOut)
    {
	    f2 = fopen(szFile, "wt");
        if (!f2) return 0;
	    //fprintf(f2, "[%s]\n", szSection);
    }

    fprintf(f2, "shapecode_%d_%s=%d\n", i, "enabled",    enabled ? 1 : 0);
	fprintf(f2, "shapecode_%d_%s=%d\n", i, "sides",      sides);
	fprintf(f2, "shapecode_%d_%s=%d\n", i, "additive",   additive);
	fprintf(f2, "shapecode_%d_%s=%d\n", i, "thickOutline",thickOutline);
	fprintf(f2, "shapecode_%d_%s=%d\n", i, "textured",   textured);
	fprintf(f2, "shapecode_%d_%s=%d\n", i, "num_inst",   instances);
	fprintf(f2, "shapecode_%d_%s=%.3f\n", i, "x",          x);
	fprintf(f2, "shapecode_%d_%s=%.3f\n", i, "y",          y);
	fprintf(f2, "shapecode_%d_%s=%.5f\n", i, "rad",        rad);
	fprintf(f2, "shapecode_%d_%s=%.5f\n", i, "ang",        ang);
	fprintf(f2, "shapecode_%d_%s=%.5f\n", i, "tex_ang",    tex_ang);
	fprintf(f2, "shapecode_%d_%s=%.5f\n", i, "tex_zoom",   tex_zoom);
	fprintf(f2, "shapecode_%d_%s=%.3f\n", i, "r",          rgba.r);
	fprintf(f2, "shapecode_%d_%s=%.3f\n", i, "g",          rgba.g);
	fprintf(f2, "shapecode_%d_%s=%.3f\n", i, "b",          rgba.b);
	fprintf(f2, "shapecode_%d_%s=%.3f\n", i, "a",          rgba.a);
	fprintf(f2, "shapecode_%d_%s=%.3f\n", i, "r2",         rgba2.r);
	fprintf(f2, "shapecode_%d_%s=%.3f\n", i, "g2",         rgba2.g);
	fprintf(f2, "shapecode_%d_%s=%.3f\n", i, "b2",         rgba2.b);
	fprintf(f2, "shapecode_%d_%s=%.3f\n", i, "a2",         rgba2.a);
	fprintf(f2, "shapecode_%d_%s=%.3f\n", i, "border_r",   border.r);
	fprintf(f2, "shapecode_%d_%s=%.3f\n", i, "border_g",   border.g);
	fprintf(f2, "shapecode_%d_%s=%.3f\n", i, "border_b",   border.b);
	fprintf(f2, "shapecode_%d_%s=%.3f\n", i, "border_a",   border.a);

    char prefix[64];
    sprintf(prefix, "shape_%d_init",      i); WriteCode(f2, i, m_szInit,     prefix);
    sprintf(prefix, "shape_%d_per_frame", i); WriteCode(f2, i, m_szPerFrame, prefix);
    //sprintf(prefix, "shape_%d_per_point", i); WriteCode(f2, i, m_szPerPoint, prefix);

    if (!fOut)
	    fclose(f2); // [sic]

    return 1;
}


void  CShape::Export(PresetWriter &pw)
{
    
    pw.SetPrefix( std::string("shapecode_") + std::to_string(index) + "_"  );

    
    pw.Write("enabled",    enabled);
    pw.Write("sides",      sides);
    pw.Write("additive",   additive);
    pw.Write("thickOutline",thickOutline);
    pw.Write("textured",   textured);
    pw.Write("num_inst",   instances);
    pw.Write("x",          x);
    pw.Write("y",          y);
    pw.Write("rad",        rad, 5);
    pw.Write("ang",        ang, 5);
    pw.Write("tex_ang",    tex_ang, 5);
    pw.Write("tex_zoom",   tex_zoom, 5);
    pw.Write("r",          rgba.r);
    pw.Write("g",          rgba.g);
    pw.Write("b",          rgba.b);
    pw.Write("a",          rgba.a);
    pw.Write("r2",         rgba2.r);
    pw.Write("g2",         rgba2.g);
    pw.Write("b2",         rgba2.b);
    pw.Write("a2",         rgba2.a);
    pw.Write("border_r",   border.r);
    pw.Write("border_g",   border.g);
    pw.Write("border_b",   border.b);
    pw.Write("border_a",   border.a);
    pw.SetPrefix("");
    
    pw.SetPrefix( std::string("shape_") + std::to_string(index) + "_"  );

    pw.WriteCode("init", m_szInit);
    pw.WriteCode("per_frame", m_szPerFrame);
    //sprintf(prefix, "shape_%d_per_point", i); WriteCode(f2, i, m_szPerPoint, prefix);
    
    pw.SetPrefix("");

}




int CWave::Import(PresetReader &pr)
{
    pr.SetPrefix( std::string("wavecode_") + std::to_string(index) + "_"  );
    pr.Serialize("enabled"   , enabled   );
    pr.Serialize("samples"   , samples   );
    pr.Serialize("sep"       , sep       );
    pr.Serialize("bSpectrum" , bSpectrum );
    pr.Serialize("bUseDots"  , bUseDots  );
    pr.Serialize("bDrawThick", bDrawThick);
    pr.Serialize("bAdditive" , bAdditive );
    pr.Serialize("scaling"   , scaling   );
    pr.Serialize("smoothing" , smoothing );
    pr.Serialize("r"         , rgba.r         );
    pr.Serialize("g"         , rgba.g         );
    pr.Serialize("b"         , rgba.b         );
    pr.Serialize("a"         , rgba.a         );
    pr.SetPrefix("");

    // READ THE CODE IN
    char prefix[64];
    sprintf(prefix, "wave_%d_init",      index); m_szInit = pr.ReadCode(prefix);
    sprintf(prefix, "wave_%d_per_frame", index); m_szPerFrame = pr.ReadCode(prefix);
    sprintf(prefix, "wave_%d_per_point", index); m_szPerPoint = pr.ReadCode(prefix);

    return 1;
}

int  CShape::Import(PresetReader &pr)
{
    pr.SetPrefix( std::string("shapecode_") + std::to_string(index) + "_"  );
	pr.Serialize("enabled"     , enabled     );
	pr.Serialize("sides"       , sides       );
	pr.Serialize("additive"    , additive    );
	pr.Serialize("thickOutline", thickOutline);
	pr.Serialize("textured"    , textured    );
	pr.Serialize("num_inst"    , instances   );
	pr.Serialize("x"           , x           );
	pr.Serialize("y"           , y           );
	pr.Serialize("rad"         , rad         );
	pr.Serialize("ang"         , ang         );
	pr.Serialize("tex_ang"     , tex_ang     );
	pr.Serialize("tex_zoom"    , tex_zoom    );
	pr.Serialize("r"           , rgba.r           );
	pr.Serialize("g"           , rgba.g           );
	pr.Serialize("b"           , rgba.b           );
	pr.Serialize("a"           , rgba.a           );
	pr.Serialize("r2"          , rgba2.r          );
	pr.Serialize("g2"          , rgba2.g          );
	pr.Serialize("b2"          , rgba2.b          );
	pr.Serialize("a2"          , rgba2.a          );
	pr.Serialize("border_r"    , border.r    );
	pr.Serialize("border_g"    , border.g    );
	pr.Serialize("border_b"    , border.b    );
	pr.Serialize("border_a"    , border.a    );
    pr.SetPrefix("");

    
    // READ THE CODE IN
    char prefix[64];
    sprintf(prefix, "shape_%d_init",      index); m_szInit = pr.ReadCode(prefix);
    sprintf(prefix, "shape_%d_per_frame", index); m_szPerFrame = pr.ReadCode(prefix);

    return 1;
}


bool CState::ImportFromText(std::string text, std::string name, std::string &errors)
{
    PROFILE_FUNCTION()
    // if any ApplyFlags are missing, the settings will be copied from pOldState.  =)

    
    // apply defaults for the stuff we will overwrite.
    Default();//RandomizePresetVars();

    m_name = name;

	PresetReader pr;
    pr.Parse(text);


	int nMilkdropPresetVersion = pr.ReadInt("MILKDROP_PRESET_VERSION", 100);
    //if (ApplyFlags != STATE_ALL)
    //    nMilkdropPresetVersion = CUR_MILKDROP_PRESET_VERSION;  //if we're mashing up, force it up to now
    

    int nWarpPSVersionInFile;
    int nCompPSVersionInFile;
    if (nMilkdropPresetVersion < 200) {
        nWarpPSVersionInFile = 0;
        nCompPSVersionInFile = 0;
    }
    else if (nMilkdropPresetVersion == 200) {
        nWarpPSVersionInFile = pr.ReadInt("PSVERSION", 2);
        nCompPSVersionInFile = nWarpPSVersionInFile;
    }
    else {
        nWarpPSVersionInFile = pr.ReadInt("PSVERSION_WARP", 2);
        nCompPSVersionInFile = pr.ReadInt("PSVERSION_COMP", 2);
    }

    // general:
    {
        pr.Serialize("fRating",m_fRating);
	    pr.Serialize("fDecay",m_fDecay);
	    pr.Serialize("fGammaAdj" ,m_fGammaAdj);
	    pr.Serialize("fVideoEchoZoom",m_fVideoEchoZoom);
	    pr.Serialize("fVideoEchoAlpha",m_fVideoEchoAlpha);
	    pr.Serialize("nVideoEchoOrientation",m_nVideoEchoOrientation);
        pr.Serialize("bRedBlueStereo", m_bRedBlueStereo);
	    pr.Serialize("bBrighten",m_bBrighten	);
	    pr.Serialize("bDarken"  ,m_bDarken	);
	    pr.Serialize("bSolarize",m_bSolarize	);
	    pr.Serialize("bInvert"  ,m_bInvert	);
	    pr.Serialize("fShader",m_fShader);
        pr.Serialize("b1n",    m_fBlur1Min);
        pr.Serialize("b2n",    m_fBlur2Min);
        pr.Serialize("b3n",    m_fBlur3Min);
        pr.Serialize("b1x",    m_fBlur1Max);
        pr.Serialize("b2x",    m_fBlur2Max);
        pr.Serialize("b3x",    m_fBlur3Max);
        pr.Serialize("b1ed",   m_fBlur1EdgeDarken);
    }

    // wave:
    {
	    pr.Serialize("nWaveMode",m_nWaveMode);
	    pr.Serialize("bAdditiveWaves",m_bAdditiveWaves);
	    pr.Serialize("bWaveDots",m_bWaveDots);
	    pr.Serialize("bWaveThick",m_bWaveThick);
	    pr.Serialize("bModWaveAlphaByVolume",m_bModWaveAlphaByVolume);
	    pr.Serialize("bMaximizeWaveColor" ,m_bMaximizeWaveColor);
	    pr.Serialize("fWaveAlpha",m_fWaveAlpha);
	    pr.Serialize("fWaveScale",m_fWaveScale);
	    pr.Serialize("fWaveSmoothing",m_fWaveSmoothing);
	    pr.Serialize("fWaveParam",m_fWaveParam);
	    pr.Serialize("fModWaveAlphaStart",m_fModWaveAlphaStart);
	    pr.Serialize("fModWaveAlphaEnd",m_fModWaveAlphaEnd);
	    pr.Serialize("wave_r",m_fWaveR);
	    pr.Serialize("wave_g",m_fWaveG);
	    pr.Serialize("wave_b",m_fWaveB);
	    pr.Serialize("wave_x",m_fWaveX);
	    pr.Serialize("wave_y",m_fWaveY);
	    pr.Serialize("nMotionVectorsX",  m_fMvX);
	    pr.Serialize("nMotionVectorsY",  m_fMvY);
	    pr.Serialize("mv_dx",  m_fMvDX);
	    pr.Serialize("mv_dy",  m_fMvDY);
	    pr.Serialize("mv_l",   m_fMvL);
	    pr.Serialize("mv_r",   m_fMvR);
	    pr.Serialize("mv_g",   m_fMvG);
	    pr.Serialize("mv_b",   m_fMvB);
//	    m_fMvA				= (pr.ReadInt ("bMotionVectorsOn",false) == 0) ? 0.0f : 1.0f; // for backwards compatibility
	    pr.Serialize("mv_a",   m_fMvA);
        for (auto wave : m_waves)
        {
            wave->Import(pr);
        }
        for (auto shape : m_shapes)
        {
            shape->Import(pr);
        }
    }

    // motion:
    {
	    pr.Serialize("zoom",m_fZoom);
	    pr.Serialize("rot",m_fRot);
	    pr.Serialize("cx",m_fRotCX);
	    pr.Serialize("cy",m_fRotCY);
	    pr.Serialize("dx",m_fXPush);
	    pr.Serialize("dy",m_fYPush);
	    pr.Serialize("warp",m_fWarpAmount);
	    pr.Serialize("sx",m_fStretchX);
	    pr.Serialize("sy",m_fStretchY);
        pr.Serialize("bTexWrap", m_bTexWrap);
	    pr.Serialize("bDarkenCenter", m_bDarkenCenter);
	    pr.Serialize("fWarpAnimSpeed",m_fWarpAnimSpeed);
	    pr.Serialize("fWarpScale",m_fWarpScale);
	    pr.Serialize("fZoomExponent",m_fZoomExponent);
	    pr.Serialize("ob_size",m_fOuterBorderSize);
	    pr.Serialize("ob_r",   m_fOuterBorderR);
	    pr.Serialize("ob_g",   m_fOuterBorderG);
	    pr.Serialize("ob_b",   m_fOuterBorderB);
	    pr.Serialize("ob_a",   m_fOuterBorderA);
	    pr.Serialize("ib_size",m_fInnerBorderSize);
	    pr.Serialize("ib_r",   m_fInnerBorderR);
	    pr.Serialize("ib_g",   m_fInnerBorderG);
	    pr.Serialize("ib_b",   m_fInnerBorderB);
	    pr.Serialize("ib_a",   m_fInnerBorderA);
        m_szPerFrameInit = pr.ReadCode("per_frame_init_");
        m_szPerFrameExpr = pr.ReadCode("per_frame_");
        m_szPerPixelExpr = pr.ReadCode("per_pixel_");
    }
    
    // warp shader
    {
        m_szWarpShadersText = pr.ReadCode("warp_");
        if (m_szWarpShadersText.empty()) 
            m_plugin->GenWarpPShaderText(m_szWarpShadersText, m_fDecay.eval(-1), m_bTexWrap);
        m_nWarpPSVersion = nWarpPSVersionInFile;
    }
    
    // comp shader
    {
        m_szCompShadersText = pr.ReadCode("comp_");
        if (m_szCompShadersText.empty())
            m_plugin->GenCompPShaderText(m_szCompShadersText, m_fGammaAdj.eval(-1), m_fVideoEchoAlpha.eval(-1), m_fVideoEchoZoom.eval(-1), m_nVideoEchoOrientation, m_fShader.eval(-1), m_bBrighten, m_bDarken, m_bSolarize, m_bInvert);
        m_nCompPSVersion = nCompPSVersionInFile;
    }

//    m_nMaxPSVersion = std::max(m_nWarpPSVersion, m_nCompPSVersion);
//    m_nMinPSVersion = std::min(m_nWarpPSVersion, m_nCompPSVersion);

    //

    m_nMaxPSVersion = 2; //std::max(m_nWarpPSVersion, m_nCompPSVersion);
    m_nMinPSVersion = 2; //std::min(m_nWarpPSVersion, m_nCompPSVersion);
    
	RecompileExpressions();
    
    if (m_plugin->GetContext()->IsThreaded())
    {
        RecompileShaders(errors);
        if (!errors.empty())
        {
            LogError("ERROR: Shader compilation '%s'\n%s\n",
                     GetName().c_str(),
                     errors.c_str());
            return false;
        }
    }

    errors.clear();
    return true;
}

bool CState::RecompileShaders(std::string &errors)
{
    PROFILE_FUNCTION()

    // compile shader code
    if (!m_shader_warp && m_nWarpPSVersion > 0 && !m_szWarpShadersText.empty())
    {
        m_shader_warp = m_plugin->RecompileShader(m_name + "_warp", m_plugin->m_szDefaultWarpVShaderText, m_szWarpShadersText, SHADER_WARP, errors);
        if (!m_shader_warp)
        {
            return false;
        }
    }

    if (!m_shader_comp && m_nCompPSVersion > 0 && !m_szCompShadersText.empty())
    {
        m_shader_comp =m_plugin->RecompileShader(m_name + "_comp",  m_plugin->m_szDefaultCompVShaderText, m_szCompShadersText,SHADER_COMP, errors);
        if (!m_shader_comp)
        {
            return false;
        }
    }
    return true;
}

int CState::GetHighestBlurTexUsed()
{
    int count = 0;
    if (m_shader_comp)
    {
        count = std::max(count, m_shader_comp->highest_blur_used);
    }
    if (m_shader_warp)
    {
        count = std::max(count, m_shader_warp->highest_blur_used);
    }
    return count;
}

void CState::GenDefaultWarpShader()
{
    if (m_nWarpPSVersion>0)
        m_plugin->GenWarpPShaderText(m_szWarpShadersText, m_fDecay.eval(-1), m_bTexWrap);
}
void CState::GenDefaultCompShader()
{
    if (m_nCompPSVersion>0)
        m_plugin->GenCompPShaderText(m_szCompShadersText, m_fGammaAdj.eval(-1), m_fVideoEchoAlpha.eval(-1), m_fVideoEchoZoom.eval(-1), m_nVideoEchoOrientation, m_fShader.eval(-1), m_bBrighten, m_bDarken, m_bSolarize, m_bInvert);
}

void CState::FreeScriptObjects()
{
    
    {
        m_perframe_init = nullptr;
        m_perframe_expression = nullptr;
        m_pervertex_expression = nullptr;
        m_perframe_context = nullptr;
        m_pervertex_context = nullptr;
        m_pervertex_buffer = nullptr;
    }
    {
        for (auto wave : m_waves)
        {
            wave->m_perframe_init       = nullptr;
            wave->m_perframe_expression = nullptr;
            wave->m_perpoint_expression = nullptr;
            wave->m_perframe_context = nullptr;
            wave->m_perpoint_context = nullptr;
            wave->m_perpoint_buffer = nullptr;
            
        }
    }
    {
        for (auto shape : m_shapes)
        {
            shape->m_perframe_init       = nullptr;
            shape->m_perframe_expression = nullptr;
            shape->m_perframe_context = nullptr;
            shape->m_perframe_buffer = nullptr;

        }
    }
    

}

void CWave::InitExpressions(CState * pState)
{
//    if (m_perframe_init->IsEmpty())
//    {
        for (int vi=0; vi<NUM_Q_VAR; vi++)
            var_pf_q[vi] = pState->q_values_after_init_code[vi];
        for (int vi=0; vi<NUM_T_VAR; vi++)
            t_values_after_init_code[vi] = 0;
//    }
//    else
    {
        // now execute the code, save the values of t1..t8, and clean up the code!

        LoadCustomWavePerFrameEvallibVars(pState);
        

            // note: q values at this point will actually be same as
            //       q_values_after_init_code[], since no per-frame code
            //       has actually been executed yet!

        m_perframe_init->Execute();

        for (int vi=0; vi<NUM_T_VAR; vi++)
            t_values_after_init_code[vi] = var_pf_t[vi];

    }

}

void CShape::InitExpressions(CState * pState)
{
    if (m_perframe_init->IsEmpty() )
    {
        for (int vi=0; vi<NUM_Q_VAR; vi++)
            var_pf_q[vi] = pState->q_values_after_init_code[vi];
        for (int vi=0; vi<NUM_T_VAR; vi++)
            t_values_after_init_code[vi] = 0;
    }
    else
    {
        // now execute the code, save the values of q1..q8, and clean up the code!

        LoadCustomShapePerFrameEvallibVars(pState);
            // note: q values at this point will actually be same as
            //       q_values_after_init_code[], since no per-frame code
            //       has actually been executed yet!



        m_perframe_init->Execute();

        for (int vi=0; vi<NUM_T_VAR; vi++)
            t_values_after_init_code[vi] = var_pf_t[vi];
    }
}

void CState::RecompileExpressions()
{
    PROFILE_FUNCTION()

    FreeScriptObjects();

    // if we're recompiling init code, clear vars to zero, and re-register built-in variables.
	{
		RegisterBuiltInVariables();
	}

    // recompile everything
    {
        m_perframe_init = m_perframe_context->Compile("init", m_szPerFrameInit);
        m_perframe_expression = m_perframe_context->Compile("PerFrame", m_szPerFrameExpr);
        m_pervertex_expression = m_pervertex_context->Compile("PerPixel", m_szPerPixelExpr);
        m_pervertex_buffer = m_pervertex_expression->CreateBuffer();

        for (auto wave : m_waves)
        {
            wave->m_perframe_init       = wave->m_perframe_context->Compile("Init", wave->m_szInit);
            wave->m_perframe_expression = wave->m_perframe_context->Compile("PerFrame", wave->m_szPerFrame);
            wave->m_perpoint_expression = wave->m_perpoint_context->Compile("PerPoint", wave->m_szPerPoint);
            wave->m_perpoint_buffer = wave->m_perpoint_expression->CreateBuffer();

        }
        for (auto shape : m_shapes)
        {
            shape->m_perframe_init       = shape->m_perframe_context->Compile("Init", shape->m_szInit);
            shape->m_perframe_expression = shape->m_perframe_context->Compile("PerFrame",shape->m_szPerFrame);
            shape->m_perframe_buffer      = shape->m_perframe_expression->CreateBuffer();
        }
    }

    // execute
    {
        {
            for (int vi=0; vi<NUM_Q_VAR; vi++)
                q_values_after_init_code[vi] = 0;
            monitor_after_init_code = 0;

//            if (m_perframe_init->IsEmpty())
//            {
//            }
//            else
            {
                // now execute the code, save the values of q1..q32, and clean up the code!

                LoadPerFrameEvallibVars();

                m_perframe_init->Execute();

                for (int vi=0; vi<NUM_Q_VAR; vi++)
                    q_values_after_init_code[vi] = var_pf_q[vi];
                monitor_after_init_code = var_pf_monitor;
            }
        }

        {
            for (auto wave : m_waves)
            {
                wave->InitExpressions(this);
            }
        }

        {
            for (auto shape : m_shapes)
            {
                shape->InitExpressions(this);
            }
        }
    }
}

void CState::RandomizePresetVars()
{
//    LogPrint("RandomizePresetVars %f\n", FRAND);
    
    m_rand_preset = Vector4(FRAND, FRAND, FRAND, FRAND);

    int k = 0;
    do
    {
        for (int i=0; i<4; i++) 
        {
            float xlate_mult = 1;//(j==0) ? 1 : 0;
            float rot_mult = 0.9f*powf(k/8.0f, 3.2f);
            m_xlate[k].x = (FRAND*2-1)*xlate_mult;
            m_xlate[k].y = (FRAND*2-1)*xlate_mult;
            m_xlate[k].z = (FRAND*2-1)*xlate_mult;
            m_rot_base[k].x = FRAND*6.28f;
            m_rot_base[k].y = FRAND*6.28f;
            m_rot_base[k].z = FRAND*6.28f;
            m_rot_speed[k].x = (FRAND*2-1)*rot_mult;
            m_rot_speed[k].y = (FRAND*2-1)*rot_mult;
            m_rot_speed[k].z = (FRAND*2-1)*rot_mult;
            k++;
        }
    }
    while (k < sizeof(m_xlate)/sizeof(m_xlate[0]));
    
}

CBlendableFloat::CBlendableFloat()
{
	m_bBlending  = false;
}

CBlendableFloat::~CBlendableFloat()
{
}

//--------------------------------------------------------------------------------

float CBlendableFloat::eval(float fTime)
{
	if (fTime < 0)
	{
		return val;
	}

	if (m_bBlending && ((fTime > m_fBlendStartTime + m_fBlendDuration) || (fTime < m_fBlendStartTime)))
	{
		m_bBlending = false;
	}

	if (!m_bBlending)
	{
		return val;
	}
	else
	{
		float mix = (fTime - m_fBlendStartTime) / m_fBlendDuration;
		return (m_fBlendFrom*(1.0f - mix) + val*mix);
	}
}

//--------------------------------------------------------------------------------

void CBlendableFloat::StartBlendFrom(CBlendableFloat *f_from, float fAnimTime, float fDuration)
{
	if (fDuration < 0.001f)
		return;

	m_fBlendFrom		= f_from->eval(fAnimTime);
	m_bBlending			= true;
	m_fBlendStartTime	= fAnimTime;
	m_fBlendDuration	= fDuration;
}




