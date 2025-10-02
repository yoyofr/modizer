
#pragma once

#include <memory>
#include <string>

#include "PresetInfo.h"

enum class UIToolbarButton
{
   FixedSpace,
   FlexibleSpace,

   NavigatePrevious,
   NavigateNext,
   SelectionLock,
   SelectionUnlock,
   NavigateShuffle,
   
   Play,
   Pause,
   Step,
   
   Like,
   Unlike,
   Blacklist,
   Unblacklist,
   
   MicrophoneOn,
   MicrophoneOff,
    
   SettingsShow,
   SettingsHide,
   
   COUNT
};


class IVizController
{
public:
    IVizController() = default;
    virtual ~IVizController() = default;
    
    virtual void OnToolbarButtonPressed(UIToolbarButton button) = 0;
    virtual bool IsToolbarButtonVisible(UIToolbarButton button) = 0;

    virtual void NavigatePrevious() = 0;
    virtual void NavigateNext() = 0;
    virtual void NavigateRandom(bool blend = true)  = 0;

    virtual bool IsDebugUIVisible() const = 0;
    virtual void ShowDebugUI() = 0;
    virtual void HideDebugUI() = 0;

    virtual bool GetControlsVisible() const = 0;
    virtual void SetControlsVisible(bool visible) = 0;

    virtual void GetPresetTitle(std::string &title) const = 0;
    
    virtual void GetAboutInfo(std::string &info) const = 0;
    
    virtual void Render(int screenId, int screenCount, float dt = (1.0f / 60.0f) ) = 0;
};

using IVizControllerPtr = std::shared_ptr<IVizController>;


