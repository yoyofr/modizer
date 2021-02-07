/*
 * Windows Dynamic Dialog Builder framework
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-09-24
 */

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <cstdint>
#include "XSFCommon.h"
#include "windowsh_wrapper.h"

template<typename T> struct Point
{
	T x, y;

	Point() : x(), y() { }
	Point(T X, T Y) : x(X), y(Y) { }
};

template<typename T> struct Size
{
	T width, height;

	Size() : width(), height() { }
	Size(T Width, T Height) : width(Width), height(Height) { }
};

template<typename T> struct Rect
{
	Point<T> position;
	Size<T> size;

	Rect() : position(), size() { }
	Rect(const Point<T> &Position, const Size<T> &Sz) : position(Position), size(Sz) { }
	Rect(T X, T Y, T Width, T Height) : position(X, Y), size(Width, Height) { }
};

class RelativePosition
{
public:
	Point<short> relativePosition;
	enum BaseType
	{
		TO_PARENT,
		TO_SIBLING
	} type;
	enum PositionType
	{
		FROM_TOP,
		FROM_BOTTOM,
		FROM_LEFT,
		FROM_RIGHT,
		FROM_TOPLEFT,
		FROM_BOTTOMLEFT,
		FROM_TOPRIGHT,
		FROM_BOTTOMRIGHT
	} positionType;

	RelativePosition(const Point<short> &RelPosition, BaseType Type, PositionType PosType) : relativePosition(RelPosition), type(Type), positionType(PosType) { }
	virtual ~RelativePosition() { }
	virtual RelativePosition *Clone() const = 0;
	Point<short> CalculatePosition(const Rect<short> &child, const Rect<short> &other);
};

class RelativePositionToParent : public RelativePosition
{
public:
	RelativePositionToParent(const Point<short> &RelPosition, PositionType PosType) : RelativePosition(RelPosition, TO_PARENT, PosType) { }
	RelativePositionToParent *Clone() const { return new RelativePositionToParent(this->relativePosition, this->positionType); }
};

class RelativePositionToSibling : public RelativePosition
{
public:
	short siblingsBack;

	RelativePositionToSibling(const Point<short> &RelPosition, PositionType PosType, short SiblingsBack = 1) : RelativePosition(RelPosition, TO_SIBLING, PosType), siblingsBack(SiblingsBack) { }
	RelativePositionToSibling *Clone() const { return new RelativePositionToSibling(this->relativePosition, this->positionType, this->siblingsBack); }
};

enum DialogControlType
{
	NO_CONTROL,
	GROUP_CONTROL,
	EDITBOX_CONTROL,
	LABEL_CONTROL,
	CHECKBOX_CONTROL,
	BUTTON_CONTROL,
	LISTBOX_CONTROL,
	COMBOBOX_CONTROL
};

class DialogTemplate;

