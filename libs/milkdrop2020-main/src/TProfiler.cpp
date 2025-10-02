
#include <map>
#include <string>

#include <vector>
#include <string>
#include <unordered_map>
#include <thread>
#include <set>
#include "platform.h"

#define USE_CHRONO 0

#ifdef WIN32
#include <Windows.h>
#include <mmsystem.h>
#endif

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

#ifdef __linux__
#include <time.h>
#endif

#ifdef EMSCRIPTEN
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include "../external/imgui/imgui_internal.h"

#include "json.h"
#include <iostream>




namespace TProfiler
{

bool g_ProfilerEnabled = false;

#if USE_CHRONO

// only chrono stuff...
using ProfilerClock = std::chrono::high_resolution_clock;

ProfilerTicks GetTime()
{
    ProfilerClock::time_point now  = ProfilerClock::now();
    ProfilerTicks ticks = now.time_since_epoch().count();
    return ticks;
}


double ToSeconds(ProfilerTicks time)
{
    return ((double)time) * 1e-9;
}

#else

ProfilerTicks  GetTime()
{
#if defined(WIN32)
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return (ProfilerTicks)count.QuadPart;
#elif defined(__APPLE__)
//    return (ProfilerTicks)mach_approximate_time();
  return (ProfilerTicks)mach_absolute_time();
#elif defined(__linux__)
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) < 0)
    {
        return 0;
    }

    // convert to nanoseconds
    ProfilerTicks ns = ((ProfilerTicks )ts.tv_sec) * 1000000000LL;
    ns += ts.tv_nsec;
    return ns;
#elif defined(EMSCRIPTEN)
    // convert to nano seconds
    return (ProfilerTicks)(emscripten_performance_now() * 1000000.0);
#else
#error Platform not supported
#endif
}

double ToSeconds(ProfilerTicks ticks)
{
#if defined(WIN32)
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return (double)ticks / (double)freq.QuadPart;
#elif defined(__APPLE__)
    static double rate = 0.0;
    if (rate == 0.0)
    {
        mach_timebase_info_data_t info;
        mach_timebase_info(&info);
        rate =  1e-9  * ((double)info.numer) / ((double)info.denom);;
    }
    return ((double)ticks) * rate;
#elif defined(__linux__)
    return ((double)ticks) * 1e-9;
#elif defined(EMSCRIPTEN)
    return ((double)ticks) * 1e-9;
#else
#error Platform not supported
#endif
}


#endif


double ToMilliSeconds(ProfilerTicks ticks)
{
    return ToSeconds(ticks) * 1000.0;
}

double ToMicroSeconds(ProfilerTicks ticks)
{
    return ToSeconds(ticks) * 1000000.0;
}



struct Category
{
    int         index;
    std::string name;
    ImU32     color;
};

using CategoryPtr = Category *;



struct Section
{
    int         index;
    std::string name;
    std::string displayName;
    CategoryPtr category = nullptr;
    bool        cpu_idle = false;
    bool        loading = false;

};

struct SectionSummary
{
    Section *section;
    int              count = 0;
    ProfilerTicks    inclusiveTime = 0;
    ProfilerTicks    exclusiveTime = 0 ;
    bool             selected = false;

};


enum class EntryPhase
{
    Begin,
    End,
    Complete,
    Instant,
    AsyncBegin,
    AsyncNext,
    AsyncEnd
};

struct ProfilerEntry
{
    EntryPhase      phase;
    SectionPtr      section;
    ProfilerTicks   start;
    ProfilerTicks   end;
    int pid;
    std::thread::id         threadId;
    uintptr_t       id;
    
    ProfilerTicks duration() const
    {
        return end - start;
    }
};

enum FrameTimeType
{
    Total,
    Active,
    Block,
    Load,
    
    MAX
};

using FrameInfoPtr = std::shared_ptr<struct FrameInfo>;
struct FrameInfo
{
    int          index;
    ProfilerTicks start;
    ProfilerTicks end;
    ProfilerTicks next;
    
    ProfilerTicks times[FrameTimeType::MAX];

    int                     main_pid;
    std::thread::id         main_tid;

    std::vector<ProfilerEntry> entries;
    
    void ReleaseMemory()
    {
        entries.clear();
        entries.shrink_to_fit();
    }
    
    size_t GetMemoryUsage()
    {
        return entries.size() * sizeof(ProfilerEntry);
    }
};


static std::mutex s_section_mutex;
static std::map<std::string, std::unique_ptr<Section> > s_sectionMap;
static std::mutex s_category_mutex;
static std::map<std::string, std::unique_ptr<Category> > s_categoryMap;

static std::map< std::thread::id, int> s_threadSectionMap;


static std::string GetDisplayName(std::string name)
{
    if (name.empty())
        return name;
    
    if (name[0] == '-' || name[0] == '+')
    {
        // obj-c
        return name;
    }

    auto index = name.find('(');
    if (index != std::string::npos)
    {
        name = name.substr(0, index);
    }
    
    index = name.rfind(' ');
    if (index != std::string::npos)
    {
        name = name.substr(index);
    }

    // trim left
    for (size_t i=0; i < name.size(); i++)
    {
        if (!std::isspace(name[i])) {
            if (i > 0)
                name = name.substr(i);
            break;
        }
    }

    return name;
    
}


CategoryPtr ResolveCategory(const char *name)
{
    if (!name)
        return nullptr;
    
    std::lock_guard<std::mutex> lock(s_category_mutex);
    CategoryPtr category;
    
    auto it = s_categoryMap.find(name);
    if (it == s_categoryMap.end())
    {
        category = new Category();
        category->index = (int)s_categoryMap.size();
        category->name = name;
        category->color = IM_COL32(128, 128, 128, 255);
        s_categoryMap[category->name] = std::unique_ptr<Category>(category);
    }
    else
    {
        category = it->second.get();
        
    }
    
    return category;
}

void ProfilerSetCategoryColor(const char *name, ImU32 color)
{
    auto cat = ResolveCategory(name);
    cat->color = color;
}




