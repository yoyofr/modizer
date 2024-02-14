#if !defined(AFX_NSFMEMORYWRITEDIALOG_H__D05FEE33_E3CB_4A3F_97CC_78F81BF87316__INCLUDED_)
#define AFX_NSFMEMORYWRITEDIALOG_H__D05FEE33_E3CB_4A3F_97CC_78F81BF87316__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NSFMemoryWriteDialog.h : �w�b�_�[ �t�@�C��
//
#include "NSFDialog.h"
/////////////////////////////////////////////////////////////////////////////
// NSFMemoryWriteDialog �_�C�A���O

class NSFMemoryWriteDialog : public CDialog, public NSFDialog
{
// �R���X�g���N�V����
public:
	NSFMemoryWriteDialog(CWnd* pParent = NULL);   // �W���̃R���X�g���N�^

// �_�C�A���O �f�[�^
	//{{AFX_DATA(NSFMemoryWriteDialog)
	enum { IDD = IDD_MEMWRITE };
	CListBox	m_wlist;
	CString	m_address;
	CString	m_value;
	//}}AFX_DATA


// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(NSFMemoryWriteDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
  int sidx[65536];

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(NSFMemoryWriteDialog)
	afx_msg void OnInsert();
	afx_msg void OnDelete();
	afx_msg void OnSend();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ �͑O�s�̒��O�ɒǉ��̐錾��}�����܂��B

#endif // !defined(AFX_NSFMEMORYWRITEDIALOG_H__D05FEE33_E3CB_4A3F_97CC_78F81BF87316__INCLUDED_)
