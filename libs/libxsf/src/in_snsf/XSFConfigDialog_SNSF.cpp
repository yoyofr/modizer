/*
 * xSF - SNSF configuration dialog
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 *
 * Partially based on the vio*sf framework
 */

#include <string>
#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wold-style-cast"
#elif defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wold-style-cast"
#endif
#include <wx/arrstr.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/gbsizer.h>
#include <wx/listbox.h>
#include <wx/stattext.h>
#include <wx/valgen.h>
#ifdef __GNUC__
# pragma GCC diagnostic pop
#elif defined(__clang__)
# pragma clang diagnostic pop
#endif
#include "XSFConfig.h"
#include "XSFConfigDialog.h"
#include "XSFConfigDialog_SNSF.h"

XSFConfigDialog_SNSF::XSFConfigDialog_SNSF(XSFConfig &newConfig, wxWindow *parent, const wxString &title) : XSFConfigDialog(newConfig, parent, title)
{
	auto separateEchoBufferCheckBox = new wxCheckBox(this->outputPanel, wxID_ANY, "Separate Echo Buffer", wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator{ &this->separateEchoBuffer });
	this->outputSizer->Add(separateEchoBufferCheckBox, { 4, 0 }, { 1, 2 }, wxALL, 5);

	auto interpolationLabel = new wxStaticText(this->outputPanel, wxID_ANY, "Interpolation");
	this->outputSizer->Add(interpolationLabel, { 5, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	wxArrayString interpolationChoices;
	interpolationChoices.Add("None");
	interpolationChoices.Add("Linear");
	interpolationChoices.Add("Guassian");
	interpolationChoices.Add("Cubic");
	interpolationChoices.Add("Sinc");
	auto interpolationChoice = new wxChoice(this->outputPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, interpolationChoices, 0, wxGenericValidator{ &this->interpolation });
	this->outputSizer->Add(interpolationChoice, { 5, 1 }, { 1, 1 }, wxALL, 5);

	auto muteLabel = new wxStaticText(this->outputPanel, wxID_ANY, "Mute");
	this->outputSizer->Add(muteLabel, { 6, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	wxArrayString muteChoices;
	for (int i = 0; i < 16; ++i)
		muteChoices.Add("BRRPCM " + std::to_string(i + 1));
	auto muteListBox = new wxListBox(this->outputPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, muteChoices, wxLB_MULTIPLE, wxGenericValidator{ &this->mute });
	this->outputSizer->Add(muteListBox, { 6, 1 }, { 1, 1 }, wxALL, 5);
}