SectionPtr GetSection(const char *name)
{
    std::lock_guard<std::mutex> lock(s_section_mutex);
    auto it = s_sectionMap.find(name);
    if (it == s_sectionMap.end())
        return nullptr;
    return it->second.get();
}


SectionPtr ResolveSection(SectionPtr *ppsection, const char *name, const char *categories)
{
    if (*ppsection) return *ppsection;

    std::lock_guard<std::mutex> lock(s_section_mutex);
    SectionPtr section;
    
    auto it = s_sectionMap.find(name);
    if (it == s_sectionMap.end())
    {
        section = new Section();
        section->index = (int)s_sectionMap.size();
        section->name = name;
        section->displayName = GetDisplayName(name);
//        LogPrint("%s => %s\n", section->name.c_str(),  section->displayName.c_str() );
        s_sectionMap[section->name] = std::unique_ptr<Section>(section);
    }
    else
    {
        section = it->second.get();
        
    }
    
    if (categories && !section->category) {
        section->category = ResolveCategory(categories);
        
        if (section->category->name == "load") {
            section->loading = true;
        }
    }
    
    *ppsection = section;
    return section;
}



struct ProfilerNode;
using ProfilerNodePtr =  std::shared_ptr<ProfilerNode>;
using ProfilerNodeWeakPtr =  std::weak_ptr<ProfilerNode>;


struct ProfilerNode
{
    int level               = 0;
    ProfilerNode *                  root  = nullptr;
    ProfilerNode *                  parent = nullptr;
    ProfilerNodePtr                  firstChild;
    ProfilerNodePtr                  nextSibling;
    
    SectionPtr            section;
    ProfilerTicks                start;
    ProfilerTicks                 end;
    ProfilerTicks                       duration  = 0;
    ProfilerTicks                       inclusiveTime = 0;
    ProfilerTicks                       exclusiveTime = 0;
    
    std::thread::id             threadId;

    bool Contains(const ProfilerEntry *other)
    {
        return (start <= other->start) && (end >= other->end);
    }

    bool Contains(const ProfilerNode *other)
    {
        return (start <= other->start) && (end >= other->end);
    }

    bool Contains(ProfilerTicks time)
    {
        return (time >= start) && (time <= end);
    }
    
    
    ProfilerTicks GetDuration()
    {
        return end - start;
    }
    
    ProfilerTicks GetChildTime()
    {
        ProfilerTicks time  = 0;
        for (auto node = firstChild; node; node = node->nextSibling)
        {
            time += node->duration;
        }
        return time;
    }
    


};
    

class ProfilerReport
{

    std::vector<ProfilerNodePtr>            m_rootNodes;
    
    std::vector<SectionSummary> m_sections;

    std::vector<float> m_categories;

     
public:
    
    int startFrame = -1;
    int endFrame = -1;
    ProfilerReport()
    {
    }
    
    const std::vector<SectionSummary>&GetSectionSummaries() {return m_sections;}

    void DrawSummary()
    {
        ImGui::BeginChild("#Profiler#Summary", ImVec2(0, 0), true,
                          //ImGuiWindowFlags_VerticalScrollbar |
                          ImGuiWindowFlags_AlwaysVerticalScrollbar);

        ImGui::BeginColumns("##summary", 5, ImGuiColumnsFlags_NoPreserveWidths);

//        ImGui::SetColumnWidth(0, 600);
        
        float w = ImGui::GetWindowWidth();

//                ImGui::SetColumnWidth(0, w - (80 - 150 - 150));
//        ImGui::SetColumnWidth(0, -1);
//
        
        ImGui::SetColumnWidth(0, w - 60 - 150 - 150 - 140);
        ImGui::SetColumnWidth(1, 60);
        ImGui::SetColumnWidth(2, 150);
        ImGui::SetColumnWidth(3, 150);
        ImGui::SetColumnWidth(4, 140);

//        ImGui::NextColumn();
//        ImGui::Text("Section");
        ImGui::Text("Frame #%d -> #%d", startFrame, endFrame);

        ImGui::NextColumn();
        ImGui::Text("Count");
        ImGui::NextColumn();
        ImGui::Text("Exclusive");
        ImGui::NextColumn();
        ImGui::Text("Inclusive");
        ImGui::NextColumn();
        ImGui::Text("Category");
        ImGui::NextColumn();
        ImGui::Separator();

//        ImGui::Separator();
//         ImGui::NewLine();

        for (auto ss : m_sections)
        {
            if (ss.count == 0)
                continue;
            
            auto section = ss.section;
            if (ImGui::Selectable(section->name.c_str(), ss.selected, ImGuiSelectableFlags_SpanAllColumns))
               ss.selected = true;
            else
                ss.selected = false;
            ImGui::NextColumn();
            ImGui::Text("%d", ss.count);
            ImGui::NextColumn();
            
            
            double percentage = 100.0 * (double)ss.exclusiveTime / (double)m_summaryTotalTime;
            ImGui::Text("%8.02f (%6.2f%%)", ToMilliSeconds(ss.exclusiveTime), percentage );
            ImGui::NextColumn();

            double percentage2 = 100.0 * (double)ss.inclusiveTime / (double)m_summaryTotalTime;
            ImGui::Text("%8.02f (%6.2f%%)", ToMilliSeconds(ss.inclusiveTime), percentage2 );
            ImGui::NextColumn();

            
            auto cat = section->category;
            if (cat)
            ImGui::Text("%s", cat->name.c_str());
            ImGui::NextColumn();

            
//            ImGui::NewLine();

        }

              
         ImGui::EndColumns();
        
        ImGui::EndChild();
    }

