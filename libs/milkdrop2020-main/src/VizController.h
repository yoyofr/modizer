#pragma once

#include "IVisualizer.h"
#include "PresetInfo.h"
#include "PresetBundle.h"
#include "IVizController.h"
#include "vis_milk2/IAudioAnalyzer.h"
#include "script/script.h"


#include <map>
#include <set>
#include <string>
#include <queue>
#include <functional>
#include <thread>
#include <future>         // std::async, std::future



class VizController : public IVizController
{
public:
    
	VizController(render::ContextPtr context, std::string assetDir, std::string userDir);
	virtual ~VizController();

    virtual bool ShouldQuit() { return m_shouldQuit; }

	virtual void OpenInputAudioFile(const char *path);
    virtual void OpenInputAudioMDZ();
    virtual void SetMicrophoneAudioSource(IAudioSourcePtr source)
    {
        m_audio_microphone = source;
    }
    
	virtual void Render(int screenId, int screenCount, float dt = (1.0f / 60.0f) ) override;
    virtual void RenderFrame(float dt);
    
    virtual void SetNextNoRandom(bool norand) override;
    virtual void SetLock(bool locked) override;
    virtual void SetBlendTime(float blendtime) override;
    virtual void SetPresetTime(float presettime) override;

    void ProcessPresetLoad();

	virtual void LoadPresetBundlesFromDir(std::string dir);
    virtual void UpdatePresetList();

    virtual bool SelectPreset(const std::string &name);
    
    virtual void SetPreset(PresetInfoPtr presetInfo, PresetPtr preset, PresetLoadArgs args);
    
    virtual bool LoadPreset(PresetInfoPtr presetInfo, PresetLoadArgs args);
    virtual void LoadPreset(PresetInfoPtr presetInfo, bool blend);
    virtual void LoadPresetAsync(PresetInfoPtr presetInfo, PresetLoadArgs args);

    void AddToPlaylist();
    
    
    virtual void OnToolbarButtonPressed(UIToolbarButton button) override;
    virtual bool IsToolbarButtonVisible(UIToolbarButton button) override;

    virtual void NavigatePrevious() override;
    virtual void NavigateNext() override;
    virtual void NavigateRandom(bool blend = true) override;
    
    virtual void GetPresetTitle(std::string &title) const override;
    
    virtual void GetAboutInfo(std::string &info) const override;

    virtual void SingleStep();
    virtual void TogglePause();
    virtual void Pause();
    virtual void Play();
    virtual bool IsPlaying() const { return !m_paused && !m_singleStep;}

    virtual void ToggleSettingsMenu();
    virtual void ToggleControlsMenu();


    void TestAllPresets(std::function<void (std::string name, std::string error)> callback);
    
    
    virtual void LoadConfig();
    virtual void SaveConfig();

    virtual void LoadRatings();
    virtual void SaveRatings();

    virtual void LoadHistory();
    virtual void SaveHistory();

    virtual void SetSelectionLock(bool lock)  { m_selectionLocked = lock;}
    virtual bool GetSelectionLock() const  { return m_selectionLocked;}
    virtual void ToggleSelectionLock()
    {
        m_selectionLocked= !m_selectionLocked;
        m_paused = false;
    }
    
    virtual bool IsMicrophoneEnabled() const
    {
        return m_audioSource == m_audio_microphone;
    }
    
    virtual void SetMicrophoneEnabled(bool enabled)
    {
        if (enabled) {
            m_audioSource = m_audio_microphone;
        } else {
            //m_audioSource = m_audio_wavfile;
            m_audioSource = m_audio_mdz;
        }
        SaveConfig();
    }

    
    virtual void ToggleMicrophone()
    {
        SetMicrophoneEnabled (!IsMicrophoneEnabled());
    }

    
    virtual void DrawDebugUI();

    void OnTestingComplete();
    
    void DrawTitleWindow();
    void DrawControlsWindow();
    void DrawSettingsWindow();
    void DrawStatusTable();
    void DrawSettingsTabs();
    void DrawPresetSelector();
    void DrawPresetPlaylist();

    
    void DrawPanel_About();
    void DrawPanel_Video();
    void DrawPanel_History();
    void DrawPanel_Presets();
    void DrawPanel_Audio();
    void DrawPanel_Log();
    void DrawPanel_Screenshots();

