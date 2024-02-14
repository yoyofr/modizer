#if !defined(AFX_NSFAUDIOPANEL_H__97C80FEC_63C5_47AE_8D11_504F136735C1__INCLUDED_)
#define AFX_NSFAUDIOPANEL_H__97C80FEC_63C5_47AE_8D11_504F136735C1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NSFAudioPanel.h : �w�b�_�[ �t�@�C��
//
#include "NSFDialog.h"

/////////////////////////////////////////////////////////////////////////////
// NSFAudioPanel �_�C�A���O

class NSFAudioPanel : public CDialog, public NSFDialog
{
// �R���X�g���N�V����
public:
	NSFAudioPanel(CWnd* pParent = NULL);   // �W���̃R���X�g���N�^

// �_�C�A���O �f�[�^
	//{{AFX_DATA(NSFAudioPanel)
	enum { IDD = IDD_AUDIOPANEL };
	CSliderCtrl	m_threshold;
	CSliderCtrl	m_quality;
	CSliderCtrl	m_filter;
	CStatic	m_desc;
	//}}AFX_DATA

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(NSFAudioPanel)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

public:
  int device_id;
  void UpdateNSFPlayerConfig(bool b);
  void SetDeviceID(int id);

// �C���v�������e�[�V����
protected:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(NSFAudioPanel)
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ �͑O�s�̒��O�ɒǉ��̐錾��}�����܂��B

#endif // !defined(AFX_NSFAUDIOPANEL_H__97C80FEC_63C5_47AE_8D11_504F136735C1__INCLUDED_)