    void DrawTree()
    {
        ImGui::BeginChild("#Profiler#DrawTree", ImVec2(0, 0), true,
                          //ImGuiWindowFlags_VerticalScrollbar |
                          ImGuiWindowFlags_AlwaysVerticalScrollbar);

     //   if (!m_rootNodes.empty())
        {
//            ImGui::Columns(3, "tree", true);
            ImGui::BeginColumns("##profiler#tree", 4, ImGuiColumnsFlags_NoPreserveWidths);


//            ImGui::SetColumnWidth(0, 600);
            
            float w = ImGui::GetWindowWidth();


            ImGui::SetColumnWidth(0, w - 150 - 150 - 140);
              ImGui::SetColumnWidth(1, 150);
              ImGui::SetColumnWidth(2, 150);
              ImGui::SetColumnWidth(3, 140);

            
            ImGui::Text("Frame #%d -> #%d", startFrame, endFrame);
            ImGui::NextColumn();
            ImGui::Text("Exc");
            ImGui::NextColumn();
            ImGui::Text("Inc");
            ImGui::NextColumn();
            ImGui::Text("Category");
            ImGui::NextColumn();
            ImGui::Separator();

            for (auto node : m_rootNodes)
            {
                OnGUI(node);
            }
            ImGui::EndColumns();
        }
        
        ImGui::EndChild();

    }
    
    ProfilerTicks m_summaryTotalTime=0;
    void ResetSummary()
    {
        m_rootNodes.clear();

        m_summaryTotalTime = 0;
    }
    
    void UpdateSummary(ProfilerNodePtr node)
    {
        auto section_summary = &m_sections[node->section->index];
        section_summary->count++;
        section_summary->inclusiveTime += node->inclusiveTime;
        section_summary->exclusiveTime += node->exclusiveTime;
        
        ProfilerNodePtr child = node->firstChild;
        while (child)
        {
            UpdateSummary(child);
            child = child->nextSibling;
        }
    }
    
    void UpdateSummary()
    {
        
        {
            std::lock_guard<std::mutex> lock(s_category_mutex);
            m_categories.resize( s_categoryMap.size() );
        }

        {
            std::lock_guard<std::mutex> lock(s_section_mutex);
            m_sections.clear();
            m_sections.resize( s_sectionMap.size() );
            for (const auto &it : s_sectionMap)
            {
                m_sections[it.second->index].section = it.second.get();
            }
        }

        
        for(auto node : m_rootNodes)
        {
            UpdateSummary(node);
        }
    }
        
    void SortSummary()
    {

        std::sort(m_sections.begin(), m_sections.end(), [](const SectionSummary &a, const SectionSummary &b) -> bool{
//            return b.inclusiveTime < a.inclusiveTime;
            return b.exclusiveTime < a.exclusiveTime;

        }  );
    }
    
    void AddChild(ProfilerNode * parent, ProfilerNodePtr child)
    {
        child->level = parent->level + 1;
        child->parent = parent;
        child->nextSibling = parent->firstChild;
        parent->firstChild = child;
    }

    
    
    void BuildTree(FrameInfoPtr frame)
    {
  
        m_summaryTotalTime += frame->next - frame->start;
        
        
        std::set< std::thread::id > threads;
        for (const auto &entry : frame->entries)
        {
            threads.insert(entry.threadId);
        }
        
        for (const auto &tid : threads)
        {
            ProfilerNode * top = nullptr;
            
//            ProfilerNodePtr root;
//             auto tid = std::this_thread::get_id();
            
            
            SectionPtr section = nullptr;
            ResolveSection(&section, "thread");
            
            section->cpu_idle = true;

            ProfilerNodePtr root =  std::make_shared<ProfilerNode>();
            root->threadId = tid;
            root->section = section;
            root->start = frame->start;
            root->end   = frame->next;
            root->duration = root->GetDuration();
            root->inclusiveTime = root->duration;
            root->exclusiveTime = root->duration;
            root->root = root.get();
            root->parent = nullptr;

            m_rootNodes.push_back(root);

            top = root.get();
            

            
            
            for (auto it = frame->entries.rbegin(); it != frame->entries.rend(); ++it)
            {
                const ProfilerEntry *entry = &*it;
                if (entry->pid != frame->main_pid || entry->threadId != tid)
                    continue;
                
                if (entry->phase != EntryPhase::Complete)
                    continue;
                
                ProfilerNodePtr node =  std::make_shared<ProfilerNode>();
                node->threadId = tid;
                node->section = entry->section;
                node->start = entry->start;
                node->end   = entry->end;
                node->duration = node->GetDuration();
                node->inclusiveTime = node->duration;
                node->exclusiveTime = node->duration;
                
                while (top && !top->Contains(entry))
                {
                    top = top->parent;
                }
                
                if (top)
                {
                    AddChild(top, node);
                }
                else
                {
                    root = node;
                }
                
                top = node.get();
                node->root = top;
                
                if (node->parent)
                {
                    // update exclusive time
                    node->parent->exclusiveTime -= node->duration;
                }
                
            }
        }
        

    }
    
    void OnGUI(ProfilerNodePtr node)
    {
//        ImDrawList* draw_list = ImGui::GetWindowDrawList();

//        const ImFont* font = draw_list->_Data->Font;
//        const float font_size = draw_list->_Data->FontSize;
        

        auto root = node->root;

        bool open = ImGui::TreeNodeEx(node.get(), node->firstChild ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_Leaf, "%s", node->section->displayName.c_str() );
        ImGui::NextColumn();

        double percentage = 100.0 * (double)node->exclusiveTime / (double)root->duration;
        ImGui::Text("%8.02f (%6.2f%%)", ToMilliSeconds(node->exclusiveTime), percentage );
        ImGui::NextColumn();

        double percentage2 = 100.0 * (double)node->inclusiveTime / (double)root->duration;
        ImGui::Text("%8.02f (%6.2f%%)", ToMilliSeconds(node->inclusiveTime), percentage2 );
        ImGui::NextColumn();

        
        auto cat =  node->section->category;
        if (cat)
            ImGui::Text("%s", cat->name.c_str());

        ImGui::NextColumn();

        if (open)
        {
            ProfilerNodePtr child = node->firstChild;
            while (child)
            {
                OnGUI(child);
                child = child->nextSibling;
            }
            
            ImGui::TreePop();
        }

    }


    
};




