
#include "VizController.h"
#include "StopWatch.h"
#include "vis_milk2/IAudioAnalyzer.h"
#include "vis_milk2/state.h"
#include <algorithm>
#include "ArchiveReader.h"
#include "imgui_support.h"
#include "ImageWriter.h"
#include "../external/imgui/imgui_internal.h"
#include "json.h"

#include <chrono>
#include <cstdio>

using namespace render;

IAudioSourcePtr OpenNullAudioSource();
IAudioSourcePtr OpenMDZAudioSource();
IAudioSourcePtr OpenALAudioSource();
IAudioSourcePtr OpenSLESAudioSource();
IAudioSourcePtr OpenUDPAudioSource();
IAudioSourcePtr OpenAVAudioEngineSource();





class TextureSet : public ITextureSet
{
protected:
    std::mutex _mutex;
    TexturePtr _selectedTexture;
    std::map<std::string, render::TexturePtr> _map;
    

public:
    virtual render::TexturePtr LookupTexture(const std::string &name) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _map.find(name);
        if (it == _map.end())
            return nullptr;
        return it->second;
    }
    
    virtual void AddTexture(const std::string &name, render::TexturePtr texture) override
    {
        if (!texture) return;
        
        std::lock_guard<std::mutex> lock(_mutex);
        _map[name] = texture;
        
        LogPrint("Loaded texture '%s' (%dx%d)\n", name.c_str(), texture->GetWidth(), texture->GetHeight());
    }
    
    virtual void GetTextureListWithPrefix(const std::string &prefix, std::vector<TexturePtr> &list) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        
        list.clear();
        list.reserve(_map.size());
        for (const auto &it : _map)
        {
            if (prefix.empty() || it.first.find(prefix) == 0)
            {
                list.push_back(it.second);
            }
        }
    }


    virtual void ShowUI() override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        
        ImGui::BeginChild("left pane", ImVec2(400, 0), true);
        for (const auto &it : _map)
        {
            if (ImGui::Selectable( it.first.c_str(), it.second == _selectedTexture)) {
                _selectedTexture = it.second;
            }
        }
        ImGui::EndChild();
    
        ImGui::SameLine();
    
        ImGui::BeginChild("right pane", ImVec2(0, 0));
        if (_selectedTexture)
        {
            ImGui::Text("%s (%dx%dx%s)",
                        _selectedTexture->GetName().c_str(),
                        _selectedTexture->GetWidth(),
                        _selectedTexture->GetHeight(),
                        render::PixelFormatToString(_selectedTexture->GetFormat())
                        );
            
            ImVec2 sz((float)_selectedTexture->GetWidth(), (float)_selectedTexture->GetHeight());
            ImGui::Image( _selectedTexture.get(), sz);
        }
        ImGui::EndChild();
    }

};

VizController::VizController(ContextPtr context, std::string assetDir, std::string userDir)
	: m_context(context), m_assetDir(assetDir), m_userDir(userDir)
{
    // seed with random number
    std::random_device device;
    m_random_generator.seed( device() );
    
    m_endpoint = "http://m1lkdr0p.com";
    
    m_navigateHistory = true;
    
    
    m_screenshotDir = PathCombine(m_userDir, "screenshots/");
    
    m_configPath = PathCombine(m_userDir, "config.json");
    m_ratingsPath = PathCombine(m_userDir, "ratings.json");
    m_historyPath = PathCombine(m_userDir, "history.json");
    
    m_audio_null = OpenNullAudioSource();
    m_audio_mdz = OpenMDZAudioSource();
    m_audio_wavfile = m_audio_null;
    m_audio_microphone = m_audio_null;
    
    m_texture_map = std::make_shared<TextureSet>();
    
    m_vizualizer = CreateVisualizer(context, m_texture_map, assetDir);
    
	m_deltaTime = 1.0f / 60.0f;
	m_frame     = 0;
	m_time      = 0.0f;
	m_singleStep = false;

	m_currentPreset = nullptr;
    
    m_selectionLocked = false;
    
    m_showProfiler = false;
    
    
    m_contentMode = ContentMode::ScaleAspectFill;
    
    
//    m_contentMode = ContentMode::ScaleAspectFit;
    
    ImGuiSupport_Init(m_context, m_assetDir);


    LogPrint("Visualizer assetDir: %s\n", assetDir.c_str());
    LogPrint("Visualizer userDir: %s\n", userDir.c_str());
}

VizController::~VizController()
{
#if PLATFORM_SUPPORTS_THREADS
    if (m_screenshotFuture.valid()) {
        m_screenshotFuture.wait();
    }
    
    if (m_presetBundleLoadFuture.valid()) {
        m_presetBundleLoadFuture.wait();
    }
#endif
    
    ImGuiSupport_Shutdown();

    
}



void VizController::OnTestingComplete()
{
    TestingDump();

    m_testing =  false;
    LogPrint("Testing complete\n");
}

bool VizController::SelectPreset(const std::string &name)
{
    auto it = m_presetLookup.find(name);
    if (it == m_presetLookup.end())
        return false;
    
    LoadPreset(it->second, false);
    return true;
}


void VizController::CaptureTestResult(PresetTestResult &profile)
{
    profile.FrameTimes = m_frameTimes;
    profile.FastFrameCount = 0;
    profile.SlowFrameCount = 0;
    
    float threshold = m_deltaTime * 1.10f * 1000.0f;
    
    float total = 0;
    float min = std::numeric_limits<float>::max();
    float max = std::numeric_limits<float>::min();
    
    for (auto time : m_frameTimes) {
        if (time > threshold) profile.SlowFrameCount++;
        else profile.FastFrameCount++;
        total += time;
        min = std::min(min, time );
        max = std::max(max, time );
    }
    float average = total / (float)m_frameTimes.size();
    profile.MinFrameTime = min;
    profile.MaxFrameTime = max;
    profile.AveFrameTime = average;
    profile.SlowPercentage = 100.0f * (float)profile.SlowFrameCount / (float)profile.FrameTimes.size();
}

bool VizController::LoadPreset(PresetInfoPtr presetInfo, PresetLoadArgs args)
{
    
    std::string errors;
    PresetPtr preset = m_vizualizer->LoadPreset(presetInfo->PresetText, presetInfo->Name, errors);
    if (!preset)
    {
        LogError("Could not load preset '%s'", presetInfo->Name.c_str());
        LogError("%s", errors.c_str());
        return false;
    }

    SetPreset(presetInfo, preset, args);
    return true;
}


void VizController::LoadPresetAsync(PresetInfoPtr presetInfo, PresetLoadArgs args)
{
#if PLATFORM_SUPPORTS_THREADS
    if (m_LoadPresetFuture.valid()) {
        m_LoadPresetFuture.wait();
    }
    
    m_LoadPresetFuture = std::async( std::launch::async,
              [this, presetInfo, args]() {

                std::string errors;
                PresetPtr preset = m_vizualizer->LoadPreset(presetInfo->PresetText, presetInfo->Name, errors);
                if (preset)
                {
                    m_dispatcher.InvokeAsync([=]() {
                        SetPreset(presetInfo, preset, args);
                    });
                }
              }
          );
#else
    LoadPreset(presetInfo, args);
#endif

}

void VizController::LoadPreset(PresetInfoPtr presetInfo, bool blend)
{
    if (!presetInfo || presetInfo == m_currentPreset) {
        return;
    }
    
    // determine load arguments
    float timeDelta = 0.40f;
    float timeMin =     m_fTimeBetweenPresets * (1.0f - timeDelta);
    float timeMax =     m_fTimeBetweenPresets * (1.0f + timeDelta);

    std::uniform_real_distribution<float> dist(timeMin, timeMax);

    PresetLoadArgs args;
    args.blendTime = blend ? m_fBlendTimeAuto : 0.0f;
    args.duration = args.blendTime + dist(m_random_generator);
    if (m_testing)
    {
        args.blendTime = 0.0f;
        args.duration = 8 / 60.0f;
    }
    
    LoadPresetAsync(presetInfo, args);
}


