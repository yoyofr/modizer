
#include "platform.h"

#ifdef WIN32
// This is the only file that should be including windows.h 
#define WIN32_LEAN_AND_MEAN           
#include <windows.h>
#include <sys/stat.h>
#if !defined(S_ISREG) && defined(S_IFMT) && defined(S_IFREG)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif


#if !defined(S_ISDIR) && defined(S_IFMT) && defined(S_IFDIR)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

#endif

#ifdef __APPLE__
#include "CoreFoundation/CoreFoundation.h"
#endif


#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/message.h>  // for mach_msg_type_number_t
#include <mach/kern_return.h>  // for kern_return_t
#include <mach/task_info.h>
#include <sys/sysctl.h>
#include "../app/CommonApple/CommonApple.h"
#endif

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#ifdef __ANDROID__
#include <android/log.h>
#include <android/asset_manager_jni.h>
extern AAssetManager *g_assetManager;

#ifdef ANDROID_CONFIG_DEBUG
#define DEBUG 1
#endif

#endif

#if defined(__APPLE__) || defined(EMSCRIPTEN)
#include <sys/types.h>
#include <dirent.h>
#endif

#include <sys/stat.h>
#include <algorithm>
#include <string>
#include <vector>
#include <time.h>
#include "path.h"

#if !defined(WIN32) && !defined(EMSCRIPTEN)

#if !defined(__ANDROID__)
#define CPPHTTPLIB_ZLIB_SUPPORT
#endif
#include "../external/cpp-httplib/httplib.h"
#endif


bool FileExists(std::string name)
{
#if defined(__ANDROID__)
    if (!g_assetManager)
        return false;

    name = PathRemoveLeadingSlash(name);


//    AAsset* AAssetManager_open(AAssetManager* mgr, const char* filename, int mode);
    AAsset* asset =  AAssetManager_open(g_assetManager, name.c_str(), AASSET_MODE_UNKNOWN);
    if (!asset)
        return false;
    AAsset_close(asset);
    return true;

#else

    struct stat s;
    if (stat(name.c_str(), &s) != 0)
    {
        return false;
    }
    
    return !S_ISDIR(s.st_mode);
#endif
}


bool DirectoryExists(const std::string &path)
{
    struct stat s;
    if (stat(path.c_str(), &s) != 0)
    {
        return false;
    }
    
    return S_ISDIR(s.st_mode);
}


int64_t FileGetLastModified(std::string path)
{
	struct stat s;
	if (stat(path.c_str(), &s) != 0)
		return 0;

	return (int64_t)s.st_mtime;
}




bool FileWriteAllText(std::string path, const std::string &text)
{
    std::string fullpath = path;
    
    FILE *file = fopen(fullpath.c_str(), "wt");
    if (!file) return false;
    
    bool result = fwrite( text.data(), 1, text.size(), file) == text.size();
    fclose(file);
    return result;
}


bool FileWriteAllBytes(std::string path, const uint8_t *data, size_t length)
{
    std::string fullpath = path;
    
    FILE *file = fopen(fullpath.c_str(), "wb");
    if (!file) return false;
    
    bool result = fwrite( data, 1, length, file) == length;
    fclose(file);
    return result;
}


bool FileWriteAllBytes(std::string path, std::vector<uint8_t> &data)
{
    return FileWriteAllBytes(path, data.data(), data.size());
}

bool FileReadAllBytes(std::string path, std::vector<uint8_t> &data)
{
#if defined(__ANDROID__)
    if (!g_assetManager)
        return false;

    path = PathRemoveLeadingSlash(path);

    AAsset* asset =  AAssetManager_open(g_assetManager, path.c_str(), AASSET_MODE_BUFFER);
    if (!asset)
        return false;

    size_t length = AAsset_getLength(asset);
    data.resize(length);
    AAsset_read(asset, data.data(), data.size());
    AAsset_close(asset);
    return true;
#else
    std::string fullpath = path;
    
    FILE *file = fopen(fullpath.c_str(), "rb");
    if (!file) return false;
    
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    data.resize(length);
    
    if (fread(data.data(), 1, data.size(), file) != data.size())
    {
        fclose(file);
        return false;
    }
    fclose(file);
    return true;
#endif
}

