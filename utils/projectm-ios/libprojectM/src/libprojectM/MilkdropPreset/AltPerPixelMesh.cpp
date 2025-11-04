#include "AltPerPixelMesh.hpp"

#include "MilkdropShader.hpp"
#include "MilkdropStaticShaders.hpp"
#include "PerFrameContext.hpp"
#include "PerPixelContext.hpp"
#include "PresetState.hpp"

#include <Renderer/BlendMode.hpp>
#include <Renderer/ShaderCache.hpp>

#include <algorithm>
#include <cmath>

#ifdef MILKDROP_PRESET_DEBUG
#include <iostream>
#endif

namespace libprojectM {
namespace MilkdropPreset {

AltPerPixelMesh::AltPerPixelMesh()
{
}

void AltPerPixelMesh::LoadWarpShader(const AltPresetState& presetState)
{
    // Compile warp shader if preset specifies one.
    if (presetState.warpShaderVersion > 0)
    {
        if (!presetState.warpShader.empty())
        {
            try
            {
                m_warpShader = std::make_unique<MilkdropShader>(MilkdropShader::ShaderType::WarpShader);
                m_warpShader->LoadCode(presetState.warpShader);
#ifdef MILKDROP_PRESET_DEBUG
                std::cerr << "[Warp Shader] Loaded preset warp shader code." << std::endl;
#endif
            }
            catch (Renderer::ShaderException& ex)
            {
#ifdef MILKDROP_PRESET_DEBUG
                std::cerr << "[Warp Shader] Error loading warp shader code:" << ex.message() << std::endl;
#else
                (void) ex; // silence unused parameter warning
#endif
                m_warpShader.reset();
            }
        }
    }
}

void AltPerPixelMesh::CompileWarpShader(PresetState& presetState,const char*shaderCode)
{
    if (m_warpShader)
    {
        try
        {
            m_warpShader->LoadTexturesAndCompile(presetState,shaderCode);
#ifdef MILKDROP_PRESET_DEBUG
            std::cerr << "[Warp Shader] Successfully compiled warp shader code." << std::endl;
#endif
        }
        catch (Renderer::ShaderException& ex)
        {
#ifdef MILKDROP_PRESET_DEBUG
            std::cerr << "[Warp Shader] Error compiling warp shader code:" << ex.message() << std::endl;
#else
            (void) ex; // silence unused parameter warning
#endif
            m_warpShader.reset();
        }
    }
}

void AltPerPixelMesh::PreCompileWarpShader(AltPresetState& presetState)
{
    if (m_warpShader)
    {
        try
        {
            m_warpShader->PreLoadTexturesAndCompile(presetState);
            presetState.preCwarpShader=m_warpShader->m_convertedCode;
#ifdef MILKDROP_PRESET_DEBUG
            std::cerr << "[Warp Shader] Successfully compiled warp shader code." << std::endl;
#endif
        }
        catch (Renderer::ShaderException& ex)
        {
#ifdef MILKDROP_PRESET_DEBUG
            std::cerr << "[Warp Shader] Error compiling warp shader code:" << ex.message() << std::endl;
#else
            (void) ex; // silence unused parameter warning
#endif
            m_warpShader.reset();
        }
    }
}


} // namespace MilkdropPreset
} // namespace libprojectM
