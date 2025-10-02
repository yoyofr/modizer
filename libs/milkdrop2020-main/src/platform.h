

#pragma once

#if __APPLE__
#include <TargetConditionals.h>
#endif

#include <locale.h>
#include <math.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>
#include <string>
#include <vector>
#include <algorithm>
#include <thread>
#include <future>
#include <random>
#include "StopWatch.h"
#include "path.h"
#include "imgui_support.h"
#include "TProfiler.h"

#ifndef FALSE
#define FALSE (1 != 1) // why not just define it as "false" or "0"?
#endif

#ifndef TRUE
#define TRUE (!FALSE)
#endif

#ifndef MAX_PATH
#define MAX_PATH          260
#endif

#ifdef _MSC_VER
#define strcasecmp stricmp
#define strncasecmp _strnicmp
#endif

#ifdef _DEBUG
#define DEBUG 1
#endif

#ifndef M_PI
#define M_E         2.71828182845904523536028747135266250   /* e              */
#define M_LOG2E     1.44269504088896340735992468100189214   /* log2(e)        */
#define M_LOG10E    0.434294481903251827651128918916605082  /* log10(e)       */
#define M_LN2       0.693147180559945309417232121458176568  /* loge(2)        */
#define M_LN10      2.30258509299404568401799145468436421   /* loge(10)       */
#define M_PI        3.14159265358979323846264338327950288f   /* pi             */
#define M_PI_2      1.57079632679489661923132169163975144   /* pi/2           */
#define M_PI_4      0.785398163397448309615660845819875721  /* pi/4           */
#define M_1_PI      0.318309886183790671537767526745028724  /* 1/pi           */
#define M_2_PI      0.636619772367581343075535053490057448  /* 2/pi           */
#define M_2_SQRTPI  1.12837916709551257389615890312154517   /* 2/sqrt(pi)     */
#define M_SQRT2     1.41421356237309504880168872420969808   /* sqrt(2)        */
#define M_SQRT1_2   0.707106781186547524400844362104849039  /* 1/sqrt(2)      */
#endif



#ifdef EMSCRIPTEN
#define PLATFORM_SUPPORTS_THREADS 0
#else
#define PLATFORM_SUPPORTS_THREADS 1
#endif

//
//
//

#if (defined(__clang__) || defined(__GNUC__))
#define PLATFORM_FMTARGS(FMT)             __attribute__((format(printf, FMT, FMT+1))) // To apply printf-style warnings to our functions.
#define PLATFORM_FMTLIST(FMT)             __attribute__((format(printf, FMT, 0)))
#else
#define PLATFORM_FMTARGS(FMT)
#define PLATFORM_FMTLIST(FMT)
#endif



std::string StringFormatV(const char *format, va_list arglist) PLATFORM_FMTLIST(1);
std::string StringFormat(const char *format, ...) PLATFORM_FMTARGS(1);

void StringFormatV(std::string *pstr, const char *format, va_list arglist);
void StringFormat(std::string *pstr, const char *format, ...);

void LogPrintV(const char *format, va_list arglist) PLATFORM_FMTLIST(1);
void LogPrint(const char *format, ...) PLATFORM_FMTARGS(1);
void LogError(const char *format, ...) PLATFORM_FMTARGS(1);
void LogDrawWindow(bool *popen);
void LogDrawPanel();

void DirectoryGetFiles(std::string dir, std::vector<std::string> &files, bool recurse = false);
bool DirectoryCreate(std::string dir);
bool DirectoryExists(const std::string &path);
void DirectoryCreateRecursive(const std::string &path);


int64_t FileGetLastModified(std::string path);
bool FileExists(std::string name);
bool FileReadAllText(std::string path, std::string &text);
bool FileWriteAllText(std::string path, const std::string &text);
bool FileWriteAllBytes(std::string path, const uint8_t *data, size_t length);
bool FileWriteAllBytes(std::string path, std::vector<uint8_t> &data);
bool FileReadAllBytes(std::string path, std::vector<uint8_t> &data);



const char *PlatformGetPlatformName();
const char *PlatformGetAppName();
const char *PlatformGetAppVersion();
const char *PlatformGetDeviceModel();
const char *PlatformGetDeviceArch();
const char *PlatformGetBuildConfig();
bool        PlatformIsDebug();



struct PlatformMemoryStats
{
    double virtual_size_mb;
    double resident_size_mb;
    double resident_size_max_mb;
};

bool PlatformGetMemoryStats(PlatformMemoryStats &stats);

std::string PlatformGetTimeString();


std::string UrlEscape(const std::string &str);

void HttpSend(std::string method, std::string url, const char *content,  size_t content_size, bool compress, const char *contentType,
              std::function<void (const std::string *)> complete);

void HttpSendFile(std::string method, std::string url, const char *contentPath,  bool compress, const char *contentType,
                  std::function<void (const std::string *)> complete);


namespace Config
{
    bool Exists(const char *name);

    bool TryGetBool(const char *name, bool *v);
    bool TryGetInt(const char *name, int *v);
    bool TryGetFloat(const char *name, float *v);
    bool TryGetString(const char *name, std::string *v);


    bool        GetBool(const char *name, bool defaultValue);
    int         GetInt(const char *name, int defaultValue);
    float       GetFloat(const char *name, float defaultValue);
    std::string GetString(const char *name, const std::string &defaultValue);
};





static inline void DispatchVoid(std::function<void ()> func)
{
#if PLATFORM_SUPPORTS_THREADS
    auto pfuture = std::make_shared<std::future<void>>();
    *pfuture = std::async(std::launch::async, [pfuture, func]() {
        func();
    });
#else
    func();
#endif
}


template<typename T>
std::future<T> Dispatch(std::function<T ()> func)
{
#if PLATFORM_SUPPORTS_THREADS
    {
        return std::async( std::launch::async, func);
    }
#else
    {
        std::promise<T> promise;
        promise.set_value(func());
        return promise.get_future();
    }
#endif
}




template<typename T>
void Dispatch(std::future<T> &future, std::function<T ()> func)
{
#if PLATFORM_SUPPORTS_THREADS
    if (future.valid()) {
        future.wait();
    }
#endif

#if PLATFORM_SUPPORTS_THREADS
    {
        future = std::async( std::launch::async, func);
    }
#else
    {
        std::promise<T> promise;
        promise.set_value(func());
        future = promise.get_future();
    }
#endif
}


template<typename T>
bool DispatchIsComplete(std::future<T> &future)
{
    if (future.valid())
    {
        if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            return true;
        }
    }
    
    return false;
}



class DispatchQueue
{
public:
    using AsyncId = uint64_t;
    
    void     Process();
    void     Invoke(std::function<void ()> action);
    AsyncId  InvokeAsync(std::function<void ()> action);
    void     Wait(AsyncId asyncId);


protected:
    std::mutex                           m_queueMutex;
    std::queue< std::function<void ()> > m_queue;
    std::mutex                           m_executeMutex;
    std::condition_variable              m_condition;
    AsyncId                              m_executeCount = 0;
    AsyncId                              m_queueCount = 0;

    
};



