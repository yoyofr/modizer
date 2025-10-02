
#pragma once

#include <chrono>
#include <string>

#define PROF_ENABLED 1

namespace TProfiler
{

//
// profiler timer
//
    using ProfilerTicks = int64_t;
    ProfilerTicks GetTime();
    double ToSeconds(ProfilerTicks ticks);



//
// profiler section
//
    struct Section;
    using SectionPtr = Section *;
    SectionPtr GetSection(const char *name);
    SectionPtr AddSection(const char *name, const char *categories = nullptr);
    SectionPtr ResolveSection(SectionPtr *ppsection, const char *name, const char *categories = nullptr);




    void AddSpan(SectionPtr section, ProfilerTicks start);
    void AddEvent(SectionPtr section);
    void AddAsyncBegin(SectionPtr section, uintptr_t async_id);
    void AddAsyncNext(SectionPtr section, uintptr_t async_id);
    void AddAsyncEnd(SectionPtr section, uintptr_t async_id);

    void AddGPUSpan(SectionPtr section, ProfilerTicks start, ProfilerTicks end);

    void OnGUI(bool *popen);
    void OnGUIPanel();

    void NextFrame();
    void SkipFrame();

    void AddCaptureLocation(std::string url);

    void CaptureNextFrame();
    bool CaptureToJsonString(std::string &json);

    void CaptureToFile(const std::string &path);
    void Clear();


    class ProfilerScope
    {
    public:
        inline ProfilerScope(SectionPtr section)
            :
            m_section(section),
            m_startTime(GetTime())
        {
        }

        inline ~ProfilerScope()
        {
            AddSpan(m_section, m_startTime);
        }
        
    protected:
        SectionPtr        m_section;
        ProfilerTicks              m_startTime;
    };


}



//
// profile section macros
//


#ifdef WIN32
#define __PRETTY_FUNCTION__ __FUNCTION__
#endif


#if PROF_ENABLED

#define PROFILE_DECLARE_SECTION(__section_var, __name, __categories)   \
    static TProfiler::SectionPtr __section_var; \
    if (!__section_var) TProfiler::ResolveSection(&__section_var, __name, __categories);


#define __PROFILE_SCOPE(__section_var, __name, __categories)   \
    static TProfiler::SectionPtr __section_var; \
    if (!__section_var) TProfiler::ResolveSection(&__section_var, __name, __categories); \
    TProfiler::ProfilerScope __scope_##__section_var (__section_var);


#define CONCAT_IMPL( x, y ) x##y
#define MACRO_CONCAT( x, y ) CONCAT_IMPL( x, y )

#define _PROFILE_SCOPE(__section_var, __name, __categories)    __PROFILE_SCOPE( __section_var , __name, __categories)

#define PROFILE_SCOPE(__name)                         _PROFILE_SCOPE( MACRO_CONCAT(_profiler_, __COUNTER__), __name, nullptr)
#define PROFILE_SCOPE_CAT(__name, __categories)       _PROFILE_SCOPE( MACRO_CONCAT(_profiler_, __COUNTER__), __name, __categories)
#define PROFILE_FUNCTION()                            PROFILE_SCOPE(__PRETTY_FUNCTION__)
#define PROFILE_FUNCTION_CAT(__categories)            PROFILE_SCOPE_CAT(__PRETTY_FUNCTION__, __categories)
#else


#define __PROFILE_SCOPE(__section_var, __name, __categories)
#define _PROFILE_SCOPE(__counter, __name, __categories)

#define PROFILE_DECLARE_SECTION(__section_var, __name, __categories)
#define PROFILE_SCOPE(__name)
#define PROFILE_SCOPE_CAT(__name, __categories)
#define PROFILE_FUNCTION()
#define PROFILE_FUNCTION_CAT(__categories)
#endif




#define PROFILE_FRAME()            \
    TProfiler::NextFrame(); \
    PROFILE_FUNCTION()

//PROFILE_SCOPE(".frame")


#define PROFILE_GPU()              PROFILE_SCOPE_CAT(".gpu.wait", "gpu")