class DialogBuilder
{
protected:
	friend class DialogTemplate;
	std::wstring title, fontName;
	uint32_t style, exstyle;
	uint16_t fontSizeInPts;
	Size<short> size;
	bool resetControls;
public:
	DialogBuilder() : title(L""), fontName(L""), style(0), exstyle(0), fontSizeInPts(0), size(), resetControls(false) { }
	DialogBuilder &WithTitle(const std::wstring &Title) { this->title = Title; return *this; }
	DialogBuilder &WithFont(const std::wstring &FontName, uint16_t FontSizeInPts) { this->fontName = FontName; this->fontSizeInPts = FontSizeInPts; return *this; }
	DialogBuilder &WithSize(short Width, short Height) { this->size.width = Width; this->size.height = Height; return *this; }
	DialogBuilder &WithSize(const Size<short> &Sz) { this->size = Sz; return *this; }
	DialogBuilder &ResetControls(bool Reset = true) { this->resetControls = Reset; return *this; }
	DialogBuilder &IsOverlapped() { this->style &= ~(WS_OVERLAPPED | WS_POPUP | WS_CHILD); this->style |= WS_OVERLAPPED; return *this; }
	DialogBuilder &IsPopup() { this->style &= ~(WS_OVERLAPPED | WS_POPUP | WS_CHILD); this->style |= WS_POPUP; return *this; }
	DialogBuilder &IsChild() { this->style &= ~(WS_OVERLAPPED | WS_POPUP | WS_CHILD); this->style |= WS_CHILD; return *this; }
	DialogBuilder &WithBorder(bool Border = true) { if (Border) this->style |= WS_BORDER; else this->style &= ~WS_BORDER; return *this; }
	DialogBuilder &WithDialogFrame(bool DialogFrame = true) { if (DialogFrame) this->style |= WS_DLGFRAME; else this->style &= ~WS_DLGFRAME; return *this; }
	DialogBuilder &WithVerticalScrollbar(bool VerticalScrollbar = true) { if (VerticalScrollbar) this->style |= WS_VSCROLL; else this->style &= ~WS_VSCROLL; return *this; }
	DialogBuilder &WithHorizontalScrollbar(bool HorizontalScrollbar = true) { if (HorizontalScrollbar) this->style |= WS_HSCROLL; else this->style &= ~WS_HSCROLL; return *this; }
	DialogBuilder &WithSystemMenu(bool SystemMenu = true) { if (SystemMenu) this->style |= WS_SYSMENU; else this->style &= ~WS_SYSMENU; return *this; }
	DialogBuilder &WithSizingBorder(bool SizingBorder = true) { if (SizingBorder) this->style |= WS_THICKFRAME; else this->style &= ~WS_THICKFRAME; return *this; }
	DialogBuilder &WithMinimizeBox(bool MinimizeBox = true) { if (MinimizeBox) this->style |= WS_MINIMIZEBOX; else this->style &= ~WS_MINIMIZEBOX; return *this; }
	DialogBuilder &WithMaximizeBox(bool MaximizeBox = true) { if (MaximizeBox) this->style |= WS_MAXIMIZEBOX; else this->style &= ~WS_MAXIMIZEBOX; return *this; }
	DialogBuilder &WithDialogModalFrame(bool DialogModalFrame = true) { if (DialogModalFrame) this->exstyle |= WS_EX_DLGMODALFRAME; else this->exstyle &= ~WS_EX_DLGMODALFRAME; return *this; }
#if WINVER >= 0x0400
	DialogBuilder &WithRaisedEdge(bool RaisedEdge = true) { if (RaisedEdge) this->exstyle |= WS_EX_WINDOWEDGE; else this->exstyle &= ~WS_EX_WINDOWEDGE; return *this; }
	DialogBuilder &WithSunkenEdge(bool SunkenEdge = true) { if (SunkenEdge) this->exstyle |= WS_EX_CLIENTEDGE; else this->exstyle &= ~WS_EX_CLIENTEDGE; return *this; }
	DialogBuilder &WithContextHelp(bool ContextHelp = true) { if (ContextHelp) this->exstyle |= WS_EX_CONTEXTHELP; else this->exstyle &= ~WS_EX_CONTEXTHELP; return *this; }
	DialogBuilder &IsControlWindow(bool ControlWindow = true) { if (ControlWindow) this->style |= DS_CONTROL; else this->style &= ~DS_CONTROL; return *this; }
	DialogBuilder &IsCentered(bool Centered = true) { if (Centered) this->style |= DS_CENTER; else this->style &= ~DS_CENTER; return *this; }
#endif
};

template<class T> class DialogControlBuilder
{
protected:
	friend class DialogTemplate;
	DialogControlType controlType;
	uint32_t style, exstyle;
	Rect<short> rect;
	short id;
	int index;
	std::unique_ptr<RelativePosition> relativePosition;

	T &me() { return dynamic_cast<T &>(*this); }
public:
	DialogControlBuilder(DialogControlType Type = NO_CONTROL) : controlType(Type), style(0), exstyle(0), rect(), id(-1), index(-1), relativePosition() { }
	virtual ~DialogControlBuilder() { }
	T &WithPosition(short X, short Y) { this->rect.position.x = X; this->rect.position.y = Y; return this->me(); }
	T &WithPosition(const Point<short> &Position) { this->rect.position = Position; return this->me(); }
	T &WithRelativePositionToParent(RelativePosition::PositionType PosType, const Point<short> &RelativePosition)
	{
		this->relativePosition.reset(new RelativePositionToParent(RelativePosition, PosType));
		return this->me();
	}
	T &WithRelativePositionToSibling(RelativePosition::PositionType PosType, const Point<short> &RelativePosition, short SiblingsBack = 1)
	{
		this->relativePosition.reset(new RelativePositionToSibling(RelativePosition, PosType, SiblingsBack));
		return this->me();
	}
	T &WithSize(short Width, short Height) { this->rect.size.width = Width; this->rect.size.height = Height; return this->me(); }
	T &WithSize(const Size<short> &Sz) { this->rect.size = Sz; return this->me(); }
	T &WithID(short ID) { this->id = ID; return this->me(); }
	T &AtIndex(int Index) { this->index = Index; return this->me(); }
	T &IsDisabled(bool Disabled = true) { if (Disabled) this->style |= WS_DISABLED; else this->style &= ~WS_DISABLED; return this->me(); }
	T &WithBorder(bool ThinBorder = true) { if (ThinBorder) this->style |= WS_BORDER; else this->style &= ~WS_BORDER; return this->me(); }
	T &WithVerticalScrollbar(bool VerticalScrollbar = true) { if (VerticalScrollbar) this->style |= WS_VSCROLL; else this->style &= ~WS_VSCROLL; return this->me(); }
	T &WithHorizontalScrollbar(bool HorizontalScrollbar = true) { if (HorizontalScrollbar) this->style |= WS_HSCROLL; else this->style &= ~WS_HSCROLL; return this->me(); }
	T &WithTabStop(bool TabStop = true) { if (TabStop) this->style |= WS_TABSTOP; else this->style &= ~WS_TABSTOP; return this->me(); }
#if WINVER >= 0x0400
	T &WithRaisedEdge(bool RaisedEdge = true) { if (RaisedEdge) this->exstyle |= WS_EX_WINDOWEDGE; else this->exstyle &= ~WS_EX_WINDOWEDGE; return this->me(); }
	T &WithSunkenEdge(bool SunkenEdge = true) { if (SunkenEdge) this->exstyle |= WS_EX_CLIENTEDGE; else this->exstyle &= ~WS_EX_CLIENTEDGE; return this->me(); }
#endif
};

