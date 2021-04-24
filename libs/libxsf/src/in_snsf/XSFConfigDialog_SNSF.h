/*
 * xSF - SNSF configuration dialog
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 *
 * Partially based on the vio*sf framework
 */

#pragma once

#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wold-style-cast"
#elif defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wold-style-cast"
#endif
#include <wx/dynarray.h>
#ifdef __GNUC__
# pragma GCC diagnostic pop
#elif defined(__clang__)
# pragma clang diagnostic pop
#endif
#include "XSFConfigDialog.h"

class wxString;
class wxWindow;
class XSFConfig;

class XSFConfigDialog_SNSF : public XSFConfigDialog
{
public:
	XSFConfigDialog_SNSF(XSFConfig &newConfig, wxWindow *parent, const wxString &title);

	bool separateEchoBuffer;
	int interpolation;
	wxArrayInt mute;
};
