/*
 * xSF - NCSF configuration
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-12-09
 *
 * Partially based on the vio*sf framework
 */

#include "XSFConfig_NCSF.h"
#include "convert.h"

unsigned XSFConfig::initSampleRate = 44100;
std::string XSFConfig::commonName = "NCSF Decoder";
std::string XSFConfig::versionNumber = "1.11.1";
unsigned XSFConfig_NCSF::initInterpolation = 4;
std::string XSFConfig_NCSF::initMutes = "0000000000000000";

XSFConfig *XSFConfig::Create()
{
	return new XSFConfig_NCSF();
}

XSFConfig_NCSF::XSFConfig_NCSF() : XSFConfig(), interpolation(0), mutes()
{
	this->supportedSampleRates.push_back(8000);
	this->supportedSampleRates.push_back(11025);
	this->supportedSampleRates.push_back(16000);
	this->supportedSampleRates.push_back(22050);
	this->supportedSampleRates.push_back(32000);
	this->supportedSampleRates.push_back(44100);
	this->supportedSampleRates.push_back(48000);
	this->supportedSampleRates.push_back(88200);
	this->supportedSampleRates.push_back(96000);
	this->supportedSampleRates.push_back(176400);
	this->supportedSampleRates.push_back(192000);
}

void XSFConfig_NCSF::LoadSpecificConfig()
{
    this->interpolation = 4;//this->configIO->GetValue("Interpolation", XSFConfig_NCSF::initInterpolation);
	/*std::stringstream mutesSS(this->configIO->GetValue("Mutes", XSFConfig_NCSF::initMutes));
	mutesSS >> this->mutes;*/
    this->mutes=0;
}

void XSFConfig_NCSF::SaveSpecificConfig()
{
	/*this->configIO->SetValue("Interpolation", this->interpolation);
	this->configIO->SetValue("Mutes", this->mutes.to_string<char>());*/
}

void XSFConfig_NCSF::CopySpecificConfigToMemory(XSFPlayer *xSFPlayer, bool)
{
	auto NCSFPlayer = static_cast<XSFPlayer_NCSF *>(xSFPlayer);
	NCSFPlayer->SetInterpolation(this->interpolation);
	NCSFPlayer->SetMutes(this->mutes);
}