class DialogGroupBuilder : public DialogControlBuilder<DialogGroupBuilder>
{
protected:
	friend class DialogTemplate;
	std::wstring groupName;
public:
	DialogGroupBuilder(const std::wstring &newGroupName) : DialogControlBuilder(GROUP_CONTROL), groupName(newGroupName) { }
};

template<class T> class DialogInGroupBuilder : public DialogControlBuilder<T>
{
protected:
	friend class DialogTemplate;
	std::wstring groupName;
public:
	DialogInGroupBuilder(DialogControlType Type) : DialogControlBuilder<T>(Type), groupName(L"") { }
	T &InGroup(const std::wstring &GroupName) { this->groupName = GroupName; return this->me(); }
};

class DialogEditBoxBuilder : public DialogInGroupBuilder<DialogEditBoxBuilder>
{
protected:
	friend class DialogTemplate;
public:
	DialogEditBoxBuilder() : DialogInGroupBuilder(EDITBOX_CONTROL) { }
	DialogEditBoxBuilder &IsLeftJustified() { this->style &= ~(ES_LEFT | ES_CENTER | ES_RIGHT); this->style |= ES_LEFT; return this->me(); }
	DialogEditBoxBuilder &IsCenterJustified() { this->style &= ~(ES_LEFT | ES_CENTER | ES_RIGHT); this->style |= ES_CENTER; return this->me(); }
	DialogEditBoxBuilder &IsRightJustified() { this->style &= ~(ES_LEFT | ES_CENTER | ES_RIGHT); this->style |= ES_RIGHT; return this->me(); }
	DialogEditBoxBuilder &IsMultiline(bool Multiline = true) { if (Multiline) this->style |= ES_MULTILINE; else this->style &= ~ES_MULTILINE; return this->me(); }
	DialogEditBoxBuilder &WithAllUppercase(bool AllUppercase = true) { if (AllUppercase) this->style |= ES_UPPERCASE; else this->style &= ~ES_UPPERCASE; return this->me(); }
	DialogEditBoxBuilder &WithAllLowercase(bool AllLowercase = true) { if (AllLowercase) this->style |= ES_LOWERCASE; else this->style &= ~ES_LOWERCASE; return this->me(); }
	DialogEditBoxBuilder &IsPasswordInput(bool PasswordInput = true) { if (PasswordInput) this->style |= ES_PASSWORD; else this->style &= ~ES_PASSWORD; return this->me(); }
	DialogEditBoxBuilder &WithAutoVScroll(bool AutoVScroll = true) { if (AutoVScroll) this->style |= ES_AUTOVSCROLL; else this->style &= ~ES_AUTOVSCROLL; return this->me(); }
	DialogEditBoxBuilder &WithAutoHScroll(bool AutoHScroll = true) { if (AutoHScroll) this->style |= ES_AUTOHSCROLL; else this->style &= ~ES_AUTOHSCROLL; return this->me(); }
	DialogEditBoxBuilder &WithDontHideSelection(bool DontHideSelection = true) { if (DontHideSelection) this->style |= ES_NOHIDESEL; else this->style &= ~ES_NOHIDESEL; return this->me(); }
	DialogEditBoxBuilder &IsReadOnly(bool ReadOnly = true) { if (ReadOnly) this->style |= ES_READONLY; else this->style &= ~ES_READONLY; return this->me(); }
	DialogEditBoxBuilder &WithWantReturn(bool WantReturn = true) { if (WantReturn) this->style |= ES_WANTRETURN; else this->style &= ~ES_WANTRETURN; return this->me(); }
};