class ProfilerLog
{
    std::mutex         m_mutex;
    std::vector<ProfilerEntry> m_entries;
    ProfilerReport m_report;
    
    std::vector<std::string> m_captureLocations;

    std::vector<FrameInfoPtr> m_frames;

    
    bool m_recording = true;
    
    
    bool m_skipFrame = false;
    bool m_captureNextFrame = false;
    ProfilerTicks m_timeBeginLog;
    ProfilerTicks m_timeBeginFrame;
    ProfilerTicks m_timeEndFrame;
    
    SectionPtr m_section_frame = nullptr;
    SectionPtr m_section_idle = nullptr;
    SectionPtr m_section_gpu_wait = nullptr;
    SectionPtr m_section_vsync = nullptr;

    std::map< std::thread::id, int> m_section_threads;



    int m_main_pid = 1;
    int m_gpu_pid = 2;
    std::thread::id m_main_tid = std::this_thread::get_id();
    
public:
    ProfilerLog()
    {
        m_entries.reserve(32 * 1024);
        m_frames.reserve(1024);
        
        m_timeBeginLog = GetTime();
        m_timeBeginFrame = m_timeBeginLog;
        m_timeEndFrame = m_timeBeginLog;
    }

    
    
    int GetThreadID(std::thread::id id)
    {
        auto it = m_section_threads.find(id);
        if (it != m_section_threads.end()) {
            return it->second;
        }
        
        int index = (int)(m_section_threads.size() + 1);
        m_section_threads[id] = index;
        return index;
    }
    
  
    template<typename T>
    void WriteMetaData(JsonWriter &writer, int pid, int tid, const char *name, const char *arg_name, T arg_value)
    {
        writer.StartObject();
        writer.Write("name", name);
        writer.Write("ph", "M");
        writer.Write("pid", pid);
        writer.Write("tid", tid);

        writer.Key("args");
        writer.StartObject();
        writer.Write(arg_name, arg_value);
        writer.EndObject();
        
        writer.EndObject();
    }
    
//    template<typename T>
    void WriteJson(JsonWriter &writer)
    {
        writer.StartObject();

        writer.Write("displayTimeUnit", "ms");

        writer.Key("otherData");
        writer.StartObject();
        writer.Write("platform", PlatformGetPlatformName());
        writer.Write("config", PlatformGetBuildConfig());
        writer.Write("version", PlatformGetAppVersion());
        writer.EndObject();

        
        writer.Key("traceEvents");
        writer.StartArray();
        
        
        // process 1
        
        int main_tid = GetThreadID(m_main_tid);
        
        WriteMetaData(writer, m_main_pid, main_tid, "process_name", "name", PlatformGetAppName());
        WriteMetaData(writer, m_main_pid, main_tid, "process_sort_index", "sort_index", m_main_pid);
        WriteMetaData(writer, m_main_pid, main_tid, "thread_name", "name", "main_thread");

        WriteMetaData(writer, m_gpu_pid, main_tid, "process_name", "name", "GPU");
        WriteMetaData(writer, m_gpu_pid, main_tid, "process_sort_index", "sort_index", m_gpu_pid);
        WriteMetaData(writer, m_gpu_pid, main_tid, "thread_name", "name", "gpu_thread");

        
        
        
        for (auto frame : m_frames)
        {
            for (const auto &entry : frame->entries)
            {
                bool idle=  entry.section->cpu_idle;
                if (entry.pid == m_gpu_pid)
                    idle = true;

                writer.StartObject();
                
                writer.Write("name", entry.section->displayName);

                double timeStart = ToMicroSeconds(entry.start - m_timeBeginLog);
                writer.Write("ts", timeStart);

                writer.Write("pid", entry.pid);


                switch (entry.phase)
                {
                    case EntryPhase::Instant:
                    case EntryPhase::Complete:
                    case EntryPhase::Begin:
                    case EntryPhase::End:
                        writer.Write("tid", GetThreadID(entry.threadId) );
                        break;
                        
                    default:
                        break;
                }
                
                
                switch (entry.phase)
                {
                    case EntryPhase::Instant:
                    {
                        writer.Write("ph", "I");
                        writer.Write("s", "t");
                        break;
                    }
                    
                    case EntryPhase::Complete:
                    {
                        double timeDuration = ToMicroSeconds(entry.end - entry.start);

                        writer.Write("ph", "X");
                        writer.Write("dur", timeDuration);
                        
                        writer.Write("tts", timeStart);
                        if (idle)
                        {
                            writer.Write("tdur", 0.0);
                        }
                        else
                        {
                            writer.Write("tdur", timeDuration);

                        }
                        break;
                    }

                    case EntryPhase::Begin:
                    {
                        writer.Write("ph", "B");
                        break;
                    }

                    case EntryPhase::End:
                    {
                        writer.Write("ph", "E");
                        break;
                    }
                        
                    case EntryPhase::AsyncBegin:
                    {
                        writer.Write("ph", "b");
                        writer.Write("id", (uint64_t)entry.id);
                        break;
                    }

                    case EntryPhase::AsyncNext:
                    {
                        writer.Write("ph", "n");
                        writer.Write("id", (uint64_t)entry.id);
                        break;
                    }

                    case EntryPhase::AsyncEnd:
                    {
                        writer.Write("ph", "e");
                        writer.Write("id", (uint64_t)entry.id);
                        break;
                    }
                        
                    default:
                        assert(0);
                        break;

                }
                
                if (entry.section->category) {
                    writer.Write("cat", entry.section->category->name);
                }

                writer.EndObject();
            }
        }
        
        writer.EndArray();
        
        writer.EndObject();
    }
    
    std::string ToJsonString()
    {
        using namespace rapidjson;
        StringBuffer s;
        PrettyWriter<StringBuffer> writer(s);
        
        JsonWriter w(writer);
        
        WriteJson(w);
        
        std::string json(s.GetString());
        return json;
    }
    
