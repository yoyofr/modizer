/*
 * xSF - NCSF Player
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-10-18
 *
 * Partially based on the vio*sf framework
 *
 * Utilizes a modified FeOS Sound System for playback
 * https://github.com/fincs/FSS
 */

#pragma once

#include <memory>
#include <bitset>
#include "XSFPlayer.h"
#include "SSEQPlayer/SDAT.h"
#include "SSEQPlayer/Player.h"

class XSFPlayer_NCSF : public XSFPlayer
{
	uint32_t sseq;
	std::vector<uint8_t> sdatData;
	std::unique_ptr<SDAT> sdat;
	Player player;
	double secondsPerSample, secondsIntoPlayback, secondsUntilNextClock;
	std::bitset<16> mutes;

	void MapNCSFSection(const std::vector<uint8_t> &section);
	bool MapNCSF(XSFFile *xSFToLoad);
	bool RecursiveLoadNCSF(XSFFile *xSFToLoad, int level);
	bool LoadNCSF();
public:
	XSFPlayer_NCSF(const std::string &filename);
	~XSFPlayer_NCSF();
	bool Load();
	void GenerateSamples(std::vector<uint8_t> &buf, unsigned offset, unsigned samples);
	void Terminate();

	void SetInterpolation(unsigned interpolation);
	void SetMutes(const std::bitset<16> &newMutes);

};
