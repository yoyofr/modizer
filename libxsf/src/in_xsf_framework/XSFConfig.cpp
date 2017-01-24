/*
 * xSF - Core configuration handler
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-10-05
 *
 * Partially based on the vio*sf framework
 */

#include "XSFConfig.h"
#include "XSFPlayer.h"
#include "convert.h"

bool XSFConfig::initPlayInfinitely = false;
std::string XSFConfig::initSkipSilenceOnStartSec = "5";
std::string XSFConfig::initDetectSilenceSec = "5";
std::string XSFConfig::initDefaultLength = "1:55";
std::string XSFConfig::initDefaultFade = "5";
std::string XSFConfig::initTitleFormat = "%game%[ - [%disc%.]%track%] - %title%";
double XSFConfig::initVolume = 1.0;
VolumeType XSFConfig::initVolumeType = VOLUMETYPE_REPLAYGAIN_ALBUM;
PeakType XSFConfig::initPeakType = PEAKTYPE_REPLAYGAIN_TRACK;

XSFConfig::XSFConfig() : playInfinitely(false), skipSilenceOnStartSec(0), detectSilenceSec(0), defaultLength(0), defaultFade(0), volume(0.0), volumeType(VOLUMETYPE_NONE), peakType(PEAKTYPE_NONE),
	sampleRate(0), titleFormat(""),supportedSampleRates()
{
}

const std::string &XSFConfig::CommonNameWithVersion()
{
	static const auto commonNameWithVersion = XSFConfig::commonName + " v" + XSFConfig::versionNumber;
	return commonNameWithVersion;
}

void XSFConfig::LoadConfig()
{
    this->playInfinitely = false;//this->configIO->GetValue("PlayInfinitely", XSFConfig::initPlayInfinitely);
    this->skipSilenceOnStartSec = 5000;//ConvertFuncs::StringToMS(this->configIO->GetValue("SkipSilenceOnStartSec", XSFConfig::initSkipSilenceOnStartSec));
    this->detectSilenceSec = 5000;//ConvertFuncs::StringToMS(this->configIO->GetValue("DetectSilenceSec", XSFConfig::initDetectSilenceSec));
    this->defaultLength = 115000;//=ConvertFuncs::StringToMS(this->configIO->GetValue("DefaultLength", XSFConfig::initDefaultLength));
    this->defaultFade = 5000;//ConvertFuncs::StringToMS(this->configIO->GetValue("DefaultFade", XSFConfig::initDefaultFade));
    this->volume = 1.0;//this->configIO->GetValue("Volume", XSFConfig::initVolume);
    this->volumeType = VOLUMETYPE_REPLAYGAIN_ALBUM;//static_cast<VolumeType>(this->configIO->GetValue("VolumeType", static_cast<int>(XSFConfig::initVolumeType)));
    this->peakType = PEAKTYPE_REPLAYGAIN_TRACK;//static_cast<PeakType>(this->configIO->GetValue("PeakType", static_cast<int>(XSFConfig::initPeakType)));
    this->sampleRate = 44100;//this->configIO->GetValue("SampleRate", XSFConfig::initSampleRate);
    this->titleFormat = "%game%[ - [%disc%.]%track%] - %title%";//this->configIO->GetValue("TitleFormat", XSFConfig::initTitleFormat);

	this->LoadSpecificConfig();
}

void XSFConfig::SaveConfig()
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

extern XSFFile *xSFFileInInfo;


void XSFConfig::CopyConfigToMemory(XSFPlayer *xSFPlayer, bool preLoad)
{
	if (preLoad)
		xSFPlayer->SetSampleRate(this->sampleRate);

	this->CopySpecificConfigToMemory(xSFPlayer, preLoad);
}

bool XSFConfig::GetPlayInfinitely() const
{
	return this->playInfinitely;
}

unsigned long XSFConfig::GetSkipSilenceOnStartSec() const
{
	return this->skipSilenceOnStartSec;
}

unsigned long XSFConfig::GetDetectSilenceSec() const
{
	return this->detectSilenceSec;
}

unsigned long XSFConfig::GetDefaultLength() const
{
	return this->defaultLength;
}

unsigned long XSFConfig::GetDefaultFade() const
{
	return this->defaultFade;
}

double XSFConfig::GetVolume() const
{
	return this->volume;
}

VolumeType XSFConfig::GetVolumeType() const
{
	return this->volumeType;
}

PeakType XSFConfig::GetPeakType() const
{
	return this->peakType;
}

const std::string &XSFConfig::GetTitleFormat() const
{
	return this->titleFormat;
}