    /*
    bool WriteToFile(const std::string &path)
    {
        FILE *file = fopen(path.c_str(), "wt");
        if (!file)
            return false;
        using namespace rapidjson;
        char buffer[0x10000];
        FileWriteStream os(file, buffer, sizeof(buffer));
        PrettyWriter<FileWriteStream> writer(os);
        WriteJson(writer);
        
        os.Flush();
        fclose(file);
        return true;
    }
     */
    

    void AddInstantEntry(SectionPtr section, ProfilerTicks time)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        uintptr_t id = 0;

        std::thread::id thread_id = std::this_thread::get_id();
        m_entries.emplace_back(ProfilerEntry{EntryPhase::Instant, section, time, time, m_main_pid, thread_id, id});
    }

    void AddEntry(SectionPtr section, ProfilerTicks start, ProfilerTicks end)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        uintptr_t id = 0;

        std::thread::id thread_id = std::this_thread::get_id();
        m_entries.emplace_back(ProfilerEntry{EntryPhase::Complete, section, start, end, m_main_pid, thread_id, id});
    }
    
    void AddGPUEntry(SectionPtr section, ProfilerTicks start, ProfilerTicks end)
    {
      std::lock_guard<std::mutex> lock(m_mutex);
        uintptr_t id = 0;
        
      m_entries.emplace_back(ProfilerEntry{EntryPhase::Complete, section, start, end, m_gpu_pid, m_main_tid, id});
    }

    void AddAsyncBegin(SectionPtr section, ProfilerTicks time, uintptr_t id)
    {
//        std::lock_guard<std::mutex> lock(m_mutex);
//        m_entries.emplace_back(ProfilerEntry{EntryPhase::AsyncBegin, section, time, time, m_main_pid, m_main_tid, id});
    }

        void AddAsyncNext(SectionPtr section, ProfilerTicks time, uintptr_t id)
        {
//            std::lock_guard<std::mutex> lock(m_mutex);
//            m_entries.emplace_back(ProfilerEntry{EntryPhase::AsyncNext, section, time, time, m_main_pid, m_main_tid, id});
        }

    void AddAsyncEnd(SectionPtr section, ProfilerTicks time, uintptr_t id)
    {
//        std::lock_guard<std::mutex> lock(m_mutex);
//        m_entries.emplace_back(ProfilerEntry{EntryPhase::AsyncEnd,  section, time, time, m_main_pid, m_main_tid, id});
    }


    void SkipFrame()
    {
        m_skipFrame = true;
    }
    
    void RecordFrame()
    {
        ProfilerSetCategoryColor("idle", IM_COL32(64, 64, 64, 255));
        ProfilerSetCategoryColor("gpu", IM_COL32(89, 255, 0, 255));
        ProfilerSetCategoryColor("wait", IM_COL32(64, 64, 0, 255));
        ProfilerSetCategoryColor("top", IM_COL32(89, 143, 180, 255));

        
        ResolveSection( &m_section_idle, ".idle", "idle");
        ResolveSection( &m_section_frame, ".frame", "top");
        ResolveSection( &m_section_gpu_wait, ".gpu.wait", "gpu");
        
        m_section_idle->cpu_idle = true;
        m_section_frame->cpu_idle = true;
        m_section_gpu_wait->cpu_idle = true;

        
        FrameInfoPtr info = std::make_shared<FrameInfo>();
        info->index = (int)m_frames.size();
        info->start=  m_timeBeginFrame;
        info->end = m_timeEndFrame;
        info->next = m_timeEndFrame;
        for (int i=0; i < FrameTimeType::MAX; i++)
            info->times[i] = 0;
        info->main_pid = m_main_pid;
        info->main_tid = m_main_tid;

        for (auto entry : m_entries)
        {
            if (entry.threadId != info->main_tid || entry.pid != info->main_pid)
                continue;
            
            if (entry.phase != EntryPhase::Complete)
                continue;
            
            if (entry.section == m_section_frame) {
                info->start=  entry.start;
                info->next = entry.end;
            }
            else
            if (entry.section->cpu_idle) {
                info->times[FrameTimeType::Block] += entry.duration();
            }
            else
            if (entry.section->loading) {
                info->times[FrameTimeType::Load] += entry.duration();
            }


        }
        
        info->times[FrameTimeType::Active] = info->end - info->start;
        info->times[FrameTimeType::Total] = info->next - info->start;
        info->times[FrameTimeType::Active] -= info->times[FrameTimeType::Block] + info->times[FrameTimeType::Load];

        
        info->entries = m_entries;
        
        m_frames.push_back(info);
        
        m_memoryUsage += (int)info->GetMemoryUsage();
        
        
        
        // prune frames to free memory
        auto it = m_frames.begin();
        while (it != m_frames.end())
        {
            if (m_memoryUsage < m_maxMemoryUsage)
                break;
            
            FrameInfoPtr frame = *it;
            if ( frame->GetMemoryUsage() > 0 )
            {
                m_memoryUsage -= (int)frame->GetMemoryUsage();
                frame->ReleaseMemory();
            }
            ++it;
        }
        
        
    }
    
    int m_maxMemoryUsage = 100 * 1000 * 1000;
    int m_memoryUsage = 0;
  
    
    std::string GetUniqueFileName()
    {
        std::string name;
        
        name = PlatformGetAppName();
        name += "-";
        name += PlatformGetPlatformName();
        if (PlatformIsDebug()) {
            name += "-debug";
        }
        name += ".";
        name += PlatformGetTimeString();
        name += ".trace.json";
        return name;

    }
    
    bool CaptureToJsonString(std::string &json)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        json = ToJsonString();
        return true;
    }
    
    
    void CaptureAndSend()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::string json = ToJsonString();
        std::string name = GetUniqueFileName();
        
        
        for (auto location : m_captureLocations)
        {
            std::string url = PathCombine(location, name);
            if (url.find("http:") == 0)
            {
                HttpSend("PUT", url, json.c_str(), json.size(), true, "application/json", nullptr);
            } else
            {
                DirectoryCreate(location);
                if (FileWriteAllText(url, json))
                {
                    LogPrint("Profile written to file %s\n", url.c_str());
                }
            }

        }
    }
    
    void AddCaptureLocation(std::string location)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_captureLocations.push_back(location);
    }

    void CaptureNextFrame()
    {
        m_captureNextFrame = true;
    }
    
    void CaptureToFile(const std::string &path)
   {
       std::lock_guard<std::mutex> lock(m_mutex);

       std::string json = ToJsonString();
       if (FileWriteAllText(path, json))
       {
           LogPrint("Profile written to file %s\n", path.c_str());

       }
   }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        m_frames.clear();
    }
    

    
    void NextFrame()
    {
        ProfilerTicks time = GetTime();;

        m_timeEndFrame  = time;
        
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            
            auto thread = std::this_thread::get_id();
            
            for (auto it = m_entries.rbegin(); it != m_entries.rend(); it++)
//            if (!m_entries.empty())
            {
                if (it->pid == m_main_pid && it->threadId == thread )
                {
                    m_timeEndFrame = it->end;
                    break;
                }
            }
        }
        
        ResolveSection( &m_section_idle, ".idle", "idle");
       // AddEntry(m_section_idle, m_timeEndFrame, time);

        ResolveSection( &m_section_frame, ".frame", "top");
