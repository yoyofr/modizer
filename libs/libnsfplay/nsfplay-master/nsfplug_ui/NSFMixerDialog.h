#if !defined(AFX_NSFMIXERDIALOG_H__A6CB3FD2_F937_43CE_9365_3BB2FAC76ECE__INCLUDED_)
#define AFX_NSFMIXERDIALOG_H__A6CB3FD2_F937_43CE_9365_3BB2FAC76ECE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NSFMixerDialog.h : �w�b�_�[ �t�@�C��
//
#include "NSFDialog.h"
#include "NSFMixerPanel.h"

/////////////////////////////////////////////////////////////////////////////
// NSFMixerDialog �_�C�A���O

class NSFMixerDialog : public CDialog, public NSFDialog
{
// �R���X�g���N�V����
public:
	NSFMixerDialog(CWnd* pParent = NULL);   // �W���̃R���X�g���N�^

  void UpdateNSFPlayerConfig(bool b);

// �_�C�A���O �f�[�^
	//{{AFX_DATA(NSFMixerDialog)
	enum { IDD = IDD_MIXERBOX };
		// ����: ClassWizard �͂��̈ʒu�Ƀf�[�^ �����o��ǉ����܂��B
	//}}AFX_DATA


// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(NSFMixerDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// ���[�U�[�ǉ�
public:
  NSFMixerPanel panel[xgm::NES_DEVICE_MAX];
  NSFMixerPanel master_panel;
  int m_kbflag;
  CMenu m_Menu;
  void SetDialogManager(NSFDialogManager *p);

// �C���v�������e�[�V����
protected:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(NSFMixerDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnReset();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnMask();
  afx_msg void OnUpall();
  afx_msg void OnDownall();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ �͑O�s�̒��O�ɒǉ��̐錾��}�����܂��B

#endif // !defined(AFX_NSFMIXERDIALOG_H__A6CB3FD2_F937_43CE_9365_3BB2FAC76ECE__INCLUDED_)
