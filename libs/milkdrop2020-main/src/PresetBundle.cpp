
#include "PresetBundle.h"
#include "ArchiveReader.h"
#include "ImageReader.h"

PresetBundlePtr LoadPresetBundleFromArchive(std::string archivePath, render::ContextPtr context)
{
    using namespace Archive;

    StopWatch sw;
    
    ArchiveReaderPtr ar = OpenArchive(archivePath);
    if (!ar) {
        return nullptr;
    }

    PresetBundlePtr bundle = std::make_shared<PresetBundle>();
    bundle->path = archivePath;
    bundle->name = PathGetFileName(archivePath);

    ArchiveFileInfo fi;
    // iterate through files of archive...
    while (ar->NextFile(fi))
    {
        const std::string &path = fi.name;
        if (path.empty()) continue;
        if (fi.is_directory) continue;
        // directory?
        if (path.back() == '/')
        {
            continue;
        }

        // hidden file?
        if (path[0] == '.' || path.find("/.") != std::string::npos)
        {
            continue;
        }

        std::string name_lower = path;
        std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);

        std::string ext = PathGetExtensionLower(path);
        if (ext == ".milk")
        {
            // it's a preset
            std::string text;
            if (ar->ExtractText(text))
            {
                PresetInfoPtr presetInfo = std::make_shared<PresetInfo>(bundle->name, PathRemoveExtension(path), text);
                bundle->presets[path] = presetInfo;
                
            }
        }
        else if (name_lower.find("textures/") != std::string::npos)
        {
            // load a texture..
            std::vector<uint8_t> data;
            if (ar->ExtractBinary(data))
            {
                std::string texname = PathRemoveExtension(PathGetFileName(path));
                
                if (context)
                {
                    render::TexturePtr texture = context->CreateTextureFromFileInMemory(texname.c_str(),
                                                             data.data(),
                                                             data.size()
                                                             );
                    if (texture)
                    {
                        bundle->textures[ texname ] = texture;
                        bundle->memoryUsage += texture->GetWidth() * texture->GetHeight() * sizeof(uint32_t);
                    }
                }
                else
                {
                    // can't access rendering context, so just load up image data
                    render::ImageDataPtr image = ImageLoadFromFileInMemory(data.data(), data.size());
                    if (image)
                    {
                        bundle->texture_image_data[texname] = image;
                        bundle->memoryUsage += image->size;
                    }
                }
            }
        }
    };
    
    bundle->loadTime = sw.GetElapsedSeconds();
    return bundle;
}



std::future<PresetBundlePtr> LoadPresetBundleFromArchiveAsync(std::string archivePath, render::ContextPtr context)
{
    return std::async( std::launch::async,
                  [archivePath, context] {
                      PresetBundlePtr bundle = LoadPresetBundleFromArchive(archivePath, context);
                      return bundle;
                  }
        );
}