template<class T> class DialogControlWithLabelBuilder : public DialogInGroupBuilder<T>
{
protected:
	friend class DialogTemplate;
	std::wstring label;
public:
	DialogControlWithLabelBuilder(DialogControlType Type, const std::wstring &Label) : DialogInGroupBuilder<T>(Type), label(Label) { }
};

class DialogLabelBuilder : public DialogControlWithLabelBuilder<DialogLabelBuilder>
{
protected:
	friend class DialogTemplate;
public:
	DialogLabelBuilder(const std::wstring &Label) : DialogControlWithLabelBuilder(LABEL_CONTROL, Label) { }
	DialogLabelBuilder &IsLeftJustified(bool WithWordWrap = false)
	{
		this->style &= ~(SS_LEFT | SS_CENTER | SS_RIGHT | SS_LEFTNOWORDWRAP);
		if (WithWordWrap)
			this->style |= SS_LEFTNOWORDWRAP;
		else
			this->style |= SS_LEFT;
		return this->me();
	}
	DialogLabelBuilder &IsCenterJustified() { this->style &= ~(SS_LEFT | SS_CENTER | SS_RIGHT | SS_LEFTNOWORDWRAP); this->style |= SS_CENTER; return this->me(); }
	DialogLabelBuilder &IsRightJustified() { this->style &= ~(SS_LEFT | SS_CENTER | SS_RIGHT | SS_LEFTNOWORDWRAP); this->style |= SS_RIGHT; return this->me(); }
	DialogLabelBuilder &WithAmpsNotTranslated(bool AmpsNotTranslated = true) { if (AmpsNotTranslated) this->style |= SS_NOPREFIX; else this->style &= ~SS_NOPREFIX; return this->me(); }
#if WINVER >= 0x0400
	DialogLabelBuilder &WithNotify(bool Notify = true) { if (Notify) this->style |= SS_NOTIFY; else this->style &= ~SS_NOTIFY; return this->me(); }
#endif
};

template<class T> class DialogButtonBaseBuilder : public DialogControlWithLabelBuilder<T>
{
protected:
	friend class DialogTemplate;
public:
	DialogButtonBaseBuilder(DialogControlType Type, const std::wstring &Label) : DialogControlWithLabelBuilder<T>(Type, Label) { }
#if WINVER >= 0x0400
	T &IsLeftJustified(bool LeftJustified = true)
	{
		if (LeftJustified)
		{
			this->style |= BS_LEFT;
			this->style &= ~BS_RIGHT;
		}
		else
			this->style &= ~BS_LEFT;
		return this->me();
	}
	T &IsRightJustified(bool RightJustified = true)
	{
		if (RightJustified)
		{
			this->style |= BS_RIGHT;
			this->style &= ~BS_LEFT;
		}
		else
			this->style &= ~BS_RIGHT;
		return this->me();
	}
	T &IsCenterJustified(bool CenterJustified = true) { if (CenterJustified) this->style |= BS_CENTER; else this->style &= ~BS_CENTER; return this->me(); }
	T &WithVerticalTop(bool VerticalTop = true)
	{
		if (VerticalTop)
		{
			this->style |= BS_TOP;
			this->style &= ~BS_BOTTOM;
		}
		else
			this->style &= ~BS_TOP;
		return this->me();
	}
	T &WithVerticalBottom(bool VerticalBottom = true)
	{
		if (VerticalBottom)
		{
			this->style |= BS_BOTTOM;
			this->style &= ~BS_TOP;
		}
		else
			this->style &= ~BS_BOTTOM;
		return this->me();
	}
	T &WithVerticalCenter(bool VerticalCenter = true) { if (VerticalCenter) this->style |= BS_VCENTER; else this->style &= ~BS_VCENTER; return this->me(); }
	T &IsMultiline(bool Multiline = true) { if (Multiline) this->style |= BS_MULTILINE; else this->style &= ~BS_MULTILINE; return this->me(); }
	T &WithNotify(bool Notify = true) { if (Notify) this->style |= BS_NOTIFY; else this->style &= ~BS_NOTIFY; return this->me(); }
	T &IsFlat(bool Flat = true) { if (Flat) this->style |= BS_FLAT; else this->style &= ~BS_FLAT; return this->me(); }
#endif
};

