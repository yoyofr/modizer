#pragma once
#include <map>
#include "afxwin.h"
#include "resource.h"
#include "NSFDialog.h"
#include "PresetManager.h"

// NSFPresetDialog �_�C�A���O

class NSFPresetDialog : public CDialog, public NSFDialog
{
	DECLARE_DYNAMIC(NSFPresetDialog)
private:
    PresetManager pman;

public:
	NSFPresetDialog(CWnd* pParent = NULL);   // �W���R���X�g���N�^
	virtual ~NSFPresetDialog();
  void Reset(CString path);
  void Open();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_PRESET };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g

	DECLARE_MESSAGE_MAP()
public:
  virtual BOOL OnInitDialog();
  afx_msg void OnCbnSelchangeCombo();
  CListBox m_listCtrl;
  afx_msg void OnBnClickedDelete();
  afx_msg void OnBnClickedSave();
  afx_msg void OnBnClickedLoad();
  afx_msg void OnBnClickedAdd();
  CString m_editValue;
  afx_msg void OnLbnDblclkList();
};
