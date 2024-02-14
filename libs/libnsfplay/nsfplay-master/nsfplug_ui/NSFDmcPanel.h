#if !defined(AFX_NSFDMCPANEL_H__3A5642E6_3DBA_4736_AC9E_070D5867260E__INCLUDED_)
#define AFX_NSFDMCPANEL_H__3A5642E6_3DBA_4736_AC9E_070D5867260E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NSFDmcPanel.h : �w�b�_�[ �t�@�C��
//
#include "NSFDialog.h"
#include "afxcmn.h"
/////////////////////////////////////////////////////////////////////////////
// NSFDmcPanel �_�C�A���O

class NSFDmcPanel : public CDialog, public NSFDialog
{
// �R���X�g���N�V����
public:
	NSFDmcPanel(CWnd* pParent = NULL);   // �W���̃R���X�g���N�^

// �_�C�A���O �f�[�^
	//{{AFX_DATA(NSFDmcPanel)
	enum { IDD = IDD_DMCPANEL };
	BOOL	m_enable_4011;
	BOOL	m_enable_pnoise;
	BOOL	m_nonlinear_mixer;
	BOOL	m_anti_click;
	BOOL	m_randomize_noise;
	BOOL	m_unmute;
	BOOL	m_tri_mute;
	BOOL	m_randomize_tri;
	BOOL	m_dpcm_reverse;
	//}}AFX_DATA

public:
  void UpdateNSFPlayerConfig(bool b);

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(NSFDmcPanel)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(NSFDmcPanel)
	afx_msg void OnEnable4011();
	afx_msg void OnEnablePnoise();
	afx_msg void OnNonlinearMixer();
	afx_msg void OnAntiNoise();
	afx_msg void OnRandomizeNoise();
	afx_msg void OnUnmute();
	afx_msg void OnTriMute();
	afx_msg void OnDpcmReverse();
	virtual BOOL OnInitDialog();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ �͑O�s�̒��O�ɒǉ��̐錾��}�����܂��B

#endif // !defined(AFX_NSFDMCPANEL_H__3A5642E6_3DBA_4736_AC9E_070D5867260E__INCLUDED_)
