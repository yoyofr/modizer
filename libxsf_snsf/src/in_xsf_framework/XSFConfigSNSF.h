/*
 * xSF - Core configuration handler
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-10-05
 *
 * Partially based on the vio*sf framework
 */

#pragma once

#include <memory>
#include "XSFPlayerSNSF.h"
#include "convert.h"

class XSFConfigSNSF
{
protected:
	double volume;
	VolumeTypeSNSF volumeType;
	PeakTypeSNSF peakType;
	unsigned sampleRate;
	std::string titleFormat;
	std::vector<unsigned> supportedSampleRates;
	
	XSFConfigSNSF();
	virtual void LoadSpecificConfig() = 0;
    virtual void SaveSpecificConfig() = 0;
	virtual void CopySpecificConfigToMemory(XSFPlayerSNSF *xSFPlayer, bool preLoad) = 0;
public:
    bool playInfinitely;
    unsigned long skipSilenceOnStartSec, detectSilenceSec, defaultLength, defaultFade;
    
	static bool initPlayInfinitely;
	static std::string initSkipSilenceOnStartSec, initDetectSilenceSec, initDefaultLength, initDefaultFade, initTitleFormat;
	static double initVolume;
	static VolumeTypeSNSF initVolumeType;
	static PeakTypeSNSF initPeakType;
	// These are not defined in XSFConfigSNSF.cpp, they should be defined in your own config's source.
	static unsigned initSampleRate;
	static std::string commonName;
	static std::string versionNumber;
	// The Create function is not defined in XSFConfigSNSF.cpp, it should be defined in your own config's source and return a pointer to your config's class.
	static XSFConfigSNSF *Create();
	static const std::string &CommonNameWithVersion();

	virtual ~XSFConfigSNSF() { }
	void LoadConfig();
	void SaveConfig();
	void CopyConfigToMemory(XSFPlayerSNSF *xSFPlayer, bool preLoad);
	
	bool GetPlayInfinitely() const;
	unsigned long GetSkipSilenceOnStartSec() const;
	unsigned long GetDetectSilenceSec() const;
	unsigned long GetDefaultLength() const;
	unsigned long GetDefaultFade() const;
	double GetVolume() const;
	VolumeTypeSNSF GetVolumeType() const;
	PeakTypeSNSF GetPeakType() const;
	const std::string &GetTitleFormat() const;
};
