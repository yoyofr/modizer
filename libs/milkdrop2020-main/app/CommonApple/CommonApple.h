

#pragma once

#include <stdint.h>
#include <string>

bool GetResourceDir(std::string &outdir);
bool GetApplicationSupportDir(std::string &outdir);

std::string AppGetShortVersionString();
std::string AppGetBuildVersionString();
