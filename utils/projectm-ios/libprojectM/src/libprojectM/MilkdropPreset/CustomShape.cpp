#define CUSTOMSHAPE_FAST_RENDER

#include "CustomShape.hpp"

#include "PresetFileParser.hpp"

#include <Renderer/BlendMode.hpp>
#include <Renderer/TextureManager.hpp>

#include <vector>

namespace libprojectM {
namespace MilkdropPreset {

CustomShape::CustomShape(PresetState& presetState)
    : m_outlineMesh(Renderer::VertexBufferUsage::StreamDraw)
    , m_fillMesh(Renderer::VertexBufferUsage::StreamDraw, true, false)
    , m_presetState(presetState)
    , m_perFrameContext(presetState.globalMemory, &presetState.globalRegisters)
{
    m_outlineMesh.SetVertexCount(100);
    m_outlineMesh.SetRenderPrimitiveType(Renderer::Mesh::PrimitiveType::LineLoop);

    m_fillMesh.SetVertexCount(102);
#ifdef CUSTOMSHAPE_FAST_RENDER
    m_fillMesh.SetRenderPrimitiveType(Renderer::Mesh::PrimitiveType::Triangles);
    m_BorderData_RenderMode.Bind();
    m_BorderData_RenderMode.InitializeAttributePointer(8);
    Renderer::VertexBuffer<Renderer::Point>::SetEnableAttributeArray(8, true);
    Renderer::Mesh::Unbind();
#else
    m_fillMesh.SetRenderPrimitiveType(Renderer::Mesh::PrimitiveType::TriangleFan);
#endif
    

    m_perFrameContext.RegisterBuiltinVariables();
}

void CustomShape::Initialize(::libprojectM::PresetFileParser& parsedFile, int index)
{
    std::string const shapecodePrefix = "shapecode_" + std::to_string(index) + "_";

    m_index = index;
    m_enabled = parsedFile.GetBool(shapecodePrefix + "enabled", m_enabled);
    m_sides = parsedFile.GetInt(shapecodePrefix + "sides", m_sides);
    m_additive = parsedFile.GetBool(shapecodePrefix + "additive", m_additive);
    m_thickOutline = parsedFile.GetBool(shapecodePrefix + "thickOutline", m_thickOutline);
    m_textured = parsedFile.GetBool(shapecodePrefix + "textured", m_textured);
    m_instances = parsedFile.GetInt(shapecodePrefix + "num_inst", m_instances);
    m_x = parsedFile.GetFloat(shapecodePrefix + "x", m_x);
    m_y = parsedFile.GetFloat(shapecodePrefix + "y", m_y);
    m_radius = parsedFile.GetFloat(shapecodePrefix + "rad", m_radius);
    m_angle = parsedFile.GetFloat(shapecodePrefix + "ang", m_angle);
    m_tex_ang = parsedFile.GetFloat(shapecodePrefix + "tex_ang", m_tex_ang);
    m_tex_zoom = parsedFile.GetFloat(shapecodePrefix + "tex_zoom", m_tex_zoom);
    m_r = parsedFile.GetFloat(shapecodePrefix + "r", m_r);
    m_g = parsedFile.GetFloat(shapecodePrefix + "g", m_g);
    m_b = parsedFile.GetFloat(shapecodePrefix + "b", m_b);
    m_a = parsedFile.GetFloat(shapecodePrefix + "a", m_a);
    m_r2 = parsedFile.GetFloat(shapecodePrefix + "r2", m_r2);
    m_g2 = parsedFile.GetFloat(shapecodePrefix + "g2", m_g2);
    m_b2 = parsedFile.GetFloat(shapecodePrefix + "b2", m_b2);
    m_a2 = parsedFile.GetFloat(shapecodePrefix + "a2", m_a2);
    m_border_r = parsedFile.GetFloat(shapecodePrefix + "border_r", m_border_r);
    m_border_g = parsedFile.GetFloat(shapecodePrefix + "border_g", m_border_g);
    m_border_b = parsedFile.GetFloat(shapecodePrefix + "border_b", m_border_b);
    m_border_a = parsedFile.GetFloat(shapecodePrefix + "border_a", m_border_a);

    // projectM addition: texture name to use for rendering the shape
    m_image = parsedFile.GetString(shapecodePrefix + "image", "");
}

void CustomShape::CompileCodeAndRunInitExpressions()
{
    m_perFrameContext.LoadStateVariables(m_presetState, *this, 0);
    m_perFrameContext.EvaluateInitCode(m_presetState.customShapeInitCode[m_index], *this);

    for (int t = 0; t < TVarCount; t++)
    {
        m_tValuesAfterInitCode[t] = *m_perFrameContext.t_vars[t];
    }

    m_perFrameContext.CompilePerFrameCode(m_presetState.customShapePerFrameCode[m_index], *this);
}

void CustomShape::Draw()
{
    static constexpr float pi = 3.141592653589793f;
    
    if (!m_enabled)
    {
        return;
    }
    
    Renderer::BlendMode::SetBlendActive(false);
    
#ifdef CUSTOMSHAPE_FAST_RENDER
    /*
     * New render draw with 1 call
     * manage all cases with 1 shader:
     *  - change of border attributes (r,g,b,a, thickness)
     *  - change of number of sides
     *  - change of additive mode
     *  - change of texture mode
     *
     * Blending managed directly in shader using framebuffer fetch extension
     */
    
    glDisable(GL_DEPTH_TEST);
    
    auto& vertexData = m_fillMesh.Vertices();
    auto& colorData = m_fillMesh.Colors();
    float textureAspectY = m_presetState.renderContext.aspectY;
    
    
    //max is 100 sides per instance
    if (m_fillMesh.VertexCount()<102*m_instances) {
        m_fillMesh.SetVertexCount(102*m_instances);
    }
    
    m_BorderData_RenderMode.Resize(102*m_instances);
    
    int vertexIdx=0;
    int totalSides=0;
    int indicesIdx=0;
    int instanceStartIdx=0;
    
    m_fillMesh.SetUseUV(true); //static_cast<int>(*m_perFrameContext.textured) != 0);
    m_shader = m_presetState.texturedBorderedShader.lock();
    m_shader->Bind();
    m_shader->SetUniformMat4x4("vertex_transformation", PresetState::orthogonalProjection);
    m_shader->SetUniformInt("texture_sampler", 0);
    
    // Textured shape, either main texture or texture from "image" key
    
    if (m_image.empty())
    {
        assert(!m_presetState.mainTexture.expired());
        m_presetState.mainTexture.lock()->Bind(0);
    }
    else
    {
        auto desc = m_presetState.renderContext.textureManager->GetTexture(m_image);
        if (!desc.Empty())
        {
            desc.Bind(0, *m_shader);
            textureAspectY = 1.0f;
        }
        else
        {
            // No texture found, fall back to main texture.
            assert(!m_presetState.mainTexture.expired());
            m_presetState.mainTexture.lock()->Bind(0);
        }
    }
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    for (int instance = 0; instance < m_instances; instance++)
    {
        m_perFrameContext.LoadStateVariables(m_presetState, *this, instance);
        m_perFrameContext.ExecutePerFrameCode();
        
        bool curTextureMode=(static_cast<int>(*m_perFrameContext.textured) != 0);
        bool curAdditiveMode=(static_cast<int>(*m_perFrameContext.additive) != 0);
        
        float borderFactor;
        float thick,thickX,thickY,thickAdd;
        if (*m_perFrameContext.border_a > 0.0001f) {
            //compute / thickness
            thick=1.0;
            if (static_cast<float>(*m_perFrameContext.thick)>0) thick=4.0;
            
            thickX=thick/static_cast<float>(m_presetState.renderContext.viewportSizeX);
            thickY=thick/static_cast<float>(m_presetState.renderContext.viewportSizeY);
            thickAdd=sqrtf(thickX*thickX+thickY*thickY);
            
            borderFactor=(static_cast<float>(*m_perFrameContext.rad)+thickAdd)/static_cast<float>(*m_perFrameContext.rad);
        } else {
            borderFactor=0.0;
            thick=0.0;
            thickX=thickY=thickAdd=0;
        }
        
        
        
        int sides = static_cast<int>(*m_perFrameContext.sides);
        if (sides < 3)
        {
            sides = 3;
        }
        if (sides > 100)
        {
            sides = 100;
        }
        
        vertexData[vertexIdx] = Renderer::Point(static_cast<float>(*m_perFrameContext.x * 2.0 - 1.0),
                                                static_cast<float>(*m_perFrameContext.y * -2.0 + 1.0));
        
        m_BorderData_RenderMode[vertexIdx].rgb =(int)(static_cast<float>(*m_perFrameContext.border_r)*255.0)<<16|
        (int)(static_cast<float>(*m_perFrameContext.border_g)*255.0)<<8|
        (int)(static_cast<float>(*m_perFrameContext.border_b)*255.0)<<0;
        m_BorderData_RenderMode[vertexIdx].a_flags =(int)(static_cast<float>(*m_perFrameContext.border_a)*255.0)<<0|
        (curAdditiveMode?0x100:0)|
        (curTextureMode?0x200:0);
        m_BorderData_RenderMode[vertexIdx].borderFlag=0.0;
        m_BorderData_RenderMode[vertexIdx].zOrder=static_cast<float>(instance)*0.0001;
        
        // x = f*255.0 & 0xFF = (f*255.0) % 256
        // f' = x/255.0 = f % (256/255)
        // 1.0 -> 255 (0xFF)
        // 2.0 -> 254 (0xFE)
        // -1.0 -> 0x01
        
        colorData[vertexIdx] = Renderer::Color::Modulo(Renderer::Color(static_cast<float>(*m_perFrameContext.r),
                                                                       static_cast<float>(*m_perFrameContext.g),
                                                                       static_cast<float>(*m_perFrameContext.b),
                                                                       static_cast<float>(*m_perFrameContext.a)));
        
        colorData[vertexIdx+1] = Renderer::Color::Modulo(Renderer::Color(static_cast<float>(*m_perFrameContext.r2),
                                                                         static_cast<float>(*m_perFrameContext.g2),
                                                                         static_cast<float>(*m_perFrameContext.b2),
                                                                         static_cast<float>(*m_perFrameContext.a2)));
        
        for (int i = 1; i < sides + 1; i++)
        {
            const float cornerProgress = static_cast<float>(i - 1) / static_cast<float>(sides);
            const float angle = cornerProgress * pi * 2.0f + static_cast<float>(*m_perFrameContext.ang) + pi * 0.25f;
            
            // Todo: There's still some issue with aspect ratio here, as everything gets squashed horizontally if Y > x.
            vertexData[vertexIdx+i] = Renderer::Point(vertexData[vertexIdx].X() + (static_cast<float>(*m_perFrameContext.rad)+thickAdd) * cosf(angle) * m_presetState.renderContext.aspectY,
                                                      vertexData[vertexIdx].Y() + (static_cast<float>(*m_perFrameContext.rad)+thickAdd) * sinf(angle));
            
            colorData[vertexIdx+i] = colorData[vertexIdx+1];
            
            
            m_BorderData_RenderMode[vertexIdx+i].rgb =(int)(static_cast<float>(*m_perFrameContext.border_r)*255.0)<<16|
            (int)(static_cast<float>(*m_perFrameContext.border_g)*255.0)<<8|
            (int)(static_cast<float>(*m_perFrameContext.border_b)*255.0)<<0;
            m_BorderData_RenderMode[vertexIdx+i].a_flags =(int)(static_cast<float>(*m_perFrameContext.border_a)*255.0)<<0|
            (curAdditiveMode?0x100:0)|
            (curTextureMode?0x200:0);
            m_BorderData_RenderMode[vertexIdx+i].borderFlag=borderFactor;
            m_BorderData_RenderMode[vertexIdx+i].zOrder=static_cast<float>(instance)*0.0001;
        }
        
        // Duplicate last vertex.
        vertexData[vertexIdx + sides + 1] = vertexData[vertexIdx+1];
        colorData[vertexIdx + sides + 1] = colorData[vertexIdx+1];
        
        m_BorderData_RenderMode[vertexIdx + sides + 1].rgb =(int)(static_cast<float>(*m_perFrameContext.border_r)*255.0)<<16|
        (int)(static_cast<float>(*m_perFrameContext.border_g)*255.0)<<8|
        (int)(static_cast<float>(*m_perFrameContext.border_b)*255.0)<<0;
        m_BorderData_RenderMode[vertexIdx + sides + 1].a_flags =(int)(static_cast<float>(*m_perFrameContext.border_a)*255.0)<<0|
        (curAdditiveMode?0x100:0)|
        (curTextureMode?0x200:0);
        m_BorderData_RenderMode[vertexIdx + sides + 1].borderFlag=borderFactor;
        m_BorderData_RenderMode[vertexIdx + sides + 1].zOrder=static_cast<float>(instance)*0.0001;
        
        if (m_fillMesh.UseUV())
        {
            auto& uvs = m_fillMesh.UVs();
            uvs[vertexIdx+0] = Renderer::TextureUV(0.5f, 0.5f);
            
            for (int i = 1; i < sides + 1; i++)
            {
                const float cornerProgress = static_cast<float>(i - 1) / static_cast<float>(sides);
                const float angle = cornerProgress * pi * 2.0f + static_cast<float>(*m_perFrameContext.tex_ang) + pi * 0.25f;
                
                uvs[vertexIdx+i] = Renderer::TextureUV(0.5f + 0.5f * cosf(angle) / static_cast<float>(*m_perFrameContext.tex_zoom) * textureAspectY,
                                                       1.0f - (0.5f - 0.5f * sinf(angle) / static_cast<float>(*m_perFrameContext.tex_zoom))); // Vertical flip required!
            }
            
            uvs[vertexIdx+sides + 1] = uvs[vertexIdx+1];
        }
        
        
        totalSides+=sides;
        //now adjust indices as we will draw with triangles and not triangles fan
        m_fillMesh.Indices().Resize(totalSides*3);
        auto& indices=m_fillMesh.Indices();
        
        for (int j=1;j<(sides+1);j++) {
            indices[indicesIdx++]=instanceStartIdx;
            indices[indicesIdx++]=instanceStartIdx+j;
            indices[indicesIdx++]=instanceStartIdx+j+1;
        }
        instanceStartIdx+=sides+2;
        
        
        vertexIdx+=sides+2;
        
    }
    m_fillMesh.Update();
    m_BorderData_RenderMode.Update();
    m_fillMesh.Draw();
    
    glBindTexture(GL_TEXTURE_2D, 0);
    Renderer::Sampler::Unbind(0);
#else
    for (int instance = 0; instance < m_instances; instance++)
    {
        m_perFrameContext.LoadStateVariables(m_presetState, *this, instance);
        m_perFrameContext.ExecutePerFrameCode();
        
        int sides = static_cast<int>(*m_perFrameContext.sides);
        if (sides < 3)
        {
            sides = 3;
        }
        if (sides > 100)
        {
            sides = 100;
        }
        
        // Additive Drawing or Overwrite
        Renderer::BlendMode::SetBlendFunction(Renderer::BlendMode::Function::SourceAlpha,
                                              static_cast<int>(*m_perFrameContext.additive) != 0
                                              ? Renderer::BlendMode::Function::One
                                              : Renderer::BlendMode::Function::OneMinusSourceAlpha);
        
        auto& vertexData = m_fillMesh.Vertices();
        auto& colorData = m_fillMesh.Colors();
        
        vertexData[0] = Renderer::Point(static_cast<float>(*m_perFrameContext.x * 2.0 - 1.0),
                                        static_cast<float>(*m_perFrameContext.y * -2.0 + 1.0));
        
        // x = f*255.0 & 0xFF = (f*255.0) % 256
        // f' = x/255.0 = f % (256/255)
        // 1.0 -> 255 (0xFF)
        // 2.0 -> 254 (0xFE)
        // -1.0 -> 0x01
        
        colorData[0] = Renderer::Color::Modulo(Renderer::Color(static_cast<float>(*m_perFrameContext.r),
                                                               static_cast<float>(*m_perFrameContext.g),
                                                               static_cast<float>(*m_perFrameContext.b),
                                                               static_cast<float>(*m_perFrameContext.a)));
        
        colorData[1] = Renderer::Color::Modulo(Renderer::Color(static_cast<float>(*m_perFrameContext.r2),
                                                               static_cast<float>(*m_perFrameContext.g2),
                                                               static_cast<float>(*m_perFrameContext.b2),
                                                               static_cast<float>(*m_perFrameContext.a2)));
        
        for (int i = 1; i < sides + 1; i++)
        {
            const float cornerProgress = static_cast<float>(i - 1) / static_cast<float>(sides);
            const float angle = cornerProgress * pi * 2.0f + static_cast<float>(*m_perFrameContext.ang) + pi * 0.25f;
            
            // Todo: There's still some issue with aspect ratio here, as everything gets squashed horizontally if Y > x.
            vertexData[i] = Renderer::Point(vertexData[0].X() + static_cast<float>(*m_perFrameContext.rad) * cosf(angle) * m_presetState.renderContext.aspectY,
                                            vertexData[0].Y() + static_cast<float>(*m_perFrameContext.rad) * sinf(angle));
            
            colorData[i] = colorData[1];
        }
        
        // Duplicate last vertex.
        vertexData[sides + 1] = vertexData[1];
        colorData[sides + 1] = colorData[1];
        
        m_fillMesh.SetUseUV(static_cast<int>(*m_perFrameContext.textured) != 0);
        
        if (m_fillMesh.UseUV())
        {
            auto shader = m_presetState.texturedShader.lock();
            shader->Bind();
            shader->SetUniformMat4x4("vertex_transformation", PresetState::orthogonalProjection);
            shader->SetUniformInt("texture_sampler", 0);
            
            // Textured shape, either main texture or texture from "image" key
            auto textureAspectY = m_presetState.renderContext.aspectY;
            if (m_image.empty())
            {
                assert(!m_presetState.mainTexture.expired());
                m_presetState.mainTexture.lock()->Bind(0);
            }
            else
            {
                auto desc = m_presetState.renderContext.textureManager->GetTexture(m_image);
                if (!desc.Empty())
                {
                    desc.Bind(0, *shader);
                    textureAspectY = 1.0f;
                }
                else
                {
                    // No texture found, fall back to main texture.
                    assert(!m_presetState.mainTexture.expired());
                    m_presetState.mainTexture.lock()->Bind(0);
                }
            }
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            
            auto& uvs = m_fillMesh.UVs();
            
            uvs[0] = Renderer::TextureUV(0.5f, 0.5f);
            
            for (int i = 1; i < sides + 1; i++)
            {
                const float cornerProgress = static_cast<float>(i - 1) / static_cast<float>(sides);
                const float angle = cornerProgress * pi * 2.0f + static_cast<float>(*m_perFrameContext.tex_ang) + pi * 0.25f;
                
                uvs[i] = Renderer::TextureUV(0.5f + 0.5f * cosf(angle) / static_cast<float>(*m_perFrameContext.tex_zoom) * textureAspectY,
                                             1.0f - (0.5f - 0.5f * sinf(angle) / static_cast<float>(*m_perFrameContext.tex_zoom))); // Vertical flip required!
            }
            
            uvs[sides + 1] = uvs[1];
        }
        else
        {
            // Untextured (creates a color gradient: center=r/g/b/a to border=r2/b2/g2/a2)
            auto shader = m_presetState.untexturedShader.lock();
            shader->Bind();
            shader->SetUniformMat4x4("vertex_transformation", PresetState::orthogonalProjection);
            shader->SetUniformFloat("vertex_point_size", 1.0);
        }
        
        m_fillMesh.Indices().Resize(sides + 2);
        m_fillMesh.Indices().MakeContinuous();
        m_fillMesh.Update();
        m_fillMesh.Draw();
        
        glBindTexture(GL_TEXTURE_2D, 0);
        Renderer::Sampler::Unbind(0);
        
        if (*m_perFrameContext.border_a > 0.0001f)
        {
            m_outlineMesh.Indices().Resize(sides);
            m_outlineMesh.Indices().MakeContinuous();
            
            auto& points = m_outlineMesh.Vertices();
            
            for (int i = 0; i < sides; i++)
            {
                points[i] = m_fillMesh.Vertex(i + 1);
            }
            
            auto shader = m_presetState.untexturedShader.lock();
            shader->Bind();
            shader->SetUniformMat4x4("vertex_transformation", PresetState::orthogonalProjection);
            shader->SetUniformFloat("vertex_point_size", 1.0);
            
            m_outlineMesh.Bind();
            
            glVertexAttrib4f(1,
                             static_cast<float>(*m_perFrameContext.border_r),
                             static_cast<float>(*m_perFrameContext.border_g),
                             static_cast<float>(*m_perFrameContext.border_b),
                             static_cast<float>(*m_perFrameContext.border_a));
            glLineWidth(1);
#ifndef USE_GLES
            glEnable(GL_LINE_SMOOTH);
#endif
            
            const auto iterations = m_thickOutline ? 4 : 1;
            
            // Need to use +/- 1.0 here instead of 2.0 used in Milkdrop to achieve the same rendering result.
            const auto incrementX = 1.0f / static_cast<float>(m_presetState.renderContext.viewportSizeX);
            const auto incrementY = 1.0f / static_cast<float>(m_presetState.renderContext.viewportSizeY);
            
            // If thick outline is used, draw the shape four times with slight offsets
            // (top left, top right, bottom right, bottom left).
            for (auto iteration = 0; iteration < iterations; iteration++)
            {
                switch (iteration)
                {
                    case 0:
                        break;
                        
                    case 1:
                        for (auto j = 0; j < sides; j++)
                        {
                            points[j].SetX(points[j].X() + incrementX);
                        }
                        break;
                        
                    case 2:
                        for (auto j = 0; j < sides; j++)
                        {
                            points[j].SetY(points[j].Y() + incrementY);
                        }
                        break;
                        
                    case 3:
                        for (auto j = 0; j < sides; j++)
                        {
                            points[j].SetX(points[j].X() - incrementX);
                        }
                        break;
                }
                
                m_outlineMesh.Update();
                m_outlineMesh.Draw();
            }
        }
    }
#endif
    
    Renderer::Mesh::Unbind();
    Renderer::Shader::Unbind();
    
#ifndef USE_GLES
    glDisable(GL_LINE_SMOOTH);
#endif
    Renderer::BlendMode::SetBlendActive(false);
}

} // namespace MilkdropPreset
} // namespace libprojectM
