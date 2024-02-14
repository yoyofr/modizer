#if !defined(AFX_NSFCONFIGDIALOG_H__78FFE715_3A96_4C72_8B03_74F1230E2C59__INCLUDED_)
#define AFX_NSFCONFIGDIALOG_H__78FFE715_3A96_4C72_8B03_74F1230E2C59__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NSFConfigDialog.h : �w�b�_�[ �t�@�C��
//
#include "NSFDialog.h"
#include "NSFConfigPageMain.h"
#include "NSFConfigPageDevice.h"
#include "NSFApuPanel.h"
#include "NSFFdsPanel.h"
#include "NSFDmcPanel.h"
#include "NSFMMC5Panel.h"
#include "NSFN106Panel.h"
#include "NSFVRC6Panel.h"
#include "NSFVRC7Panel.h"
#include "NSFFME7Panel.h"

/////////////////////////////////////////////////////////////////////////////
// NSFConfigDialog

class NSFConfigDialog : public CPropertySheet, public NSFDialog
{
	DECLARE_DYNAMIC(NSFConfigDialog)
//
protected:
  NSFConfigPageMain *mainPage;
  NSFConfigPageDevice *devicePage[xgm::NES_DEVICE_MAX];
  NSFApuPanel *apuPanel;
  NSFFdsPanel *fdsPanel;
  NSFDmcPanel *dmcPanel;
  NSFMMC5Panel *mmc5Panel;
  NSFN106Panel *n106Panel;
  NSFVRC6Panel *vrc6Panel;
  NSFVRC7Panel *vrc7Panel;
  NSFFME7Panel *fme7Panel;
  
// �R���X�g���N�V����
public:
	NSFConfigDialog(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	NSFConfigDialog(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
// �A�g���r���[�g
public:
  CButton m_ok, m_cancel, m_apply;

// �I�y���[�V����
public:
  void SetDialogManager(NSFDialogManager *p);
  void UpdateNSFPlayerConfig(bool b);
  void Open();

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(NSFConfigDialog)
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
public:
	virtual ~NSFConfigDialog();

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:
	//{{AFX_MSG(NSFConfigDialog)
  afx_msg void OnOK();
  afx_msg void OnApply();
  afx_msg void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
  virtual BOOL OnInitDialog();
  afx_msg void OnClose();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ �͑O�s�̒��O�ɒǉ��̐錾��}�����܂��B

#endif // !defined(AFX_NSFCONFIGDIALOG_H__78FFE715_3A96_4C72_8B03_74F1230E2C59__INCLUDED_)
