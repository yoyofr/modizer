/*
 * xSF - SNSF configuration
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-09-24
 *
 * Partially based on the vio*sf framework
 *
 * NOTE: 16-bit sound is always enabled, the player is currently limited to
 * creating a 16-bit PCM for Winamp and can not handle 8-bit sound from
 * snes9x.
 */

#include "XSFConfig_SNSF.h"
#include "convert.h"
#include "snes9x/apu/apu.h"

enum
{
	idSixteenBitSound = 1000,
	idReverseStereo,
	idResampler,
	idMutes
};

unsigned XSFConfig::initSampleRate = 44100;
std::string XSFConfig::commonName = "SNSF Decoder";
std::string XSFConfig::versionNumber = "0.9b";
//bool XSFConfig_SNSF::initSixteenBitSound = true;
bool XSFConfig_SNSF::initReverseStereo = false;
unsigned XSFConfig_SNSF::initResampler = 1;
std::string XSFConfig_SNSF::initMutes = "00000000";

XSFConfig *XSFConfig::Create()
{
	return new XSFConfig_SNSF();
}

XSFConfig_SNSF::XSFConfig_SNSF() : XSFConfig(), /*sixteenBitSound(false), */reverseStereo(false), mutes(), resampler(0)
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
	//this->sixteenBitSound = this->configIO->GetValue("SixteenBitSound", XSFConfig_SNSF::initSixteenBitSound);
	this->reverseStereo = this->configIO->GetValue("ReverseStereo", XSFConfig_SNSF::initReverseStereo);
	this->resampler = this->configIO->GetValue("Resampler", XSFConfig_SNSF::initResampler);
	std::stringstream mutesSS(this->configIO->GetValue("Mutes", XSFConfig_SNSF::initMutes));
	mutesSS >> this->mutes;
}

void XSFConfig_SNSF::SaveSpecificConfig()
{
	//this->configIO->SetValue("SixteenBitSound", this->sixteenBitSound);
	this->configIO->SetValue("ReverseStereo", this->reverseStereo);
	this->configIO->SetValue("Resampler", this->resampler);
	this->configIO->SetValue("Mutes", this->mutes.to_string<char>());
}

void XSFConfig_SNSF::GenerateSpecificDialogs()
{
	/*this->configDialog.AddCheckBoxControl(DialogCheckBoxBuilder(L"Sixteen-Bit Sound").WithSize(80, 10).InGroup(L"Output").WithRelativePositionToSibling(RelativePosition::FROM_BOTTOMLEFT, Point<short>(0, 7), 2).WithTabStop().
		WithID(idSixteenBitSound));*/
	this->configDialog.AddCheckBoxControl(DialogCheckBoxBuilder(L"Reverse Stereo").WithSize(80, 10).InGroup(L"Output").WithRelativePositionToSibling(RelativePosition::FROM_BOTTOMLEFT, Point<short>(0, 7), 2).WithTabStop().
		WithID(idReverseStereo));
	this->configDialog.AddLabelControl(DialogLabelBuilder(L"Resampler").WithSize(50, 8).InGroup(L"Output").WithRelativePositionToSibling(RelativePosition::FROM_BOTTOMLEFT, Point<short>(0, 10)).IsLeftJustified());
	this->configDialog.AddComboBoxControl(DialogComboBoxBuilder().WithSize(78, 14).InGroup(L"Output").WithRelativePositionToSibling(RelativePosition::FROM_TOPRIGHT, Point<short>(5, -3)).WithTabStop().IsDropDownList().
		WithID(idResampler));
	this->configDialog.AddLabelControl(DialogLabelBuilder(L"Mute").WithSize(50, 8).InGroup(L"Output").WithRelativePositionToSibling(RelativePosition::FROM_BOTTOMLEFT, Point<short>(0, 10), 2).IsLeftJustified());
	this->configDialog.AddListBoxControl(DialogListBoxBuilder().WithSize(78, 45).WithExactHeight().InGroup(L"Output").WithRelativePositionToSibling(RelativePosition::FROM_TOPRIGHT, Point<short>(5, -3)).WithID(idMutes).WithBorder().
		WithVerticalScrollbar().WithMultipleSelect().WithTabStop());
}

