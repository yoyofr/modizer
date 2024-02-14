#if !defined(AFX_NSFFDSPANEL_H__FE81618A_15CD_40A7_B503_C43DA1BFC0D7__INCLUDED_)
#define AFX_NSFFDSPANEL_H__FE81618A_15CD_40A7_B503_C43DA1BFC0D7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NSFFdsPanel.h : �w�b�_�[ �t�@�C��
//
#include "NSFDialog.h"
/////////////////////////////////////////////////////////////////////////////
// NSFFdsPanel �_�C�A���O

class NSFFdsPanel : public CDialog, public NSFDialog
{
// �R���X�g���N�V����
public:
	NSFFdsPanel(CWnd* pParent = NULL);   // �W���̃R���X�g���N�^

// �_�C�A���O �f�[�^
	//{{AFX_DATA(NSFFdsPanel)
	enum { IDD = IDD_FDSPANEL };
	UINT	m_nCutoff;
	BOOL	m_4085_reset;
	BOOL	m_write_protect;
	//}}AFX_DATA

public:
	void UpdateNSFPlayerConfig(bool b);

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(NSFFdsPanel)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(NSFFdsPanel)
	afx_msg void OnChangeCutoff();
	afx_msg void On4085Reset();
	afx_msg void OnWriteProtect();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ �͑O�s�̒��O�ɒǉ��̐錾��}�����܂��B

#endif // !defined(AFX_NSFFDSPANEL_H__FE81618A_15CD_40A7_B503_C43DA1BFC0D7__INCLUDED_)
