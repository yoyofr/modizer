

#pragma once


#include <string>


bool ImageWriteToPNG(const std::string &path, const uint32_t *data, int width, int height);
bool ImageWriteToBMP(const std::string &path, const uint32_t *data, int width, int height);
bool ImageWriteToTGA(const std::string &path, const uint32_t *data, int width, int height);
bool ImageWriteToHDR(const std::string &path, const float *data, int width, int height);
bool ImageWriteToFile(const std::string &path, const uint32_t *data, int width, int height);