void VizController::SetPreset(PresetInfoPtr presetInfo, PresetPtr preset, PresetLoadArgs args)
{
    StopWatch sw;
    if (m_currentPreset && !m_frameTimes.empty())
    {
        if (IsTesting() && m_captureScreenshots)
            CaptureScreenshot();
        
        if (IsTesting() && m_captureProfiles)
        {
            std::string path = PathCombine(m_userDir, presetInfo->Name);
            path += ".trace.json";
            
            TProfiler::CaptureToFile(path);
            TProfiler::Clear();
        }

        // store information for testing
        PresetTestResult result;
        CaptureTestResult(result);
        m_currentPreset->TestResult = std::make_shared<PresetTestResult>(result);
    }
    m_frameTimes.clear();

    m_vizualizer->SetPreset(preset, args);
    m_currentPreset = presetInfo;
    m_currentPreset->Progress = 0.0f;
    m_paused = false;


    
    LogPrint("LoadPreset: (%d/%d) '%s' (load: %fms blend:%fs duration:%.1fs)\n",
             presetInfo->Index,
             (int)m_presetList.size(),
             presetInfo->Name.c_str(),
             sw.GetElapsedMilliSeconds(),
             args.blendTime, args.duration );
}


void VizController::TestAllPresets(std::function<void (std::string name, std::string error)> callback)
{
    int errorCount = 0;
    int totalCount = 0;
    for (auto preset : m_presetList.List())
    {
        LogPrint("Loading Preset '%s'\n", preset->Name.c_str());
        
//        LogPrint("%s\n", preset->PresetText.c_str());
        std::string error;
        auto loaded = m_vizualizer->LoadPreset(preset->PresetText, preset->Name, error);
        if (!loaded) {
            if (callback)
            {
                callback(preset->Name, error);
                errorCount++;
            }
        }
        
        totalCount++;
    }
    
    LogPrint("TestAllPresets (%d/%d)\n", errorCount, totalCount);
    
}

void VizController::UpdatePresetList()
{
    // rebuild preset list...
    m_presetList.clear();
    m_presetLookup.clear();

    for (auto bundle : m_presetBundles)
    {
        if (!bundle->enabled) continue;
        
        for (const auto &it : bundle->presets)
        {
            std::string path = it.first;
            PresetInfoPtr preset = it.second;

            if (m_presetLookup.find(preset->Name) == m_presetLookup.end())
            {
                m_presetList.Add(preset);
                m_presetLookup[preset->Name] = preset;
            }
        }

        for (const auto &it : bundle->textures)
        {
            m_texture_map->AddTexture( it.first, it.second);
        }
    }
    
    m_presetList.Sort();
    
    // set indices
    int index = 1;
    for(auto preset : m_presetList.List())
    {
        preset->Index = index++;
    }
    
    m_presetListFiltered = m_presetList;
    
    LoadRatings();
    LoadHistory();
    
    
    NavigateRandom();
}

void VizController::ProcessPresetLoad()
{
    // handle preset loading (async if possible..)
    
    if (m_presetBundleLoadFuture.valid())
    {
        if (m_presetBundleLoadFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            m_presetBundleActiveLoad.clear();
            
            // get preset bundle
            PresetBundlePtr bundle = m_presetBundleLoadFuture.get();
            
            
            // create textures for all image data we have (if we couldn't do this async in another thread)
            for (const auto &it : bundle->texture_image_data)
            {
                std::string name = it.first;
                render::ImageDataPtr image = it.second;
                auto texture = m_context->CreateTexture(name.c_str(), image->width, image->height, image->format, image->data);
                if (texture)
                {
                    bundle->textures[ name ] = texture;
                }
            }
            bundle->texture_image_data.clear();
            
            m_presetBundles.push_back(bundle);

            LogPrint("Loaded preset bundle '%s' (presets:%d textures:%d time:%fs)\n",
                     bundle->name.c_str(),
                     (int)bundle->presets.size(),
                     (int)bundle->textures.size(),
                     bundle->loadTime);
            
            // clear future
            m_presetBundleLoadFuture = std::future<PresetBundlePtr>();
            
            if (m_presetBundleLoadQueue.empty())
            {
                // update preset list after loading is complete....
                UpdatePresetList();
            }
        }
    }
    else
    {
        if (!m_presetBundleLoadQueue.empty())
        {
            // dequeue next bundle...
            std::string archivePath = m_presetBundleLoadQueue.front();
            m_presetBundleLoadQueue.pop();
            
            m_presetBundleActiveLoad = archivePath;
            
            auto context = m_context;
#if PLATFORM_SUPPORTS_THREADS
            if (context->IsThreaded())
            {
                m_presetBundleLoadFuture = std::async( std::launch::async,
                              [archivePath, context] {
                                  PresetBundlePtr bundle = LoadPresetBundleFromArchive(archivePath, context);
                                  return bundle;
                              }
                    );
            }
            else
#endif
            {
                std::promise<PresetBundlePtr> promise;
                m_presetBundleLoadFuture = promise.get_future();
                
                PresetBundlePtr bundle = LoadPresetBundleFromArchive(archivePath, context);
                promise.set_value(bundle);
            }

        }

    }
}



void VizController::LoadPresetBundlesFromDir(std::string dir)
{
	std::vector<std::string> files;
	DirectoryGetFiles(dir, files, true);
    
	for (auto file : files)
	{
        std::string ext = PathGetExtensionLower(file);
        if (ext == ".zip" || ext == ".7z")
        {
            // add to load queue....
            m_presetBundleLoadQueue.push(file);
        }
	}
}



void VizController::TestingStart()
{
//    m_captureScreenshots = false;
//    m_captureProfiles = true;
    
    auto preset = m_presetListFiltered.SelectIndex(0);
    if (preset)
    {
        LogPrint("Testing started\n");
        m_testing = true;

        LoadPreset(preset, false);
    }

}


std::string VizController::TestingResultsToString()
{
    using namespace rapidjson;
    StringBuffer s;
    PrettyWriter<StringBuffer> writer(s);

    writer.StartObject();

    writer.Key("metadata");
    writer.StartObject();
    writer.Key("platform");
    writer.String(PlatformGetPlatformName());
    writer.Key("config");
    writer.String(PlatformGetBuildConfig());
    writer.Key("version");
    writer.String(PlatformGetAppVersion());
    writer.EndObject();

    
    writer.Key("presets");
    writer.StartArray();
    
    for (auto preset : m_presetList.List())
      {
          auto profile = preset->TestResult;
          if (profile && !profile->FrameTimes.empty() )
          {
//             LogPrint("\"%s\" ave:%3.2f fast:%d slow:%d (%.2f%%)",
//                        preset->Name.c_str(),
//                         profile.AveFrameTime,
//                         profile.FastFrameCount,
//                         profile.SlowFrameCount,
//                         pct
//                         );
              
              
              writer.StartObject();
              writer.Key("name");
              writer.String(preset->Name);
              writer.Key("ave");
              writer.Double(profile->AveFrameTime);
              writer.Key("fast");
              writer.Int(profile->FastFrameCount);
              writer.Key("slow");
              writer.Int(profile->SlowFrameCount);
              writer.Key("pct");
              writer.Double(profile->SlowPercentage);

              writer.EndObject();
              
          }
      }
    
    writer.EndArray();

    writer.EndObject();

    return s.GetString();
}


void VizController::TestingDump()
{
   
    
    std::string json(TestingResultsToString());
//    std::cout << json << '\n';
    
    std::string name;
    name = PlatformGetAppName();
    name += "-";
    name += PlatformGetPlatformName();
    if (PlatformIsDebug()) {
        name += "-debug";
    }
    name += ".";
    name += PlatformGetTimeString();
    name += ".test-results.json";
    
    std::string path = PathCombine(m_userDir, name);
    if (FileWriteAllText(path, json)) {
        LogPrint("Wrote test results to: %s\n", path.c_str());
    }
    
    std::string serverUrl;
    if (Config::TryGetString("SERVER_URL", &serverUrl)) {
        std::string url = PathCombine(PathCombine(serverUrl, "appdata"), name);
        HttpSend("PUT", url, json.c_str(), json.size(), true, "application/json", nullptr);
    }

}

void VizController::TestingAbort()
{
    if (!m_testing)
        return;
    
    TestingDump();
  
    
    m_testing =  false;
    m_selectionLocked = true;
    LogPrint("Testing paused\n");
}


void VizController::OpenInputAudioFile(const char *path)
{
    IAudioSourcePtr OpenWavFileAudioSource(const char *path);
    
    std::string fullPath = PathCombine(m_assetDir, path);

    if (!FileExists(fullPath))
    {
        LogError("File does not exist %s\n", fullPath.c_str());
        return;
    }

    std::string ext = PathGetExtensionLower(fullPath);
    if (ext == ".wav")
    {
        m_audio_wavfile = OpenWavFileAudioSource(fullPath.c_str());
    }
    
    m_audioSource = m_audio_wavfile;
}

void VizController::OpenInputAudioMDZ()
{
    m_audioSource = m_audio_mdz;
}




void VizController::SingleStep()
{
    m_singleStep = true;
    m_paused = false;
}

