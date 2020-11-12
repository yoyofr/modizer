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
#include "../in_xsf_framework/convert.h"
#include "../in_xsf_framework/XSFPlayer.h"
#include "desmume/NDSSystem.h"

class XSFPlayer_2SF : public XSFPlayer
{
	std::vector<uint8_t> rom;

	void Map2SFSection(const std::vector<uint8_t> &section);
	bool Map2SF(XSFFile *xSFToLoad);
	bool RecursiveLoad2SF(XSFFile *xSFToLoad, int level);
	bool Load2SF(XSFFile *xSFToLoad);
public:
	XSFPlayer_2SF(const std::string &filename);
#ifdef _WIN32
	XSFPlayer_2SF(const std::wstring &filename);
#endif
	~XSFPlayer_2SF() { this->Terminate(); }
	bool Load();
	void GenerateSamples(std::vector<uint8_t> &buf, unsigned offset, unsigned samples);
	void Terminate();
};

