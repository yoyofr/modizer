/*
 * xSF - SNSF configuration
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 *
 * Partially based on the vio*sf framework
 */

#include <bitset>
#include <sstream>
#include <string>
#include <cstddef>
#include <cstdint>
#include "windowsh_wrapper.h"
#include "XSFApp.h"
#include "XSFConfig.h"
#include "XSFConfig_SNSF.h"
#include "XSFConfigDialog_SNSF.h"
#include "convert.h"
#include "snes9x/apu/apu.h"

class wxWindow;
class XSFConfigDialog;
class XSFPlayer;

const unsigned XSFConfig::initSampleRate = 44100;
const std::string XSFConfig::commonName = "SNSF Decoder";
const std::string XSFConfig::versionNumber = "0.9b";

XSFApp *XSFApp::Create()
{
	return new XSFApp();
}

XSFConfig *XSFConfig::Create()
{
	return new XSFConfig_SNSF();
}

XSFConfig_SNSF::XSFConfig_SNSF() : XSFConfig(), separateEchoBuffer(false), interpolation(0), mutes()
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

void XSFConfig_SNSF::LoadSpecificConfig()
{
	this->separateEchoBuffer = this->configIO->GetValue("SeparateEchoBuffer", XSFConfig_SNSF::initSeparateEchoBuffer);
	this->interpolation = this->configIO->GetValue("Interpolation", XSFConfig_SNSF::initInterpolation);
	std::stringstream mutesSS(this->configIO->GetValue("Mutes", XSFConfig_SNSF::initMutes));
	mutesSS >> this->mutes;
}

void XSFConfig_SNSF::SaveSpecificConfig()
{
	this->configIO->SetValue("SeparateEchoBuffer", this->separateEchoBuffer);
	this->configIO->SetValue("Interpolation", this->interpolation);
	this->configIO->SetValue("Mutes", this->mutes.to_string<char>());
}

void XSFConfig_SNSF::InitializeSpecificConfigDialog(XSFConfigDialog *dialog)
{
	auto snsfDialog = static_cast<XSFConfigDialog_SNSF *>(dialog);
	snsfDialog->separateEchoBuffer = this->separateEchoBuffer;
	snsfDialog->interpolation = static_cast<int>(this->interpolation);
	for (std::size_t x = 0, numMutes = this->mutes.size(); x < numMutes; ++x)
		if (this->mutes[x])
			snsfDialog->mute.Add(x);
}

void XSFConfig_SNSF::ResetSpecificConfigDefaults(XSFConfigDialog *dialog)
{
	auto snsfDialog = static_cast<XSFConfigDialog_SNSF *>(dialog);
	snsfDialog->separateEchoBuffer = XSFConfig_SNSF::initSeparateEchoBuffer;
	snsfDialog->interpolation = XSFConfig_SNSF::initInterpolation;
	snsfDialog->mute.Clear();
	auto tmpMutes = std::bitset<8>(XSFConfig_SNSF::initMutes);
	for (std::size_t x = 0, numMutes = tmpMutes.size(); x < numMutes; ++x)
		if (tmpMutes[x])
			snsfDialog->mute.Add(x);
}

void XSFConfig_SNSF::SaveSpecificConfigDialog(XSFConfigDialog *dialog)
{
	auto snsfDialog = static_cast<XSFConfigDialog_SNSF *>(dialog);
	this->separateEchoBuffer = snsfDialog->separateEchoBuffer;
	this->interpolation = snsfDialog->interpolation;
	for (std::size_t x = 0, numMutes = this->mutes.size(); x < numMutes; ++x)
		this->mutes[x] = snsfDialog->mute.Index(x) != wxNOT_FOUND;
}

void XSFConfig_SNSF::CopySpecificConfigToMemory(XSFPlayer *, bool preLoad)
{
	if (preLoad)
	{
		memset(&Settings, 0, sizeof(Settings));
		Settings.SeparateEchoBuffer = this->separateEchoBuffer;
		Settings.InterpolationMethod = this->interpolation;
	}
	else
		S9xSetSoundControl(static_cast<std::uint8_t>(this->mutes.to_ulong()) ^ 0xFF);
}

void XSFConfig_SNSF::About(HWND parent)
{
	MessageBoxW(parent, ConvertFuncs::StringToWString(XSFConfig::commonName + " v" + XSFConfig::versionNumber + ", using xSF Winamp plugin framework (based on the vio*sf plugins) by Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]\n\n"
		"Utilizes modified snes9x v" VERSION " for audio playback.").c_str(), ConvertFuncs::StringToWString(XSFConfig::commonName + " v" + XSFConfig::versionNumber).c_str(), MB_OK);
}

XSFConfigDialog *XSFConfig_SNSF::CreateDialogBox(wxWindow *window, const std::string &title)
{
	return new XSFConfigDialog_SNSF(*this, window, title);
}
