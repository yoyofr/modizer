/*
 * PatternClipboard.h
 * ------------------
 * Purpose: Implementation of the pattern clipboard mechanism
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "Snd_defs.h"
#include "PatternCursor.h"

OPENMPT_NAMESPACE_BEGIN

struct ModCommandPos;
class CSoundFile;

//===========================
class PatternClipboardElement
//===========================
{
public:
	
	std::string content;
	CString description;

public:

	PatternClipboardElement() { };
	PatternClipboardElement(const std::string &data, const CString &desc) : content(data), description(desc) { };
};


//====================
class PatternClipboard
//====================
{
	friend class PatternClipboardDialog;

public:

	enum PasteModes
	{
		pmOverwrite = 0,
		pmMixPaste,
		pmMixPasteIT,
		pmPasteFlood,
		pmPushForward,
	};

	// Clipboard index type
	typedef size_t clipindex_t;

protected:

	// The one and only pattern clipboard object.
	static PatternClipboard instance;

	// Active internal clipboard index
	clipindex_t activeClipboard;
	// Internal clipboard collection
	std::vector<PatternClipboardElement> clipboards;

public:

	// Copy a range of patterns to both the system clipboard and the internal clipboard.
	static bool Copy(CSoundFile &sndFile, ORDERINDEX first, ORDERINDEX last, bool onlyOrders);
	// Copy a pattern selection to both the system clipboard and the internal clipboard.
	static bool Copy(CSoundFile &sndFile, PATTERNINDEX pattern, PatternRect selection);
	// Try pasting a pattern selection from the system clipboard.
	static bool Paste(CSoundFile &sndFile, ModCommandPos &pastePos, PasteModes mode, ORDERINDEX curOrder, PatternRect &pasteRect, bool &orderChanged);
	// Try pasting a pattern selection from an internal clipboard.
	static bool Paste(CSoundFile &sndFile, ModCommandPos &pastePos, PasteModes mode, ORDERINDEX curOrder, PatternRect &pasteRect, clipindex_t internalClipboard, bool &orderChanged);
	// Copy one of the internal clipboards to the system clipboard.
	static bool SelectClipboard(clipindex_t which);
	// Switch to the next internal clipboard.
	static bool CycleForward();
	// Switch to the previous internal clipboard.
	static bool CycleBackward();
	// Set the maximum number of internal clipboards.
	static void SetClipboardSize(clipindex_t maxEntries);
	// Return the current number of clipboards.
	static clipindex_t GetClipboardSize() { return instance.clipboards.size(); };
	// Check whether patterns can be pasted from clipboard
	static bool CanPaste();

protected:

	PatternClipboard() : activeClipboard(0) { SetClipboardSize(1); };

	static std::string GetFileExtension(const char *ext, bool addPadding);

	// Create the clipboard text for a pattern selection
	static std::string CreateClipboardString(CSoundFile &sndFile, PATTERNINDEX pattern, PatternRect selection);

	// Parse clipboard string and perform the pasting operation.
	static bool HandlePaste(CSoundFile &sndFile, ModCommandPos &pastePos, PasteModes mode, const std::string &data, ORDERINDEX curOrder, PatternRect &pasteRect, bool &orderChanged);

	// System-specific clipboard functions
	static bool ToSystemClipboard(const PatternClipboardElement &clipboard) { return ToSystemClipboard(clipboard.content); };
	static bool ToSystemClipboard(const std::string &data);
	static bool FromSystemClipboard(std::string &data);
};


//===========================================
class PatternClipboardDialog : public CDialog
//===========================================
{
protected:

	// The one and only pattern clipboard dialog object
	static PatternClipboardDialog instance;

	// Edit class for clipboard name editing
	class CInlineEdit : public CEdit
	{
	protected:
		PatternClipboardDialog &parent;
	public:
		CInlineEdit(PatternClipboardDialog &dlg);

		DECLARE_MESSAGE_MAP();

		afx_msg void OnKillFocus(CWnd *newWnd);
	};

	CSpinButtonCtrl numClipboardsSpin;
	CListBox clipList;
	CInlineEdit editNameBox;
	int posX, posY;
	bool isLocked, isCreated;

public:

	static void UpdateList();
	static void Show();
	static void Toggle() { if(instance.isCreated) instance.OnCancel(); else instance.Show(); }

protected:

	PatternClipboardDialog();

	virtual void DoDataExchange(CDataExchange* pDX);
	DECLARE_MESSAGE_MAP();

	virtual void OnOK();
	virtual void OnCancel();
	
	afx_msg void OnNumClipboardsChanged();
	afx_msg void OnSelectClipboard();

	// Edit clipboard name
	afx_msg void OnEditName();
	void OnEndEdit(bool apply = true);
};

OPENMPT_NAMESPACE_END