bool FileReadAllText(std::string path, std::string &text)
{
#if defined(__ANDROID__)
    if (!g_assetManager)
        return false;

    path = PathRemoveLeadingSlash(path);


//    AAsset* AAssetManager_open(AAssetManager* mgr, const char* filename, int mode);
    AAsset* asset =  AAssetManager_open(g_assetManager, path.c_str(), AASSET_MODE_BUFFER);
    if (!asset)
        return false;

    size_t length = AAsset_getLength(asset);

    char *buffer = new char[length];
    AAsset_read(asset, buffer, length);
    text = std::string(buffer, buffer + length);
    delete[] buffer;
    AAsset_close(asset);
    return true;

#else
    FILE *file = fopen(path.c_str(), "rt");
    if (!file) {
        return false;
    }
    
    for (;;)
    {
        int ch = fgetc(file);
        if (ch == EOF)
            break;
        text += (char)ch;
    }
    fclose(file);
    return true;
#endif
    
}



std::string StringFormatV(const char *format, va_list arglist)
{
	char str[64 * 1024];
	int count = vsnprintf(str, sizeof(str), format, arglist);
	(void)count;
	str[sizeof(str) - 1] = '\0';
	return str;
}


std::string StringFormat(const char *format, ...)
{
	va_list arglist;
	va_start(arglist, format);
	std::string str = StringFormatV(format, arglist);
	va_end(arglist);
	return str;
}



void StringFormatV(std::string *pstr, const char *format, va_list arglist)
{
    char str[64 * 1024];
    int count = vsnprintf(str, sizeof(str), format, arglist);
    (void)count;
    str[sizeof(str) - 1] = '\0';
    *pstr = str;
}


void StringFormat(std::string *pstr, const char *format, ...)
{
    va_list arglist;
    va_start(arglist, format);
    StringFormatV(pstr, format, arglist);
    va_end(arglist);
}



class LogBuffer
{

private:
    std::string            _log;
    std::vector<size_t>    _lineOffsets;
    std::mutex             _mutex;
    ImGuiTextFilter     _Filter;
    bool _AutoScroll = true;

public:
    
    LogBuffer()
    {
        Clear();
    }

    void Clear()
    {
        std::lock_guard<std::mutex> _lock(_mutex);
        
        _log.reserve(8 * 1024);
        _log.clear();
        _lineOffsets.reserve(1024);
        _lineOffsets.clear();
        _lineOffsets.push_back(0);
    }
    
    void Append(const std::string &str)
    {
        std::lock_guard<std::mutex> _lock(_mutex);

        for (char c : str)
        {
            _log.push_back(c);

            if (c == '\n')
                _lineOffsets.push_back( _log.size() );
        }
    }
    
    void Append(char c)
    {
        std::lock_guard<std::mutex> _lock(_mutex);

        _log.push_back(c);

        if (c == '\n')
            _lineOffsets.push_back( _log.size() );
    }
    

