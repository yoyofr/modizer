#pragma once

#include "Renderer/Mesh.hpp"
#include "AltPresetState.hpp"

#include <Renderer/Shader.hpp>

namespace libprojectM {
namespace MilkdropPreset {

class PresetState;
class PerFrameContext;
class PerPixelContext;
class MilkdropShader;

/**
 * @brief The "per-pixel" transformation mesh.
 *
 * This mesh is responsible for most of the motion types in presets. Each mesh vertex
 * is transposed (also scaled, from the center) or rotated to create a frame-by-frame motion.
 * Fragment shader interpolation is then used to create smooth transitions in the space
 * between the grid points.
 *
 * A higher resolution grid means better quality, especially for rotations, but also quickly
 * increases the CPU usage as the per-pixel expression needs to be run for every grid point.
 *
 * The mesh size can be changed between frames, the class will reallocate the buffers if needed.
 */
class AltPerPixelMesh
{
public:
    AltPerPixelMesh();

    /**
     * @brief Loads the warp shader, if the preset uses one.
     * @param presetState The preset state to retrieve the shader from.
     */
    void LoadWarpShader(const AltPresetState& presetState);

    /**
     * @brief Loads the required textures and compiles the warp shader.
     * @param presetState The preset state to retrieve the configuration values from.
     */
    void CompileWarpShader(PresetState& presetState,const char*shaderCode=NULL);
    void PreCompileWarpShader(AltPresetState& presetState);

    /**
     * @brief Renders the transformation mesh.
     * @param presetState The preset state to retrieve the configuration values from.
     * @param presetPerFrameContext The per-frame context to retrieve the initial vars from.
     * @param perPixelContext The per-pixel code context to use.
     */
    void Draw(const PresetState& presetState,
              const PerFrameContext& perFrameContext,
              PerPixelContext& perPixelContext);


private:
    std::weak_ptr<Renderer::Shader> m_perPixelMeshShader;             //!< Special shader which calculates the per-pixel UV coordinates.
    std::unique_ptr<MilkdropShader> m_warpShader;                     //!< The warp shader. Either preset-defined or a default shader.
    Renderer::Sampler m_perPixelSampler{GL_CLAMP_TO_EDGE, GL_LINEAR}; //!< The main texture sampler.
};

} // namespace MilkdropPreset
} // namespace libprojectM
