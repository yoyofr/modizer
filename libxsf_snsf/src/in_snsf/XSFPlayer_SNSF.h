/*
 * xSF - 2SF Player
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-09-24
 *
 * Based on a modified vio2sf v0.22c
 *
 * Partially based on the vio*sf framework
 *
 * Utilizes a modified DeSmuME v0.9.9 SVN for playback
 * http://desmume.org/
 */

#pragma once

#include <memory>
#include <zlib.h>
#include "convert.h"
#include "XSFPlayerSNSF.h"

class XSFPlayer_SNSF : public XSFPlayerSNSF
{
public:
    XSFPlayer_SNSF(const std::string &filename);
#ifdef _WIN32
    XSFPlayer_SNSF(const std::wstring &filename);
#endif
    ~XSFPlayer_SNSF() { this->Terminate(); }
    bool Load();
    void GenerateSamples(std::vector<uint8_t> &buf, unsigned offset, unsigned samples);
    void Terminate();
};