    void    Draw(const char* title, bool* p_open)
    {
        ImGui::SetNextWindowSize(ImVec2(1100, 400), ImGuiCond_Once);

        if (ImGui::Begin(title, p_open))
        {
            DrawPanel();
        }
        ImGui::End();
    }

    
    void    DrawPanel()
    {
        std::lock_guard<std::mutex> _lock(_mutex);


        // Options menu
        if (ImGui::BeginPopup("Options"))
        {
            ImGui::Checkbox("Auto-scroll", &_AutoScroll);
            ImGui::EndPopup();
        }

        // Main window
        if (ImGui::Button("Options"))
            ImGui::OpenPopup("Options");
        ImGui::SameLine();
        bool clear = ImGui::Button("Clear");
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");
        ImGui::SameLine();
        _Filter.Draw("Filter", -100.0f);

        ImGui::Separator();
        ImGui::BeginChild("scrolling", ImVec2(0,0), false, ImGuiWindowFlags_HorizontalScrollbar |
                          ImGuiWindowFlags_AlwaysVerticalScrollbar);

        if (clear)
            Clear();
        if (copy)
            ImGui::LogToClipboard();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        if (_Filter.IsActive())
        {
            // In this example we don't use the clipper when Filter is enabled.
            // This is because we don't have a random access on the result on our filter.
            // A real application processing logs with ten of thousands of entries may want to store the result of search/filter.
            // especially if the filtering function is not trivial (e.g. reg-exp).

            int lineCount = (int)_lineOffsets.size() - 1;
            const char* buf = _log.data();
            const char* buf_end = buf + _log.size();

            for (int line_no = 0; line_no < lineCount; line_no++)
            {
                const char* line_start = buf + _lineOffsets[line_no];
                const char* line_end = (line_no < lineCount) ? (buf + _lineOffsets[line_no + 1] - 1) : buf_end;
                if (_Filter.PassFilter(line_start, line_end))
                    ImGui::TextUnformatted(line_start, line_end);
            }
        }
        else
        {
            // The simplest and easy way to display the entire buffer:
            //   ImGui::TextUnformatted(buf_begin, buf_end);
            // And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward to skip non-visible lines.
            // Here we instead demonstrate using the clipper to only process lines that are within the visible area.
            // If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them on your side is recommended.
            // Using ImGuiListClipper requires A) random access into your data, and B) items all being the  same height,
            // both of which we can handle since we an array pointing to the beginning of each line of text.
            // When using the filter (in the block of code above) we don't have random access into the data to display anymore, which is why we don't use the clipper.
            // Storing or skimming through the search result would make it possible (and would be recommended if you want to search through tens of thousands of entries)
            
            ImGuiListClipper clipper;
            
            const char* buf = _log.data();
            const char* buf_end = buf + _log.size();
            int lineCount = (int)_lineOffsets.size() - 1;
            clipper.Begin(lineCount);
            while (clipper.Step())
            {
                for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                {
                    const char* line_start = buf + _lineOffsets[line_no];
                    const char* line_end = (line_no < lineCount) ? (buf + _lineOffsets[line_no + 1] - 1) : buf_end;
                    ImGui::TextUnformatted(line_start, line_end);
                }
            }
            clipper.End();
        }
        ImGui::PopStyleVar();

        if (_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
    }

    
};


static LogBuffer s_logBuffer;


void LogDrawWindow(bool *popen)
{
    if (*popen)
    {
        s_logBuffer.Draw("Log", popen);
    }
}

void LogDrawPanel()
{
    s_logBuffer.DrawPanel();
}


void LogOutput(const std::string &str)
{
#ifdef WIN32
    OutputDebugStringA(str.c_str());
    fputs(str.c_str(), stdout);
#elif defined(__ANDROID__)
    __android_log_print(ANDROID_LOG_INFO, "milkdrop2", "%s\n", str.c_str() );
#else
    fputs(str.c_str(), stdout);
#endif
    
    s_logBuffer.Append(str);

}

void LogOutputError(const std::string &str)
{
#ifdef WIN32
    OutputDebugStringA(str.c_str());
    fputs(str.c_str(), stderr);
#elif defined(__ANDROID__)
    __android_log_print(ANDROID_LOG_ERROR, "milkdrop2", "%s\n", str.c_str() );
#else
    fputs(str.c_str(), stderr);
#endif
    
    s_logBuffer.Append(str);
}

void LogPrint(const char *format, ...)
{
	va_list arglist;
	va_start(arglist, format);

    std::string str = StringFormatV(format, arglist);
    LogOutput(str);

	va_end(arglist);
}

void LogError(const char *format, ...)
{

	va_list arglist;
	va_start(arglist, format);

    std::string str = std::string("ERROR: ") + StringFormatV(format, arglist);
    LogOutputError(str);

	va_end(arglist);
}

bool DirectoryCreate(std::string path)
{
#ifdef WIN32
    bool result = CreateDirectoryA(path.c_str(), nullptr);
 #else
    bool result = (mkdir(path.c_str(), S_IRWXU) == 0);
#endif
    return result;
}


void DirectoryCreateRecursive(const std::string &path)
{
    if (path.empty())
        return;
    
    if (DirectoryExists(path)) {
        return;
    }
    
    // create parent directory....
    DirectoryCreateRecursive( PathGetDirectory(path) );
    
    DirectoryCreate(path);
}



void DirectoryGetFiles(std::string dir, std::vector<std::string> &files, bool recurse)
{
#ifdef WIN32
	WIN32_FIND_DATA fd;
	memset(&fd, 0, sizeof(fd));

	std::string mask = dir + "\\*.*";

	HANDLE hFind = FindFirstFile(mask.c_str(), &fd);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		return;
	}