    virtual bool IsDebugUIVisible() const override {return m_showUI;}
    virtual void ShowDebugUI() override {m_showUI = true;}
    virtual void HideDebugUI() override {m_showUI = false;}

    virtual void SetCurrentPresetRating(float rating) ;
    virtual float GetCurrentPresetRating() const;

    virtual bool GetControlsVisible() const override  {return m_showControls;}
    virtual void SetControlsVisible(bool visible) override  { m_showControls = visible; }

    virtual bool GetSettingsVisible() const  {return m_showSettings;}
    virtual void SetSettingsVisible(bool visible)  { m_showSettings = visible; }

    void CaptureAllScreenshots(bool overwriteExisting);
    
    virtual void CaptureTestResult(PresetTestResult &result);
    virtual void CaptureScreenshot();
    
    virtual void CaptureScreenshot(std::string path);
    
    virtual std::string GetScreenshotPath(PresetInfoPtr preset);
    
    bool IsTesting() {return m_testing;}
    void TestingStart();
    void TestingAbort();
    void TestingDump();
    std::string TestingResultsToString();
    

    void TestServer();
public:
    DispatchQueue               m_dispatcher;
    
	render::ContextPtr					m_context;

    std::vector<render::DisplayInfo> m_screens;

    std::vector<render::TexturePtr>          m_screenshots;
    int m_screenshotIndex = 0;
	std::string                 m_assetDir;
    std::string                 m_userDir;
    std::string                 m_configPath;
    std::string                 m_ratingsPath;
    std::string                 m_historyPath;
    std::string                 m_screenshotDir;
	PresetInfoPtr               m_currentPreset;
    std::string                 m_endpoint;

    bool        m_navigateHistory = true;
    bool        m_navigationNextNoRandom = false;
    PresetList	m_presetList;
    PresetList  m_presetListFiltered;
    PresetList  m_presetHistory;
    

    std::map<std::string, PresetInfoPtr>    m_presetLookup;
    
    std::queue< std::string >            m_presetBundleLoadQueue;
    std::string                          m_presetBundleActiveLoad;
    std::future<PresetBundlePtr>         m_presetBundleLoadFuture;      // future for next preset load
    std::vector<PresetBundlePtr>         m_presetBundles;       // loaded preset bundles
        
    ImGuiTextFilter     m_presetFilter;

    bool                        m_shouldQuit = false;
    
    bool                        m_selectionLocked = false;
    ContentMode                 m_contentMode = ContentMode::ScaleAspectFill;
    ITextureSetPtr              m_texture_map;
	IVisualizerPtr				m_vizualizer;

    IAudioSourcePtr             m_audio_null;
    IAudioSourcePtr             m_audio_wavfile;
    IAudioSourcePtr             m_audio_mdz;
    IAudioSourcePtr             m_audio_microphone;
    IAudioSourcePtr             m_audioSource;
    float                       m_audioGain = 1.0f;
    
	int                         m_frame = 0;
	float                       m_deltaTime = 0.0f;
    float                       m_renderTime = 0.0f;
    float                       m_frameTime = 0.0f;
	float                       m_time = 0.0f;
    float                       m_maxOutputSize = 2048.0f;
	bool						m_singleStep = false;
	bool						m_paused = false;
    bool                        m_showUI  = false;
    bool                        m_showControls  = true;
    bool                        m_showSettings  = false;
    bool                        m_showTitle  = true;
    bool                        m_showAudioWindow = false;
    bool                        m_showProfiler = false;
    bool                        m_showDemoWindow = false;

    StopWatch                   m_frameTimer;
    
    bool    m_testing = false;
    bool    m_captureScreenshots = false;
    bool    m_captureProfiles = false;
    bool    m_captureScreenshotsFast = false;

    std::future<bool>   m_screenshotFuture;
    
    std::future<void> m_LoadPresetFuture;
    
    float        m_fBlendTimeAuto= 2.7f;        // blend time when preset auto-switches
    float        m_fTimeBetweenPresets = 15.0f;        // <- this is in addition to m_fBlendTimeAuto

    std::mt19937     m_random_generator;
    
    std::vector<float> m_frameTimes;
    std::vector<float> m_frameTimeHistory;
};

using VizControllerPtr = std::shared_ptr<VizController>;


VizControllerPtr CreateVizController(render::ContextPtr context, std::string assetDir, std::string userDir);
