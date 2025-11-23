/**
 * projectM -- Milkdrop-esque visualisation SDK
 * Copyright (C)2003-2004 projectM Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * See 'LICENSE.txt' included within this release
 *
 */

#include "AltMilkdropPreset.hpp"

#include "Factory.hpp"
#include "MilkdropPresetExceptions.hpp"
#include "PresetFileParser.hpp"

#ifdef MILKDROP_PRESET_DEBUG
#include <iostream>
#endif

namespace libprojectM {
namespace MilkdropPreset {

AltMilkdropPreset::AltMilkdropPreset(const std::string& absoluteFilePath)
    : m_absoluteFilePath(absoluteFilePath)
    , m_perFrameContext(m_state.globalMemory, &m_state.globalRegisters)
    , m_perPixelContext(m_state.globalMemory, &m_state.globalRegisters)
    //, m_darkenCenter(m_state)
    //, m_border(m_state)
{
    Load(absoluteFilePath);
}

AltMilkdropPreset::AltMilkdropPreset(std::istream& presetData)
    : m_perFrameContext(m_state.globalMemory, &m_state.globalRegisters)
    , m_perPixelContext(m_state.globalMemory, &m_state.globalRegisters)
    //, m_darkenCenter(m_state)
    //, m_border(m_state)
{
    Load(presetData);
}

void AltMilkdropPreset::Initialize(const Renderer::RenderContext& renderContext)
{
    assert(renderContext.textureManager);
    m_state.renderContext = renderContext;
//    m_state.blurTexture.Initialize(renderContext);
//    m_state.LoadShaders();

    // Initialize variables and code now we have a proper render state.
    CompileCodeAndRunInitExpressions();

    // Update framebuffer and texture sizes if needed
//    m_framebuffer.SetSize(renderContext.viewportSizeX, renderContext.viewportSizeY);
//    m_motionVectorUVMap->SetSize(renderContext.viewportSizeX, renderContext.viewportSizeY);
    if (m_state.mainTexture.expired())
    {
        m_state.mainTexture = m_framebuffer.GetColorAttachmentTexture(1, 0);
    }

    m_perPixelMesh.PreCompileWarpShader(m_state);
    m_finalComposite.PreCompileCompositeShader(m_state);
}

void AltMilkdropPreset::GetShadersCode(uint32_t *warpP,uint32_t *compP) {
    *warpP=m_state.warpP;
    *compP=m_state.compP;
}

void AltMilkdropPreset::SetShadersCode(uint32_t warpP,uint32_t compP) {
}

void AltMilkdropPreset::RenderFrame(const libprojectM::Audio::FrameAudioData& audioData, const Renderer::RenderContext& renderContext)
{
    
}

auto AltMilkdropPreset::OutputTexture() const -> std::shared_ptr<Renderer::Texture>
{
    // the composited image is always stored in the "current" framebuffer after a frame is rendered.
    return NULL;
}

void AltMilkdropPreset::DrawInitialImage(const std::shared_ptr<Renderer::Texture>& image, const Renderer::RenderContext& renderContext)
{
}

void AltMilkdropPreset::BindFramebuffer()
{
}

void AltMilkdropPreset::PerFrameUpdate()
{
}

void AltMilkdropPreset::Load(const std::string& pathname)
{
#ifdef MILKDROP_PRESET_DEBUG
    std::cerr << "[Preset] Loading preset from file \"" << pathname << "\"." << std::endl;
#endif

    SetFilename(ParseFilename(pathname));

    ::libprojectM::PresetFileParser parser;

    if (!parser.Read(pathname))
    {
#ifdef MILKDROP_PRESET_DEBUG
        std::cerr << "[Preset] Could not parse preset file." << std::endl;
#endif
        throw MilkdropPresetLoadException("Could not parse preset file \"" + pathname + "\"");
    }

    InitializePreset(parser);
}

void AltMilkdropPreset::Load(std::istream& stream)
{
#ifdef MILKDROP_PRESET_DEBUG
    std::cerr << "[Preset] Loading preset from stream." << std::endl;
#endif

    ::libprojectM::PresetFileParser parser;

    if (!parser.Read(stream))
    {
#ifdef MILKDROP_PRESET_DEBUG
        std::cerr << "[Preset] Could not parse preset data." << std::endl;
#endif
        throw MilkdropPresetLoadException("Could not parse preset data.");
    }

    InitializePreset(parser);
}

void AltMilkdropPreset::InitializePreset(::libprojectM::PresetFileParser& parsedFile)
{
    // Create the offscreen rendering surfaces.
//    m_motionVectorUVMap = std::make_shared<Renderer::TextureAttachment>(GL_RG16F, GL_RG, GL_FLOAT, 0, 0);
//    m_framebuffer.CreateColorAttachment(0, 0); // Main image 1
//    m_framebuffer.CreateColorAttachment(1, 0); // Main image 2
//
//    Renderer::Framebuffer::Unbind();

    // Load global init variables into the state
    m_state.Initialize(parsedFile);

    // Register code context variables
    m_perFrameContext.RegisterBuiltinVariables();
    m_perPixelContext.RegisterBuiltinVariables();

    // Custom waveforms:
//    for (int i = 0; i < CustomWaveformCount; i++)
//    {
//        auto wave = std::make_unique<CustomWaveform>(m_state);
//        wave->Initialize(parsedFile, i);
//        m_customWaveforms[i] = std::move(wave);
//    }
//
//    // Custom shapes:
//    for (int i = 0; i < CustomShapeCount; i++)
//    {
//        auto shape = std::make_unique<CustomShape>(m_state);
//        shape->Initialize(parsedFile, i);
//        m_customShapes[i] = std::move(shape);
//    }

    // Preload shaders
    LoadShaderCode();
}

void AltMilkdropPreset::CompileCodeAndRunInitExpressions()
{
    // Per-frame init and code
    m_perFrameContext.LoadStateVariables(m_state);
    m_perFrameContext.EvaluateInitCode(m_state);
    m_perFrameContext.CompilePerFrameCode(m_state.perFrameCode);

    // Per-vertex code
    m_perPixelContext.CompilePerPixelCode(m_state.perPixelCode);

//    for (int i = 0; i < CustomWaveformCount; i++)
//    {
//        auto& wave = m_customWaveforms[i];
//        wave->CompileCodeAndRunInitExpressions(m_perFrameContext);
//    }
//
//    for (int i = 0; i < CustomShapeCount; i++)
//    {
//        auto& shape = m_customShapes[i];
//        shape->CompileCodeAndRunInitExpressions();
//    }
}

void AltMilkdropPreset::LoadShaderCode()
{
    m_perPixelMesh.LoadWarpShader(m_state);
    m_finalComposite.LoadCompositeShader(m_state);
}

auto AltMilkdropPreset::ParseFilename(const std::string& filename) -> std::string
{
    const std::size_t start = filename.find_last_of('/');

    if (start == std::string::npos || start >= (filename.length() - 1))
    {
        return "";
    }

    return filename.substr(start + 1, filename.length());
}


} // namespace MilkdropPreset
} // namespace libprojectM