	do
	{
		if (fd.cFileName[0] == '.')
			continue;

		std::string path = std::string(dir) + "\\" + std::string(fd.cFileName);

		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
		{
			if (recurse)
			{
				DirectoryGetFiles(path.c_str(), files, recurse);
			}
			continue;
		}

		files.push_back(path);
	} while (FindNextFile(hFind, &fd));

	FindClose(hFind);
#elif defined(__APPLE__) || defined(EMSCRIPTEN)
    
    // replace \\ with /
    std::replace(dir.begin(), dir.end(), '\\', '/');
    
    DIR *dp = opendir(dir.c_str());
    if (!dp)
        return;
    
    for (;;)
    {
        struct dirent *ep = readdir (dp);
        if (!ep) break;
        if (ep->d_name[0] == '.') continue; // skip '.' anything

        std::string path = std::string(dir) + "/" + std::string(ep->d_name);

        if ((ep->d_type & DT_DIR) !=0 ) {
            if (recurse) {
                DirectoryGetFiles(path.c_str(), files, recurse);
            }
        }
        
        if ((ep->d_type & DT_REG) !=0 ) {
            files.push_back(path);
        }
        
    }
    
    closedir (dp);
#elif defined(__ANDROID__)
    if (!g_assetManager)
        return;

    dir = PathRemoveLeadingSlash(dir);

    AAssetDir * adir = AAssetManager_openDir(g_assetManager, dir.c_str());
    if (!adir)
        return;

    const char *name;
    while ((name = AAssetDir_getNextFileName(adir)))
    {
        std::string assetName = PathCombine(dir, name);
        files.push_back(assetName);
        
        if (recurse) {
            DirectoryGetFiles(assetName, files, recurse);
        }
    }
    AAssetDir_close(adir);
#else
#error Unsupported platform    
#endif
}


const char *PlatformGetAppName()
{
    return "m1lkdr0p";
}

const char *PlatformGetPlatformName()
{
#if defined(OCULUS)
    const char *platform = "oculus";
#elif defined(EMSCRIPTEN)
    const char *platform = "web";
#elif TARGET_OS_IPHONE
    const char *platform = "iphone";
#elif TARGET_OS_MAC
    const char *platform = "macos";
#elif TARGET_OS_TVOS
    const char *platform = "tvos";
#elif __ANDROID__
    const char *platform = "android";
#elif WIN32
    const char* platform = "windows";
#else
    #error Unknown Platform
#endif
    return platform;
}

const char *PlatformGetBuildConfig()
{
#ifdef DEBUG
    return "debug";
#elif PROFILE
    return "profile";
#else
    return "release";
#endif

}
bool        PlatformIsDebug()
{
#ifdef DEBUG
    return true;
#else
    return false;
#endif
    
}

#ifdef WIN32
struct tm* gmtime_r(time_t * _clock, struct tm* _result)
{
_gmtime64_s(_result, _clock);
return _result;
}
struct tm* localtime_r(time_t * _clock, struct tm* _result)
{
_localtime64_s(_result, _clock);
return _result;
}
#endif

std::string PlatformGetTimeString()
{
    auto now = std::chrono::system_clock::now();
    time_t in_time_t = std::chrono::system_clock::to_time_t(now);
    struct tm  buf;
    localtime_r(&in_time_t, &buf);

    char timestr[128];
    strftime(timestr, sizeof(timestr), "%Y-%m-%dT%H-%M-%SZ", &buf);
    return std::string(timestr);
}

const char *PlatformGetAppVersion()
{
#if __APPLE__
    static std::string str;
    if (str.empty()) {
        str = AppGetShortVersionString();
        str += '@';
        str += AppGetBuildVersionString();
    }
    return str.c_str();
#else
    return "1.3.0";
#endif
}


#if __APPLE__


static double ToMB(uint64_t bytes)
{
    return ((double)bytes) / (1024.0 * 1024.0);
    
}
bool PlatformGetMemoryStats(PlatformMemoryStats &stats)
{
    mach_task_basic_info_data_t taskinfo = {0};
    mach_msg_type_number_t outCount = MACH_TASK_BASIC_INFO_COUNT;
    kern_return_t error = task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&taskinfo, &outCount);
    if (error != KERN_SUCCESS) {
        return false;
    }
    
    stats.virtual_size_mb = ToMB(taskinfo.virtual_size);
    stats.resident_size_mb = ToMB(taskinfo.resident_size);
    stats.resident_size_max_mb = ToMB(taskinfo.resident_size_max);
    return true;
}

