/*
 * xSF - NCSF configuration
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-10-05
 *
 * Partially based on the vio*sf framework
 */

#pragma once

#include <bitset>
#include <memory>
#include "XSFConfig.h"
#include "XSFPlayer_NCSF.h"

class XSFConfig_NCSF;

class XSFConfig_NCSF : public XSFConfig
{
protected:
	static unsigned initInterpolation;
	static std::string initMutes;

	friend class XSFConfig;
	friend struct SoundViewData;
	
	XSFConfig_NCSF();
	void LoadSpecificConfig();
	void SaveSpecificConfig();
	void GenerateSpecificDialogs();
	void CopySpecificConfigToMemory(XSFPlayer *xSFPlayer, bool preLoad);

public:
    unsigned interpolation;
    std::bitset<16> mutes;


};