void VizController::TogglePause()
{
    m_paused = !m_paused;
}

void VizController::Pause()
{
    m_paused = true;
}

void VizController::Play()
{
    m_paused = false;
}

float VizController::GetCurrentPresetRating() const
{
    if (!m_currentPreset) return 0;
    return m_currentPreset->Rating;
}

void VizController::SetCurrentPresetRating(float rating)
{
    if (m_currentPreset)
    {
        if (rating != m_currentPreset->Rating)
        {
            m_currentPreset->Rating = rating;

            
            if (!m_endpoint.empty())
            {
                // post a rating publicly
                std::string url = m_endpoint + "/rating";
                
                rapidjson::StringBuffer str;
                rapidjson::Writer<rapidjson::StringBuffer> writer(str);
                writer.StartObject();
                writer.Key(m_currentPreset->Name);
                writer.Double(rating);
                writer.EndObject();

                HttpSend("POST", url, str.GetString(), str.GetSize(), false, "application/json", nullptr);
            }
            
            
            SaveRatings();
        }
        
        if (m_currentPreset->Rating < 0)
        {
            NavigateRandom(false);
        }
    }
    
}


void VizController::NavigatePrevious()
{
    
    auto preset = m_navigateHistory ? m_presetHistory.SelectPrevious() : m_presetListFiltered.SelectPrevious();
    if (preset)
    {
        SetSelectionLock(true);
        LoadPreset(preset, false);
    }
    else
    {
        //
    }
}


void VizController::NavigateNext()
{
    auto preset = m_navigateHistory ? m_presetHistory.SelectNext() : m_presetListFiltered.SelectNext();
    if (preset)
    {
        SetSelectionLock(false);
        LoadPreset(preset, false);
    }
    else
    {
        NavigateRandom(false);
    }
    
}


void VizController::NavigateRandom(bool blend)
{
    auto preset = m_presetListFiltered.SelectRandom(m_random_generator);
    if (preset) {
        SetSelectionLock(false);
        LoadPreset(preset, blend);
        
        // add to preset history...
        m_presetHistory.Add(preset);
        m_presetHistory.SelectLast();
        SaveHistory();
    }
    
}



static bool ToolbarButton(const char *id, const char *tooltip)
{
    float size = 48;
//    ImVec2 bsize(64, 64);
    ImVec2 bsize(size,size);

//    const ImVec2 label_size = ImGui::CalcTextSize(id, NULL, true);
    
    ImGui::SameLine();
    
//    ImGui::BeginChild(id, bsize, true, ImGuiWindowFlags_NoScrollbar);
    
    auto fy = ImGui::GetStyle().FramePadding;
    auto bta = ImGui::GetStyle().ButtonTextAlign;
//    auto fr = ImGui::GetStyle().FrameRounding;
    ImGui::GetStyle().FramePadding = ImVec2(10, 10);
    ImGui::GetStyle().ButtonTextAlign = ImVec2(0.5, 1.0);

//    ImGui::GetStyle().ButtonTextAlign.x = 0.5;
//    ImGui::GetStyle().ButtonTextAlign.y = 0.90;

    
    
//    ImGui::GetStyle().FrameRounding = 0.5;
//    ImGui::GetStyle().FramePadding.y = 0;
    bool result = ImGui::ButtonEx(id, bsize, ImGuiButtonFlags_None) ;//ImGuiButtonFlags_AlignTextBaseLine);
    
//    ImGui::GetStyle().FrameRounding = fr;
    ImGui::GetStyle().FramePadding = fy;
    ImGui::GetStyle().ButtonTextAlign = bta;
    
//    bool result = ImGui::SmallButton(id);
    if (ImGui::IsItemHovered())
         ImGui::SetTooltip("%s", tooltip);
    
//    ImGui::EndChild();
    return result;
}

static bool ToolbarToggleButton(const char *id_on, const char *id_off, bool toggle, const char *tooltip)
{
    if (toggle)
          ImGui::PushStyleColor(ImGuiCol_Button,
                                       ImGui::GetColorU32(ImGuiCol_ButtonActive)
                                       );

    bool result = ToolbarButton(toggle ? id_on : id_off, tooltip);

    if (toggle)
        ImGui::PopStyleColor(1);

    return result;

}

static void ToolbarSeparator(float width = 64)
{
    ImVec2 bsize(width,64);
    ImGui::SameLine();
    ImGui::InvisibleButton("seperator2", ImVec2(bsize.x / 2,  -1.0f));

}

void VizController::DrawTitleWindow()
{
    if (!m_presetBundleActiveLoad.empty())
    {
        ImVec2 pos(20, ImGui::GetIO().DisplaySize.y / 2 );
        ImVec2 margin(20, 20);
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize (ImVec2(ImGui::GetIO().DisplaySize.x - pos.x - margin.x, 0) );
//            ImGui::SetNextWindowContentSize (ImVec2(ImGui::GetIO().DisplaySize.x - 80, 0) );
        if (ImGui::Begin("BundleLoadStatus", &m_showTitle, 0
                         | ImGuiWindowFlags_NoMove
                         | ImGuiWindowFlags_NoDecoration
                         | ImGuiWindowFlags_NoTitleBar
//                         | ImGuiWindowFlags_NoResize
                         | ImGuiWindowFlags_NoScrollbar
                         | ImGuiWindowFlags_AlwaysAutoResize
                         | ImGuiWindowFlags_NoSavedSettings
                         | ImGuiWindowFlags_NoFocusOnAppearing
                         | ImGuiWindowFlags_NoNav
                         ))
        {
            ImGui::TextWrapped("Loading preset bundle:\n"
                               );

            ImGui::TextWrapped("'%s'\n",
                               m_presetBundleActiveLoad.c_str()
                               );
            
        }
        ImGui::End();
        

    }
    
    #if 1
        {
            
            ImVec2 pos(20, 10);
            ImVec2 margin(20, 20);
            ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));
            ImGui::SetNextWindowSize (ImVec2(ImGui::GetIO().DisplaySize.x - pos.x - margin.x, 0) );
//            ImGui::SetNextWindowContentSize (ImVec2(ImGui::GetIO().DisplaySize.x - 80, 0) );
            if (ImGui::Begin("PresetTitle", &m_showTitle, 0
                             | ImGuiWindowFlags_NoMove
                             | ImGuiWindowFlags_NoDecoration
                             |  ImGuiWindowFlags_NoTitleBar
                             //                         | ImGuiWindowFlags_NoResize
                             | ImGuiWindowFlags_NoScrollbar
                             | ImGuiWindowFlags_AlwaysAutoResize
                             | ImGuiWindowFlags_NoSavedSettings
                             | ImGuiWindowFlags_NoFocusOnAppearing
                             | ImGuiWindowFlags_NoNav
                             ))
            {
                {
  //                  auto &presetName = m_vizualizer->GetPresetName();
//                    ImGui::TextWrapped("%s\n", presetName.c_str());

                    if (m_currentPreset)
                    {
                        ImGui::TextWrapped("(%d/%d) %s\n",
                                           m_currentPreset->Index,
                                           (int)m_presetList.size(),
                                           m_currentPreset->Name.c_str());

                    }
                }
                
            }
            ImGui::End();
            
        }
    #endif
  

}

void VizController::GetPresetTitle(std::string &title) const
{
    if (m_currentPreset)
    {
         StringFormat(&title,
                        "(%d/%d) %s",
                           m_currentPreset->Index,
                           (int)m_presetList.size(),
                           m_currentPreset->Name.c_str()
                        );

    } else
    {
        title.clear();
    }

}


bool VizController::IsToolbarButtonVisible(UIToolbarButton button)
{
    switch (button)
    {
        case UIToolbarButton::NavigatePrevious:
        case UIToolbarButton::NavigateNext:
        case UIToolbarButton::NavigateShuffle:
            return true;

        case UIToolbarButton::SelectionLock:
            return !GetSelectionLock();
            
        case UIToolbarButton::SelectionUnlock:
            return GetSelectionLock();
            
        case UIToolbarButton::Pause:
            return IsPlaying();

        case UIToolbarButton::Play:
            return !IsPlaying();

        case UIToolbarButton::Step:
            return true;
            
        case UIToolbarButton::MicrophoneOn:
            return IsMicrophoneEnabled();
            
        case UIToolbarButton::MicrophoneOff:
            return !IsMicrophoneEnabled();

        case UIToolbarButton::SettingsShow:
            return !GetSettingsVisible();

        case UIToolbarButton::SettingsHide:
            return GetSettingsVisible();

        case UIToolbarButton::Blacklist:
            return GetCurrentPresetRating() != -1.0f;

        case UIToolbarButton::Unblacklist:
            return GetCurrentPresetRating() == -1.0f;

        case UIToolbarButton::Like:
            return GetCurrentPresetRating() != 1.0f;
            
        case UIToolbarButton::Unlike:
            return GetCurrentPresetRating() == 1.0f;

            
        default:
            return true;

    }
}