#else
bool PlatformGetMemoryStats(PlatformMemoryStats &stats)
{
    return false;
}
#endif

#if __APPLE__

static const char *GetSysCtlString(const char *key)
{
    size_t size = 0;
    ::sysctlbyname(key, NULL, &size, NULL, 0);
    
    char *str = new char[size + 1];
    ::sysctlbyname(key, str, &size, NULL, 0);
    str[size] = 0;
    return str;
}
#endif

const char *PlatformGetDeviceModel()
{
#if __APPLE__

    static std::string str;
    if (str.empty())
    {
        str = GetSysCtlString(
                                 // "hw.machine"
                                 "hw.model"
                                 
                                 );
    }
    return str.c_str();
#elif EMSCRIPTEN
    static const char *_agent;
    if (!_agent)
    {
         _agent = (const char *)EM_ASM_INT({
            const agent = window.navigator.userAgent;
            const ptr = allocate(intArrayFromString(agent), ALLOC_NORMAL);
            return ptr;
        });
    }
    return _agent;

#else
    return "<unknown>";
#endif
}

const char *PlatformGetDeviceArch()
{
#if __APPLE__

    static std::string str;
    if (str.empty())
    {
        str = GetSysCtlString(
                                 "hw.machine"
                                 );
    }
    return str.c_str();
    
#elif EMSCRIPTEN
    return "WebAssembly";
#else
    return "<unknown>";
#endif
}


namespace Config {

    bool Exists(const char *name)
    {
        return getenv(name) != NULL;
    }

    bool TryGetBool(const char *name, bool *v)
    {
        const char *e = getenv(name);
        if (!e) return false;
        
        *v = false;
        if (strcmp(e, "1") == 0) *v = true;
        return true;
    }
    bool TryGetInt(const char *name, int *v)
    {
        const char *e = getenv(name);
        if (!e) return false;
        *v = atoi(e);
        return true;

    }
    bool TryGetFloat(const char *name, float *v)
    {
        const char *e = getenv(name);
        if (!e) return false;
        *v = (float)atof(e);
        return true;
    }


    bool TryGetString(const char *name, std::string *v)
    {
        const char *e = getenv(name);
        if (!e) return false;
        *v = e;
        return true;
       
    }


    bool GetBool(const char *name, bool defaultValue)
    {
        bool v;
        if (!TryGetBool(name, &v)) {
            return defaultValue;
        }
        return v;
    }

    int GetInt(const char *name, int defaultValue)
    {
        int v;
        if (!TryGetInt(name, &v)) {
            return defaultValue;
        }
        return v;
    }

    float GetFloat(const char *name, float defaultValue)
    {
        float v;
        if (!TryGetFloat(name, &v)) {
            return defaultValue;
        }
        return v;
    }
    std::string GetString(const char *name, const std::string &defaultValue)
    {
        std::string str;
        if (!TryGetString(name, &str)) {
            return defaultValue;
        }
        return str;
    }

}


void DispatchQueue::Invoke(std::function<void ()> action)
{
    auto asyncId = InvokeAsync(action);
    Wait(asyncId);
}

uint64_t DispatchQueue::InvokeAsync(std::function<void ()> action)
{
    std::unique_lock<std::mutex> lock(m_queueMutex);
    m_queue.push(action);
    return ++m_queueCount;
}

void DispatchQueue::Wait(uint64_t asyncId)
{
    std::unique_lock<std::mutex> lock(m_executeMutex);
    
    m_condition.wait(lock, [this, asyncId]() -> bool {
        int64_t delta = m_executeCount - asyncId;
        return delta >= 0;
    });
}

void DispatchQueue::Process()
{
    for (;;)
    {
        std::function<void ()> action = nullptr;
        
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            if (m_queue.empty())
                return;
                
            action = m_queue.front();
            m_queue.pop();
        }
                
        if (action)
        {
            action();
        }
        
        {
            std::unique_lock<std::mutex> lock(m_executeMutex);
            m_executeCount++;
            m_condition.notify_all();
        }
    }
}


