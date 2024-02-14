#if !defined(AFX_NSFMEMORYDIALOG_H__C9B73D82_2DAB_4352_8D15_9CE32AD22EC4__INCLUDED_)
#define AFX_NSFMEMORYDIALOG_H__C9B73D82_2DAB_4352_8D15_9CE32AD22EC4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NSFMemoryDialog.h : �w�b�_�[ �t�@�C��
//
#include "NSFDialog.h"
/////////////////////////////////////////////////////////////////////////////
// NSFMemoryDialog �_�C�A���O

class NSFMemoryDialog : public CDialog, public NSFDialog
{
// �R���X�g���N�V����
public:
	NSFMemoryDialog(CWnd* pParent = NULL);   // �W���̃R���X�g���N�^
  ~NSFMemoryDialog();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(NSFMemoryDialog)
	enum { IDD = IDD_MEMORY };
	//}}AFX_DATA

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(NSFMemoryDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

protected:  
  xgm::UINT8 dumpbuf[65536];
  CFont dlgFont;

// �C���v�������e�[�V����
protected:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(NSFMemoryDialog)
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnJapanese();
	afx_msg void OnEnglish();
	afx_msg void OnMemwrite();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ �͑O�s�̒��O�ɒǉ��̐錾��}�����܂��B

#endif // !defined(AFX_NSFMEMORYDIALOG_H__C9B73D82_2DAB_4352_8D15_9CE32AD22EC4__INCLUDED_)