class DialogButtonBuilder : public DialogButtonBaseBuilder<DialogButtonBuilder>
{
protected:
	friend class DialogTemplate;
public:
	DialogButtonBuilder(const std::wstring &Label) : DialogButtonBaseBuilder(BUTTON_CONTROL, Label) { }
	DialogButtonBuilder &IsDefault(bool Default = true) { if (Default) this->style |= BS_DEFPUSHBUTTON; else this->style &= ~BS_DEFPUSHBUTTON; return this->me(); }
};

class DialogCheckBoxBuilder : public DialogButtonBaseBuilder<DialogCheckBoxBuilder>
{
protected:
	friend class DialogTemplate;
public:
	DialogCheckBoxBuilder(const std::wstring &Label) : DialogButtonBaseBuilder(CHECKBOX_CONTROL, Label) { this->style |= BS_AUTOCHECKBOX; }
	DialogCheckBoxBuilder &WithTextOnLeft(bool TextOnLeft = true) { if (TextOnLeft) this->style |= BS_LEFTTEXT; else this->style &= ~BS_LEFTTEXT; return this->me(); }
	DialogCheckBoxBuilder &LikePushButton(bool PushButton = true) { if (PushButton) this->style |= BS_PUSHLIKE; else this->style &= ~BS_PUSHLIKE; return this->me(); }
};

class DialogListBoxBuilder : public DialogInGroupBuilder<DialogListBoxBuilder>
{
protected:
	friend class DialogTemplate;
public:
	DialogListBoxBuilder() : DialogInGroupBuilder(LISTBOX_CONTROL) { }
	DialogListBoxBuilder &WithNotify(bool Notify = true) { if (Notify) this->style |= LBS_NOTIFY; else this->style &= ~LBS_NOTIFY; return this->me(); }
	DialogListBoxBuilder &WithSort(bool Sort = true) { if (Sort) this->style |= LBS_SORT; else this->style &= ~LBS_SORT; return this->me(); }
	DialogListBoxBuilder &WithMultipleSelect(bool MultipleSelect = true) { if (MultipleSelect) this->style |= LBS_MULTIPLESEL; else this->style &= ~LBS_MULTIPLESEL; return this->me(); }
	DialogListBoxBuilder &WithExactHeight(bool ExactHeight = true) { if (ExactHeight) this->style |= LBS_NOINTEGRALHEIGHT; else this->style &= ~LBS_NOINTEGRALHEIGHT; return this->me(); }
	DialogListBoxBuilder &WithMultipleColumns(bool MultipleColumns = true) { if (MultipleColumns) this->style |= LBS_MULTICOLUMN; else this->style &= ~LBS_MULTICOLUMN; return this->me(); }
	DialogListBoxBuilder &WithExtendedSelect(bool ExtendedSelect = true) { if (ExtendedSelect) this->style |= LBS_EXTENDEDSEL; else this->style &= ~LBS_EXTENDEDSEL; return this->me(); }
	DialogListBoxBuilder &WithDisabledNoScroll(bool DisabledNoScroll = true) { if (DisabledNoScroll) this->style |= LBS_DISABLENOSCROLL; else this->style &= ~LBS_DISABLENOSCROLL; return this->me(); }
#if WINVER >= 0x0400
	DialogListBoxBuilder &WithNoSelect(bool NoSelect = true) { if (NoSelect) this->style |= LBS_NOSEL; else this->style &= ~LBS_NOSEL; return this->me(); }
#endif
};