static bool IsReservedChar(char c, const char *reserved)
{
    if (c < 0 || !isprint(c))
    {
        return true;
    }

    for (int j = 0; reserved[j] != 0; j++)
    {
        if (c == reserved[j])
        {
            return true;
        }
    }
    return false;
}

static char ToHexDigit(char val)
{
    val &= 0xF;
    if (val < 10)
    {
        return '0' + val;
    }
    else
    {
        return 'A' + val - 10;
    }
}


std::string UrlEscape(const std::string &in, const char *reserved)
{
    std::string out;

    int length = (int)in.size();
    out.reserve(length);
    for (int i = 0; i < length; i++)
    {
        char c = in[i];
        if (IsReservedChar(c, reserved))
        {
            out += '%';
            out += ToHexDigit((c >> 4) & 0xF);
            out += ToHexDigit((c >> 0) & 0xF);
        }
        else
        {
            out += in[i];
        }
    }

    return out;
}

std::string UrlEscape(const std::string &in)
{
    return UrlEscape(in, "/ %#&+,:;=?@");
}


struct ParsedURL
{
    std::string server;
    int         port;
    std::string path;
};


bool ParseURL(std::string url, ParsedURL &parsed)
{
    std::string rest = url;

    auto index = rest.find("://");
    if (index == std::string::npos) return false;
    
    std::string protocol = rest.substr(0, index);
    rest = rest.substr(index + 3);
    

    index = rest.find_first_of(":/");
    if (index == std::string::npos) return false;
    
    std::string port;
    if (rest[index] == ':') {

        parsed.server = rest.substr(0, index);
        rest = rest.substr(index + 1);
        
        index = rest.find('/');
        if (index != std::string::npos) {
            port = rest.substr(0, index);
            rest = rest.substr(index);
            parsed.path = rest;
        } else {
            port = rest;
            parsed.path = "/";
        }
        

    } else {
        parsed.server = rest.substr(index);
        parsed.path = rest.substr(index);
        port = "80";
    }
    
    parsed.port =  atoi(port.c_str());;
    return true;
}

void HttpSendFile(std::string method, std::string url, const char *contentPath,  bool compress, const char *contentType,
                  std::function<void (const std::string *)> complete)
{
    std::vector<uint8_t> data;
    
    if (!FileReadAllBytes(contentPath, data)) {
        std::string error = "Could not read file at path: ";
        error += contentPath;
        complete(&error);
        return;
    }
    
    HttpSend(method, url, (const char *)data.data(), data.size(), compress, contentType, complete);
}

void HttpSend(std::string method, std::string url, const char *content,  size_t content_size, bool compress, const char *contentType, std::function<void (const std::string *)> complete)
{
#if !defined(WIN32) && !defined(EMSCRIPTEN)
    using namespace httplib;

    
    ParsedURL parsed;
    if (!ParseURL(url, parsed))
    {
        return;
    }
    
    StopWatch sw;
    
    
 //   LogPrint("[HTTP] %s %s %s %d\n", method.c_str(), url.c_str(), parsed.server.c_str(), parsed.port );
    LogPrint("[HTTP] %s %s\n", method.c_str(), url.c_str() );

    
    Client cli(parsed.server.c_str(), parsed.port);
    
    std::shared_ptr<Response> response;
    
    ContentProvider provider = [content](size_t offset, size_t length, DataSink &sink) {
        if (content)
            sink.write( content + offset, length );
        if (sink.done)
            sink.done();
    };
    
    std::string path = UrlEscape(parsed.path);
    
    if (method == "GET")
    {
        response = cli.Get(path.c_str() );
    }
    else if (method == "POST")
    {
        if (content)
        {
            cli.set_compress(compress);
            response = cli.Post(path.c_str(), content_size, provider, contentType );
        }
    }
    else if (method == "PUT")
    {
        if (content)
        {
            cli.set_compress(compress);
            response = cli.Put(path.c_str(), content_size, provider, contentType );
        }
    }
    else if (method == "DELETE")
    {
        response = cli.Delete(path.c_str());
    }


    
    
    if (response)
    {
        if (complete)
            complete( &response->body);
        
        LogPrint("[HTTP] complete: %s %f seconds\n", response->body.c_str(), sw.GetElapsedSeconds());
    }
    else
    {
        
        LogError("[HTTP] error\n");
        
        if (complete)
            complete( nullptr );

    }
#endif
}

