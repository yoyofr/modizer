
#include "AltFinalComposite.hpp"

#include "PresetState.hpp"

#include <Renderer/BlendMode.hpp>

#include <cstddef>

#ifdef MILKDROP_PRESET_DEBUG
#include <iostream>
#endif

namespace libprojectM {
namespace MilkdropPreset {

static std::string const defaultCompositeShader = "shader_body\n{\nret = tex2D(sampler_main, uv).xyz;\n}";

AltFinalComposite::AltFinalComposite()
//    : m_compositeMesh(Renderer::VertexBufferUsage::StreamDraw, true, true)
{
//    m_compositeMesh.SetRenderPrimitiveType(Renderer::Mesh::PrimitiveType::Triangles);

    // Add attribute array for radius and angle information to the mesh.
//    m_compositeMesh.Bind();
//    m_radiusAngle.Bind();
//    m_radiusAngle.Resize(vertexCount);
//    m_radiusAngle.InitializeAttributePointer(3);
//    Renderer::VertexBuffer<Renderer::Point>::SetEnableAttributeArray(3, true);

    // Pre-allocate vertex and index buffers
//    m_compositeMesh.SetVertexCount(vertexCount);
//    m_compositeMesh.Indices().Resize(indexCount);
//    m_compositeMesh.Update();
//
//    Renderer::Mesh::Unbind();
}

void AltFinalComposite::LoadCompositeShader(const AltPresetState& presetState)
{
    if (presetState.compositeShaderVersion > 0)
    {
        m_compositeShader = std::make_unique<MilkdropShader>(MilkdropShader::ShaderType::CompositeShader);
        if (!presetState.compositeShader.empty())
        {
            try
            {
                m_compositeShader->LoadCode(presetState.compositeShader);
#ifdef MILKDROP_PRESET_DEBUG
                std::cerr << "[Composite Shader] Loaded composite shader code." << std::endl;
#endif
            }
            catch (Renderer::ShaderException& ex)
            {
#ifdef MILKDROP_PRESET_DEBUG
                std::cerr << "[Composite Shader] Error loading composite warp shader code:" << ex.message() << std::endl;
                std::cerr << "[Composite Shader] Using fallback shader." << std::endl;
#else
                (void) ex; // silence unused parameter warning
#endif
                // Fall back to default shader
                m_compositeShader = std::make_unique<MilkdropShader>(MilkdropShader::ShaderType::CompositeShader);
                m_compositeShader->LoadCode(defaultCompositeShader);
            }
        }
        else
        {
            m_compositeShader->LoadCode(defaultCompositeShader);
#ifdef MILKDROP_PRESET_DEBUG
            std::cerr << "[Composite Shader] Loaded default composite shader code." << std::endl;
#endif
        }
    }
    else
    {
//        // Video echo OR gamma adjustment with random hue.
//        m_videoEcho = std::make_unique<VideoEcho>(presetState);
//        if (presetState.brighten ||
//            presetState.darken ||
//            presetState.solarize ||
//            presetState.invert)
//        {
//            m_filters = std::make_unique<Filters>(presetState);
//        }
    }
}

void AltFinalComposite::CompileCompositeShader(PresetState& presetState,const char *shader)
{
    if (m_compositeShader)
    {
        try
        {
            m_compositeShader->LoadTexturesAndCompile(presetState);
#ifdef MILKDROP_PRESET_DEBUG
            std::cerr << "[Composite Shader] Successfully compiled composite shader code." << std::endl;
#endif
        }
        catch (Renderer::ShaderException& ex)
        {
#ifdef MILKDROP_PRESET_DEBUG
            std::cerr << "[Composite Shader] Error compiling composite warp shader code:" << ex.message() << std::endl;
            std::cerr << "[Composite Shader] Using fallback shader." << std::endl;
#else
            (void) ex; // silence unused parameter warning
#endif
            // Fall back to default shader
            m_compositeShader = std::make_unique<MilkdropShader>(MilkdropShader::ShaderType::CompositeShader);
            m_compositeShader->LoadCode(defaultCompositeShader);
            m_compositeShader->LoadTexturesAndCompile(presetState);
        }
    }
}

void AltFinalComposite::PreCompileCompositeShader(AltPresetState& presetState)
{
    if (m_compositeShader)
    {
        try
        {
            m_compositeShader->PreLoadTexturesAndCompile(presetState);
            presetState.preCcompositeShader=m_compositeShader->m_convertedCode;
#ifdef MILKDROP_PRESET_DEBUG
            std::cerr << "[Composite Shader] Successfully compiled composite shader code." << std::endl;
#endif
        }
        catch (Renderer::ShaderException& ex)
        {
#ifdef MILKDROP_PRESET_DEBUG
            std::cerr << "[Composite Shader] Error compiling composite warp shader code:" << ex.message() << std::endl;
            std::cerr << "[Composite Shader] Using fallback shader." << std::endl;
#else
            (void) ex; // silence unused parameter warning
#endif
            // Fall back to default shader
//            m_compositeShader = std::make_unique<MilkdropShader>(MilkdropShader::ShaderType::CompositeShader);
//            m_compositeShader->LoadCode(defaultCompositeShader);
//            m_compositeShader->LoadTexturesAndCompile(presetState);
        }
    }
}



auto AltFinalComposite::HasCompositeShader() const -> bool
{
    return m_compositeShader != nullptr;
}

} // namespace MilkdropPreset
} // namespace libprojectM
