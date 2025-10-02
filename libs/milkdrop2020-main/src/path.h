

#pragma once

#include <string>

std::string PathCombine(const std::string &base, const std::string &relative);
std::string PathGetExtension(const std::string &path);
std::string PathGetExtensionLower(const std::string &path);
std::string PathGetFileName(const std::string &path);
std::string PathGetDirectory(const std::string &path);
std::string PathRemoveExtension(const std::string &path);
std::string PathChangeExtension(const std::string &path, const std::string &ext);
std::string PathRemoveLeadingSlash(const std::string &path);
std::string PathGetFullPath(const std::string& path);