void VizController::OnToolbarButtonPressed(UIToolbarButton button)
{
    switch (button)
    {
        case UIToolbarButton::NavigatePrevious:
            NavigatePrevious();
            break;
        case UIToolbarButton::NavigateNext:
            NavigateNext();
            break;
            
        case UIToolbarButton::NavigateShuffle:
            NavigateRandom();
            break;

        case UIToolbarButton::SelectionLock:
            SetSelectionLock(true);
            break;
            
        case UIToolbarButton::SelectionUnlock:
            SetSelectionLock(false);
            break;
            
        case UIToolbarButton::Pause:
            Pause();
            break;

        case UIToolbarButton::Play:
            Play();
            break;

        case UIToolbarButton::Step:
            SingleStep();
            break;
            
        case UIToolbarButton::MicrophoneOn:
            SetMicrophoneEnabled(false);
            break;
            
        case UIToolbarButton::MicrophoneOff:
            SetMicrophoneEnabled(true);
            break;
            
        case UIToolbarButton::SettingsShow:
            SetSettingsVisible(true);
            break;

        case UIToolbarButton::SettingsHide:
            SetSettingsVisible(false);
            break;

        case UIToolbarButton::Blacklist:
            SetCurrentPresetRating(-1.0f);
            break;

        case UIToolbarButton::Unblacklist:
            SetCurrentPresetRating(0.0f);
            break;

        case UIToolbarButton::Like:
            SetCurrentPresetRating(1.0f);
            break;

        case UIToolbarButton::Unlike:
            SetCurrentPresetRating(0.0f);
            break;


            
        default:
            break;

    }
    

}

void VizController::DrawControlsWindow()
{
    {

        
        ImVec2 pos(20, 50);
        ImVec2 margin(20, 20);
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize (ImVec2(ImGui::GetIO().DisplaySize.x - pos.x - margin.x, 0) );

        
        //        ImGui::SetNextWindowContentWidth(  ImGui::GetIO().DisplaySize.x - 80);

//        ImGui::SetNextWindowPos(ImVec2(20, ImGui::GetIO().DisplaySize.y - 60), ImGuiCond_Always, ImVec2(0.0f, 0.0f));
//        ImGui::SetNextWindowContentWidth(800);
        if (ImGui::Begin(
                         //m_currentPreset->Name.c_str(),
                         "MilkdropControls",
                         &m_showControls, 0
                         | ImGuiWindowFlags_NoMove
                         | ImGuiWindowFlags_NoDecoration
                         |  ImGuiWindowFlags_NoTitleBar
                         | ImGuiWindowFlags_NoResize
                         | ImGuiWindowFlags_NoScrollbar
//                         | ImGuiWindowFlags_AlwaysAutoResize
                         | ImGuiWindowFlags_NoSavedSettings
  //                       | ImGuiWindowFlags_NoFocusOnAppearing
                         | ImGuiWindowFlags_NoNav
                         ))
        {
            //                    ImGui::PushFont(g_font_awesome);
            
            ImGui::SetWindowFontScale(1.0f);
            
            
            if (ToolbarButton(ICON_FA_COG,"Toggle settings"))
            {
                ToggleSettingsMenu();
            }
            
            ToolbarSeparator();
            
            if (ToolbarButton(GetSelectionLock() ? ICON_FA_LOCK : ICON_FA_UNLOCK,"Lock preset switching"))
            {
                ToggleSelectionLock();
            }
            

            
            if (ToolbarButton(ICON_FA_ARROW_LEFT,"Select previous preset"))
            {
                NavigatePrevious();
                
            }

            
            if (ToolbarButton(ICON_FA_ARROW_RIGHT,"Select next preset"))
                
            {
                NavigateNext();
            }
            
            if (ToolbarButton(ICON_FA_RANDOM,"Select random preset"))
                
            {
                NavigateRandom();
            }
            
            
            if (ToolbarButton(!m_paused ? ICON_FA_PAUSE : ICON_FA_PLAY,"Pause visualizer"))
            {
                TogglePause();
            }

            ToolbarSeparator();
            
            float rating = GetCurrentPresetRating();

            if (ToolbarToggleButton(ICON_FA_THUMBS_DOWN, ICON_FA_THUMBS_DOWN, rating == -1.0f,  "Blacklist preset"))
            {
                SetCurrentPresetRating( rating == -1.0f ? 0.0f : -1.0f);
            }

            if (ToolbarToggleButton(ICON_FA_HEART, ICON_FA_HEART_O, rating == 1.0f, "Like preset"))
            {
                SetCurrentPresetRating( rating == 1.0f ? 0.0f : 1.0f);
            }


//            if (ToolbarToggleButton(ICON_FA_BUG, rating == "Flag preset as broken/buggy"))
//            {
//
//        }


//            ToolbarSeparator();

//            if (ToolbarButton(ICON_FA_THUMBS_UP,""))
//            {
////                TogglePause();
//            }
//

            
//
//
//
//            if (ToolbarButton(ICON_FA_STEP_FORWARD,"Step a single preset frame"))
//            {
//                SingleStep();
//            }

            
            
            ToolbarSeparator();
            

            if (ToolbarToggleButton(ICON_FA_SIGN_IN, ICON_FA_SIGN_IN, IsTesting(), "Toggle testing mode"))
            {

                if (!IsTesting())
                    TestingStart();
                else
                    TestingAbort();
            }
//
//            if (ToolbarButton(ICON_FA_CAMERA,"Capture screenshot"))
//            {
//                CaptureScreenshot();
//            }
//
////            if (ToolbarButton(ICON_FA_CLOCK_O, "Capture performance profile"))
//            if (ToolbarButton(ICON_FA_CLOCK_O, "Profiler"))
//            {
//                m_showProfiler = !m_showProfiler;
////                TProfiler::CaptureNextFrame();
//            }
//
//
//
//
            
            
            {
                ImGui::SameLine();
                bool microphone = IsMicrophoneEnabled();
                if (ToolbarButton(microphone ? ICON_FA_MICROPHONE : ICON_FA_MICROPHONE_SLASH, "Enable microphone"))
                {
                    ToggleMicrophone();
                    SaveConfig();
                }
            }
             
            
            
          
            
//            auto pos = ImGui::GetCursorPos();
//            auto sz = ImGui::GetWindowSize();
//
            
//            ToolbarSeparator(sz.x - pos.x - 64);
            
            
//            pos.x = sz.x - 80;
//            ImGui::SetCursorPos(pos);
//
            if (ToolbarButton(ICON_FA_TIMES,"Close controls"))
            {
                ToggleControlsMenu();
            }

            
            ImGui::SetWindowFontScale(1.0f);


        }
        ImGui::End();
    }
    
}



static void TableNameValue(const char *name, const char *format, ...)
{
    char str[64 * 1024];
    
    va_list arglist;
    va_start(arglist, format);
    int count = vsnprintf(str, sizeof(str), format, arglist);
    va_end(arglist);
    (void)count;
    str[sizeof(str) - 1] = '\0';

    
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(name);
    ImGui::TableSetColumnIndex(1);
    ImGui::TextUnformatted(str);
}




