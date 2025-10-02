

#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>
#include "platform.h"



struct PresetTestResult
{
    std::vector<float>  FrameTimes;
    float MinFrameTime;
    float MaxFrameTime;
    float AveFrameTime;
    float SlowPercentage;
    int     SlowFrameCount;
    int     FastFrameCount;
};

using PresetTestResultPtr = std::shared_ptr<PresetTestResult>;

class PresetGroup;
using PresetGroupPtr = std::shared_ptr<PresetGroup>;


enum class PresetRating
{
    None,
    Broken,
    Blacklist,
    Favorite
};

class PresetInfo
{
public:
    PresetInfo(std::string bundleName, std::string name, std::string presetText)
    : BundleName(bundleName), Name(name), ShortName(PathGetFileName(name)), PresetText(presetText)
    {
        
        SortKey = Name;
        for (size_t i=0; i < SortKey.size(); i++)
        {
            SortKey[i] = tolower(SortKey[i]);
        }
    }

    const std::string     BundleName;
    const std::string     ShortName;
    const std::string     Name;
    std::string           SortKey;
   // std::string             NameWithIndex;
    
    std::string           PresetText;       // text content of preset (if preloaded as text)

    PresetTestResultPtr  TestResult;        // test result if testing has been completed
    
    float           Progress = 0.0f;
    float           Rating = 0.0f;
    int             Index = 0;
    bool            Loaded = false;
    int             DisplayCount = 0;
    std::string     LoadErrors;
};

using PresetInfoPtr = std::shared_ptr<PresetInfo>;



class PresetList
{
public:
    
    int GetPosition() const {return _position;}
    
    void Add(PresetInfoPtr preset)
    {
        _list.push_back(preset);
        _itemAdded = true;
    }

    PresetInfoPtr SelectIndex(int index)
    {
        if (index < 0 || index >= (int)_list.size())
        {
            return nullptr;
        }
        
        auto preset =  _list[index];
        _position = index;
        return preset;
    }
    
    PresetInfoPtr SelectLast()
    {
        return SelectIndex((int)_list.size() - 1);
    }
    
    
    PresetInfoPtr SelectNext(bool wrap = false)
    {
        if (wrap && (_position + 1) >= (int)_list.size()) {
            return SelectIndex(0);
        }
        return SelectIndex(_position + 1);
    }

    PresetInfoPtr SelectPrevious(bool wrap = false)
    {
        if (wrap && (_position <= 0)) {
            return SelectIndex( (int)_list.size() - 1 );
        }
        return SelectIndex(_position - 1);
    }

    
    template<typename TRand>
    PresetInfoPtr SelectRandom(TRand &random_generator)
    {
        if (_list.empty())
            return nullptr;
        
        std::uniform_int_distribution<int> dist(0, (int)(_list.size() - 1) ) ;
        for (int i=0; i < 64; i++)
        {
            int preset_index = dist(random_generator);
            auto preset = SelectIndex(preset_index);
            if (preset->Rating >= 0)
            {
                return preset;
            }
        }
        
        return nullptr;

    }
    
    void Sort()
    {
        // sort them by name
        std::sort(_list.begin(), _list.end(),
              [](const PresetInfoPtr &a, const PresetInfoPtr &b) -> bool{ return a->SortKey < b->SortKey; }
          );
    }
    
    void Sort(ImGuiTableSortSpecs *specs);

    void clear()
    {
        _list.clear();
    }
    
    bool empty() const {return _list.empty();}

    
    size_t size() const {return _list.size();}
    
    const std::vector< PresetInfoPtr > &List() const
    {
        return _list;
    }
    
    bool itemAdded() {return _itemAdded;}
    void clearItemAdded() {_itemAdded = false;}
    
protected:
    int                            _position = 0;
    std::vector< PresetInfoPtr > _list;
    bool _itemAdded = false;
};

using PresetListPtr = std::shared_ptr<PresetList>;