INT_PTR CALLBACK XSFConfig_SNSF::ConfigDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			// Sixteen-Bit Sound
			/*if (this->sixteenBitSound)
				SendMessageW(GetDlgItem(hwndDlg, idSixteenBitSound), BM_SETCHECK, BST_CHECKED, 0);*/
			// Reverse Stereo
			if (this->reverseStereo)
				SendMessageW(GetDlgItem(hwndDlg, idReverseStereo), BM_SETCHECK, BST_CHECKED, 0);
			// Resampler
			SendMessageW(GetDlgItem(hwndDlg, idResampler), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Linear Resampler"));
			SendMessageW(GetDlgItem(hwndDlg, idResampler), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Hermite Resampler"));
			SendMessageW(GetDlgItem(hwndDlg, idResampler), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Bspline Resampler"));
			SendMessageW(GetDlgItem(hwndDlg, idResampler), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Osculating Resampler"));
			SendMessageW(GetDlgItem(hwndDlg, idResampler), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Sinc Resampler"));
			SendMessageW(GetDlgItem(hwndDlg, idResampler), CB_SETCURSEL, this->resampler, 0);
			// Mutes
			for (int x = 0, numMutes = this->mutes.size(); x < numMutes; ++x)
			{
				SendMessageW(GetDlgItem(hwndDlg, idMutes), LB_ADDSTRING, 0, reinterpret_cast<LPARAM>((L"BRRPCM " + wstringify(x + 1)).c_str()));
				SendMessageW(GetDlgItem(hwndDlg, idMutes), LB_SETSEL, this->mutes[x], x);
			}
			break;
		case WM_COMMAND:
			break;
	}

	return XSFConfig::ConfigDialogProc(hwndDlg, uMsg, wParam, lParam);
}

void XSFConfig_SNSF::ResetSpecificConfigDefaults(HWND hwndDlg)
{
	//SendMessageW(GetDlgItem(hwndDlg, idSixteenBitSound), BM_SETCHECK, XSFConfig_SNSF::initSixteenBitSound ? BST_CHECKED : BST_UNCHECKED, 0);
	SendMessageW(GetDlgItem(hwndDlg, idReverseStereo), BM_SETCHECK, XSFConfig_SNSF::initReverseStereo ? BST_CHECKED : BST_UNCHECKED, 0);
	SendMessageW(GetDlgItem(hwndDlg, idResampler), CB_SETCURSEL, XSFConfig_SNSF::initResampler, 0);
	auto tmpMutes = std::bitset<8>(XSFConfig_SNSF::initMutes);
	for (int x = 0, numMutes = tmpMutes.size(); x < numMutes; ++x)
		SendMessageW(GetDlgItem(hwndDlg, idMutes), LB_SETSEL, tmpMutes[x], x);
}

void XSFConfig_SNSF::SaveSpecificConfigDialog(HWND hwndDlg)
{
	//this->sixteenBitSound = SendMessageW(GetDlgItem(hwndDlg, idSixteenBitSound), BM_GETCHECK, 0, 0) == BST_CHECKED;
	this->reverseStereo = SendMessageW(GetDlgItem(hwndDlg, idReverseStereo), BM_GETCHECK, 0, 0) == BST_CHECKED;
	this->resampler = static_cast<unsigned>(SendMessageW(GetDlgItem(hwndDlg, idResampler), CB_GETCURSEL, 0, 0));
	for (int x = 0, numMutes = this->mutes.size(); x < numMutes; ++x)
		this->mutes[x] = !!SendMessageW(GetDlgItem(hwndDlg, idMutes), LB_GETSEL, x, 0);
}

void XSFConfig_SNSF::CopySpecificConfigToMemory(XSFPlayer *, bool preLoad)
{
	if (preLoad)
	{
		memset(&Settings, 0, sizeof(Settings));
		//Settings.SixteenBitSound = this->sixteenBitSound;
		Settings.ReverseStereo = this->reverseStereo;
	}
	else
		S9xSetSoundControl(static_cast<uint8_t>(this->mutes.to_ulong()) ^ 0xFF);
}

void XSFConfig_SNSF::About(HWND parent)
{
	MessageBoxW(parent, ConvertFuncs::StringToWString(XSFConfig::commonName + " v" + XSFConfig::versionNumber + ", using xSF Winamp plugin framework (based on the vio*sf plugins) by Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]\n\n"
		"Utilizes modified snes9x v1.53 for audio playback.").c_str(), ConvertFuncs::StringToWString(XSFConfig::commonName + " v" + XSFConfig::versionNumber).c_str(), MB_OK);
}