class DialogComboBoxBuilder : public DialogInGroupBuilder<DialogComboBoxBuilder>
{
protected:
	friend class DialogTemplate;
public:
	DialogComboBoxBuilder() : DialogInGroupBuilder(COMBOBOX_CONTROL) { }
	DialogComboBoxBuilder &IsSimple() { this->style &= ~(CBS_SIMPLE | CBS_DROPDOWN | CBS_DROPDOWNLIST); this->style |= CBS_SIMPLE; return this->me(); }
	DialogComboBoxBuilder &IsDropDown() { this->style &= ~(CBS_SIMPLE | CBS_DROPDOWN | CBS_DROPDOWNLIST); this->style |= CBS_DROPDOWN; return this->me(); }
	DialogComboBoxBuilder &IsDropDownList() { this->style &= ~(CBS_SIMPLE | CBS_DROPDOWN | CBS_DROPDOWNLIST); this->style |= CBS_DROPDOWNLIST; return this->me(); }
	DialogComboBoxBuilder &WithAutoHScroll(bool AutoHScroll = true) { if (AutoHScroll) this->style |= CBS_AUTOHSCROLL; else this->style &= ~CBS_AUTOHSCROLL; return this->me(); }
	DialogComboBoxBuilder &WithSort(bool Sort = true) { if (Sort) this->style |= CBS_SORT; else this->style &= ~CBS_SORT; return this->me(); }
	DialogComboBoxBuilder &WithExactHeight(bool ExactHeight = true) { if (ExactHeight) this->style |= CBS_NOINTEGRALHEIGHT; else this->style &= ~CBS_NOINTEGRALHEIGHT; return this->me(); }
	DialogComboBoxBuilder &WithDisabledNoScroll(bool DisabledNoScroll = true) { if (DisabledNoScroll) this->style |= CBS_DISABLENOSCROLL; else this->style &= ~CBS_DISABLENOSCROLL; return this->me(); }
#if WINVER >= 0x0400
	DialogComboBoxBuilder &WithAllLowercase(bool AllLowercase = true) { if (AllLowercase) this->style |= CBS_LOWERCASE; else this->style &= ~CBS_LOWERCASE; return this->me(); }
	DialogComboBoxBuilder &WithAllUppercase(bool AllUppercase = true) { if (AllUppercase) this->style |= CBS_UPPERCASE; else this->style &= ~CBS_UPPERCASE; return this->me(); }
#endif
};

class DialogTemplate
{
	class DialogControl;

	typedef std::vector<std::unique_ptr<DialogControl>> Controls;

	class DialogControl
	{
	protected:
		DialogControlType controlType;
		uint32_t style, exstyle;
		Rect<short> rect;
		short id;
		std::unique_ptr<RelativePosition> relativePosition;

		friend class DialogTemplate;
		DialogControl() : controlType(NO_CONTROL), style(0), exstyle(0), rect(), id(-1), relativePosition() { }
		DialogControl(const DialogControl &control) : controlType(control.controlType), style(control.style), exstyle(control.exstyle), rect(control.rect), id(control.id),
			relativePosition(control.relativePosition ? control.relativePosition->Clone() : nullptr) { }
		DialogControl &operator=(const DialogControl &control)
		{
			this->controlType = control.controlType;
			this->style = control.style;
			this->exstyle = control.exstyle;
			this->rect = control.rect;
			this->id = control.id;
			if (control.relativePosition)
				this->relativePosition.reset(control.relativePosition->Clone());
			else
				this->relativePosition.reset();

			return *this;
		}
	public:
		virtual ~DialogControl() { }
		template<typename Control, typename Builder> static std::unique_ptr<Control> CreateControl(const DialogControlBuilder<Builder> &builder)
		{
			auto control = std::unique_ptr<Control>(new Control());

			control->controlType = builder.controlType;
			control->style = builder.style;
			control->exstyle = builder.exstyle;
			control->rect = builder.rect;
			control->id = builder.id;
			if (builder.relativePosition)
				control->relativePosition.reset(builder.relativePosition->Clone());

			return control;
		}
		virtual uint16_t GetControlCount() const { return 1; }
		virtual short GetControlHeight() const { return this->rect.size.height; }
		virtual std::vector<uint8_t> GenerateControlTemplate() const = 0;
		virtual DialogControl *Clone() const = 0;
	};

	class DialogGroup : public DialogControl
	{
	protected:
		DialogTemplate::Controls controls;
		std::wstring groupName;

		friend class DialogTemplate;
		friend class DialogControl;
		DialogGroup() : DialogControl(), controls(), groupName(L"") { }
		DialogGroup(const DialogGroup &control) : DialogControl(control), controls(), groupName(control.groupName)
		{
			std::for_each(control.controls.begin(), control.controls.end(), [&](const std::unique_ptr<DialogControl> &ctl) { this->controls.push_back(std::unique_ptr<DialogControl>(ctl->Clone())); });
		}
		DialogGroup &operator=(const DialogGroup &control)
		{
			DialogControl::operator=(control);

			this->controls.clear();
			std::for_each(control.controls.begin(), control.controls.end(), [&](const std::unique_ptr<DialogControl> &ctl) { this->controls.push_back(std::unique_ptr<DialogControl>(ctl->Clone())); });

			this->groupName = control.groupName;

			return *this;
		}
	public:
		static std::unique_ptr<DialogGroup> CreateControl(const DialogControlBuilder<DialogGroupBuilder> &builder)
		{
			auto control = DialogControl::CreateControl<DialogGroup>(builder);

			control->groupName = dynamic_cast<const DialogGroupBuilder &>(builder).groupName;

			return control;
		}
		void CalculatePositions(bool doRightAndBottom = false);
		void CalculateSize();
		uint16_t GetControlCount() const;
		std::vector<uint8_t> GenerateControlTemplate() const;
		DialogGroup *Clone() const { return new DialogGroup(*this); }
	};

