/*
 * xSF - SNSF configuration
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 *
 * Partially based on the vio*sf framework
 */

#pragma once

#include <bitset>
#include <string>
#include "windowsh_wrapper.h"
#include "XSFConfig.h"

class wxWindow;
class XSFConfigDialog;
class XSFPlayer;

class XSFConfig_SNSF : public XSFConfig
{
protected:
	static constexpr bool initSeparateEchoBuffer = false;
	static constexpr unsigned initInterpolation = 2;
	inline static const std::string initMutes = "00000000";

	friend class XSFConfig;
	bool separateEchoBuffer;
	unsigned interpolation;
	std::bitset<8> mutes;

	XSFConfig_SNSF();
	void LoadSpecificConfig() override;
	void SaveSpecificConfig() override;
	void InitializeSpecificConfigDialog(XSFConfigDialog *dialog) override;
	void ResetSpecificConfigDefaults(XSFConfigDialog *dialog) override;
	void SaveSpecificConfigDialog(XSFConfigDialog *dialog) override;
	void CopySpecificConfigToMemory(XSFPlayer *xSFPlayer, bool preLoad) override;
public:
	void About(HWND parent) override;
	XSFConfigDialog *CreateDialogBox(wxWindow *window, const std::string &title) override;
};
