#if !defined(AFX_NSFEASYDIALOG_H__1C420639_8E4D_4D95_AF54_EB89E50152ED__INCLUDED_)
#define AFX_NSFEASYDIALOG_H__1C420639_8E4D_4D95_AF54_EB89E50152ED__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NSFEasyDialog.h : �w�b�_�[ �t�@�C��
//

/////////////////////////////////////////////////////////////////////////////
// NSFEasyDialog �_�C�A���O
#include "NSFDialog.h"
#include "afxwin.h"
#include "PresetManager.h"
#include "afxcmn.h"

class NSFEasyDialog : public CDialog, public NSFDialog
{
// �R���X�g���N�V����
public:
	NSFEasyDialog(CWnd* pParent = NULL);   // �W���̃R���X�g���N�^

// �_�C�A���O �f�[�^
	//{{AFX_DATA(NSFEasyDialog)
	enum { IDD = IDD_EASY };
	CSliderCtrl	m_qualityCtrl;
	CSliderCtrl	m_lpfCtrl;
	//}}AFX_DATA

  void UpdateNSFPlayerConfig(bool b);


// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(NSFEasyDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(NSFEasyDialog)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
  CComboBox m_comboCtrl;
  PresetManager pman;
  afx_msg void OnCbnSelchangeCombo();
  CSliderCtrl m_hpfCtrl;
  CSliderCtrl m_masterCtrl;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ �͑O�s�̒��O�ɒǉ��̐錾��}�����܂��B

#endif // !defined(AFX_NSFEASYDIALOG_H__1C420639_8E4D_4D95_AF54_EB89E50152ED__INCLUDED_)
