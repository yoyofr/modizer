/*
 * xSF - Core configuration handler
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-10-05
 *
 * Partially based on the vio*sf framework
 */

#pragma once

#include <memory>
#include "XSFPlayer.h"
#include "convert.h"

class XSFConfig
{
protected:
	double volume;
	VolumeType volumeType;
	PeakType peakType;
	unsigned sampleRate;
	std::string titleFormat;
	std::vector<unsigned> supportedSampleRates;
	
	XSFConfig();
	virtual void LoadSpecificConfig() = 0;
    virtual void SaveSpecificConfig() = 0;
	virtual void CopySpecificConfigToMemory(XSFPlayer *xSFPlayer, bool preLoad) = 0;
public:
    bool playInfinitely;
    unsigned long skipSilenceOnStartSec, detectSilenceSec, defaultLength, defaultFade;
    
	static bool initPlayInfinitely;
	static std::string initSkipSilenceOnStartSec, initDetectSilenceSec, initDefaultLength, initDefaultFade, initTitleFormat;
	static double initVolume;
	static VolumeType initVolumeType;
	static PeakType initPeakType;
	// These are not defined in XSFConfig.cpp, they should be defined in your own config's source.
	static unsigned initSampleRate;
	static std::string commonName;
	static std::string versionNumber;
	// The Create function is not defined in XSFConfig.cpp, it should be defined in your own config's source and return a pointer to your config's class.
	static XSFConfig *Create();
	static const std::string &CommonNameWithVersion();

	virtual ~XSFConfig() { }
	void LoadConfig();
	void SaveConfig();
	void CopyConfigToMemory(XSFPlayer *xSFPlayer, bool preLoad);
	
	bool GetPlayInfinitely() const;
	unsigned long GetSkipSilenceOnStartSec() const;
	unsigned long GetDetectSilenceSec() const;
	unsigned long GetDefaultLength() const;
	unsigned long GetDefaultFade() const;
	double GetVolume() const;
	VolumeType GetVolumeType() const;
	PeakType GetPeakType() const;
	const std::string &GetTitleFormat() const;
};
