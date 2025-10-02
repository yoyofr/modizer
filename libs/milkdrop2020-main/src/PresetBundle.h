

#pragma once

#include "PresetInfo.h"
#include "render/MDcontext.h"

struct PresetBundle
{
    bool                                        enabled = true;
    std::string                                 path;
    std::string                                 name;
    std::map<std::string, PresetInfoPtr>        presets; // presets created for this bundle
    std::map<std::string, render::TexturePtr >  textures; // render textures created for this bundle
    std::map<std::string, render::ImageDataPtr >  texture_image_data; // texture image data for the platforms that cannot create textures in another thread
    double                                      loadTime = 0;
    int                                         memoryUsage = 0;
};

using PresetBundlePtr = std::shared_ptr<PresetBundle>;

PresetBundlePtr LoadPresetBundleFromArchive(std::string archivePath, render::ContextPtr context);

std::future<PresetBundlePtr> LoadPresetBundleFromArchiveAsync(std::string archivePath, render::ContextPtr context);


