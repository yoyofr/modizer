
#pragma once

#include "render/MDcontext.h"
#include "audio/IAudioSource.h"

enum class ContentMode
{
    ScaleToFill,    // aspect ratio may change
    ScaleAspectFit, // aspect ratio will not change, no content is cropped, may have black areas around contetn
    ScaleAspectFill // aspect ratio will not change, content may be cropped
};


class CState;
using Preset = CState;
using PresetPtr = std::shared_ptr<Preset>;


struct PresetLoadArgs
{
    float blendTime = 0.0f;
    float duration  = 10.0f;
};


class ITextureSet
{
public:
    virtual ~ITextureSet() = default;
    
    virtual render::TexturePtr LookupTexture(const std::string &name) = 0;
    virtual void AddTexture(const std::string &name, render::TexturePtr texture) = 0;
    virtual void GetTextureListWithPrefix(const std::string &prefix, std::vector<render::TexturePtr> &list) = 0;
    virtual void ShowUI() = 0;

};
using ITextureSetPtr = std::shared_ptr<ITextureSet>;


class IVisualizer
{
public:
    virtual ~IVisualizer() = default;
    virtual void Draw(ContentMode mode, float alpha = 1.0f) =0;
    virtual render::TexturePtr GetInternalTexture() = 0;
    virtual render::TexturePtr GetOutputTexture() =0;
    virtual void CaptureScreenshot(render::TexturePtr texture, Vector2 pos, Size2D size) =0;

    virtual void Render(float dt, Size2D output_size, IAudioSourcePtr audioSource) =0;
    virtual void DrawDebugUI() = 0;
    virtual void DrawAudioUI() = 0;
    virtual void SetRandomSeed(uint32_t seed) = 0;

    virtual PresetPtr   LoadPreset(const std::string &text, std::string name, std::string &errors)  = 0;
    virtual void        SetPreset(PresetPtr preset, PresetLoadArgs args) = 0;
    

    virtual const std::string &GetPresetName() = 0;
    virtual float GetPresetProgress() = 0;
    
    virtual void SetNewDuration(float duration) = 0;
};

using IVisualizerPtr = std::shared_ptr<IVisualizer>;


IVisualizerPtr CreateVisualizer(render::ContextPtr context, ITextureSetPtr texture_map, std::string pluginDir);
