/*
 * xSF - Core configuration handler
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-10-05
 *
 * Partially based on the vio*sf framework
 */

#include "XSFConfigSNSF.h"
#include "XSFPlayerSNSF.h"
#include "convert.h"

bool XSFConfigSNSF::initPlayInfinitely = false;
std::string XSFConfigSNSF::initSkipSilenceOnStartSec = "5";
std::string XSFConfigSNSF::initDetectSilenceSec = "5";
std::string XSFConfigSNSF::initDefaultLength = "1:55";
std::string XSFConfigSNSF::initDefaultFade = "5";
std::string XSFConfigSNSF::initTitleFormat = "%game%[ - [%disc%.]%track%] - %title%";
double XSFConfigSNSF::initVolume = 1.0;
VolumeTypeSNSF XSFConfigSNSF::initVolumeType = VOLUMETYPESNSF_REPLAYGAIN_ALBUM;
PeakTypeSNSF XSFConfigSNSF::initPeakType = PEAKTYPESNSF_REPLAYGAIN_TRACK;

XSFConfigSNSF::XSFConfigSNSF() : playInfinitely(false), skipSilenceOnStartSec(0), detectSilenceSec(0), defaultLength(0), defaultFade(0), volume(0.0), volumeType(VOLUMETYPESNSF_NONE), peakType(PEAKTYPESNSF_NONE),
	sampleRate(0), titleFormat(""),supportedSampleRates()
{
}

const std::string &XSFConfigSNSF::CommonNameWithVersion()
{
	static const auto commonNameWithVersion = XSFConfigSNSF::commonName + " v" + XSFConfigSNSF::versionNumber;
	return commonNameWithVersion;
}

void XSFConfigSNSF::LoadConfig()
{
    this->playInfinitely = false;//this->configIO->GetValue("PlayInfinitely", XSFConfigSNSF::initPlayInfinitely);
    this->skipSilenceOnStartSec = 5000;//ConvertFuncs::StringToMS(this->configIO->GetValue("SkipSilenceOnStartSec", XSFConfigSNSF::initSkipSilenceOnStartSec));
    this->detectSilenceSec = 5000;//ConvertFuncs::StringToMS(this->configIO->GetValue("DetectSilenceSec", XSFConfigSNSF::initDetectSilenceSec));
    this->defaultLength = 115000;//=ConvertFuncs::StringToMS(this->configIO->GetValue("DefaultLength", XSFConfigSNSF::initDefaultLength));
    this->defaultFade = 5000;//ConvertFuncs::StringToMS(this->configIO->GetValue("DefaultFade", XSFConfigSNSF::initDefaultFade));
    this->volume = 1.0;//this->configIO->GetValue("Volume", XSFConfigSNSF::initVolume);
    this->volumeType = VOLUMETYPESNSF_REPLAYGAIN_ALBUM;//static_cast<VolumeType>(this->configIO->GetValue("VolumeType", static_cast<int>(XSFConfigSNSF::initVolumeType)));
    this->peakType = PEAKTYPESNSF_REPLAYGAIN_TRACK;//static_cast<PeakType>(this->configIO->GetValue("PeakType", static_cast<int>(XSFConfigSNSF::initPeakType)));
    this->sampleRate = 44100;//this->configIO->GetValue("SampleRate", XSFConfigSNSF::initSampleRate);
    this->titleFormat = "%game%[ - [%disc%.]%track%] - %title%";//this->configIO->GetValue("TitleFormat", XSFConfigSNSF::initTitleFormat);

	this->LoadSpecificConfig();
}

void XSFConfigSNSF::SaveConfig()
{
	/*this->configIO->SetValue("PlayInfinitely", this->playInfinitely);
	this->configIO->SetValue("SkipSilenceOnStartSec", ConvertFuncs::MSToString(this->skipSilenceOnStartSec));
	this->configIO->SetValue("DetectSilenceSec", ConvertFuncs::MSToString(this->detectSilenceSec));
	this->configIO->SetValue("DefaultLength", ConvertFuncs::MSToString(this->defaultLength));
	this->configIO->SetValue("DefaultFade", ConvertFuncs::MSToString(this->defaultFade));
	this->configIO->SetValue("Volume", this->volume);
	this->configIO->SetValue("VolumeType", this->volumeType);
	this->configIO->SetValue("PeakType", this->peakType);
	this->configIO->SetValue("SampleRate", this->sampleRate);
	this->configIO->SetValue("TitleFormat", this->titleFormat);
*/
	this->SaveSpecificConfig();
}

extern XSFFileSNSF *xSFFileInInfo;


void XSFConfigSNSF::CopyConfigToMemory(XSFPlayerSNSF *xSFPlayer, bool preLoad)
{
	if (preLoad)
		xSFPlayer->SetSampleRate(this->sampleRate);

	this->CopySpecificConfigToMemory(xSFPlayer, preLoad);
}

bool XSFConfigSNSF::GetPlayInfinitely() const
{
	return this->playInfinitely;
}

unsigned long XSFConfigSNSF::GetSkipSilenceOnStartSec() const
{
	return this->skipSilenceOnStartSec;
}

unsigned long XSFConfigSNSF::GetDetectSilenceSec() const
{
	return this->detectSilenceSec;
}

unsigned long XSFConfigSNSF::GetDefaultLength() const
{
	return this->defaultLength;
}

unsigned long XSFConfigSNSF::GetDefaultFade() const
{
	return this->defaultFade;
}

double XSFConfigSNSF::GetVolume() const
{
	return this->volume;
}

VolumeTypeSNSF XSFConfigSNSF::GetVolumeType() const
{
	return this->volumeType;
}

PeakTypeSNSF XSFConfigSNSF::GetPeakType() const
{
	return this->peakType;
}

const std::string &XSFConfigSNSF::GetTitleFormat() const
{
	return this->titleFormat;
}