void VizController::DrawSettingsWindow()
{

    const char *appName = PlatformGetAppName();
    
    ImVec2 pos(20, 120);
    ImVec2 margin(20, 80
                  );
    
    ImGui::SetNextWindowPos(pos, ImGuiCond_Once, ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize (ImVec2(ImGui::GetIO().DisplaySize.x - pos.x - margin.x, ImGui::GetIO().DisplaySize.y - pos.y - margin.y) );

    if (!ImGui::Begin(appName, &m_showSettings, 0
                     | ImGuiWindowFlags_NoMove
//                     | ImGuiWindowFlags_NoDecoration
                   |  ImGuiWindowFlags_NoTitleBar
                     | ImGuiWindowFlags_NoResize
                     | ImGuiWindowFlags_NoScrollbar
//                     | ImGuiWindowFlags_AlwaysAutoResize
                     | ImGuiWindowFlags_NoSavedSettings
                     | ImGuiWindowFlags_NoFocusOnAppearing
//                     | ImGuiWindowFlags_MenuBar
                     | ImGuiWindowFlags_NoNav
                     ))
    {
        ImGui::End();
        return;
    }
    
    static bool showMenu = false;
    
    // Menu Bar
    if (showMenu && ImGui::BeginMenuBar())
    {

        if (ImGui::BeginMenu("View"))
        {
            
             
//            if (ImGui::MenuItem("Preset Editor"))
//            {
//                m_vizualizer->ShowPresetEditor();
//            }
//            
//            if (ImGui::MenuItem("Preset Debugger"))
//            {
//                m_vizualizer->ShowPresetDebugger();
//            }
            
            ImGui::MenuItem("Profiler", "P", &m_showProfiler);
            ImGui::MenuItem("ImGui Demo Window", NULL, &m_showDemoWindow);
            

            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Debug"))
        {
            
            if (ImGui::MenuItem("Load Empty Preset"))
            {
//                SelectEmptyPreset();
            }
            
//                if (ImGui::MenuItem("Run Unit Tests"))
//                {
//                    RunUnitTests();
//                }
    
            if (ImGui::MenuItem("Capture Screenshot"))
            {
                CaptureScreenshot();
            }
    
            bool testing = IsTesting();
            if (ImGui::MenuItem("Test All Presets", NULL, &testing))
            {
                if (!IsTesting())
                {
                    TestingStart();
                } else
                {
                    TestingAbort();
                }
            }
            
            

//            ImGui::MenuItem("Use Quad Line Renderer", NULL, &UseQuadLines);
//                ImGui::MenuItem("Use Correct Blur", NULL, &UseCorrectBlur);

//                ImGui::MenuItem("Metrics", NULL, &show_app_metrics);
//                ImGui::MenuItem("Style Editor", NULL, &show_app_style_editor);
//                ImGui::MenuItem("About Dear ImGui", NULL, &show_app_about);
            ImGui::EndMenu();
        }

//            if (ImGui::BeginMenu("Help"))
//            {
//                ImGui::MenuItem("Metrics", NULL, &show_app_metrics);
//                ImGui::MenuItem("Style Editor", NULL, &show_app_style_editor);
//                ImGui::MenuItem("About Dear ImGui", NULL, &show_app_about);
//                ImGui::EndMenu();
//            }
        ImGui::EndMenuBar();
    }
    
    DrawSettingsTabs();
    
    
    
    
    
    ImGui::End();
   
}



static void AppendNameValue(std::string &table, const char *name, const char *format, ...)
{
    char str[64 * 1024];
    
    va_list arglist;
    va_start(arglist, format);
    int count = vsnprintf(str, sizeof(str), format, arglist);
    va_end(arglist);
    (void)count;
    str[sizeof(str) - 1] = '\0';

    table += name;
    table += '\t';
    table += str;
    table += '\n';
}

static void AppendSeparator(std::string &table)
{
    table += '\n';
}



void VizController::GetAboutInfo(std::string &info) const
{
    AppendNameValue(info, "Platform", "%s", PlatformGetPlatformName() );
    AppendNameValue(info, "App Version", "%s", PlatformGetAppVersion() );
    AppendNameValue(info, "Build Config", "%s",  PlatformGetBuildConfig() );
    AppendNameValue(info, "Device Model", "%s", PlatformGetDeviceModel() );
    AppendNameValue(info, "Architecture", "%s", PlatformGetDeviceArch() );
    
    AppendNameValue(info, "IMGUI Version", "%s", ImGui::GetVersion() );
    
    AppendSeparator(info);
    
    AppendNameValue(info, "Elapsed Time", "%.2fs",  m_time );
    
    AppendNameValue(info, "Presets Loaded", "%d", (int)m_presetHistory.size() );
    AppendNameValue(info, "Presets Total", "%d", (int)m_presetList.size() );

    PlatformMemoryStats memstats;
    if (PlatformGetMemoryStats(memstats)) {
        
        AppendSeparator(info);
        AppendNameValue(info, "Virtual Mem", "%.2fMB", (memstats.virtual_size_mb));
        AppendNameValue(info, "Resident Mem", "%.2fMB", (memstats.resident_size_mb));
        AppendNameValue(info, "Resident Max", "%.2fMB", (memstats.resident_size_max_mb));
    }
    
    AppendSeparator(info);

    AppendNameValue(info, "Frame Rate", "%3.2f", 1000.0f / m_frameTime );
    AppendNameValue(info, "FrameTime", "%3.2fms", m_frameTime );
    AppendNameValue(info, "RenderTime", "%3.2fms", m_renderTime );
}

void VizController::DrawPanel_About()
{
    int table_flags =
    ImGuiTableFlags_BordersInnerV|
    ImGuiTableFlags_SizingFixedFit|
    ImGuiTableFlags_ScrollY
    ;
    
    ImVec2 sz = ImGui::GetContentRegionAvail();
    if (ImGui::BeginTable("##stats_columns", 2, table_flags, sz))
    {
        //            ImGui::TableNextRow();
        ImGui::TableNextColumn();
        
        //TableNameValue("App", "%s", PlatformGetAppName() );
        TableNameValue("Platform", "%s", PlatformGetPlatformName() );
        TableNameValue("App Version", "%s", PlatformGetAppVersion() );
        TableNameValue("Build Config", "%s",  PlatformGetBuildConfig() );
        TableNameValue("Device Model", "%s", PlatformGetDeviceModel() );
        TableNameValue("Architecture", "%s", PlatformGetDeviceArch() );
        
        TableNameValue("IMGUI Version", "%s", ImGui::GetVersion() );
        
        ImGui::Separator();
        
        TableNameValue("Elapsed Time", "%.2fs",  m_time );
        
        TableNameValue("Presets Loaded", "%d", (int)m_presetHistory.size() );
        TableNameValue("Presets Total", "%d", (int)m_presetList.size() );
        PlatformMemoryStats memstats;
        
        if (PlatformGetMemoryStats(memstats)) {
            
            ImGui::Separator();
            TableNameValue("Virtual Mem", "%.2fMB", (memstats.virtual_size_mb));
            TableNameValue("Resident Mem", "%.2fMB", (memstats.resident_size_mb));
            TableNameValue("Resident Max", "%.2fMB", (memstats.resident_size_max_mb));
        }
        
        ImGui::Separator();

        TableNameValue("Frame Rate", "%3.2f", 1000.0f / m_frameTime );
        TableNameValue("FrameTime", "%3.2fms", m_frameTime );
        TableNameValue("RenderTime", "%3.2fms", m_renderTime );
        
        ImGui::Separator();
        
        ImGui::EndTable();
    }
    
}

void VizController::DrawPanel_Video()
{
    int table_flags =
    ImGuiTableFlags_BordersInnerV|
    ImGuiTableFlags_SizingFixedFit|
    ImGuiTableFlags_ScrollY

    ;
    
    ImVec2 sz = ImGui::GetContentRegionAvail();

    if (ImGui::BeginTable("##video_table", 2, table_flags, sz))
    {
//                ImGui::TableSetColumnWidth(0, 240 );
//                ImGui::TableSetColumnWidth(1, 800 );

//            ImGui::TableNextRow();
//                ImGui::TableNextColumn();
    
        
        TableNameValue("Video Driver", "%s", m_context->GetDriver().c_str() );
        TableNameValue("Video Device", "%s", m_context->GetDesc().c_str() );
        TableNameValue("Shading Language", "%s", m_context->GetShadingLanguage().c_str() );

        auto internal_texture = m_vizualizer->GetInternalTexture();
        TableNameValue("Internal Texture", "%dx%dx%s", internal_texture->GetWidth(), internal_texture->GetHeight(), PixelFormatToString(internal_texture->GetFormat()) );

        auto texture = m_vizualizer->GetOutputTexture();
        TableNameValue("Output Texture", "%dx%dx%s", texture->GetWidth(), texture->GetHeight(), PixelFormatToString(texture->GetFormat()) );


        ImGui::Separator();

        int screenId = 0;
        for (auto info : m_screens)
        {
            

            ImGui::TableNextRow();
            
            TableNameValue("Screen", "#%d", screenId);
            TableNameValue("Size", "%dx%d", info.size.width,
                           info.size.height
                           );
            TableNameValue("Format", "%s", PixelFormatToString(info.format));
            TableNameValue("RefreshRate", "%.fhz", info.refreshRate);
            TableNameValue("Scale", "%.1f", info.scale);
            TableNameValue("HDR Enabled", "%s", info.hdr ? "true" : "false");
            TableNameValue("MaxEDR", "%.2f", info.maxEDR);
            TableNameValue("ColorSpace", "%s", info.colorSpace.c_str());

            ImGui::Separator();

            screenId++;
        }

        
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("");
        ImGui::TableNextColumn();
        
        {
            
            
//                    IMGUI_API void          PlotHistogram(const char* label, const float* values, int values_count, int values_offset = 0, const char* overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0, 0), int stride = sizeof(float));

          ImGui::PlotHistogram("",
                               m_frameTimeHistory.data(),
                               (int)m_frameTimeHistory.size(),
                                0, // values offset
                               "", // overlay text
                               0.0f, 64.0f, // scale_min scale_msx
                               ImVec2(512,80)
                               );
        }
        ImGui::TableNextColumn();

        
        ImGui::EndTable();
    }

}


void PresetList::Sort(ImGuiTableSortSpecs *specs)
{
    auto selected = (_position < (int)_list.size()) ? _list[_position] : nullptr;
    
    
    std::sort( _list.begin(), _list.end(),
          [specs](const PresetInfoPtr &p1, const PresetInfoPtr &p2) -> bool
          {
                if (specs)
                for (int n = 0; n < specs->SpecsCount; n++)
                {
                    const ImGuiTableColumnSortSpecs &sort_spec = specs->Specs[n];
                    auto &a = sort_spec.SortDirection == ImGuiSortDirection_Ascending ? p2 : p1;
                    auto &b = sort_spec.SortDirection == ImGuiSortDirection_Ascending ? p1 : p2;
                    switch (sort_spec.ColumnIndex)
                    {
                        case 0:
                            return a->Index < b->Index;
                        case 1:
                            return a->Name < b->Name;
                        case 2:
                            if (a->Rating == b->Rating)
                                break;
                            return a->Rating < b->Rating;
                        case 3:
                        {
                            float counta = a->TestResult ? a->TestResult->AveFrameTime : 0;
                            float countb = b->TestResult ? b->TestResult->AveFrameTime : 0;
                            if (counta == countb)
                                break;
                            return counta < countb;
                        }
                        default:
                            break;
                    }
                }
        
                return p1->Index < p2->Index;
            }
      );

    
    if (selected)
    {
        // find new position after sorting...
        _position = 0;
        for (auto preset : _list)
        {
            if (preset == selected)
                break;
            _position++;
        }
    }
}


static int DrawPresetTableUI(PresetList &list, bool sortable, bool forceresort)
{
    int result = -1;
    
    ImGuiTableFlags flags =
    ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV
    | ImGuiTableFlags_RowBg
    | ImGuiTableFlags_ScrollY
    
    
    //        | ImGuiTableFlags_ColumnsWidthFixed
    ; // | ImGuiTableFlags_ContextMenuInBody;
    
    if (sortable)
    {
        flags |= ImGuiTableFlags_Sortable;
    }
    
    ImVec2 sz = ImGui::GetContentRegionAvail();
    if (ImGui::BeginTable("##presets", 4, flags, sz))
    {
        
        ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending, 40);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0);
        ImGui::TableSetupColumn("*", ImGuiTableColumnFlags_WidthFixed, 20);
        ImGui::TableSetupColumn("Perf", ImGuiTableColumnFlags_WidthFixed, 100);
        
        ImGui::TableHeadersRow();


        
        if (sortable)
        {
            ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs();
            if (forceresort) sort_specs->SpecsDirty = true;
            if (sort_specs && sort_specs->SpecsDirty)
            {
                list.Sort(sort_specs);
                sort_specs->SpecsDirty = false;
            }
        }
        
        auto &v = list.List();
        
        ImGuiListClipper clipper((int)list.size(),   ImGui::GetTextLineHeightWithSpacing() );
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
            {
                auto preset = v[i];
                bool is_selected = (list.GetPosition() == i);
                
                
                ImGui::TableNextColumn();
                ImGui::Text("%d",  preset->Index );
                
                
                ImGui::TableNextColumn();
                if (ImGui::Selectable(preset->Name.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns))
                {
                    result = i;
                }
                
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
                
                
                ImGui::TableNextColumn();

                if (preset->Rating < 0)
                {
                    ImGui::TextUnformatted( "-" );
                } else if (preset->Rating > 0)
                {
                    ImGui::TextUnformatted( "+" );
                }
                
                ImGui::TableNextColumn();


                if (is_selected)
                {
                    float progress = preset->Progress;
                    ImGui::ProgressBar(progress, ImVec2(128.0f, ImGui::GetTextLineHeight() ));
                }
                else
                if (preset->TestResult)
                {
                    auto profile = preset->TestResult;
//
//                    ImGui::Text("%3.2f",
//                                profile->AveFrameTime
//                                );

                    
                    ImGui::Text("%.2f%%",
                                100.0f - profile->SlowPercentage
                                );

//                    ImGui::Text("frametime:%3.2f fast:%d slow:%d (%.2f%%)",
//                                profile->AveFrameTime,
//                                profile->FastFrameCount,
//                                profile->SlowFrameCount,
//                                profile->SlowPercentage
//                                );

                }
                
            }
        }
        
        if (list.itemAdded())
        {
            ImGui::SetScrollY(clipper.ItemsHeight * list.GetPosition() );
            list.clearItemAdded();
        }
        
        ImGui::EndTable();
    }
    
    return result;

}