	class DialogControlWithoutLabel : public DialogControl
	{
	protected:
		uint16_t type;

		friend class DialogTemplate;
		friend class DialogControl;
		DialogControlWithoutLabel() : DialogControl(), type(0) { }
		DialogControlWithoutLabel(const DialogControlWithoutLabel &control) : DialogControl(control), type(control.type) { }
		DialogControlWithoutLabel &operator=(const DialogControlWithoutLabel &control)
		{
			DialogControl::operator=(control);

			this->type = control.type;

			return *this;
		}
	public:
		template<typename Control, typename Builder> static std::unique_ptr<Control> CreateControl(const DialogControlBuilder<Builder> &builder, uint16_t Type)
		{
			auto control = DialogControl::CreateControl<Control>(builder);

			control->type = Type;

			return control;
		}
		virtual std::vector<uint8_t> GenerateControlTemplate() const;
		virtual DialogControlWithoutLabel *Clone() const { return new DialogControlWithoutLabel(*this); }
	};

	class DialogControlWithLabel : public DialogControl
	{
	protected:
		uint16_t type;
		std::wstring label;

		friend class DialogTemplate;
		friend class DialogControl;
		DialogControlWithLabel() : DialogControl(), type(0), label(L"") { }
		DialogControlWithLabel(const DialogControlWithLabel &control) : DialogControl(control), type(control.type), label(control.label) { }
		DialogControlWithLabel &operator=(const DialogControlWithLabel &control)
		{
			DialogControl::operator=(control);

			this->type = control.type;
			this->label = control.label;

			return *this;
		}
	public:
		template<typename Control, typename Builder> static std::unique_ptr<Control> CreateControl(const DialogControlBuilder<Builder> &builder, uint16_t Type)
		{
			auto control = DialogControl::CreateControl<Control>(builder);

			control->type = Type;
			control->label = dynamic_cast<const DialogControlWithLabelBuilder<Builder> &>(builder).label;

			return control;
		}
		virtual std::vector<uint8_t> GenerateControlTemplate() const;
		virtual DialogControlWithLabel *Clone() const { return new DialogControlWithLabel(*this); }
	};

	class DialogEditBox : public DialogControlWithoutLabel
	{
	protected:
		friend class DialogTemplate;
		friend class DialogControl;
		DialogEditBox() : DialogControlWithoutLabel() { }
	public:
		static std::unique_ptr<DialogEditBox> CreateControl(const DialogControlBuilder<DialogEditBoxBuilder> &builder)
		{
			return DialogControlWithoutLabel::CreateControl<DialogEditBox>(builder, 0x0081);
		}
	};

	class DialogLabel : public DialogControlWithLabel
	{
	protected:
		friend class DialogTemplate;
		friend class DialogControl;
		DialogLabel() : DialogControlWithLabel() { }
	public:
		static std::unique_ptr<DialogLabel> CreateControl(const DialogControlBuilder<DialogLabelBuilder> &builder)
		{
			return DialogControlWithLabel::CreateControl<DialogLabel>(builder, 0x0082);
		}
	};

	class DialogButton : public DialogControlWithLabel
	{
	protected:
		friend class DialogTemplate;
		friend class DialogControl;
		DialogButton() : DialogControlWithLabel() { }
	public:
		template<typename Builder> static std::unique_ptr<DialogButton> CreateControl(const DialogControlBuilder<Builder> &builder)
		{
			return DialogControlWithLabel::CreateControl<DialogButton>(builder, 0x0080);
		}
	};

	class DialogListBox : public DialogControlWithoutLabel
	{
	protected:
		friend class DialogTemplate;
		friend class DialogControl;
		DialogListBox() : DialogControlWithoutLabel() { }
	public:
		static std::unique_ptr<DialogListBox> CreateControl(const DialogControlBuilder<DialogListBoxBuilder> &builder)
		{
			return DialogControlWithoutLabel::CreateControl<DialogListBox>(builder, 0x0083);
		}
	};

