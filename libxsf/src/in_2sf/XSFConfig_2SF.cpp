/*
 * xSF - 2SF configuration
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-09-24
 *
 * Partially based on the vio*sf framework
 */

#include <bitset>
#include "XSFPlayer.h"
#include "XSFConfig.h"
#include "convert.h"
#include "desmume/NDSSystem.h"
#include "desmume/version.h"


class XSFConfig_2SF : public XSFConfig
{
protected:
	static unsigned initInterpolation;
	static std::string initMutes;

	friend class XSFConfig;
	
	XSFConfig_2SF();
	void LoadSpecificConfig();
	void SaveSpecificConfig();
	void CopySpecificConfigToMemory(XSFPlayer *xSFPlayer, bool preLoad);
public:
    unsigned interpolation;
    std::bitset<16> mutes;

};

unsigned XSFConfig::initSampleRate = 44100;
std::string XSFConfig::commonName = "2SF Decoder";
std::string XSFConfig::versionNumber = "0.9b";
unsigned XSFConfig_2SF::initInterpolation = 2;
std::string XSFConfig_2SF::initMutes = "0000000000000000";

XSFConfig *XSFConfig::Create()
{
	return new XSFConfig_2SF();
}

XSFConfig_2SF::XSFConfig_2SF() : XSFConfig(), interpolation(0), mutes()
{
	this->supportedSampleRates.push_back(44100);
}

void XSFConfig_2SF::LoadSpecificConfig()
{
    this->interpolation = 2;//this->configIO->GetValue("Interpolation", XSFConfig_2SF::initInterpolation);
	//std::stringstream mutesSS(this->configIO->GetValue("Mutes", XSFConfig_2SF::initMutes));
	//mutesSS >> this->mutes;
    mutes=0;
}

void XSFConfig_2SF::SaveSpecificConfig()
{
	/*this->configIO->SetValue("Interpolation", this->interpolation);
	this->configIO->SetValue("Mutes", this->mutes.to_string<char>());*/
}


void XSFConfig_2SF::CopySpecificConfigToMemory(XSFPlayer *, bool preLoad)
{
	if (!preLoad)
	{
		CommonSettings.spuInterpolationMode = static_cast<SPUInterpolationMode>(this->interpolation);
		for (size_t x = 0, numMutes = this->mutes.size(); x < numMutes; ++x)
			CommonSettings.spu_muteChannels[x] = this->mutes[x];
	}
}