void VizController::DrawPanel_History()
{
    if (ImGui::Button("Clear"))
    {
        m_presetHistory.clear();
        SaveHistory();
    }

    int selected = DrawPresetTableUI( m_presetHistory, false, false);
    if (selected >= 0)
    {
        auto preset = m_presetHistory.SelectIndex( selected);
        LoadPreset(preset, false);
        m_navigateHistory = true;
    }
    
}

void VizController::DrawPanel_Presets()
{
    
    //            ImGui::BeginChild("left pane", ImVec2(400, 0), true);
    //
    //            static PresetGroupPtr selectedGroup;
    //            for (auto group : m_presetGroups)
    //            {
    //                if (!group->Presets.empty())
    //                {
    //                    bool enabled = true;
    //                    ImGui::Checkbox("", &enabled);
    //                    ImGui::SameLine();
    //                    if (ImGui::Selectable(group->Name.c_str(), group == selectedGroup))
    //                        selectedGroup = group;
    //                }
    //            }
    //            ImGui::EndChild();
    //
    //            ImGui::SameLine();
    
    ImGui::BeginGroup();
    
    bool presetFilterUpdate = false;
    bool force_resort = false;
    if (m_presetFilter.Draw("Search", 200.0f)) presetFilterUpdate = true;
    
    if (presetFilterUpdate || m_presetListFiltered.empty())
    {
        if (!m_presetFilter.IsActive())
        {
            m_presetListFiltered = m_presetList;
        }
        else
        {
            m_presetListFiltered.clear();
            for (auto preset : m_presetList.List())
            {
                if (m_presetFilter.PassFilter(preset->Name.c_str()))
                {
                    m_presetListFiltered.Add(preset);
                }
            }
        }
        
        force_resort = true;
    }
    
    
    ImGui::SameLine();
    
    if (!IsTesting())
    {
        if (ImGui::Button("Start Tests"))
        {
            TestingStart();
        }
    }
    else
    {
    
        if (ImGui::Button("End Tests"))
        {
            TestingAbort();
        }
    }
    
    
    int selected = DrawPresetTableUI(m_presetListFiltered, true, force_resort);
    if (selected >= 0)
    {
        auto preset = m_presetListFiltered.SelectIndex(selected);
        if (preset)
        {
            LoadPreset(preset, false);
            m_navigateHistory = false;
        }
    }
    


    ImGui::EndGroup();
    

}

void VizController::DrawPanel_Audio()
{
    int table_flags =
    ImGuiTableFlags_BordersInnerV|
    ImGuiTableFlags_SizingFixedFit|
    ImGuiTableFlags_ScrollY
    ;
    
    ImVec2 sz = ImGui::GetContentRegionAvail();
    if (ImGui::BeginTable("##audio_table", 2, table_flags, sz))
    {
//                ImGui::TableSetColumnWidth(0, 240 );
//                ImGui::TableSetColumnWidth(1, 800 );
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        ImGui::Text("Audio Source");
        ImGui::TableNextColumn();
        
        //IAudioSourcePtr sources[] = {m_audio_null,m_audio_wavfile, m_audio_microphone, nullptr};
        IAudioSourcePtr sources[] = {m_audio_null,m_audio_wavfile,m_audio_mdz, m_audio_microphone, nullptr};

        for (int i=0; sources[i]; i++)
        {
            auto source = sources[i];
            bool selected = (source == m_audioSource);
            if (ImGui::RadioButton( source->GetDescription().c_str(), selected ))
            {
                m_audioSource = source;
                SaveConfig();
            }
        }

        
        ImGui::TableNextColumn();

        ImGui::Separator();

        ImGui::Text("Gain");
        ImGui::TableNextColumn();
        if (ImGui::SliderFloat("", &m_audioGain, 0.0f, 8.0f)) {
            SaveConfig();
        }
        ImGui::TableNextColumn();

        
        ImGui::Separator();

        TableNameValue("Description", "%s", m_audioSource->GetDescription().c_str());
        
        m_vizualizer->DrawAudioUI();;
        
        ImGui::Separator();
        
        ImGui::EndTable();
    }
    

}

