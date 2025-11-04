#include "Factory.hpp"

#include "IdlePreset.hpp"
#include "MilkdropPreset.hpp"
#include "AltMilkdropPreset.hpp"

namespace libprojectM {
namespace MilkdropPreset {

std::unique_ptr<::libprojectM::Preset> Factory::LoadPresetFromFile(const std::string& filename)
{
    std::string path;
    auto protocol = PresetFactory::Protocol(filename, path);
    if (protocol == "idle")
    {
        return IdlePresets::allocate();
    }
    else if (protocol == "" || protocol == "file")
    {
        return std::make_unique<MilkdropPreset>(path);
    }
    else if (protocol == "preload")
    {
        return std::make_unique<AltMilkdropPreset>(path);
    }
    else if (protocol == "precomp")
    {
        return std::make_unique<MilkdropPreset>(path);
    }
    else
    {
        // ToDO: Throw unsupported protocol exception instead to provide more information.
        return nullptr;
    }
}

std::unique_ptr<Preset> Factory::LoadPresetFromStream(std::istream& data)
{
    return std::make_unique<MilkdropPreset>(data);
}

} // namespace MilkdropPreset
} // namespace libprojectM
