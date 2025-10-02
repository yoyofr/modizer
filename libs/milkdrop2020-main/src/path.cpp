
#include <stdlib.h>
#include <algorithm>
#include "path.h"
#include "platform.h"


std::string PathAddTrailingSlash(const std::string &path);


std::string PathCombine(const std::string &base, const std::string &relative)
{
	if (!relative.empty() && relative[0] == '/') {
		return relative;
	}
    std::string path = PathAddTrailingSlash(base);    
    path += relative;
	return path;
}



std::string PathGetFullPath(const std::string& path)
{
    std::string result;

#ifdef WIN32
    char buffer[_MAX_PATH];
    _fullpath(buffer, path.c_str(), sizeof(buffer));
#else
    char buffer[PATH_MAX];
    (void)realpath(path.c_str(), buffer);
#endif
    result = buffer;

    return result;
}


std::string PathGetExtension(const std::string &path)
{
    size_t pos = path.find_last_of("./\\");
    if (pos != std::string::npos && path[pos] == '.')
    {
        return path.substr(pos);
    }
    else
    {
        return "";
    }
}

std::string PathGetExtensionLower(const std::string &path)
{
    std::string ext = PathGetExtension(path);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

std::string PathGetFileName(const std::string &path)
{
    size_t pos = path.find_last_of("/\\");
    
    if (pos != std::string::npos)
    {
        return path.substr(pos + 1);
    }
    else
    {
        return path;
    }
}

std::string PathGetDirectory(const std::string &path)
{
    size_t pos = path.find_last_of("/\\");
    
    if (pos != std::string::npos)
    {
        return path.substr(0, pos);
    }
    else
    {
        return std::string();
    }
}


std::string PathGetLastComponent(const std::string &path)
{
    size_t pos = path.find_last_of("/\\");
    size_t end = path.size();
    if (pos == (end - 1))
    {
        // we have a trailing slash, skip it
        end = pos;
        pos = path.find_last_of("/\\", pos - 1);
    }
    
    if (pos != std::string::npos)
    {
        return path.substr(pos + 1, end - pos - 1);
    }
    else
    {
        return std::string();
    }
}

std::string PathRemoveExtension(const std::string &path)
{
    size_t pos = path.find_last_of("./\\");
    if (pos != std::string::npos && path[pos] == '.')
    {
        return path.substr(0, pos);
    }
    else
    {
        return path;
    }
}

std::string PathChangeExtension(const std::string &path, const std::string &ext)
{
    return PathRemoveExtension(path) + ext;
}


std::string PathRemoveLeadingSlash(const std::string &path)
{
    std::string result = path;
    if (result.size() > 0)
    {
        char c = result[0];
        if (c == '/' || c == '\\')
        {
            // remove trailing slash
            result.erase(result.begin());
        }
    }
    return result;
}


std::string PathRemoveTrailingSlash(const std::string &path)
{
    std::string result = path;
    if (result.size() > 0)
    {
        char last = result[result.size()-1];
        if (last == '/' || last == '\\')
        {
            // remove trailing slash
            result.resize(result.size()-1);
        }
    }
    return result;
}

std::string PathAddTrailingSlash(const std::string &path)
{
    std::string result = path;
    if (result.size() > 0)
    {
        char last = result[result.size()-1];
        if (last != '/' && last != '\\')
        {
            // add trailing slash
            result.push_back('/');
        }
    }
    else
    {
        // add trailing slash to empty path
        result.push_back('/');
    }
    return result;
}