	class DialogComboBox : public DialogControlWithoutLabel
	{
	protected:
		friend class DialogTemplate;
		friend class DialogControl;
		DialogComboBox() : DialogControlWithoutLabel() { }
	public:
		static std::unique_ptr<DialogComboBox> CreateControl(const DialogControlBuilder<DialogComboBoxBuilder> &builder)
		{
			return DialogControlWithoutLabel::CreateControl<DialogComboBox>(builder, 0x0085);
		}
		short GetControlHeight() const { return (this->style & CBS_SIMPLE && !(this->style & CBS_DROPDOWNLIST)) ? this->rect.size.height : 14; }
	};

	std::wstring title;
	uint32_t style, exstyle;
	std::wstring fontName;
	uint16_t fontSizeInPts;
	Size<short> size;
	DialogTemplate::Controls controls;
	std::vector<uint8_t> templateData;

	template<typename Builder> void AddControlToGroup(std::unique_ptr<DialogControl> &&control, const DialogControlBuilder<Builder> &builder)
	{
		const auto &groupBuilder = dynamic_cast<const DialogInGroupBuilder<Builder> &>(builder);
		if (groupBuilder.groupName.empty())
		{
			if (builder.index == -1)
				this->controls.push_back(std::move(control));
			else
				this->controls.insert(this->controls.begin() + builder.index, std::move(control));
		}
		else
		{
			for (auto curr = this->controls.begin(), end = this->controls.end(); curr != end; ++curr)
			{
				if ((*curr)->controlType != GROUP_CONTROL)
					continue;
				DialogGroup *group = dynamic_cast<DialogGroup *>(curr->get());
				if (group->groupName == groupBuilder.groupName)
				{
					if (builder.index == -1)
						group->controls.push_back(std::move(control));
					else
						group->controls.insert(group->controls.begin() + builder.index, std::move(control));
					return;
				}
			}
			throw std::runtime_error("Group " + ConvertFuncs::WStringToString(groupBuilder.groupName) + " was not found.");
		}
	}
	uint16_t GetTotalControlCount() const;
	bool CalculateControlPosition(short index, bool doRightAndBottom = false);
	void CalculateSize();
public:
	DialogTemplate() : title(L""), style(0), exstyle(0), fontName(L""), fontSizeInPts(0), size(), controls(), templateData() { }
	DialogTemplate(const DialogBuilder &builder) : title(builder.title), style(builder.style), exstyle(builder.exstyle), fontName(builder.fontName), fontSizeInPts(builder.fontSizeInPts),
		size(builder.size), controls(), templateData() { }
	DialogTemplate(const DialogTemplate &dlg) : title(dlg.title), style(dlg.style), exstyle(dlg.style), fontName(dlg.fontName), fontSizeInPts(dlg.fontSizeInPts), size(dlg.size),
		controls(), templateData()
	{
		std::for_each(dlg.controls.begin(), dlg.controls.end(), [&](const std::unique_ptr<DialogControl> &control) { this->controls.push_back(std::unique_ptr<DialogControl>(control->Clone())); });
	}
	DialogTemplate &operator=(const DialogBuilder &builder)
	{
		this->title = builder.title;
		this->style = builder.style;
		this->exstyle = builder.exstyle;
		this->fontName = builder.fontName;
		this->fontSizeInPts = builder.fontSizeInPts;
		this->size = builder.size;
		if (builder.resetControls)
			this->controls.clear();

		return *this;
	}
	DialogTemplate &operator=(const DialogTemplate &dlg)
	{
		this->title = dlg.title;
		this->style = dlg.style;
		this->exstyle = dlg.exstyle;
		this->fontName = dlg.fontName;
		this->fontSizeInPts = dlg.fontSizeInPts;
		this->size = dlg.size;
		this->controls.clear();
		std::for_each(dlg.controls.begin(), dlg.controls.end(), [&](const std::unique_ptr<DialogControl> &control) { this->controls.push_back(std::unique_ptr<DialogControl>(control->Clone())); });

		return *this;
	}
	void AddGroupControl(const DialogControlBuilder<DialogGroupBuilder> &builder);
	void AddEditBoxControl(const DialogControlBuilder<DialogEditBoxBuilder> &builder);
	void AddLabelControl(const DialogControlBuilder<DialogLabelBuilder> &builder);
	void AddCheckBoxControl(const DialogControlBuilder<DialogCheckBoxBuilder> &builder);
	void AddButtonControl(const DialogControlBuilder<DialogButtonBuilder> &builder);
	void AddListBoxControl(const DialogControlBuilder<DialogListBoxBuilder> &builder);
	void AddComboBoxControl(const DialogControlBuilder<DialogComboBoxBuilder> &builder);
	void AutoSize();
	const DLGTEMPLATE *GenerateTemplate();
};