//        AddEntry(m_section_frame, m_timeBeginFrame, time);
        
        ResolveSection( &m_section_vsync, "vblank");
        AddInstantEntry(m_section_vsync, time);

        {
            std::lock_guard<std::mutex> lock(m_mutex);




            if (m_recording && !m_skipFrame)
            {
                RecordFrame();
            }

            m_timeBeginFrame = time;
            m_skipFrame = false;


            // add to frame entries
            m_entries.clear();
        }
        
        if (m_captureNextFrame)
        {
            CaptureAndSend();
            m_captureNextFrame = false;
        }

    }

    void Dump()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        for (const auto &entry : m_entries)
        {
            double seconds = ToMicroSeconds(entry.end -  entry.start);
            LogPrint("%s %f\n", entry.section->name.c_str(), seconds);
        }


    }
    
    
    
    ImU32 color_bg = IM_COL32(32, 32, 32, 255);
    ImU32 color_idle = IM_COL32(64, 64, 64, 255);
    ImU32 color_active = IM_COL32(89, 143, 180, 255);
    ImU32 color_gpu = IM_COL32(89, 255, 0, 255);
    ImU32 color_load = IM_COL32(255, 89, 0, 255);
    ImU32 color_line = IM_COL32(255, 0, 0, 128);
    ImU32 color_white = IM_COL32(255, 255, 255, 255);

    ImU32 colors[FrameTimeType::MAX] = { color_idle, color_active, color_gpu, color_load};

    int m_frameScrollTo = -1;
    bool m_frameScrollToEnd = true;
    int m_frameViewWidth = 0;
    int m_frameViewStart = 0;
    int m_frameViewEnd = 0;
    int m_frameSelectionStart = -1;
    int m_frameSelectionEnd = -1;
    int m_mouseDragStart = -1;

    
    void DrawStackedBar(ImDrawList* draw_list, ImVec2 pos, ImVec2 size, int count, const ProfilerTicks *times, const ImU32 *colors)
    {
        float x = pos.x;
        float y0 = pos.y;
        
        for (int i=0; i < count; i++)
        {
            float height = (float)(ToMilliSeconds(times[i]));

            float y1 = y0 - height * size.y;
            draw_list->AddRectFilled(ImVec2(x, y0), ImVec2(x + size.x, y1), colors[i] );
            
            if (i != 0)
                y0 = y1;
        }
    }
    
    
    void DrawFramesSummary()
    {
        float bw = 1;
        float spacing = 0;
        float bh = 32;
        float scaley = 1.0f;
        float dx = bw + spacing;
        float dy = bh * scaley;


        ImGui::SetNextWindowContentSize(ImVec2(dx * m_frames.size(), dy));
        ImGui::BeginChild("#Profiler#DrawFramesSummary", ImVec2(0, dy+30), true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 size = ImGui::GetContentRegionAvail();
        float width = ImGui::GetWindowWidth();

        
//        float x = pos.x;
        float y = pos.y;

        

        float maxw = size.x;
        float maxh = bh  * scaley;
        
        

        int start = (int)std::floor(ImGui::GetScrollX() / dx);
        if (start < 0)
        {
            start = 0;
        }
        
        size.y  = bh  * scaley;
        
        draw_list->PushClipRect(pos, pos + size, true);
        draw_list->AddRectFilled(pos, pos + size, color_bg );

        y += maxh;
        
        size_t numFrames = m_frames.size();
        int frameWidth = (int)(width / dx);
                
        float x = pos.x;
        
        x += start * dx;

        for (int i= 0; i < frameWidth; i++)
        {
            int frame_index = start + i;
            if (frame_index >= numFrames)
                break;

            FrameInfoPtr info = GetFrameInfo(frame_index);
            if (!info)
                continue;
            
            DrawStackedBar(draw_list, ImVec2(x, y), ImVec2(bw, scaley), FrameTimeType::MAX, info->times, colors );
            x += dx;
        }
        
        
        
        {
            float x = pos.x;
            float y = pos.y + maxh;

            for (float ly = 0; ly < maxh; ly += 16.66f )
            {
                draw_list->AddLine(ImVec2(x, y  - ly * scaley),
                                   ImVec2(x + maxw, y - ly * scaley),
                                   color_line, 1);

            }
        }
        

        float y0 = y;
        float y1 = y - maxh;
        float x0 = ((float)m_frameViewStart - start) * dx;
        float x1 = ((float)m_frameViewEnd - start) * dx;
        draw_list->AddRectFilled(ImVec2(pos.x + x0, y0),
                                 ImVec2(pos.x + x1, y1),
                                 IM_COL32(255, 255, 255, 32) );

        draw_list->AddRect(ImVec2(pos.x + x0, y0 - 1),
                                 ImVec2(pos.x + x1, y1 + 1 ),
                                 IM_COL32(255, 255, 255, 190), 0 );

        
        if (m_frameSelectionStart  > 0)
        {
            float x0 = pos.x + (m_frameSelectionStart - start)* dx;
            draw_list->AddLine(
                               ImVec2(x0, y0),
                               ImVec2(x0, y1),
                               color_white,
                               1);
        }
        
        if (m_frameSelectionEnd > 0)
        {
            float x0 = pos.x + (m_frameSelectionEnd - start)* dx;
            draw_list->AddLine(
                               ImVec2(x0, y0),
                               ImVec2(x0, y1),
                               color_white,
                               1);
        }
        
        draw_list->PopClipRect();

        
        ImGui::InvisibleButton("frames_preview", ImVec2(maxw, maxh));
        
        
        if (ImGui::IsItemHovered())
        {
            ImVec2 mouse_pos = ImGui::GetIO().MousePos  - pos;
            int mouse_frame = (int)std::floor((mouse_pos.x / dx)) + start;

            if (ImGui::IsMouseDown(0))
            {
                if (GetFrameInfo(mouse_frame))
                {
                    m_frameScrollTo  = mouse_frame;
                    m_frameScrollToEnd = false;
                }
                else
                {
                    m_frameScrollToEnd = true;
                }
                
            }
            
        }
        
        
        ImGui::EndChild();

        
        
    }
    
    FrameInfoPtr GetFrameInfo(int index)
    {
        if (index < 0 || index >= m_frames.size())
            return nullptr;;
        
        return m_frames[index];
    }
    
    void SelectFrame(int index)
    {
        SelectFrameRange(index, index);

    }

    void SelectFrameRange(int start, int end)
    {
        if (start > end) std::swap(start, end);
        
        m_frameSelectionStart = start;
        m_frameSelectionEnd = end;
        
        
    }

    
    
    void DrawFrames()
    {
        float bw = 6;
        float spacing = 1.0f;
        float bh = 64;
        float scaley = 1.5f;
        float dx = bw + spacing;
        float dy = bh * scaley;

        
        
        ImGui::SetNextWindowContentSize(ImVec2(dx * m_frames.size(), dy));
        ImGui::BeginChild("#Profiler#DrawFrames", ImVec2(0, dy + 30), true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);

        //CalcWindowExpectedSize
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 size = ImGui::GetContentRegionAvail();
        
        float width = ImGui::GetWindowWidth();

        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        float x = pos.x;
        float y = pos.y;
            

        // number of visible frames
        m_frameViewWidth = (int)(width / dx);
        
        float maxw = size.x;
        float maxh = bh  * scaley;
        
        size.y  = bh  * scaley;
        
        draw_list->PushClipRect(pos, pos + size, true);
        draw_list->AddRectFilled(pos, pos + size, color_bg );

        y += maxh;
        
        if (m_frameScrollToEnd)
        {
            // stick to end
            float scrollTo = (dx * m_frames.size() -  m_frameViewWidth);
            if (scrollTo < 0) scrollTo = 0;
            ImGui::SetScrollX(scrollTo);
        }
        else if (m_frameScrollTo >= 0)
        {
            float frameScrollTo = (float)(m_frameScrollTo - m_frameViewWidth / 2);
            if (frameScrollTo < 0) frameScrollTo = 0;
            ImGui::SetScrollX(frameScrollTo * dx);
            m_frameScrollTo = -1;
        }
        

        int start = (int)std::floor(ImGui::GetScrollX() / dx);
        if (start < 0)
        {
            start = 0;
        }
        

        
        m_frameViewStart = start;
        m_frameViewEnd = start + m_frameViewWidth;
        
        x+= start * dx;

                
        for (int i= 0; i < (int)m_frameViewWidth; i++)
        {
            int frameIndex = start + i;
            if (frameIndex >= m_frames.size())
                break;
            
            FrameInfoPtr info = GetFrameInfo(frameIndex);
            if (!info)
                continue;
            
            
            DrawStackedBar(draw_list, ImVec2(x, y), ImVec2(bw, scaley), FrameTimeType::MAX, info->times, colors );
    
            x += dx;
        }
        
        float fx0 = pos.x + ((float)(m_frameSelectionStart )) * dx;
        float fx1 = pos.x + ((float)(m_frameSelectionEnd + 1 )) * dx;
        
        float y0 = y;
        float y1 = y - maxh;
        draw_list->AddRectFilled(ImVec2(fx0, y),
                                 ImVec2(fx1, y - maxh),
                                 IM_COL32(255, 255, 255, 32) );


        
           if (m_frameSelectionStart  >= 0)
           {
               draw_list->AddLine(
                                  ImVec2(fx0, y0),
                                  ImVec2(fx0, y1),
                                  color_white,
                                  1);
           }
           
           if (m_frameSelectionEnd >= 0)
           {
               draw_list->AddLine(
                                  ImVec2(fx1, y0),
                                  ImVec2(fx1, y1),
                                  color_white,
                                  1);
           }
        
     
        {
            float x = pos.x;
            float y = pos.y + maxh;

            for (float ly = 0; ly < maxh; ly += 16.66f )
            {
                draw_list->AddLine(ImVec2(x, y  - ly * scaley),
                                   ImVec2(x + maxw, y - ly * scaley),
                                   color_line, 1);

            }
        }
        
        const char *label = "Frame Time";
        draw_list->AddText(ImVec2(pos.x + 8, pos.y + 32), color_white, label, label + strlen(label));
//
//                    char str[256];
//                    sprintf(str, "Frame #%d ", m_frameSelectionStart);
//        draw_list->AddText(ImVec2(pos.x + 8, pos.y + 8), color_white, str, str + strlen(str));
//

        /*
        if (start < m_frames.size())
        {
            const FrameInfo *first_info = m_frames[start];
//            double time = GetLength(m_timeBeginLog, first_info->start);
//
//            char str[256];
//            sprintf(str, "%04.3f", time);
                         char str[256];
                         sprintf(str, "Frame ", time);
             draw_list->AddText(ImVec2(pos.x + 8, pos.y + 8), color_white, str, str + strlen(str));
            
        }
         */
        
        draw_list->PopClipRect();
        

        
        ImGui::InvisibleButton("frames", ImVec2(maxw, maxh));
        
        if (ImGui::IsItemHovered())
        {
            
//            ImVec2 mouse_pos = ImVec2(ImGui::GetIO().MousePos.x - pos.x, ImGui::GetIO().MousePos.y - pos.y);
            ImVec2 mouse_pos = ImGui::GetIO().MousePos  - pos;

            int mouse_frame = (int)std::floor((mouse_pos.x / dx)) ; //+ start;
            if (GetFrameInfo(mouse_frame))
            {
                if (ImGui::IsMouseDown(0))
                {
                    if (ImGui::IsMouseDragPastThreshold(0, 0.1f))
                    {
                        SelectFrameRange(m_mouseDragStart, mouse_frame);
                        
                        
                    }
                    else
                    {
                        m_mouseDragStart = mouse_frame;
//                        m_frameScrollTo  = start;
                        SelectFrame(mouse_frame);
                    }
                    
                  //  m_frameScrollTo  = mouse_frame + m_frameViewWidth / 2;
                    m_frameScrollToEnd = false;
                    
                    
                }
            }
         
            
        }
        
        ImGui::EndChild();

        
    }
    
    void UpdateReport()
    {
        if (m_report.startFrame == m_frameSelectionStart &&
            m_report.endFrame == m_frameSelectionEnd)
        {
            return;
        }
        

        m_report.ResetSummary();

        for (int i=m_frameSelectionStart; i <= m_frameSelectionEnd; i++)
        {
            FrameInfoPtr info = GetFrameInfo(i);
            if (info)
            {
                m_report.BuildTree(info);
            }
        }

        m_report.UpdateSummary();

        m_report.SortSummary();
        m_report.startFrame = m_frameSelectionStart;
        m_report.endFrame = m_frameSelectionEnd;


    }
    
    
    void DrawToolbar()
    {
        
        if (ImGui::Button("Record"))
        {
            m_recording = !m_recording;
        }
        ImGui::SameLine();

        
        if (ImGui::Button("Save"))
        {
            CaptureNextFrame();
        }
        
        ImGui::SameLine();

        if (ImGui::ButtonEx("Load", ImVec2(0,0), ImGuiButtonFlags_Disabled))
        {
            //Capture();
        }
        ImGui::SameLine();

        
        
        if (ImGui::Button("Clear"))
        {
            Clear();
        }
        ImGui::SameLine();

        if (ImGui::Checkbox("Enabled", &g_ProfilerEnabled))
        {
//            g_ProfilerEnabled = !g_ProfilerEnabled;
        }

        if (PlatformIsDebug())
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1,0.2,0.2,1), "WARNING: Profiling debug configuration");
        }

        
        ImGui::NewLine();

    }
    
    void OnGUIPanel()
    {
        DrawToolbar();


        std::lock_guard<std::mutex> lock(m_mutex);
       
        DrawFramesSummary();
        ImGui::Spacing();

        DrawFrames();
        ImGui::Spacing();
        
        

        
        if (ImGui::BeginTabBar("ProfilerTabs", 0))
        {
            if (ImGui::BeginTabItem("Tree")) {
                UpdateReport();
                m_report.DrawTree();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Summary")) {
                UpdateReport();
                m_report.DrawSummary();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        

        
        ImGui::Spacing();
//        m_report.DrawSummary();

    }
    
    
    void OnGUI(bool *popen)
    {
        if (!*popen)
            return;

        
        ImGui::SetNextWindowSize (ImVec2(800, 600), ImGuiCond_Once );
        
        if (!ImGui::Begin("Profiler", popen))
        {
            ImGui::End();
            return;
        }
        

        OnGUIPanel();
        
        ImGui::End();
        
    }

};

static ProfilerLog s_log;


void NextFrame()
{
    s_log.NextFrame();
}

void SkipFrame()
{
    s_log.SkipFrame();
}


void OnGUI(bool *popen)
{
    s_log.OnGUI(popen);
    
}


void OnGUIPanel()
{
    s_log.OnGUIPanel();
    
}


void AddSpan(SectionPtr section, ProfilerTicks start)
{
    if (!g_ProfilerEnabled) return;
    
    ProfilerTicks end = GetTime();
    s_log.AddEntry(section, start, end);
}

void AddAsyncBegin(SectionPtr section, uintptr_t id)
{
    if (!g_ProfilerEnabled) return;

    ProfilerTicks time = GetTime();
    s_log.AddAsyncBegin(section, time, id);
}

void AddAsyncNext(SectionPtr section, uintptr_t id)
{
    if (!g_ProfilerEnabled) return;

    ProfilerTicks time = GetTime();
    s_log.AddAsyncNext(section, time, id);
}

void AddAsyncEnd(SectionPtr section, uintptr_t id)
{
    if (!g_ProfilerEnabled) return;

    ProfilerTicks time = GetTime();
    s_log.AddAsyncEnd(section, time, id);
}



void AddGPUSpan(SectionPtr section, ProfilerTicks start,  ProfilerTicks end)
{
    if (!g_ProfilerEnabled) return;

    s_log.AddGPUEntry(section, start, end);
}




void AddEvent(SectionPtr section)
{
    if (!g_ProfilerEnabled) return;

    ProfilerTicks time = GetTime();
    s_log.AddEntry(section, time, time);
}

void CaptureNextFrame()
{
    s_log.CaptureNextFrame();
}

void CaptureToFile(const std::string &path)
{
    s_log.CaptureToFile(path);
}


void AddCaptureLocation(std::string location)
{
    s_log.AddCaptureLocation(location);
}

void Clear()
{
    s_log.Clear();
}

bool CaptureToJsonString(std::string &json)
{
    return s_log.CaptureToJsonString(json);
}



}