void VizController::DrawPanel_Log()
{
    LogDrawPanel();
}

void VizController::DrawPanel_Screenshots()
{
    if (m_screenshots.empty())
    {
        while (m_screenshots.size() < 32)
        {
            auto texture = m_context->CreateRenderTarget("screenshot", 128, 128, PixelFormat::RGBA8Unorm );
            m_context->SetRenderTarget(texture, "clear", LoadAction::Clear );
            m_screenshots.push_back(texture);
        }
        
        m_context->SetRenderTarget(nullptr);
    }
    
    auto fp = ImGui::GetStyle().FramePadding;
    ImGui::GetStyle().FramePadding = ImVec2(0,0);
    
     
    int i = 0;
    for (auto texture : m_screenshots)
    {
        ImGui::Image( texture.get(),  ImVec2(128, 128) );

        if (++i < 8) {
            ImGui::SameLine();
        } else {
//                    ImGui::NewLine();
            i = 0;
        }
    }
    
    ImGui::GetStyle().FramePadding = fp;
}



void VizController::DrawSettingsTabs()
{


    if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
    {
        if (ImGui::BeginTabItem("About"))
        {
            DrawPanel_About();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("History"))
        {
            DrawPanel_History();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Presets"))
        {
            DrawPanel_Presets();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Video"))
        {
            DrawPanel_Video();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Audio"))
        {
            DrawPanel_Audio();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Log"))
        {
            DrawPanel_Log();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Textures"))
        {
            if (m_texture_map)
                m_texture_map->ShowUI();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Profiler"))
        {
            TProfiler::OnGUIPanel();
            ImGui::EndTabItem();
        }
        
        
        if (ImGui::BeginTabItem("Screenshots"))
        {
            DrawPanel_Screenshots();
            ImGui::EndTabItem();
        }
        
        
        if (ImGui::BeginTabItem("Debug"))
        {
            if (ImGui::Button("ImGui Debug Window"))
            {
                m_showDemoWindow = !m_showDemoWindow;
            }

            if (ImGui::Button("Test All Presets"))
            {
                if (!IsTesting())
                {
                    TestingStart();
                } else
                {
                    TestingAbort();
                }
            }
            

            ImGui::EndTabItem();
        }
        
        
        ImGui::EndTabBar();
    }

}


std::string VizController::GetScreenshotPath(PresetInfoPtr preset)
{
    std::string spath =  preset->Name + ".png";  // GetScreenshotPath(preset);
    std::string path = PathCombine(m_screenshotDir, spath);
    return path;
}

void VizController::CaptureScreenshot()
{
    if (!m_currentPreset)
        return;
    

    std::string spath =  m_currentPreset->Name + ".png";  // GetScreenshotPath(preset);
    std::string path = PathCombine(m_screenshotDir, spath);

    CaptureScreenshot(path);
}



static void SwapRGBA(ImageDataPtr image)
{
    for (int y = 0; y < image->height; y++)
    {
        uint8_t *ptr = image->data + y * image->pitch;
        for (int x = 0; x < image->width; x++)
        {
            // swap BGRA -> RGBA
            std::swap(ptr[0], ptr[2]);
            ptr += 4;
        }
    }
}


void VizController::CaptureScreenshot(std::string path)
{
    PROFILE_FUNCTION()
    
    auto texture = m_context->CreateRenderTarget("screenshot", 512, 512, PixelFormat::BGRA8Unorm );

    
    
    if (texture->GetFormat() != PixelFormat::RGBA8Unorm &&
        texture->GetFormat() != PixelFormat::BGRA8Unorm)
    {
        LogError("ERROR: Unsupported texture format for screen captures\n");
        return;
        
    }

    // capture screenshot...
    m_vizualizer->CaptureScreenshot(texture, Vector2(0,0),  texture->GetSize() );

    m_context->GetImageDataAsync(texture, [path] (ImageDataPtr image) {
            if (!image) {
                LogError("ERROR: Could not get image data from texture\n");
                return;
            }
        
#if 1
            DispatchVoid([path, image]() -> void {

                StopWatch sw;
              //  assert(image->format == PixelFormat::RGBA8Unorm);
                
        //        if (image->format == PixelFormat::RGBA32Float)
        //        {
        //            ImageWriteToHDR(path, (float *)image->data, image->width, image->height);
        //            LogPrint("Screenshot saved to: %s (%.2fms)\n", path.c_str(), sw.GetElapsedMilliSeconds());
        //        }
                
                // swap BGRA -> RGBA
                if (image->format == PixelFormat::BGRA8Unorm)
                {
                    SwapRGBA(image);
                    image->format = PixelFormat::RGBA8Unorm;
                }
                

                DirectoryCreateRecursive( PathGetDirectory(path) ) ;

                if (ImageWriteToFile(path, (const uint32_t *)image->data, image->width, image->height))
                {
                    LogPrint("Screenshot saved to: %s (%.2fms)\n", path.c_str(), sw.GetElapsedMilliSeconds());
                    
//                    std::string serverUrl;
//                    if (Config::TryGetString("SERVER_URL", &serverUrl)) {
////                        std::string name = PathGetFileName(path);
//                        std::string url = PathCombine(PathCombine(serverUrl, "appdata"), subpath);
//                        HttpSendFile("POST", url, fullPath.c_str(), false, "image/png", nullptr);
//                    }
                } else
                {
                    LogError("ERROR saving screenshot to: %s\n", path.c_str());
                   
                }
            });
#endif
        });

    
}

    

    
extern void DebugGUIInputDevice();

void VizController::DrawDebugUI()
{
    
    PROFILE_FUNCTION_CAT("ui");
    
    // clicking on nothing will toggle debug UI
    if (ImGui::IsMouseClicked(0))
    {
        if (!m_showUI)
        {
            m_showUI = true;
        }
        else if (!ImGui::GetIO().WantCaptureMouse )
        {
            m_showUI = false;
        }
    }

    
    if (!ImGui::IsAnyItemActive())
    {
        if (ImGui::IsKeyPressed( KEYCODE_L))
        {
            SetSelectionLock(!m_selectionLocked);
        }
        
        
        if (ImGui::IsKeyPressed( KEYCODE_P))
        {
            TogglePause();
        }
        
        if (ImGui::IsKeyPressed( KEYCODE_LEFT, false))
        {
            NavigatePrevious();
        }
        
        if (ImGui::IsKeyPressed( KEYCODE_RIGHT, false))
        {
            if (ImGui::GetIO().KeyShift)
                SingleStep();
            else
                NavigateNext();
        }
        
        if (ImGui::IsKeyPressed( KEYCODE_SPACE, false))
        {
            NavigateRandom();
        }

        
        if (ImGui::IsKeyPressed( KEYCODE_ESCAPE))
        {
            ToggleControlsMenu();
        }
    }
    
    if (m_showUI)
    {

        if (m_showControls)
        {
            if (m_showTitle)
            {
                DrawTitleWindow();
            }

            DrawControlsWindow();
        }


        if (m_showSettings)
        {
            DrawSettingsWindow();
            if (m_vizualizer)
                m_vizualizer->DrawDebugUI();
        }

        if (m_showProfiler)
            TProfiler::OnGUI(&m_showProfiler);

        
        
        if (m_showDemoWindow)
        {
            ImGui::ShowDemoWindow(&m_showDemoWindow);
        }
    }
}


void VizController::RenderFrame(float dt)
{
    PROFILE_FUNCTION();

    // measure time between frames
    m_frameTime = m_frameTimer.GetElapsedMilliSeconds();
    m_frameTimer.Restart();
    
    m_frameTimes.push_back(m_frameTime);
    
    if (m_frameTimeHistory.empty())
        m_frameTimeHistory.resize(256);
    
    m_frameTimeHistory.push_back(m_frameTime);
    m_frameTimeHistory.erase(m_frameTimeHistory.begin());

    
    if (m_audio_microphone && m_audioSource != m_audio_microphone) {
        m_audio_microphone->StopCapture();
    }
    
    if (!m_audioSource) {
        m_audioSource = m_audio_null;
    }
    
    

    // compute size of visualizer we want to render to, depends on displaysize
    Size2D size = m_context->GetDisplaySize();
    
    // clamp size of rendering to max output size, to cap performance
    int maxSize = std::max(size.width, size.height);
    if (m_maxOutputSize > 0 && maxSize > m_maxOutputSize)
    {
        float scale = m_maxOutputSize / (float)maxSize;
        size.width  = (int)( (float)size.width  * scale);
        size.height = (int)((float) size.height * scale);
    }
    
    
    m_vizualizer->Render(dt, size, m_audioSource);
    
    m_time += dt;
    m_frame++;
    
    if (!m_screenshots.empty())
    {
        // capture screenshot each frame...
        m_screenshotIndex %= m_screenshots.size();
        auto screenshot = m_screenshots[m_screenshotIndex];
        m_vizualizer->CaptureScreenshot(screenshot, Vector2(0,0), screenshot->GetSize() );
        m_screenshotIndex++;
    }
    

    if (m_currentPreset)
    {
        float old_progress = m_currentPreset->Progress;
        float progress = m_vizualizer->GetPresetProgress();
        m_currentPreset->Progress = progress;

        if (old_progress < 1.0f &&  progress >= 1.0f)
        {            
            // preset is complete
            
            if (IsTesting())
            {
                auto preset = m_presetListFiltered.SelectNext();
                if (preset)
                {
                    LoadPreset(preset, false);
                }
                else
                {
                    OnTestingComplete();
                }
            }
            else  if (!m_selectionLocked)
            {
                NavigateNext();
            }
        }
    }

}




void VizController::Render(int screenId, int screenCount, float dt)
{
    PROFILE_FUNCTION();
    
    m_dispatcher.Process();
    
    ProcessPresetLoad();
    
    m_deltaTime = dt;
    
    m_screens.resize(screenCount);
    m_screens[screenId] = m_context->GetDisplayInfo();


    // render visualizer to last screen
    if (screenId == (screenCount - 1))
    {
        
        StopWatch sw;
         
        if (!m_paused || m_singleStep)
        {
            RenderFrame(m_deltaTime);
    
            if (m_singleStep)
            {
                m_singleStep = false;
                m_paused     = true;
            }
        }
        else
        {

            TProfiler::SkipFrame();
        }

        
        // display on screen
        m_context->SetRenderTarget(nullptr, "DrawVisualizer", LoadAction::Clear);
        m_context->SetBlendDisable();
        if (m_currentPreset)
            m_vizualizer->Draw(m_contentMode, 1.0f);
        

        m_renderTime = sw.GetElapsedMilliSeconds();
        
    }

    
    
    if (screenId == 0)
    {
        // ImGui rendering to first screen
        
        ImGuiSupport_NewFrame();
        
        
        DrawDebugUI();
        m_context->SetRenderTarget(nullptr, "ImGui", (screenCount > 1) ? LoadAction::Clear : LoadAction::Load);
        ImGuiSupport_Render();
    }
}

void  VizController::ToggleSettingsMenu()
{
    m_showSettings = !m_showSettings;
    
}

void  VizController::ToggleControlsMenu()
{
    m_showUI = !m_showUI;
    
}


static bool LoadJsonFile(rapidjson::Document &doc, std::string path)
{
    std::string text;
    if (!FileReadAllText( path, text )) {
        return false;
    }

    doc.Parse(text);
    if (doc.HasParseError() || !doc.IsObject())
    {
        return false;
    }
    
    return true;
}

void VizController::LoadConfig()
{
    rapidjson::Document doc;
    if (!LoadJsonFile(doc, m_configPath))
        return;

    if (doc.HasMember("audio"))
    {
        const auto &audio = doc["audio"].GetObject();
        if (audio.HasMember("source"))
        {
            std::string source = audio["source"].GetString();
            if (source == "null") m_audioSource = m_audio_null; else
            if (source == "microphone") m_audioSource = m_audio_microphone; else
            if (source == "wavfile") m_audioSource = m_audio_wavfile;
            if (source == "mdz") m_audioSource = m_audio_mdz;
        }
        
        if (audio.HasMember("gain"))
        {
            m_audioGain = audio["gain"].GetFloat();
        }
    }

}


void VizController::SaveConfig()
{
    JsonStringWriter writer;
    writer.StartObject();

    {
        writer.Key("video");
        writer.StartObject();
        // TODO:
        writer.EndObject();
    }

    
    {
        writer.Key("audio");
        writer.StartObject();
     
        writer.Key("source");
        if (m_audioSource == m_audio_null) {
            writer.Value("null");
        }
        else if (m_audioSource == m_audio_microphone) {
            writer.Value("microphone");
        }
        else if (m_audioSource == m_audio_wavfile) {
            writer.Value("wavfile");
        }
        else if (m_audioSource == m_audio_mdz) {
            writer.Value("mdz");
        }

        writer.Write("gain", m_audioGain);

        writer.EndObject();
    }
    
    writer.EndObject();
    assert(writer.IsComplete());
    writer.SaveToFile(m_configPath);
}


void VizController::LoadRatings()
{
    rapidjson::Document doc;
    if (!LoadJsonFile(doc, m_ratingsPath))
        return;

    if (doc.HasMember("ratings"))
    {
        const auto &ratings = doc["ratings"].GetObject();
        
        for (const auto &member : ratings)
        {
            auto name = member.name.GetString();
            auto it = m_presetLookup.find(name);
            if (it != m_presetLookup.end())
            {
                it->second->Rating = member.value.GetFloat();
            }
        }
    }

}



void VizController::SaveRatings()
{
    JsonStringWriter writer;
    writer.StartObject();
 
    
    {
        writer.Key("ratings");
        
        writer.StartObject();
        for (auto preset : m_presetList.List())
        {
            if (preset->Rating != 0.0f)
            {
                writer.Key( preset->Name ).Value( preset->Rating  );
            }
        }
        writer.EndObject();
    }
    writer.EndObject();
    assert(writer.IsComplete());
    writer.SaveToFile(m_ratingsPath);

}



void VizController::LoadHistory()
{
    rapidjson::Document doc;
    if (!LoadJsonFile(doc, m_historyPath))
        return;

    if (doc.HasMember("history"))
    {
        const auto &history = doc["history"].GetArray();
        
        m_presetHistory.clear();
        for (const auto &element : history)
        {
            auto name = element.GetString();
            auto it = m_presetLookup.find(name);
            if (it != m_presetLookup.end())
            {
                m_presetHistory.Add(it->second);
            }
        }
        
        m_presetHistory.SelectLast();
    }

}

void VizController::SaveHistory()
{
    JsonStringWriter writer;
    writer.StartObject();
    
    writer.Key("history");
    {
        writer.StartArray();
        for (auto preset : m_presetHistory.List())
        {
            writer.Value( preset->Name );
        }
        writer.EndArray();
    }
    writer.EndObject();
    assert(writer.IsComplete());
    writer.SaveToFile(m_historyPath);
}



VizControllerPtr CreateVizController(ContextPtr context, std::string assetDir, std::string userDir)
{
    PROFILE_FUNCTION()

    
    TProfiler::AddCaptureLocation(userDir);

    auto vizController = std::make_shared<VizController>(context, assetDir, userDir);
    

    // add app supplied presets...
    vizController->LoadPresetBundlesFromDir( PathCombine(assetDir, "presets") );
    
    // add user supplied presets...
    vizController->LoadPresetBundlesFromDir( PathCombine(userDir, "presets") );
    
    //vizController->OpenInputAudioFile("audio/audio.wav");
    vizController->OpenInputAudioMDZ();

#if defined(__APPLE__)
   vizController->SetMicrophoneAudioSource(OpenAVAudioEngineSource());
//    vizController->SetMicrophoneAudioSource(OpenALAudioSource());
#elif defined(WIN32)
   vizController->SetMicrophoneAudioSource(OpenNullAudioSource());
#elif defined(__ANDROID__) || defined(OCULUS)
    vizController->SetMicrophoneAudioSource(OpenSLESAudioSource());
#else
    vizController->SetMicrophoneAudioSource(OpenNullAudioSource());
#endif
    
    vizController->LoadConfig();

    
    if (PlatformIsDebug())
    {
        vizController->ToggleControlsMenu();
        vizController->ShowDebugUI();
    }
    else
    {
        vizController->HideDebugUI();
    }

    
    
    return vizController;
}




